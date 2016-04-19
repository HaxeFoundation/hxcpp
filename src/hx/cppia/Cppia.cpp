#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <set>

#include "Cppia.h"
#include "CppiaStream.h"
#include <stdlib.h>


#ifdef HX_ANDROID
  #define atof(x) strtod(x,0)
#endif

// Really microsoft?
#ifdef interface
  #undef interface
#endif


namespace hx
{
//#define SJLJ_RETURN 1


#ifdef DEBUG_RETURN_TYPE
int gLastRet = etVoid;
#endif

static bool isNumeric(ExprType t) { return t==etInt || t==etFloat; }

static int sTypeSize[] = { 0, 0, sizeof(hx::Object *), sizeof(String), sizeof(Float), sizeof(int) };

void cppiaClassMark(CppiaClassInfo *inClass,hx::MarkContext *__inCtx);
void cppiaClassVisit(CppiaClassInfo *inClass,hx::VisitContext *__inCtx);
int getScriptId(hx::Class inClass);

// --- TypeData ----
TypeData::TypeData(String inModule)
{
   Array<String> parts = inModule.split(HX_CSTRING("::"));
   if (parts[0].length==0)
      parts->shift();
   name = parts->join(HX_CSTRING("."));
   cppiaClass = 0;
   haxeClass = null();
   haxeBase = 0;
   linked = false;
   arrayType = arrNotArray;
   interfaceBase = 0;
   isInterface = false;
}
void TypeData::mark(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(name);
   HX_MARK_MEMBER(haxeClass);
   if (cppiaClass)
      cppiaClassMark(cppiaClass,__inCtx);
}

#ifdef HXCPP_VISIT_ALLOCS
void TypeData::visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(name);
   HX_VISIT_MEMBER(haxeClass);
   if (cppiaClass)
      cppiaClassVisit(cppiaClass,__inCtx);
}
#endif


struct StackLayout;

void cppiaClassInit(CppiaClassInfo *inClass, CppiaCtx *ctx, int inPhase);


static int sScriptId = 0;

const char **sgNativeNameSlots = 0;
int sgNativeNameSlotCount = 0;


// --- CppiaModule ----

CppiaModule::CppiaModule()
{
   main = 0;
   layout = 0;
   creatingClass = 0;
   creatingFunction = 0;
   scriptId = ++sScriptId;
   strings = Array_obj<String>::__new(0,0);
   if (sgNativeNameSlotCount>0)
      for(int i=2;i<sgNativeNameSlotCount;i++)
         interfaceSlots[sgNativeNameSlots[i]] = i;

}

void CppiaModule::setDebug(CppiaExpr *outExpr, int inFileId, int inLine)
{
   outExpr->className = creatingClass;
   outExpr->functionName = creatingFunction;
   outExpr->filename = cStrings[inFileId].c_str();
   outExpr->line = inLine;
}

void CppiaModule::boot(CppiaCtx *ctx)
{
   // boot (statics)
   for(int i=0;i<classes.size();i++)
      cppiaClassInit(classes[i],ctx,0);
   // run __init__
   for(int i=0;i<classes.size();i++)
      cppiaClassInit(classes[i],ctx,1);
}

int CppiaModule::getInterfaceSlot(const std::string &inName)
{
   InterfaceSlots::iterator it = interfaceSlots.find(inName);
   if (it==interfaceSlots.end())
   {
      #if (HXCPP_API_LEVEL >= 330)
      int result = interfaceSlots.size()+1;
      #else
      int result = interfaceSlots.size()+2;
      #endif
      interfaceSlots[inName] = result;
      return result;
   }
   return it->second;
}


int CppiaModule::findInterfaceSlot(const std::string &inName)
{
   InterfaceSlots::iterator it = interfaceSlots.find(inName);
   if (it==interfaceSlots.end())
      return -1;
   return it->second;
}

void ScriptableRegisterNameSlots(const char *inNames[], int inLength)
{
   sgNativeNameSlots = inNames;
   sgNativeNameSlotCount = inLength;
}

// --- StackLayout ---



// 'this' pointer is in slot 0 and captureSize(0) ...
StackLayout::StackLayout(StackLayout *inParent) :
   size( sizeof(void *) ), captureSize(sizeof(void *)), parent(inParent)
{
}


void StackLayout::dump(Array<String> &inStrings, std::string inIndent)
{
   printf("%sCapture:\n",inIndent.c_str());
   for(int i=0;i<captureVars.size();i++)
      printf("%s %s\n", inIndent.c_str(), inStrings[captureVars[i]->nameId].__s );
   if (parent)
      parent->dump(inStrings,inIndent + "   ");
}


CppiaStackVar *StackLayout::findVar(int inId)
{
   if (varMap[inId])
      return varMap[inId];
   CppiaStackVar *var = parent ? parent->findVar(inId) : 0;
   if (!var)
      return 0;

   CppiaStackVar *closureVar = new CppiaStackVar(var,size,captureSize);
   varMap[inId] = closureVar;
   captureVars.push_back(closureVar);
   return closureVar;
}

// --- CppiaCtx functions ----------------------------------------

#ifdef CPPIA_JIT
void CppiaExpr::genCode(CppiaCompiler &compiler, const Addr &inDest, ExprType resultType)
{
   compiler.trace(getName());
}



int SLJIT_CALL objectToInt(hx::Object *obj) { return obj->__ToInt(); }
void SLJIT_CALL frameToDouble(CppiaCtx *inCtx) { inCtx->returnFloat( inCtx->getObject() ); }
//void objectToInt(CppiaCtx *inCtx) { inCtx->returnInt( inCtx->getObject() ); }
void SLJIT_CALL objectToDouble(CppiaCtx *inCtx) { inCtx->returnFloat( inCtx->getObject() ); }
void SLJIT_CALL objectToDoublePointer(CppiaCtx *inCtx)
{
   *(double *)inCtx->pointer = (*(hx::Object **)inCtx->pointer)->__ToDouble();
}
void SLJIT_CALL objectToStringPointer(CppiaCtx *inCtx)
{
   *(String *)inCtx->pointer = (*(hx::Object **)inCtx->pointer)->toString();
}
void SLJIT_CALL objectToString(CppiaCtx *inCtx) { inCtx->returnString( inCtx->getObject() ); }
void SLJIT_CALL stringToObject(CppiaCtx *inCtx) { inCtx->returnObject( inCtx->getString() ); }
void SLJIT_CALL intToObject(CppiaCtx *inCtx) { inCtx->returnObject( inCtx->getInt() ); }
void SLJIT_CALL doubleToObject(CppiaCtx *inCtx) { inCtx->returnObject( inCtx->getFloat() ); }

#endif




// --- CppiaDynamicExpr ----------------------------------------
// Delegates to 'runObject'

struct CppiaDynamicExpr : public CppiaExpr
{
   CppiaDynamicExpr(const CppiaExpr *inSrc=0) : CppiaExpr(inSrc) {}

   const char *getName() { return "CppiaDynamicExpr"; }

   virtual int         runInt(CppiaCtx *ctx)    {
      hx::Object *obj = runObject(ctx);
      return ValToInt(obj);
   }
   virtual Float       runFloat(CppiaCtx *ctx) { return ValToFloat(runObject(ctx)); }
   virtual ::String    runString(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      BCR_CHECK;
      return result ? result->toString() : String();
   }
   virtual void        runVoid(CppiaCtx *ctx)   { runObject(ctx); }
   virtual hx::Object *runObject(CppiaCtx *ctx) = 0;

   #ifdef CPPIA_JIT
   virtual void genObject(CppiaCompiler &compiler, const Addr &inDest)  { }

   void preGen(CppiaCompiler &compiler)
   {
      AllocTemp pointerSave(compiler);
   }

   void genCode(CppiaCompiler &compiler, const Addr &inDest, ExprType resultType)
   {
      switch(resultType)
      {
         case etInt:
            genObject(compiler, Reg(0));
            compiler.call( objectToInt, 1 );
            if (inDest!=Reg(0))
               compiler.move32( inDest, Reg(0) );
            break;
         case etFloat:
            {
            genObject(compiler, Reg(0) );
            CtxMemberVal pointer(offsetof(CppiaCtx, pointer));
            compiler.move(TempReg(),pointer);
            compiler.move(StarAddr(TempReg()),Reg(0));
            compiler.call( objectToDoublePointer, 1 );
            compiler.emitf( SLJIT_DMOV, inDest, TempReg() );
            }
            break;
         case etString:
            {
            genObject(compiler, Reg(0) );
            CtxMemberVal pointer(offsetof(CppiaCtx, pointer));
            compiler.move(TempReg(),pointer);
            compiler.move(StarAddr(TempReg()),Reg(0));
            compiler.call( objectToStringPointer, 1 );
            compiler.move32( inDest, StarAddr(TempReg()) );
            compiler.move( inDest.offset(4), StarAddr(TempReg(),(4)) );
            }
            break;
 
         default:
            genObject(compiler, inDest);
      }
   }
   #endif


};

// --- CppiaVoidExpr ----------------------------------------
// Delegates to 'runInt'
struct CppiaVoidExpr : public CppiaExpr
{
   CppiaVoidExpr(const CppiaExpr *inSrc=0) : CppiaExpr(inSrc) {}

   const char *getName() { return "CppiaVoidExpr"; }

   ExprType getType() { return etVoid; }

   virtual int  runInt(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   virtual Float       runFloat(CppiaCtx *ctx) { runVoid(ctx); return 0.0; }
   virtual ::String    runString(CppiaCtx *ctx) { runVoid(ctx); return String(); }
   virtual hx::Object *runObject(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   virtual void        runVoid(CppiaCtx *ctx) = 0;
};




// --- CppiaIntExpr ----------------------------------------
// Delegates to 'runInt'

struct CppiaIntExpr : public CppiaExpr
{
   CppiaIntExpr(const CppiaExpr *inSrc=0) : CppiaExpr(inSrc) {}

   const char *getName() { return "CppiaIntExpr"; }
   ExprType getType() { return etInt; }

   void runVoid(CppiaCtx *ctx) { runInt(ctx); }
   Float runFloat(CppiaCtx *ctx) { return runInt(ctx); }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runInt(ctx)).mPtr; }
   String runString(CppiaCtx *ctx) { return String(runInt(ctx)); }

   int runInt(CppiaCtx *ctx) = 0;
};


// --- CppiaBoolExpr ----------------------------------------
// Delegates to 'runInt'

struct CppiaBoolExpr : public CppiaIntExpr
{
   CppiaBoolExpr(const CppiaExpr *inSrc=0) : CppiaIntExpr(inSrc) {}

   const char *getName() { return "CppiaBoolExpr"; }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runInt(ctx) ? true : false).mPtr; }
   String runString(CppiaCtx *ctx) { return runInt(ctx)?HX_CSTRING("true") : HX_CSTRING("false");}
};


// ------------------------------------------------------



CppiaExpr *createStaticAccess(CppiaExpr *inSrc, FieldStorage inType, void *inPtr);

static void ReadExpressions(Expressions &outExpressions, CppiaStream &stream,int inN=-1)
{
   int count = inN>=0 ? inN : stream.getInt();
   outExpressions.resize(count);

   for(int i=0;i<count;i++)
      outExpressions[i] =  createCppiaExpr(stream);
}



static void LinkExpressions(Expressions &ioExpressions, CppiaModule &data)
{
   for(int i=0;i<ioExpressions.size();i++)
      ioExpressions[i] = ioExpressions[i]->link(data);
}


CppiaExpr *convertToFunction(CppiaExpr *inExpr);


// --- CppiaConst -------------------------------------------

struct CppiaConst
{
   enum Type { cInt, cFloat, cString, cBool, cNull, cThis, cSuper };

   Type type;
   int  ival;
   Float  dval;

   CppiaConst() : type(cNull), ival(0), dval(0) { }

   void fromStream(CppiaStream &stream)
   {
      std::string tok = stream.getToken();
      if (tok[0]=='i')
      {
         type = cInt;

         dval = ival = stream.getInt();
      }
      else if (tok=="true")
      {
         type = cInt;
         dval = ival = 1;
      }
      else if (tok=="false")
      {
         type = cInt;
         dval = ival = 0;
      }
      else if (tok[0]=='f')
      {
         type = cFloat;
         int strIndex = stream.getInt();
         String val = stream.module->strings[strIndex];
         dval = atof(val.__s);
      }
      else if (tok[0]=='s')
      {
         type = cString;
         ival = stream.getInt();
      }
      else if (tok=="NULL")
         type = cNull;
      else if (tok=="THIS")
         type = cThis;
      else if (tok=="SUPER")
         type = cSuper;
      else
         throw "unknown const value";
   }
};




// --- ScriptCallable ----------------------------------------------------

static String sInvalidArgCount = HX_CSTRING("Invalid arguement count");


#ifdef CPPIA_JIT
void SLJIT_CALL argToInt(CppiaCtx *ctx) { ctx->pushInt( (* (hx::Object **)(ctx->pointer))->__ToInt() ); }
void SLJIT_CALL argToDouble(CppiaCtx *ctx) { ctx->pushFloat( (* (hx::Object **)(ctx->pointer))->__ToDouble() ); }
void SLJIT_CALL argToString(CppiaCtx *ctx) { ctx->pushString( (* (hx::Object **)(ctx->pointer))->__ToString() ); }
#endif

struct ScriptCallable : public CppiaDynamicExpr
{
   int returnTypeId;
   ExprType returnType;
   int argCount;
   int stackSize;
   
   std::vector<CppiaStackVar> args;
   std::vector<bool>          hasDefault;
   std::vector<CppiaConst>    initVals;
   CppiaExpr *body;
   CppiaModule *data;
   #ifdef CPPIA_JIT
   CppiaCompiled compiled;
   #else
   void    *compiled;
   #endif

   std::vector<CppiaStackVar *> captureVars;
   int                          captureSize;

   ScriptCallable(CppiaStream &stream)
   {
      body = 0;
      stackSize = 0;
      data = 0;
      returnTypeId = stream.getInt();
      returnType = etVoid;
      argCount = stream.getInt();
      args.resize(argCount);
      hasDefault.resize(argCount);
      initVals.resize(argCount);
      captureSize = 0;
      compiled = 0;
      for(int a=0;a<argCount;a++)
      {
         args[a].fromStream(stream);
         bool init = stream.getBool();
         hasDefault[a] = init;
         if (init)
            initVals[a].fromStream(stream);
      }
      body = createCppiaExpr(stream);
   }

   ScriptCallable(CppiaExpr *inBody) : CppiaDynamicExpr(inBody)
   {
      returnTypeId = 0;
      returnType = etVoid;
      argCount = 0;
      stackSize = 0;
      captureSize = 0;
      body = inBody;
      compiled = 0;
   }

   ~ScriptCallable()
   {
      #ifdef CPPIA_JIT
      if (compiled)
         CppiaCompiler::freeCompiled(compiled);
      #endif
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      StackLayout *oldLayout = inModule.layout;
      StackLayout layout(oldLayout);
      inModule.layout = &layout;
      data = &inModule;

      returnType = inModule.types[ returnTypeId ]->expressionType;
      layout.returnType = returnType;

      for(int a=0;a<args.size();a++)
         args[a].link(inModule);

      body = body->link(inModule);

      captureVars.swap(layout.captureVars);
      captureSize = layout.captureSize;

      stackSize = layout.size;
      inModule.layout = oldLayout;
      return this;
   }

   ExprType getType() { return returnType; }


   #ifdef CPPIA_JIT
   void preGenArgs(CppiaCompiler &compiler, CppiaExpr *inThis, Expressions &inArgs)
   {
      if (inThis)
        inThis->preGen(compiler);

      for(int a=0;a<argCount && a<inArgs.size();a++)
      {
         CppiaStackVar &var = args[a];
         if (!hasDefault[a] && var.expressionType==etString)
         {
            AllocTemp result(compiler);
            inArgs[a]->preGen(compiler);
         }
         else
         {
            if (!hasDefault[a] && var.expressionType==etFloat)
               compiler.registerFTemp();
            inArgs[a]->preGen(compiler);
         }
      }
   }


   void genPushDefault(CppiaCompiler &compiler, int inArg, bool pushNullToo)
   {
      CtxMemberVal stack(offsetof(CppiaCtx, pointer));
      compiler.move(Reg(0),stack);
      StarAddr pointer(Reg(0));

      CppiaStackVar &var = args[inArg];
      switch(var.expressionType)
      {
          case etInt:
             compiler.move32( pointer, ConstValue(initVals[inArg].ival) );
             compiler.add( stack, Reg(0), ConstValue( sizeof(int) ) );
             break;
          case etFloat:
             compiler.move( pointer, ConstRef(&initVals[inArg].dval), SLJIT_DMOV );
             compiler.add( stack, Reg(0), ConstValue( sizeof(double) ) );
             break;
          case etString:
             {
                // TODO - const string / null
                const String &str = data->strings[ initVals[inArg].ival ];
                compiler.move32( pointer, ConstValue(str.length) );
                compiler.move( pointer.offset(4), ConstValue(str.__s) );
                compiler.add( stack, Reg(0), ConstValue( sizeof(int) + sizeof(void*) ) );
             }
             break;
          default:
             {
                // TODO : GC on CppiaConst Dynamixc...
                Dynamic val;
                switch(initVals[inArg].type)
                {
                    case CppiaConst::cInt: val = initVals[inArg].ival; break;
                    case CppiaConst::cFloat: val = initVals[inArg].dval; break;
                    case CppiaConst::cString: val = data->strings[ initVals[inArg].ival ]; break;
                    default: ;
                }
                if (val.mPtr || pushNullToo)
                {
                   compiler.move( pointer, ConstValue(val.mPtr) );
                   compiler.add( stack, Reg(0), ConstValue( sizeof(void *) ) );
                }
             }
       }
   }
 

   void genArgs(CppiaCompiler &compiler, CppiaExpr *inThis, Expressions &inArgs)
   {
      int inCount = inArgs.size();
      bool badCount = argCount<inCount;

      for(int i=inCount;i<argCount && !badCount;i++)
         if (!hasDefault[i])
            badCount = true;

      if (badCount)
      {
         printf("Arg count mismatch %d!=%d ?\n", argCount, (int)inArgs.size());
         printf(" %s at %s:%d %s\n", getName(), filename, line, functionName);
         CPPIA_CHECK(0);
         throw Dynamic(HX_CSTRING("Arg count error"));
         //return;
      }

      CtxMemberVal stack(offsetof(CppiaCtx, pointer));


      StarAddr pointer(Reg(1));
      // Push this ...
      if (inThis)
      {
         inThis->genCode(compiler, Reg(0), etObject);
         compiler.move(Reg(1),stack);
         compiler.move(pointer,Reg(0));
      }
      else
      {
         compiler.move(Reg(1),stack);
         StarAddr pointer(Reg(1));
         compiler.move( pointer, ConstValue(0) );
      }

      compiler.add( stack, Reg(1), ConstValue( sizeof(void *) ) );


      for(int a=0;a<argCount;a++)
      {
         CppiaStackVar &var = args[a];
         // TODO capture
         if (hasDefault[a])
         {
            if (a>=inCount)
               genPushDefault(compiler,a,true);
            else
            {
                // Gen Object onto stack ...
                inArgs[a]->genCode(compiler, Reg(1), etObject );
                if (var.expressionType!=etObject)
                {
                   // Check for null
                   sljit_jump *notNull = compiler.ifNotZero( Reg(1) );

                   genPushDefault(compiler,a,false);

                   sljit_jump *doneArg = compiler.jump(SLJIT_JUMP);

                   compiler.jumpHere(notNull);

                   // Reg1 holds object
                   switch(var.expressionType)
                   {
                      case etInt:
                         compiler.move( Reg(0), CtxReg() );
                         compiler.call( objectToInt, 2);
                         break;
                      case etFloat:
                         compiler.move( Reg(0), CtxReg() );
                         compiler.call( objectToDouble, 1);
                         break;
                      case etString:
                         compiler.move( Reg(0), CtxReg() );
                         compiler.call( objectToString, 1);
                         break;
                      default:

                   compiler.jumpHere(doneArg);
                   }
                }
                else
                {
                   compiler.move(Reg(1),stack);
                   StarAddr pointer(Reg(1));
                   compiler.move(pointer,Reg(2));
                }
             }
         }
         else
         {
            StarAddr pointer(Reg(1));
            switch(var.expressionType)
            {
               case etInt:
                  inArgs[a]->genCode(compiler, Reg(0), etInt);
                  compiler.move(Reg(1),stack);
                  compiler.move(pointer,Reg(0));
                  compiler.add( stack, Reg(1), ConstValue( sizeof(int) ) );
                  break;
               case etFloat:
                  inArgs[a]->genCode(compiler, FReg(0), etFloat);
                  compiler.move(Reg(1),stack);
                  compiler.move(pointer,FReg(0),SLJIT_DMOV);
                  compiler.add( stack, Reg(1), ConstValue( sizeof(double) ) );
                  break;
               case etString:
                  {
                  AllocTemp result(compiler);
                  inArgs[a]->genCode(compiler, result, etString);
                  compiler.move(Reg(1),stack);
                  compiler.move32(pointer,result);
                  compiler.move(pointer.offset(4),result.offset(4));
                  compiler.add( stack, Reg(1), ConstValue( sizeof(String) ) );
                  }
                  break;
               default:
                  inArgs[a]->genCode(compiler, Reg(0), etObject);
                  compiler.add( stack, Reg(1), ConstValue( sizeof(void *) ) );
            }
         }
      }
   }
   #endif


   void pushArgs(CppiaCtx *ctx, hx::Object *inThis, Expressions &inArgs)
   {
      BCR_VCHECK;
      int inCount = inArgs.size();
      bool badCount = argCount<inCount;

      for(int i=inCount;i<argCount && !badCount;i++)
         if (!hasDefault[i])
            badCount = true;

      if (badCount)
      {
         printf("Arg count mismatch %d!=%d ?\n", argCount, (int)inArgs.size());
         printf(" %s at %s:%d %s\n", getName(), filename, line, functionName);
         CPPIA_CHECK(0);
         throw Dynamic(HX_CSTRING("Arg count error"));
         //return;
      }


      ctx->push( inThis );

      for(int a=0;a<argCount;a++)
      {
         CppiaStackVar &var = args[a];
         // TODO capture
         if (hasDefault[a])
         {
            bool makeNull = a>=inCount;

            if (var.expressionType == etString)
            {
               String val;
               if (!makeNull)
                  val = inArgs[a]->runString(ctx);
               if (val==null() && initVals[a].type==CppiaConst::cString)
                  val = data->strings[ initVals[a].ival ];

               BCR_VCHECK;
               ctx->pushString(val);
               continue;
            }

            hx::Object *obj = makeNull ? 0 : inArgs[a]->runObject(ctx);
            BCR_VCHECK;
            switch(var.expressionType)
            {
               case etInt:
                  ctx->pushInt( obj ? obj->__ToInt() : initVals[a].ival );
                  break;
               case etFloat:
                  ctx->pushFloat( (Float)(obj ? obj->__ToDouble() : initVals[a].dval) );
                  break;
               case etString: // Handled above
                  break;
               default:
                  if (obj)
                     ctx->pushObject(obj);
                  else
                  {
                     switch(initVals[a].type)
                     {
                        case CppiaConst::cInt:
                           ctx->pushObject( Dynamic(initVals[a].ival).mPtr );
                           break;
                        case CppiaConst::cFloat:
                           ctx->pushObject( Dynamic(initVals[a].dval).mPtr );
                           break;
                        case CppiaConst::cString:
                           ctx->pushObject( Dynamic(data->strings[ initVals[a].ival ]).mPtr );
                           break;
                        default:
                           ctx->pushObject(null());
                     }

                  }
            }
         }
         else
         {
            switch(var.expressionType)
            {
               case etInt:
                  ctx->pushInt(inArgs[a]->runInt(ctx));
                  break;
               case etFloat:
                  ctx->pushFloat(inArgs[a]->runFloat(ctx));
                  break;
               case etString:
                  ctx->pushString(inArgs[a]->runString(ctx));
                  break;
               default:
                  ctx->pushObject(inArgs[a]->runObject(ctx));
            }
            BCR_VCHECK;
         }
      }
   }



   void pushArgsDynamic(CppiaCtx *ctx, hx::Object *inThis, Array<Dynamic> &inArgs)
   {
      BCR_VCHECK;

      int inCount = inArgs==null() ? 0 : inArgs->length;
      /*
      bool badCount = argCount<inCount;
      for(int i=inCount;i<argCount && !badCount;i++)
         if (!hasDefault[i])
            badCount = true;

      if (badCount)
      {
         printf("Dynamic Arg count mismatch %d!=%d ?\n", argCount, inCount);
         printf(" %s at %s:%d %s\n", getName(), filename, line, functionName);
         CPPIA_CHECK(0);
         throw Dynamic(HX_CSTRING("Arg count error"));
         //return;
      }
      */


      ctx->push( inThis );

      for(int a=0;a<argCount;a++)
      {
         CppiaStackVar &var = args[a];
         // TODO capture
         if (hasDefault[a])
         {
            hx::Object *obj = a<inCount ?inArgs[a].mPtr : 0;
            BCR_VCHECK;
            switch(var.expressionType)
            {
               case etInt:
                  ctx->pushInt( obj ? obj->__ToInt() : initVals[a].ival );
                  break;
               case etFloat:
                  ctx->pushFloat( (Float)(obj ? obj->__ToDouble() : initVals[a].dval) );
                  break;
               case etString:
                  ctx->pushString( obj ? obj->toString() :
                              initVals[a].type == CppiaConst::cNull ? String() :
                                data->strings[ initVals[a].ival ] );
                  break;

               default:
                  if (obj)
                     ctx->pushObject(obj);
                  else
                  {
                     switch(initVals[a].type)
                     {
                        case CppiaConst::cInt:
                           ctx->pushObject( Dynamic(initVals[a].ival).mPtr );
                           break;
                        case CppiaConst::cFloat:
                           ctx->pushObject( Dynamic(initVals[a].dval).mPtr );
                           break;
                        case CppiaConst::cString:
                           ctx->pushObject( Dynamic(data->strings[ initVals[a].ival ]).mPtr );
                           break;
                        default:
                           ctx->pushObject(null());
                     }

                  }
            }
         }
         else if (a<inCount)
         {
            switch(var.expressionType)
            {
               case etInt:
                  ctx->pushInt(inArgs[a]);
                  break;
               case etFloat:
                  ctx->pushFloat(inArgs[a]);
                  break;
               case etString:
                  ctx->pushString(inArgs[a]);
                  break;
               default:
                  ctx->pushObject(inArgs[a].mPtr);
            }
            BCR_VCHECK;
         }
         else
         {
            // Push cpp defaults...
            switch(var.expressionType)
            {
               case etInt:
                  ctx->pushInt(0);
                  break;
               case etFloat:
                  ctx->pushFloat(0.0);
                  break;
               case etString:
                  ctx->pushString(String());
                  break;
               default:
                  ctx->pushObject(0);
            }
 
         }
      }
   }


   // Return the closure
   hx::Object *runObject(CppiaCtx *ctx)
   {
      return createClosure(ctx,this);
   }

   const char *getName() { return "ScriptCallable"; }
   String runString(CppiaCtx *ctx) { return HX_CSTRING("#function"); }

   void runVoid(CppiaCtx *ctx) { }

   // Run the actual function
   void runFunction(CppiaCtx *ctx)
   {
      #ifdef CPPIA_JIT
      if (compiled)
      {
         //printf("Running compiled code...\n");
         compiled(ctx);
         //printf("Done.\n");
      }
      else
      #endif
      {
         if (stackSize)
         {
            memset(ctx->pointer, 0 , stackSize );
            ctx->pointer += stackSize;
         }
         body->runVoid(ctx);
      }
   }

   void addStackVarsSpace(CppiaCtx *ctx)
   {
      if (stackSize)
      {
         memset(ctx->pointer, 0 , stackSize );
         ctx->pointer += stackSize;
      }
   }

   
   bool pushDefault(CppiaCtx *ctx,int arg)
   {
      if (!hasDefault[arg])
         return false;

      switch(args[arg].expressionType)
      {
         case etInt:
            if (initVals[arg].type==CppiaConst::cFloat)
               ctx->pushInt( initVals[arg].dval );
            else
               ctx->pushInt( initVals[arg].ival );
            break;
         case etFloat:
            if (initVals[arg].type==CppiaConst::cFloat)
               ctx->pushFloat( initVals[arg].dval );
            else
               ctx->pushFloat( initVals[arg].ival );
            break;
         case etString:
            ctx->pushString( initVals[arg].type == CppiaConst::cNull ? String() : data->strings[initVals[arg].ival] );
            break;
         default:
            switch(initVals[arg].type)
            {
               case CppiaConst::cInt:
                  ctx->pushObject( Dynamic(initVals[arg].ival).mPtr );
                  break;
               case CppiaConst::cFloat:
                  ctx->pushObject( Dynamic(initVals[arg].dval).mPtr );
                  break;
               case CppiaConst::cString:
                  ctx->pushObject( Dynamic(data->strings[ initVals[arg].ival ]).mPtr );
                  break;
               default:
                  ctx->pushObject(null());
            }

      }
      return true;
   }

   void addExtraDefaults(CppiaCtx *ctx,int inHave)
   {
      if (inHave>argCount)
      {
         return;
         //throw sInvalidArgCount;
      }

      for(int a=inHave;a<argCount;a++)
      {
         CppiaStackVar &var = args[a];
         if (!pushDefault(ctx,a))
         {
            return;
            throw sInvalidArgCount;
         }
      }
   }


   #ifdef CPPIA_JIT
   void compile()
   {
      if (!compiled && body)
      {
         CppiaCompiler compiler;

         body->preGen(compiler);

         compiler.enter(0, stackSize);

         body->genCode(compiler, AddrVoid(), etVoid);

         compiler.ret();
         compiled = compiler.generate();
      }
   }
   #endif

};




// --- CppiaFunction ----

CppiaFunction::CppiaFunction(CppiaModule *inCppia,bool inIsStatic,bool inIsDynamic) :
   cppia(*inCppia), isStatic(inIsStatic), isDynamic(inIsDynamic), funExpr(0)
{
   linked = false;
   vtableSlot = -1;
}

void CppiaFunction::load(CppiaStream &stream,bool inExpectBody)
{
   nameId = stream.getInt();
   name = cppia.strings[ nameId ].__s;
   stream.module->creatingFunction = name.c_str();
   returnType = stream.getInt();
   argCount = stream.getInt();
   DBGLOG("  Function %s(%d) : %s %s%s\n", name.c_str(), argCount, cppia.typeStr(returnType), isStatic?"static":"instance", isDynamic ? " DYNAMIC": "");
   args.resize(argCount);
   for(int a=0;a<argCount;a++)
   {
      ArgInfo arg = args[a];
      arg.nameId = stream.getInt();
      arg.optional = stream.getBool();
      arg.typeId = stream.getInt();
      DBGLOG("    arg %c%s:%s\n", arg.optional?'?':' ', cppia.identStr(arg.nameId), cppia.typeStr(arg.typeId) );
   }
   if (inExpectBody)
      funExpr = (ScriptCallable *)createCppiaExpr(stream);
   stream.module->creatingFunction = 0;
}

void CppiaFunction::link( )
{
   if (!linked)
   {
      linked = true;
      if (funExpr)
         funExpr = (ScriptCallable *)(funExpr->link(cppia));
   }
}

#ifdef CPPIA_JIT
void CppiaFunction::compile()
{
   if (funExpr)
      funExpr->compile();
}
#endif


class CppiaEnumBase : public EnumBase_obj
{
public:
   #if (HXCPP_API_LEVEL<330)
   CppiaClassInfo *classInfo; 
   #endif

   CppiaEnumBase(CppiaClassInfo *inInfo) { classInfo = inInfo; }

   ::hx::ObjectPtr<hx::Class_obj > __GetClass() const;
	::String GetEnumName( ) const;
	::String __ToString() const;
};



struct CppiaEnumConstructor
{
   struct Arg
   {
      Arg(int inNameId, int inTypeId) : nameId(inNameId), typeId(inTypeId) { }
      int nameId;
      int typeId;
   };
 
   std::vector<Arg> args;
   CppiaClassInfo   *classInfo;
   int              nameId;
   Dynamic          value;
   int              index;
   String           name;

   CppiaEnumConstructor(CppiaModule &inModule, CppiaStream &inStream, CppiaClassInfo *inClassInfo)
   {
      classInfo = inClassInfo;
      nameId = inStream.getInt();
      index = -1;
      name = String();
      int argCount = inStream.getInt();
      for(int a=0;a<argCount;a++)
      {
         int nameId = inStream.getInt();
         int typeId = inStream.getInt();
         args.push_back( Arg(nameId,typeId) );
      }
         
   }
   hx::Object *create( Array<Dynamic> inArgs )
   {
      bool ok = inArgs.mPtr ? (inArgs->length==args.size()) : args.size()==0;
      if (!ok)
         throw Dynamic(HX_CSTRING("Bad enum arg count"));
      if (args.size()==0)
         return value.mPtr;
      #if (HXCPP_API_LEVEL >= 330)
      EnumBase_obj *result = new ((int)args.size()*sizeof(cpp::Variant)) CppiaEnumBase(classInfo);
      result->_hx_setIdentity(name, index, args.size());
      for(int i=0;i<args.size();i++)
         result->_hx_init( i, inArgs[i] );
      #else
      EnumBase_obj *result = new CppiaEnumBase(classInfo);
      result->__Set(name, index, inArgs);
      #endif
      return result;
   }
};


void runFunExpr(CppiaCtx *ctx, ScriptCallable *inFunExpr, hx::Object *inThis, Expressions &inArgs );
hx::Object *runFunExprDynamic(CppiaCtx *ctx, ScriptCallable *inFunExpr, hx::Object *inThis, Array<Dynamic> &inArgs );
void runFunExprDynamicVoid(CppiaCtx *ctx, ScriptCallable *inFunExpr, hx::Object *inThis, Array<Dynamic> &inArgs );

hx::Class_obj *createCppiaClass(CppiaClassInfo *);
void  linkCppiaClass(hx::Class_obj *inClass, CppiaModule &cppia, String inName);


typedef std::vector<CppiaFunction *> Functions;
typedef std::map<std::string, ScriptCallable *> FunctionMap;

struct CppiaClassInfo
{
   CppiaModule &cppia;
   std::vector<int> implements;
   bool      isInterface;
   bool      isLinked;
   bool      isEnum;
   int       typeId;
   TypeData  *type;
   int       superId;
   TypeData *superType;
   int       classSize;
   int       extraData;
   int       dynamicMapOffset;
   int       interfaceSlotSize;
   void      **vtable;
   std::string name;
   #if (HXCPP_API_LEVEL>=330)
   std::map<int, void *> interfaceScriptTables;
   #else
   std::map<std::string, void **> interfaceVTables;
   #endif
   std::set<String> nativeProperties;
   hx::Class     mClass;

   HaxeNativeClass *haxeBase;

   Functions memberFunctions;
   FunctionMap memberGetters;
   FunctionMap memberSetters;
   std::vector<CppiaVar *> memberVars;
   std::vector<CppiaVar *> dynamicFunctions;

   Functions staticFunctions;
   FunctionMap staticGetters;
   FunctionMap staticSetters;
   std::vector<CppiaVar *> staticVars;
   std::vector<CppiaVar *> staticDynamicFunctions;

   std::vector<CppiaEnumConstructor *> enumConstructors;

   CppiaFunction  *newFunc;
   ScriptCallable *initFunc;
   CppiaExpr      *enumMeta;

   CppiaClassInfo(CppiaModule &inCppia) : cppia(inCppia)
   {
      isLinked = false;
      haxeBase = 0;
      extraData = 0;
      classSize = 0;
      newFunc = 0;
      initFunc = 0;
      enumMeta = 0;
      isInterface = false;
      interfaceSlotSize = 0;
      superType = 0;
      typeId = 0;
      vtable = 0;
      type = 0;
      mClass.mPtr = 0;
      dynamicMapOffset = 0;
   }

   /*
   class CppiaClass *getCppiaClass()
   {
      return (class CppiaClass *)mClass.mPtr;
   }
   */

   hx::Object *createInstance(CppiaCtx *ctx,Expressions &inArgs, bool inCallNew = true)
   {
      hx::Object *obj = haxeBase->factory(vtable,extraData);

      createDynamicFunctions(obj);

      if (newFunc && inCallNew)
         runFunExpr(ctx, newFunc->funExpr, obj, inArgs );

      return obj;
   }

   hx::Object *createInstance(CppiaCtx *ctx,Array<Dynamic> &inArgs)
   {
      hx::Object *obj = haxeBase->factory(vtable,extraData);

      createDynamicFunctions(obj);

      if (newFunc)
         runFunExprDynamicVoid(ctx, newFunc->funExpr, obj, inArgs );

      return obj;
   }


   inline void createDynamicFunctions(hx::Object *inThis)
   {
      for(int d=0;d<dynamicFunctions.size();d++)
         dynamicFunctions[d]->createDynamic(inThis);
   }

   inline bool isNativeProperty(const String &inString)
   {
      return nativeProperties.find(inString) != nativeProperties.end();
   }

   int __GetType() { return isEnum ? vtEnum : vtClass; }

   int getEnumIndex(String inName)
   {
      for(int i=0;i<enumConstructors.size();i++)
      {
        if (enumConstructors[i]->name==inName)
           return i;
      }

      throw Dynamic(HX_CSTRING("Bad enum index"));
      return 0;
   }

   bool implementsInterface(CppiaClassInfo *inInterface)
   {
      for(int i=0;i<implements.size();i++)
         if (implements[i] == inInterface->typeId)
            return true;
      return false;
   }


   ScriptCallable *findFunction(bool inStatic,int inId)
   {
      Functions &funcs = inStatic ? staticFunctions : memberFunctions;
      for(int i=0;i<funcs.size();i++)
      {
         if (funcs[i]->nameId == inId)
            return funcs[i]->funExpr;
      }
      return 0;
   }

   ScriptCallable *findInterfaceFunction(const std::string &inName)
   {
      Functions &funcs = memberFunctions;
      for(int i=0;i<funcs.size();i++)
      {
         if ( cppia.strings[funcs[i]->nameId].__s == inName)
            return funcs[i]->funExpr;
      }
      return 0;
   }

   ScriptCallable *findFunction(bool inStatic, const String &inName)
   {
      Functions &funcs = inStatic ? staticFunctions : memberFunctions;
      for(int i=0;i<funcs.size();i++)
      {
         if ( cppia.strings[funcs[i]->nameId] == inName)
            return funcs[i]->funExpr;
      }
      return 0;
   }

   inline ScriptCallable *findFunction(FunctionMap &inMap,const String &inName)
   {
      FunctionMap::iterator it = inMap.find(inName.__s);
      if (it!=inMap.end())
         return it->second;
      return 0;

   }

   inline ScriptCallable *findMemberGetter(const String &inName)
      { return findFunction(memberGetters,inName); }

   inline ScriptCallable *findMemberSetter(const String &inName)
      { return findFunction(memberSetters,inName); }

   inline ScriptCallable *findStaticGetter(const String &inName)
      { return findFunction(staticGetters,inName); }

   inline ScriptCallable *findStaticSetter(const String &inName)
      { return findFunction(staticSetters,inName); }


   bool getField(hx::Object *inThis, String inName, hx::PropertyAccess  inCallProp, Dynamic &outValue)
   {
      if (inCallProp==paccDynamic)
         inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;

      if (inCallProp)
      {
         ScriptCallable *getter = findMemberGetter(inName);
         if (getter)
         {
            Array<Dynamic> empty;
            outValue.mPtr = runFunExprDynamic(CppiaCtx::getCurrent(),getter,inThis, empty);
            return true;
         }
      }

      CppiaExpr *closure = findFunction(false,inName);
      if (closure)
      {
         outValue.mPtr = createMemberClosure(inThis,(ScriptCallable*)closure);
         return true;
      }

      // Look for dynamic function (variable)
      for(int i=0;i<dynamicFunctions.size();i++)
      {
         if (cppia.strings[ dynamicFunctions[i]->nameId  ]==inName)
         {
            CppiaVar *d = dynamicFunctions[i];

            outValue = d->getValue(inThis);

            return true;
         }
      }

      for(int i=0;i<memberVars.size();i++)
      {
         CppiaVar &var = *memberVars[i];
         if (var.name==inName)
         {
            outValue = var.getValue(inThis);
            return true;
         }
      }

      hx::FieldMap *map = dynamicMapOffset ? (hx::FieldMap *)( (char *)inThis + dynamicMapOffset ) :
                           inThis->__GetFieldMap();
      if (map)
      {
         if (hx::FieldMapGet(map,inName,outValue))
            return true;
      }

      //printf("Get field not found (%s) %s\n", inThis->toString().__s,inName.__s);
      return false;
   }

   bool setField(hx::Object *inThis, String inName, Dynamic inValue, hx::PropertyAccess  inCallProp, Dynamic &outValue)
   {
      if (inCallProp==paccDynamic)
         inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;

      if (inCallProp)
      {
         //printf("Set field %s %s = %s\n", inThis->toString().__s, inName.__s, inValue->toString().__s);
         ScriptCallable *setter = findMemberSetter(inName);
         if (setter)
         {
            Array<Dynamic> args = Array_obj<Dynamic>::__new(1,1);
            args[0] = inValue;
            outValue.mPtr = runFunExprDynamic(CppiaCtx::getCurrent(),setter,inThis, args);
            return true;
         }
      }


      // Look for dynamic function (variable)
      for(int i=0;i<dynamicFunctions.size();i++)
      {
         if (cppia.strings[ dynamicFunctions[i]->nameId  ]==inName)
         {
            CppiaVar *d = dynamicFunctions[i];
            outValue = d->setValue(inThis,inValue);
            return true;
         }
      }

      for(int i=0;i<memberVars.size();i++)
      {
         CppiaVar &var = *memberVars[i];
         if (var.name==inName)
         {
            outValue = var.setValue(inThis, inValue);
            return true;
         }
      }

      hx::FieldMap *map = dynamicMapOffset ? (hx::FieldMap *)( (char *)inThis + dynamicMapOffset ) :
                           inThis->__GetFieldMap();
      if (map)
      {
         FieldMapSet(map, inName, inValue);
         outValue = inValue;
         return true;
      }

      // Fall though to haxe base
      //printf("Set field not found (%s) %s map=%p o=%d\n", inThis->toString().__s,inName.__s, map, dynamicMapOffset);

      return false;
   }


   int findFunctionSlot(int inName)
   {
      for(int i=0;i<memberFunctions.size();i++)
         if (memberFunctions[i]->nameId==inName)
            return memberFunctions[i]->vtableSlot;
      return -1;
   }

   ExprType findFunctionType(CppiaModule &inModule, int inName)
   {
      for(int i=0;i<memberFunctions.size();i++)
         if (memberFunctions[i]->nameId==inName)
            return inModule.types[ memberFunctions[i]->returnType ]->expressionType;
      return etVoid;
   }


   CppiaVar *findVar(bool inStatic,int inId)
   {
      std::vector<CppiaVar *> &vars = inStatic ? staticVars : memberVars;
      for(int i=0;i<vars.size();i++)
      {
         if (vars[i]->nameId == inId)
            return vars[i];
      }

      std::vector<CppiaVar *> &dvars = inStatic ? staticDynamicFunctions : dynamicFunctions;
      for(int i=0;i<dvars.size();i++)
      {
         if (dvars[i]->nameId == inId)
            return dvars[i];
      }

      if (superType && superType->cppiaClass)
         return superType->cppiaClass->findVar(inStatic,inId);

      return 0;
   }

   void dumpVars(const char *inMessage, std::vector<CppiaVar *> &vars)
   {
      printf(" %s:\n", inMessage);
      for(int i=0;i<vars.size();i++)
         printf("   %d] %s (%d)\n", i, cppia.strings[ vars[i]->nameId ].__s, vars[i]->nameId );
   }

   void dumpFunctions(const char *inMessage, std::vector<CppiaFunction *> &funcs)
   {
      printf(" %s:\n", inMessage);
      for(int i=0;i<funcs.size();i++)
         printf("   %d] %s (%d)\n", i, cppia.strings[ funcs[i]->nameId ].__s, funcs[i]->nameId);
   }


   void dump()
   {
      printf("Class %s\n", name.c_str());
      dumpFunctions("Member functions",memberFunctions);
      dumpFunctions("Static functions",staticFunctions);
      dumpVars("Member vars",memberVars);
      dumpVars("Member dyns",dynamicFunctions);
      dumpVars("Static vars",staticVars);
      dumpVars("Static dyns",staticDynamicFunctions);
   }

   CppiaEnumConstructor *findEnum(int inFieldId)
   {
      for(int i=0;i<enumConstructors.size();i++)
         if (enumConstructors[i]->nameId==inFieldId)
            return enumConstructors[i];
      return 0;
   }

   #if (HXCPP_API_LEVEL < 330)
   void **getInterfaceVTable(const std::string &inName)
   {
      return interfaceVTables[inName];
   }
   #endif

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(mClass);
      for(int i=0;i<enumConstructors.size();i++)
      {
         HX_MARK_MEMBER(enumConstructors[i]->value);
         HX_MARK_MEMBER(enumConstructors[i]->name);
      }
      for(int i=0;i<staticVars.size();i++)
         staticVars[i]->mark(__inCtx);
      for(int i=0;i<staticDynamicFunctions.size();i++)
         staticDynamicFunctions[i]->mark(__inCtx);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(mClass);
      for(int i=0;i<enumConstructors.size();i++)
      {
         HX_VISIT_MEMBER(enumConstructors[i]->value);
         HX_VISIT_MEMBER(enumConstructors[i]->name);
      }
      for(int i=0;i<staticVars.size();i++)
         staticVars[i]->visit(__inCtx);
      for(int i=0;i<staticDynamicFunctions.size();i++)
         staticDynamicFunctions[i]->visit(__inCtx);
   }
#endif



   bool load(CppiaStream &inStream)
   {
      CppiaOp op = inStream.getOp();
      isInterface = isEnum = false;

      if (op==IaClass)
         isInterface = false;
      else if (op==IaInterface)
         isInterface = true;
      else if (op==IaEnum)
         isEnum = true;
      else
      {
         DBGLOG("Invalid class field op %d\n", op);
         throw "Bad class type";
      }

      std::string tok;

       typeId = inStream.getInt();
       mClass.mPtr = createCppiaClass(this);

       superId = isEnum ? 0 : inStream.getInt();
       int implementCount = isEnum ? 0 : inStream.getInt();
       implements.resize(implementCount);
       for(int i=0;i<implementCount;i++)
          implements[i] = inStream.getInt();

       name = cppia.typeStr(typeId);
       DBGLOG("Class %s %s\n", name.c_str(), isEnum ? "enum" : isInterface ? "interface" : "class" );

       bool isNew = //(resolved == null() || !IsCppiaClass(resolved) ) &&
                   (!HaxeNativeClass::findClass(name) && !HaxeNativeInterface::findInterface(name) );

       if (isNew && isEnum)
       {
          hx::Class cls =  hx::Class_obj::Resolve(String(name.c_str()));
          if (cls.mPtr && getScriptId(cls)==0)
          {
             DBGLOG("Found existing enum %s - ignore\n", name.c_str());
             isNew = false;
          }
       }

       if (isNew)
       {
          cppia.types[typeId]->cppiaClass = this;
       }
       else
       {
          DBGLOG("Already has registered %s - ignore\n",name.c_str());
       }

       inStream.module->creatingClass = name.c_str();

       int fields = inStream.getInt();
       for(int f=0;f<fields;f++)
       {
          if (isEnum)
          {
             CppiaEnumConstructor *enumConstructor = new CppiaEnumConstructor(cppia,inStream,this);
             enumConstructors.push_back( enumConstructor );
          }
          else
          {
             tok = inStream.getToken();
             if (tok=="FUNCTION")
             {
                bool isStatic = inStream.getStatic();
                bool isDynamic = inStream.getInt();
                CppiaFunction *func = new CppiaFunction(&cppia,isStatic,isDynamic);
                func->load(inStream,!isInterface);
                if (isDynamic)
                {
                   if (isStatic)
                      staticDynamicFunctions.push_back( new CppiaVar(func) );
                   else
                      dynamicFunctions.push_back( new CppiaVar(func) );
                }
                else
                {
                   if (isStatic)
                      staticFunctions.push_back(func);
                   else
                      memberFunctions.push_back(func);
                }
             }
             else if (tok=="VAR")
             {
                bool isStatic = inStream.getStatic();
                CppiaVar *var = new CppiaVar(isStatic);
                if (isStatic)
                   staticVars.push_back(var);
                else
                   memberVars.push_back(var);
                var->load(inStream);
             }
             else if (tok=="IMPLDYNAMIC")
             {
                // Fill in later
                dynamicMapOffset = -1;
             }
             else if (tok=="INLINE")
             {
                // OK
             }
             else
                throw "unknown field type";
          }
       }
       if (isEnum)
       {
          if (inStream.getBool())
             enumMeta = createCppiaExpr(inStream);
       }
       inStream.module->creatingClass = 0;
       return isNew;
   }

   #if (HXCPP_API_LEVEL<330)
   void **createInterfaceVTable(int inTypeId)
   {
      std::vector<CppiaExpr *> vtable;

      HaxeNativeInterface *interface = cppia.types[inTypeId]->interfaceBase;
      // Native-defined interface...
      if (interface)
      {
         vtable.push_back( findInterfaceFunction("toString") );
         ScriptNamedFunction *functions = interface->functions;
         if (functions != 0) {
            for(ScriptNamedFunction *f = functions; f->name; f++)
               if (strcmp(f->name,"toString"))
                  vtable.push_back( findInterfaceFunction(f->name) );
         }
            
      }

      CppiaClassInfo *cls = cppia.types[inTypeId]->cppiaClass;
      if (!cls && !interface)
         throw "vtable for unknown class";

      if (cls && !cls->isInterface)
         throw "vtable for non-interface";

      if (cls)
      {
         for(int i=0;i<cls->memberFunctions.size();i++)
         {
            CppiaFunction *func = cls->memberFunctions[i];
            vtable.push_back( findFunction(false,func->nameId) );
         }
      }

      void **result = new void *[ vtable.size() + 1];
      result[0] = this;
      result++;
      memcpy(result, &vtable[0], sizeof(void *)*vtable.size());
      return result;
   }
   #endif

   hx::Class *getSuperClass()
   {
      DBGLOG("getSuperClass %s %d\n", name.c_str(), superId);
      if (!superId)
         return 0;

      TypeData *superType = cppia.types[ superId ];
      if (!superType)
         throw "Unknown super type!";
      if (superType->cppiaClass)
         return &superType->cppiaClass->mClass;
      return &superType->haxeClass;
   }

   void addMemberFunction(Functions &ioCombined, CppiaFunction *inNewFunc)
   {
      for(int j=0;j<ioCombined.size();j++)
         if (ioCombined[j]->name==inNewFunc->name)
         {
            ioCombined[j] = inNewFunc;
            return;
         }
      ioCombined.push_back(inNewFunc);
   }

   void linkTypes()
   {
      if (isLinked)
         return;
      isLinked = true;

      type = cppia.types[typeId];
      mClass->mName = type->name;
      superType = superId ? cppia.types[ superId ] : 0;
      CppiaClassInfo  *cppiaSuper = superType ? superType->cppiaClass : 0;
      // Link super first
      if (superType && superType->cppiaClass)
         superType->cppiaClass->linkTypes();

      // implemented interfaces before main class
      for(int i=0;i<implements.size();i++)
         if (cppia.types[ implements[i] ]->cppiaClass)
            cppia.types[ implements[i] ]->cppiaClass->linkTypes();


      // Add super interfaces ...
      TypeData *extraInterfaces = cppia.types[ superId ];
      while(extraInterfaces)
      {
         if (extraInterfaces->cppiaClass)
         {
            CppiaClassInfo &parent = *extraInterfaces->cppiaClass;
            std::vector<int> &impl = parent.implements;
            for(int i=0;i<impl.size();i++)
            {
               bool found = false;
               for(int j=0;j<implements.size() && !found; j++)
                  found = implements[j] == impl[i];
               if (!found)
                  implements.push_back(impl[i]);
            }
            extraInterfaces = cppia.types[ parent.superId ];
         }
         else
            break;
      }


      DBGLOG(" Linking class '%s' ", type->name.__s);
      if (!superType)
      {
         DBGLOG("script base\n");
      }
      else if (cppiaSuper)
      {
         DBGLOG("extends script '%s'\n", superType->name.__s);
      }
      else
      {
         DBGLOG("extends haxe '%s'\n", superType->name.__s);
      }

      // Link class before we combine the function list...
      linkCppiaClass(mClass.mPtr,cppia,type->name);


      haxeBase = type->haxeBase;
      if (!haxeBase && !isInterface)
         throw "No base defined for non-interface";

      classSize = haxeBase ? haxeBase->mDataOffset : 0;

      // Combine member vars ...
      if (cppiaSuper)
      {
         classSize = cppiaSuper->classSize;

         if (cppiaSuper->dynamicMapOffset!=0)
            dynamicMapOffset = cppiaSuper->dynamicMapOffset;

         std::vector<CppiaVar *> combinedVars(cppiaSuper->memberVars );
         for(int i=0;i<memberVars.size();i++)
         {
            for(int j=0;j<combinedVars.size();j++)
               if (combinedVars[j]->nameId==memberVars[i]->nameId)
                  printf("Warning duplicate member var %s\n", cppia.strings[memberVars[i]->nameId].__s);
            combinedVars.push_back(memberVars[i]);
         }
         memberVars.swap(combinedVars);

         std::vector<CppiaVar *> combinedDynamics(cppiaSuper->dynamicFunctions );
         for(int i=0;i<dynamicFunctions.size();i++)
         {
            bool found = false;
            for(int j=0;j<combinedDynamics.size() && !found;j++)
               if (dynamicFunctions[i]->nameId == combinedDynamics[j]->nameId)
               {
                  // Overwrite
                  dynamicFunctions[i]->offset = combinedDynamics[j]->offset;
                  combinedDynamics[j] = dynamicFunctions[i];
                  found = true;
               }
            if (!found)
               combinedDynamics.push_back(dynamicFunctions[i]);
         }
         dynamicFunctions.swap(combinedDynamics);

         // Combine member functions ...
         Functions combinedFunctions(cppiaSuper->memberFunctions );
         if (isInterface)
         {
            // 'implements' interfaces are like extra super-classes for interfaces
            // For non-interface classes, these function will show up in the members anyhow
            for(int i=0;i<implements.size();i++)
            {
               CppiaClassInfo  *cppiaInterface = cppia.types[implements[i]]->cppiaClass;
               if (cppiaInterface)
               {
                  Functions &intfFuncs = cppiaInterface->memberFunctions;
                  for(int j=0;j<intfFuncs.size();j++)
                     addMemberFunction(combinedFunctions, intfFuncs[j]);
               }
            }
         }


         for(int i=0;i<memberFunctions.size();i++)
            addMemberFunction(combinedFunctions, memberFunctions[i]);

         memberFunctions.swap(combinedFunctions);
      }


      // Non-interface classes will have haxeBase
      if (haxeBase)
      {
         // Calculate table offsets...
         DBGLOG("  base haxe size %s = %d\n", haxeBase->name.c_str(), classSize);
         for(int i=0;i<memberVars.size();i++)
         {
            if (memberVars[i]->offset)
            {
               DBGLOG("   super var %s @ %d\n", cppia.identStr(memberVars[i]->nameId), memberVars[i]->offset);
            }
            else
            {
               DBGLOG("   link var %s @ %d\n", cppia.identStr(memberVars[i]->nameId), classSize);
               memberVars[i]->linkVarTypes(cppia,classSize);
            }
         }
         for(int i=0;i<dynamicFunctions.size();i++)
         {
            if (dynamicFunctions[i]->offset)
            {
               DBGLOG("   super dynamic function %s @ %d\n", cppia.identStr(dynamicFunctions[i]->nameId), dynamicFunctions[i]->offset);
            }
            else
            {
               DBGLOG("   link dynamic function %s @ %d\n", cppia.identStr(dynamicFunctions[i]->nameId), classSize);
               dynamicFunctions[i]->linkVarTypes(cppia,classSize);
            }
         }

         if (dynamicMapOffset==-1)
         {
            dynamicMapOffset = classSize;
            classSize += sizeof( hx::FieldMap * );
         }

         extraData = classSize - haxeBase->mDataOffset;
      }


      DBGLOG("  script member vars size = %d\n", extraData);
 
      for(int i=0;i<staticVars.size();i++)
      {
         DBGLOG("   link static var %s\n", cppia.identStr(staticVars[i]->nameId));
         staticVars[i]->linkVarTypes(cppia);
      }

      for(int i=0;i<staticDynamicFunctions.size();i++)
      {
         DBGLOG("   link dynamic static var %s\n", cppia.identStr(staticDynamicFunctions[i]->nameId));
         staticDynamicFunctions[i]->linkVarTypes(cppia);
      }


      // Combine vtable positions...
      DBGLOG("  format haxe callable vtable (%d)....\n", (int)memberFunctions.size());
      std::vector<std::string> table;
      if (haxeBase)
         haxeBase->addVtableEntries(table);
      for(int i=0;i<table.size();i++)
         DBGLOG("   base table[%d] = %s\n", i, table[i].c_str() );


      int vtableSlot = table.size();
      for(int i=0;i<memberFunctions.size();i++)
      {
         int idx = -1;
         for(int j=0;j<table.size();j++)
            if (table[j] == memberFunctions[i]->name)
            {
               idx = j;
               break;
            }
         if (idx<0)
         {
            if (isInterface)
            {
               idx = cppia.getInterfaceSlot(memberFunctions[i]->name);
               if (idx==-1)
                  throw "Missing function in interface";
               if (idx>interfaceSlotSize)
                  interfaceSlotSize = idx;
               idx = -idx;
               DBGLOG("Using interface vtable[%d] = %s\n", idx, memberFunctions[i]->name.c_str() );
            }
            else
            {
               idx = vtableSlot++;
               DBGLOG("   cppia slot [%d] = %s\n", idx, memberFunctions[i]->name.c_str() );
            }
         }
         else
            DBGLOG("   override slot [%d] = %s\n", idx, memberFunctions[i]->name.c_str() );
         memberFunctions[i]->setVTableSlot(idx);
      }


      // Create interface vtables...
      for(int i=0;i<implements.size();i++)
      {
         int id = implements[i];
         #if (HXCPP_API_LEVEL < 330)
         void **vtable = createInterfaceVTable(id);
         #endif
         while(id > 0)
         {
            TypeData *interface = cppia.types[id];
            CppiaClassInfo  *cppiaInterface = interface->cppiaClass;
            #if (HXCPP_API_LEVEL >= 330)
            HaxeNativeInterface *native = HaxeNativeInterface::findInterface( interface->name.__s );
            if (native)
            {
               interfaceScriptTables[interface->name.hash()] = native->scriptTable;

               ScriptNamedFunction *functions = native->functions;
               if (functions != 0)
               {
                  for(ScriptNamedFunction *f = functions; f->name; f++)
                  {
                     int slot = cppia.getInterfaceSlot(f->name);
                     if (slot<0)
                        printf("Interface slot '%s' not found\n",f->name);
                     if (slot>interfaceSlotSize)
                        interfaceSlotSize = slot;
                  }
               }
            }
            #else
            interfaceVTables[ interface->name.__s ] = vtable;
            #endif

            if (!cppiaInterface)
               break;

            Functions &intfFuncs = cppiaInterface->memberFunctions;
            for(int f=0;f<intfFuncs.size();f++)
            {
               int slot = cppia.getInterfaceSlot(intfFuncs[f]->name);
               if (slot<0)
                  printf("Interface slot '%s' not found\n",intfFuncs[f]->name.c_str());
               if (slot>interfaceSlotSize)
                  interfaceSlotSize = slot;
            }
            id =  cppiaInterface->superId;
         }
      }

      if (interfaceSlotSize)
         interfaceSlotSize++;

      vtable = new void*[vtableSlot + 2 + interfaceSlotSize];
      memset(vtable, 0, sizeof(void *)*(vtableSlot+2+interfaceSlotSize));
      vtable += interfaceSlotSize;
      *vtable++ = this;

      DBGLOG("  vtable size %d -> %p\n", vtableSlot, vtable);


      // Extract special function ...
      for(int i=0;i<staticFunctions.size(); )
      {
         if (staticFunctions[i]->name == "new")
         {
            newFunc = staticFunctions[i];
            staticFunctions.erase( staticFunctions.begin() + i);
         }
         else if (staticFunctions[i]->name == "__init__")
         {
            initFunc = staticFunctions[i]->funExpr;
            staticFunctions.erase( staticFunctions.begin() + i);
         }
         else
            i++;

      }

      for(int i=0;i<enumConstructors.size();i++)
      {
         CppiaEnumConstructor &e = *enumConstructors[i];
         e.name = cppia.strings[e.nameId];
         if (e.args.size()==0)
         {
            EnumBase base = new CppiaEnumBase(this);
            e.value = base;
            #if (HXCPP_API_LEVEL>=330)
            base->_hx_setIdentity(cppia.strings[e.nameId],i,0);
            #else
            base->__Set( cppia.strings[e.nameId],i,null() );
            #endif
         }
         else
         {
            e.index = i;
            e.value = createEnumClosure(e);
         }
      }

      if (!newFunc && cppiaSuper && cppiaSuper->newFunc)
      {
         //throw "No chaining constructor";
         newFunc = cppiaSuper->newFunc;
      }

      DBGLOG("  this constructor %p\n", newFunc);
   }

   #ifdef CPPIA_JIT
   void compile()
   {
      for(int i=0;i<staticFunctions.size();i++)
      {
         DBGLOG(" Compile %s::%s\n", name.c_str(), staticFunctions[i]->name.c_str() );
         staticFunctions[i]->compile();
      }
      if (newFunc)
         newFunc->compile();

      if (initFunc)
         initFunc->compile();

      // Functions
      for(int i=0;i<memberFunctions.size();i++)
      {
         DBGLOG(" Compile member %s::%s\n", name.c_str(), memberFunctions[i]->name.c_str() );
         memberFunctions[i]->compile();
      }
   }
   #endif

   void link()
   {
      int newPos = -1;
      for(int i=0;i<staticFunctions.size();i++)
      {
         DBGLOG(" Link %s::%s\n", name.c_str(), staticFunctions[i]->name.c_str() );
         staticFunctions[i]->link();
      }
      if (newFunc)
         newFunc->link();

      if (initFunc)
         initFunc = (ScriptCallable *)initFunc->link(cppia);

      // Functions
      for(int i=0;i<memberFunctions.size();i++)
      {
         DBGLOG(" Link member %s::%s\n", name.c_str(), memberFunctions[i]->name.c_str() );
         memberFunctions[i]->link();
      }

      for(int i=0;i<staticDynamicFunctions.size();i++)
         staticDynamicFunctions[i]->link(cppia);

      for(int i=0;i<dynamicFunctions.size();i++)
         dynamicFunctions[i]->link(cppia);

      for(int i=0;i<memberFunctions.size();i++)
      {
         vtable[ memberFunctions[i]->vtableSlot ] = memberFunctions[i]->funExpr;
         if (interfaceSlotSize)
         {
            int interfaceSlot = cppia.findInterfaceSlot( memberFunctions[i]->name );
            if (interfaceSlot>0 && interfaceSlot<interfaceSlotSize)
               vtable[ -interfaceSlot ] = memberFunctions[i]->funExpr;
         }
      }

      // Vars ...
      if (!isInterface)
      {
         for(int i=0;i<memberVars.size();i++)
         {
            CppiaVar &var = *memberVars[i];
            var.link(cppia);
            if (var.readAccess == CppiaVar::accCall || var.readAccess==CppiaVar::accCallNative)
            {
               ScriptCallable *getter = findFunction(false,HX_CSTRING("get_") + var.name);
               if (!getter)
               {
                  dump();
                  throw Dynamic(HX_CSTRING("Could not find getter for ") + var.name);
               }
               DBGLOG("  found getter for %s.%s\n", name.c_str(), var.name.__s);
               memberGetters[var.name.__s] = getter;
            }
            if (var.writeAccess == CppiaVar::accCall || var.writeAccess==CppiaVar::accCallNative)
            {
               ScriptCallable *setter = findFunction(false,HX_CSTRING("set_") + var.name);
               if (!setter)
                  throw Dynamic(HX_CSTRING("Could not find setter for ") + var.name);
               DBGLOG("  found setter for %s.%s\n", name.c_str(), var.name.__s);
               memberSetters[var.name.__s] = setter;
            }

            if (var.readAccess == CppiaVar::accCallNative || var.writeAccess == CppiaVar::accCallNative)
               nativeProperties.insert( var.name );
         }
      }
   
      for(int i=0;i<staticVars.size();i++)
      {
         CppiaVar &var = *staticVars[i];
         var.link(cppia);
         if (var.readAccess == CppiaVar::accCall || var.readAccess == CppiaVar::accCallNative)
         {
            ScriptCallable *getter = findFunction(true,HX_CSTRING("get_") + var.name);
            if (!getter)
               throw Dynamic(HX_CSTRING("Could not find getter for ") + var.name);
            DBGLOG("  found getter for %s.%s\n", name.c_str(), var.name.__s);
            staticGetters[var.name.__s] = getter;
         }
         if (var.writeAccess == CppiaVar::accCall || var.writeAccess == CppiaVar::accCallNative)
         {
            ScriptCallable *setter = findFunction(true,HX_CSTRING("set_") + var.name);
            if (!setter)
               throw Dynamic(HX_CSTRING("Could not find setter for ") + var.name);
            DBGLOG("  found setter for %s.%s\n", name.c_str(), var.name.__s);
            staticSetters[var.name.__s] = setter;
         }

         if (var.readAccess == CppiaVar::accCallNative || var.writeAccess == CppiaVar::accCallNative)
            nativeProperties.insert( var.name );
      }

      if (enumMeta)
         enumMeta = enumMeta->link(cppia);


      mClass->mStatics = GetClassFields();
      //printf("Found haxeBase %s = %p / %d\n", cppia.types[typeId]->name.__s, haxeBase, dataSize );
   }

   void init(CppiaCtx *ctx, int inPhase)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( (hx::Object *) 0 ); // this
      AutoStack save(ctx,pointer);
 
      if (inPhase==0)
      {
         for(int i=0;i<staticVars.size();i++)
         {
            staticVars[i]->runInit(ctx);
            if (staticVars[i]->name==HX_CSTRING("__meta__"))
            {
               // Todo - delete/clean value
               mClass->__meta__ = staticVars[i]->objVal;
               staticVars.erase( staticVars.begin() + i );
               i--;
            }
         }
         if (enumMeta)
            mClass->__meta__ = enumMeta->runObject(ctx);

         for(int i=0;i<staticDynamicFunctions.size();i++)
            staticDynamicFunctions[i]->runInit(ctx);
      }
      else if (inPhase==1)
      {
         if (initFunc)
            initFunc->runVoid(ctx);
      }
   }


   Dynamic getStaticValue(const String &inName,hx::PropertyAccess  inCallProp)
   {
      if (inCallProp==paccDynamic)
         inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;
      if (inCallProp)
      {
         ScriptCallable *getter = findStaticGetter(inName);
         if (getter)
         {
            Array<Dynamic> empty;
            return runFunExprDynamic(CppiaCtx::getCurrent(),getter,0, empty);
         }
      }

      CppiaExpr *closure = findFunction(true,inName);
      if (closure)
         return createMemberClosure(0,(ScriptCallable*)closure);

      // Look for dynamic function (variable)
      for(int i=0;i<staticDynamicFunctions.size();i++)
      {
         if (cppia.strings[ staticDynamicFunctions[i]->nameId  ]==inName)
         {
            CppiaVar *d = staticDynamicFunctions[i];

            return d->getStaticValue();
         }
      }

      for(int i=0;i<staticVars.size();i++)
      {
         CppiaVar &var = *staticVars[i];
         if (var.name==inName)
            return var.getStaticValue();
      }

      printf("Get static field not found (%s) %s\n", name.c_str(),inName.__s);
      return null();
   }


   bool hasStaticValue(const String &inName)
   {
      if (findStaticGetter(inName))
         return true;

      CppiaExpr *closure = findFunction(true,inName);
      if (closure)
         return true;

      // Look for dynamic function (variable)
      for(int i=0;i<staticDynamicFunctions.size();i++)
         if (cppia.strings[ staticDynamicFunctions[i]->nameId  ]==inName)
            return true;

      for(int i=0;i<staticVars.size();i++)
      {
         CppiaVar &var = *staticVars[i];
         if (var.name==inName)
            return true;
      }

      return false;
   }


   Dynamic setStaticValue(const String &inName,const Dynamic &inValue ,hx::PropertyAccess  inCallProp)
   {
      if (inCallProp==paccDynamic)
         inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;
      if (inCallProp)
      {
         ScriptCallable *setter = findStaticSetter(inName);
         if (setter)
         {
            Array<Dynamic> args = Array_obj<Dynamic>::__new(1,1);
            args[0] = inValue;
            return runFunExprDynamic(CppiaCtx::getCurrent(),setter,0, args);
         }
      }


      // Look for dynamic function (variable)
      for(int i=0;i<staticDynamicFunctions.size();i++)
      {
         if (cppia.strings[ staticDynamicFunctions[i]->nameId  ]==inName)
         {
            CppiaVar *d = staticDynamicFunctions[i];
            return d->setStaticValue(inValue);
         }
      }

      for(int i=0;i<staticVars.size();i++)
      {
         CppiaVar &var = *staticVars[i];
         if (var.name==inName)
         {
            return var.setStaticValue(inValue);
         }
      }

      printf("Set static field not found (%s) %s\n", name.c_str(), inName.__s);
      return null();

   }

   void GetInstanceFields(hx::Object *inObject, Array<String> &ioFields)
   {
      for(int i=0;i<memberVars.size();i++)
      {
         CppiaVar &var = *memberVars[i];
         ioFields->push(var.name);
      }

      if (inObject)
      {
         hx::FieldMap *map = inObject->__GetFieldMap();
         if (map)
            FieldMapAppendFields(map, ioFields);
      }
   }
   
   Array<String> GetClassFields()
   {
      Array<String> result = Array_obj<String>::__new();

      if (isEnum)
      {
         for(int i=0;i<enumConstructors.size();i++)
            result->push(enumConstructors[i]->name);
      }
      else
      {

         for(int i=0;i<staticVars.size();i++)
         {
            CppiaVar &var = *staticVars[i];
            if (var.isVirtual)
               continue;
            result->push(var.name);
         }

         for(int i=0;i<staticDynamicFunctions.size();i++)
         {
            CppiaVar &var = *staticDynamicFunctions[i];
            if (var.isVirtual)
               continue;
            result->push(var.name);
         }

         for(int i=0;i<staticFunctions.size();i++)
         {
            CppiaFunction &func = *staticFunctions[i];
            result->push(func.getName());
         }
      }

      return result;
   }

   inline Dynamic &getFieldMap(hx::Object *inThis)
   {
      return *(Dynamic *)( (char *)inThis + dynamicMapOffset );
   }


   inline void markInstance(hx::Object *inThis, hx::MarkContext *__inCtx)
   {
      if (dynamicMapOffset)
         HX_MARK_MEMBER(getFieldMap(inThis));

      for(int i=0;i<dynamicFunctions.size();i++)
         dynamicFunctions[i]->mark(inThis, __inCtx);
      // Only script members?
      for(int i=0;i<memberVars.size();i++)
         memberVars[i]->mark(inThis, __inCtx);
   }

#ifdef HXCPP_VISIT_ALLOCS
   inline void visitInstance(hx::Object *inThis, hx::VisitContext *__inCtx)
   {
      if (dynamicMapOffset)
         HX_VISIT_MEMBER(getFieldMap(inThis));

      for(int i=0;i<dynamicFunctions.size();i++)
         dynamicFunctions[i]->visit(inThis, __inCtx);
      for(int i=0;i<memberVars.size();i++)
         memberVars[i]->visit(inThis, __inCtx);
   }
#endif
};


bool TypeData::isClassOf(Dynamic inInstance)
{
   if (cppiaClass)
      return cppiaClass->mClass->VCanCast(inInstance.mPtr);
   else if (haxeClass.mPtr)
      return __instanceof(inInstance,haxeClass);

   return false;
}



void cppiaClassInit(CppiaClassInfo *inClass, CppiaCtx *ctx, int inPhase)
{
   inClass->init(ctx,inPhase);
}


void cppiaClassMark(CppiaClassInfo *inClass,hx::MarkContext *__inCtx)
{
   inClass->mark(__inCtx);
}
#ifdef HXCPP_VISIT_ALLOCS
void cppiaClassVisit(CppiaClassInfo *inClass,hx::VisitContext *__inCtx)
{
   inClass->visit(__inCtx);
}
#endif

// --- Enum Base ---
::hx::ObjectPtr<hx::Class_obj > CppiaEnumBase::__GetClass() const
{
   return classInfo->mClass;
}

::String CppiaEnumBase::GetEnumName( ) const
{
   return classInfo->mClass->mName;
}

::String CppiaEnumBase::__ToString() const
{
   #if (HXCPP_API_LEVEL>=330)
   return classInfo->mClass->mName + HX_CSTRING(".") + _hx_tag;
   #else
   return classInfo->mClass->mName + HX_CSTRING(".") + tag;
   #endif
}







class CppiaClass : public hx::Class_obj
{
public:
   CppiaClassInfo *info;

   CppiaClass(CppiaClassInfo *inInfo)
   {
      info = inInfo;
   }

   void linkClass(CppiaModule &inModule,String inName)
   {
      mName = inName;
      mSuper = info->getSuperClass();
      DBGLOG("LINK %p ########################### %s -> %p\n", this, mName.__s, mSuper );
      mStatics = Array_obj<String>::__new(0,0);


      Array<String> base;
      if (mSuper)
         base = (*mSuper)->mMembers;

      mMembers = base==null() ?Array_obj<String>::__new(0,0) : base->copy();

      for(int i=0;i<info->memberFunctions.size();i++)
      {
         CppiaFunction *func = info->memberFunctions[i];
         String name = func->getName();
         if (base==null() || base->Find(name)<0)
            mMembers->push(name);
      }

      for(int i=0;i<info->memberVars.size();i++)
      {
         CppiaVar *var = info->memberVars[i];
         if (var->isVirtual)
            continue;
         String name = inModule.strings[var->nameId];
         if (base==null() || base->Find(name)<0)
            mMembers->push( name );
      }

      for(int i=0;i<info->dynamicFunctions.size();i++)
      {
         CppiaVar *var = info->dynamicFunctions[i];
         if (var->isVirtual)
            continue;
         String name = inModule.strings[var->nameId];
         if (base==null() || base->Find(name)<0)
            mMembers->push(name);
      }

      bool overwrite = false;
      hx::Class old = hx::Class_obj::Resolve(inName);
      // Overwrite cppia classes
      if (old.mPtr && dynamic_cast<CppiaClass *>( old.mPtr ) )
         overwrite = true;
      registerScriptable(overwrite);
   }


   Dynamic ConstructEmpty()
   {
      if (info->isEnum)
         return Dynamic();

      Expressions none;
      return info->createInstance(CppiaCtx::getCurrent(),none,false);
   } 

   Dynamic ConstructArgs(hx::DynamicArray inArgs)
   {
      return info->createInstance(CppiaCtx::getCurrent(),inArgs);
   }

   bool __IsEnum() { return info->isEnum; }

   Dynamic ConstructEnum(String inName,hx::DynamicArray inArgs)
   {
      if (!info->isEnum)
         return Dynamic();

      int index = info->getEnumIndex(inName);
	   return info->enumConstructors[index]->create(inArgs);
   }

   bool VCanCast(hx::Object *inPtr)
   {
      if (!inPtr)
         return false;

      hx::Class c = inPtr->__GetClass();
      if (!c.mPtr)
         return false;

      if (info->isInterface)
      {
         // Can only be this cppia interface if it is a cppia class...
         CppiaClass *cppiaClass = dynamic_cast<CppiaClass *>(c.mPtr);
         if (!cppiaClass)
            return false;
         return cppiaClass->info->implementsInterface(info);
      }


      hx::Class_obj *classPtr = c.mPtr;
      while(classPtr)
      {
         if (classPtr==this)
            return true;
         if (info->isEnum)
            return false;
         classPtr = classPtr->GetSuper().mPtr;
      }

      return false;
   }


   hx::Val __Field(const String &inName,hx::PropertyAccess inCallProp)
   {
      if (inName==HX_CSTRING("__meta__"))
         return __meta__;
      return info->getStaticValue(inName,inCallProp);
   }

   hx::Val __SetField(const String &inName,const hx::Val &inValue ,hx::PropertyAccess inCallProp)
   {
      return info->setStaticValue(inName,inValue,inCallProp);
   }

   bool __HasField(const String &inName)
   {
      if (inName==HX_CSTRING("__meta__"))
         return __meta__!=null();

      return info->hasStaticValue(inName);
   }

};


hx::Class_obj *createCppiaClass(CppiaClassInfo *inInfo) { return new CppiaClass(inInfo); }
void  linkCppiaClass(hx::Class_obj *inClass, CppiaModule &cppia, String inName)
{
   ((CppiaClass *)inClass)->linkClass(cppia,inName);
}


int getScriptId(hx::Class inClass)
{
   hx::Class_obj *ptr = inClass.mPtr;
   if (!ptr)
      return 0;
   CppiaClass *cls = dynamic_cast<CppiaClass *>(ptr);
   if (!cls)
      return 0;
   return cls->info->cppia.scriptId;
}




CppiaExpr *convertToFunction(CppiaExpr *inExpr) { return new ScriptCallable(inExpr); }



class CppiaClosure : public hx::Object
{
public:
   inline void *operator new( size_t inSize, int inExtraDataSize )
     { return hx::InternalNew(inSize + inExtraDataSize,true); }
   inline void operator delete(void *,int) {}

   ScriptCallable *function;

   CppiaClosure(CppiaCtx *ctx, ScriptCallable *inFunction)
   {
      function = inFunction;

      unsigned char *base = ((unsigned char *)this) + sizeof(CppiaClosure);

      *(hx::Object **)base = ctx->getThis(false);

      for(int i=0;i<function->captureVars.size();i++)
      {
         CppiaStackVar *var = function->captureVars[i];
         int size = sTypeSize[var->expressionType];
         memcpy( base+var->capturePos, ctx->frame + var->fromStackPos, size );
      }
   }

   hx::Object **getThis() const
   {
      unsigned char *base = ((unsigned char *)this) + sizeof(CppiaClosure);
      return (hx::Object **)base;
   }

   // Create member closure...
   CppiaClosure(hx::Object *inThis, ScriptCallable *inFunction)
   {
      function = inFunction;
      *getThis() = inThis;
   }


   Dynamic doRun(CppiaCtx *ctx, int inHaveArgs)
   {
      function->addExtraDefaults(ctx,inHaveArgs);
      function->addStackVarsSpace(ctx);

      unsigned char *base = ((unsigned char *)this) + sizeof(CppiaClosure);
      *(hx::Object **)ctx->frame =  *(hx::Object **)base;

      for(int i=0;i<function->captureVars.size();i++)
      {
         CppiaStackVar *var = function->captureVars[i];
         int size = sTypeSize[var->expressionType];
         memcpy( ctx->frame+var->stackPos, base + var->capturePos, size);
      }

      switch(function->returnType)
      {
         case etFloat:
            return ctx->runFloat( function );
         case etInt:
            return ctx->runInt( function );
         case etString:
            return ctx->runString( function );
         case etObject:
            return ctx->runObject( function );
         default: break;
      }
      ctx->runVoid( function );
      return null();
   }

   void pushArg(CppiaCtx *ctx, int a, Dynamic inValue)
   {
      // Developer has used dynamic to call a closure with wrong # parameters
      if (a>=function->args.size())
         return;

      if (!inValue.mPtr && function->pushDefault(ctx,a) )
          return;

      switch(function->args[a].expressionType)
      {
         case etInt:
            ctx->pushInt(inValue->__ToInt());
            return;
         case etFloat:
            ctx->pushFloat(inValue->__ToDouble());
            break;
         case etString:
            ctx->pushString(inValue.mPtr ? inValue->toString() : String());
            break;
         default:
            ctx->pushObject(inValue.mPtr);
      }
   }



   Dynamic __Run(const Array<Dynamic> &inArgs)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();

      AutoStack a(ctx);
      ctx->pointer += sizeof(hx::Object *);

      int haveArgs = !inArgs.mPtr ? 0 : inArgs->length;
      if (haveArgs>function->argCount)
         throw sInvalidArgCount;

      for(int a=0; a<haveArgs; a++)
         pushArg(ctx,a,inArgs[a]);

      return doRun(ctx,haveArgs);
   }

   Dynamic __run()
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack a(ctx);
      ctx->pointer += sizeof(hx::Object *);
      return doRun(ctx,0);
   }

   Dynamic __run(D a)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArg(ctx,0,a);
      return doRun(ctx,1);
   }
   Dynamic __run(D a,D b)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArg(ctx,0,a);
      pushArg(ctx,1,b);
      return doRun(ctx,2);
   }
   Dynamic __run(D a,D b,D c)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArg(ctx,0,a);
      pushArg(ctx,1,b);
      pushArg(ctx,2,c);
      return doRun(ctx,3);
   }
   Dynamic __run(D a,D b,D c,D d)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArg(ctx,0,a);
      pushArg(ctx,1,b);
      pushArg(ctx,2,c);
      pushArg(ctx,3,d);
      return doRun(ctx,4);

   }
   Dynamic __run(D a,D b,D c,D d,D e)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArg(ctx,0,a);
      pushArg(ctx,1,b);
      pushArg(ctx,2,c);
      pushArg(ctx,3,d);
      pushArg(ctx,4,e);
      return doRun(ctx,5);
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(*getThis());
      char *base = ((char *)this) + sizeof(CppiaClosure);
      for(int i=0;i<function->captureVars.size();i++)
         function->captureVars[i]->markClosure(base,__inCtx);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(*getThis());
      char *base = ((char *)this) + sizeof(CppiaClosure);
      for(int i=0;i<function->captureVars.size();i++)
         function->captureVars[i]->visitClosure(base,__inCtx);
   }
#endif
   virtual void *__GetHandle() const { return *getThis(); }

   int __Compare(const hx::Object *inRHS) const
   {
      const CppiaClosure *other = dynamic_cast<const CppiaClosure *>(inRHS);
      if (!other)
         return -1;
      return (function==other->function && *getThis()==*other->getThis())? 0 : -1;
   }



   int __GetType() const { return vtFunction; }
   int __ArgCount() const { return function->args.size(); }

   String toString() { return HX_CSTRING("function"); }
};


hx::Object *createClosure(CppiaCtx *ctx, ScriptCallable *inFunction)
{
   return new (inFunction->captureSize) CppiaClosure(ctx,inFunction);
}

hx::Object *createMemberClosure(hx::Object *inThis, ScriptCallable *inFunction)
{
   return new (sizeof(hx::Object *)) CppiaClosure(inThis,inFunction);
}


class EnumConstructorClosure : public hx::Object
{
public:
   CppiaEnumConstructor &constructor;

   EnumConstructorClosure(CppiaEnumConstructor &c) : constructor(c)
   {
   }

   Dynamic __Run(const Array<Dynamic> &inArgs)
   {
      return constructor.create(inArgs);
   }

   int __GetType() const { return vtFunction; }
   int __ArgCount() const { return constructor.args.size(); }

   String toString() { return HX_CSTRING("#function(") + constructor.name + HX_CSTRING(")"); }
};



hx::Object *createEnumClosure(CppiaEnumConstructor &inContructor)
{
   return new EnumConstructorClosure(inContructor);
}



void CppiaModule::where(CppiaExpr *e)
{
   if (linkingClass && layout)
      printf(" in %s::%p\n", linkingClass->name.c_str(), layout);
   printf("   %s at %s:%d %s\n", e->getName(), e->filename, e->line, e->functionName);
}




void runFunExpr(CppiaCtx *ctx, ScriptCallable *inFunExpr, hx::Object *inThis, Expressions &inArgs )
{
   unsigned char *pointer = ctx->pointer;
   inFunExpr->pushArgs(ctx, inThis, inArgs);
   BCR_VCHECK;
   AutoStack save(ctx,pointer);
   ctx->runVoid( inFunExpr );
}


hx::Object *runFunExprDynamic(CppiaCtx *ctx, ScriptCallable *inFunExpr, hx::Object *inThis, Array<Dynamic> &inArgs )
{
   unsigned char *pointer = ctx->pointer;
   inFunExpr->pushArgsDynamic(ctx, inThis, inArgs);
   AutoStack save(ctx,pointer);
   return runContextConvertObject(ctx, inFunExpr->getType(), inFunExpr );
}


void runFunExprDynamicVoid(CppiaCtx *ctx, ScriptCallable *inFunExpr, hx::Object *inThis, Array<Dynamic> &inArgs )
{
   unsigned char *pointer = ctx->pointer;
   inFunExpr->pushArgsDynamic(ctx, inThis, inArgs);
   AutoStack save(ctx,pointer);
   ctx->runVoid(inFunExpr);
}



struct BlockCallable : public ScriptCallable
{
   BlockCallable(CppiaExpr *inExpr) : ScriptCallable(inExpr)
   {
   }

   ExprType getType() { return body->getType(); }

   void runVoid(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      body->runVoid(ctx);
   }
   int runInt(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      return body->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      return body->runFloat(ctx);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      return body->runObject(ctx);
   }
};


struct BlockExpr : public CppiaExpr
{
   Expressions expressions;

   BlockExpr(CppiaStream &stream)
   {
      ReadExpressions(expressions,stream);
   }

   CppiaExpr *link(CppiaModule &data)
   {
      if (data.layout==0)
      {
         CppiaExpr *blockFunc = new BlockCallable(this);
         blockFunc->link(data);
         return blockFunc;
      }

      LinkExpressions(expressions,data);
      return this;
   }

   const char *getName() { return "BlockExpr"; }
   virtual ExprType getType()
   {
      if (expressions.size()==0)
         return etNull;
      return expressions[expressions.size()-1]->getType();
   }

   #define BlockExprRun(ret,name,defVal) \
     ret name(CppiaCtx *ctx) \
     { \
        int last = expressions.size()-1; \
        for(int a=0;a<last;a++) \
        { \
          CPPIA_STACK_LINE(expressions[a]); \
          expressions[a]->runVoid(ctx); \
          BCR_CHECK; \
        } \
        if (last>=0) \
           return expressions[last]->name(ctx); \
        return defVal; \
     }
   BlockExprRun(int,runInt,0)
   BlockExprRun(Float ,runFloat,0)
   BlockExprRun(String,runString,null())
   BlockExprRun(hx::Object *,runObject,0)
   void  runVoid(CppiaCtx *ctx)
   {
      CppiaExpr **e = &expressions[0];
      CppiaExpr **end = e+expressions.size();
      for(;e<end && !ctx->breakContReturn;e++)
      {
         CPPIA_STACK_LINE((*e));
         (*e)->runVoid(ctx);
      }
   }


   #ifdef CPPIA_JIT
   void preGen(CppiaCompiler &compiler)
   {
      for(int i=0;i<expressions.size();i++)
          expressions[i]->preGen(compiler);
   }

   void genCode(CppiaCompiler &compiler, const Addr &inDest, ExprType resultType)
   {
      int n = expressions.size();
      for(int i=0;i<n;i++)
      {
         if (i<n-1 || resultType==etVoid)
            expressions[i]->genCode(compiler, AddrVoid(), etVoid);
         else
         {
            // TODO - store save register?
            expressions[i]->genCode(compiler, inDest, resultType);
         }
      }
   }
   #endif

};

struct IfElseExpr : public CppiaExpr
{
   CppiaExpr *condition;
   CppiaExpr *doIf;
   CppiaExpr *doElse;

   const char *getName() { return "IfElseExpr"; }
   IfElseExpr(CppiaStream &stream)
   {
      condition = createCppiaExpr(stream);
      doIf = createCppiaExpr(stream);
      doElse = createCppiaExpr(stream);
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      condition = condition->link(inModule);
      doIf = doIf->link(inModule);
      doElse = doElse->link(inModule);
      return this;
   }

   void runVoid(CppiaCtx *ctx)
   {
      if (condition->runInt(ctx))
         doIf->runVoid(ctx);
      else
         doElse->runVoid(ctx);
   }
   #define IF_ELSE_RUN(TYPE,NAME) \
   TYPE NAME(CppiaCtx *ctx) \
   { \
      if (condition->runInt(ctx)) \
      { \
         BCR_CHECK; \
         return doIf->NAME(ctx); \
      } \
      BCR_CHECK; \
      return doElse->NAME(ctx); \
   }
   IF_ELSE_RUN(hx::Object *,runObject)
   IF_ELSE_RUN(int,runInt)
   IF_ELSE_RUN(String,runString)
   IF_ELSE_RUN(Float,runFloat)
};


struct IfExpr : public CppiaDynamicExpr
{
   CppiaExpr *condition;
   CppiaExpr *doIf;

   IfExpr(CppiaStream &stream)
   {
      condition = createCppiaExpr(stream);
      doIf = createCppiaExpr(stream);
   }

   const char *getName() { return "IfExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      condition = condition->link(inModule);
      doIf = doIf->link(inModule);
      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   void runVoid(CppiaCtx *ctx)
   {
      if (condition->runInt(ctx))
      {
         BCR_VCHECK;
         doIf->runVoid(ctx);
      }
   }
};


struct CppiaIsNull : public CppiaBoolExpr
{
   CppiaExpr *condition;

   CppiaIsNull(CppiaStream &stream) { condition = createCppiaExpr(stream); }

   const char *getName() { return "IsNull"; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      condition = condition->link(inModule);
      return this;
   }

   int runInt(CppiaCtx *ctx)
   {
      return condition->runObject(ctx)==0;
   }

};


struct CppiaIsNotNull : public CppiaBoolExpr
{
   CppiaExpr *condition;

   CppiaIsNotNull(CppiaStream &stream) { condition = createCppiaExpr(stream); }

   const char *getName() { return "IsNotNull"; }
   CppiaExpr *link(CppiaModule &inModule) { condition = condition->link(inModule); return this; }

   int runInt(CppiaCtx *ctx)
   {
      return condition->runObject(ctx)!=0;
   }
};


#ifdef CPPIA_JIT
void convertResult(CppiaCompiler &compiler, const Addr &dest, ExprType destType, ExprType srcType)
{
   CtxMemberVal returnAddr(offsetof(CppiaCtx,frame));
   compiler.move( TempReg(), returnAddr );
   StarAddr returnVal(TempReg(),0);

   // hmm
   if (srcType==etVoid)
      return;

      switch(destType)
      {
         case etInt:
            switch(srcType)
            {
               case etInt:
                  compiler.move32( dest, StarAddr(Reg(0)) );
                  break;
               case etFloat:
                  compiler.emitf( SLJIT_CONVI_FROMD, dest, returnVal );
                  break;
               case etObject:
                  compiler.move( Reg(0), returnVal );
                  compiler.call( objectToInt, 1 );
                  if (dest!=Reg(0))
                     compiler.move( dest, Reg(0) );
                  break;
               case etString:
                  // Hmm
                  break;
               default: ;
            }
            break;

         case etFloat:
            switch(srcType)
            {
               case etInt:
                  compiler.emitf( SLJIT_CONVD_FROMI, dest, returnVal );
                  break;
               case etFloat:
                  compiler.emitf( SLJIT_DMOV, dest, returnVal );
                  break;
               case etObject:
                  compiler.move( Reg(0), CtxReg() );
                  compiler.call( objectToDouble, 1 );
                  compiler.emitf( SLJIT_DMOV, dest, returnVal );
                  break;
               case etString:
                  // Hmm
                  break;
               default: ;
            }
            break;

         case etObject:
            switch(srcType)
            {
               case etInt:
                  compiler.move( Reg(0), CtxReg() );
                  compiler.call( intToObject, 1 );
                  break;
               case etFloat:
                  compiler.move( Reg(0), CtxReg() );
                  compiler.call( doubleToObject, 1 );
                  break;
               case etObject:
                  break;
               case etString:
                  compiler.move( Reg(0), CtxReg() );
                  compiler.call( stringToObject, 1 );
                  break;
               default: ;
            }
            compiler.move( dest, returnVal );
            break;
 


         case etString:
            switch(srcType)
            {
               case etInt:
                  // Hmm
                  break;
               case etFloat:
                  // Hmm
                  break;
               case etObject:
                  break;
               case etString:
                  compiler.move( Reg(0), CtxReg() );
                  compiler.call( objectToString, 1 );
               default: ;
            }
            compiler.move32( dest,returnVal );
            compiler.move( dest.offset(4),returnVal.offset(4) );
            break;
 
         default: ;
      }
}
#endif



struct CallFunExpr : public CppiaExpr
{
   Expressions args;
   CppiaExpr   *thisExpr;
   ScriptCallable     *function;
   ExprType    returnType;

   CallFunExpr(const CppiaExpr *inSrc, CppiaExpr *inThisExpr, ScriptCallable *inFunction, Expressions &ioArgs )
      : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      function = inFunction;
      thisExpr = inThisExpr;
      returnType = etVoid;
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      LinkExpressions(args,inModule);
      // Should already be linked
      //function = (ScriptCallable *)function->link(inModule);
      if (thisExpr)
         thisExpr = thisExpr->link(inModule);
      returnType = inModule.types[ function->returnTypeId ]->expressionType;
      return this;
   }

   const char *getName() { return "CallFunExpr"; }
   ExprType getType() { return returnType; }

   #define CallFunExprVal(ret,name,funcName) \
   ret name(CppiaCtx *ctx) \
   { \
      unsigned char *pointer = ctx->pointer; \
      function->pushArgs(ctx,thisExpr?thisExpr->runObject(ctx):ctx->getThis(false),args); \
      BCR_CHECK; \
      AutoStack save(ctx,pointer); \
      return funcName(ctx, function->getType(), function); \
   }
   CallFunExprVal(int,runInt, runContextConvertInt);
   CallFunExprVal(Float ,runFloat, runContextConvertFloat);
   CallFunExprVal(String,runString, runContextConvertString);
   CallFunExprVal(hx::Object *,runObject,  runContextConvertObject);


   void runVoid(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      function->pushArgs(ctx,thisExpr?thisExpr->runObject(ctx):ctx->getThis(false),args);
      if (ctx->breakContReturn) return;


      AutoStack save(ctx,pointer);
      ctx->runVoid(function);
   }

   #ifdef CPPIA_JIT
   void preGen(CppiaCompiler &compiler)
   {
      AllocTemp pointer(compiler);
      AllocTemp frame(compiler);
      function->preGenArgs(compiler,thisExpr, args);
   }

   static void SLJIT_CALL callScriptable(CppiaCtx *inCtx, ScriptCallable *inScriptable)
   {
      // compiled?
      printf("callScriptable %p(%p) -> %p\n", inCtx, CppiaCtx::getCurrent(), inScriptable );
      printf(" name = %s\n", inScriptable->getName());
      inScriptable->runFunction(inCtx);
      printf(" Done scipt callable\n");
   }

   void genCode(CppiaCompiler &compiler, const Addr &inDest, ExprType resultType)
   {
      compiler.trace("Function called implementation");

      /*
      CTmp sp = cSp;
      CTmp frame = cFrame;
      pushArgs();
      cFrame = cSp;
      cTrace("set frame");
      cCall(callScriptable, ctx, ConstValue(function) );
      cSp = sp;
      cFrame = frame;
      cCheckException(); // if stack.exp (goto handler or return)
      cConvert(inDest,resultType,function->getType() );
      */



      AllocTemp pointer(compiler);
      AllocTemp frame(compiler);

      // AutoStack
      compiler.move(pointer, CtxMemberVal(offsetof(CppiaCtx,pointer) ) );
      compiler.move(frame, CtxMemberVal(offsetof(CppiaCtx,frame) ) );
 
      compiler.trace("gen args...");
      // Push args
      function->genArgs(compiler,thisExpr,args);

      compiler.trace("set frame...");
      // Set frame=pointer for new function
      compiler.move(CtxMemberVal(offsetof(CppiaCtx,frame) ), pointer );

      compiler.trace("Call out...");
      // call function / leave hole for calling?
      // Result is at pointer
      compiler.move( Reg(0), CtxReg() );

      // TODO - compiled version
      compiler.move( Reg(1), ConstValue(function) );
      compiler.call( callScriptable, 2 );

      compiler.trace("Restore stack...");
      // ~AutoStack
      compiler.move(CtxMemberVal(offsetof(CppiaCtx,frame) ), frame );
      compiler.move(CtxMemberVal(offsetof(CppiaCtx,pointer) ), pointer );

      if (compiler.exceptionHandler)
      {
         sljit_jump *notZero = compiler.ifNotZero( CtxMemberVal(offsetof(CppiaCtx,exception)) );
         compiler.exceptionHandler->push_back(notZero);
      }
      else
      {
         sljit_jump *isZero = compiler.ifZero( CtxMemberVal(offsetof(CppiaCtx,exception)) );
         compiler.ret( );
         compiler.jumpHere(isZero);
      }

      convertResult(compiler, inDest, resultType, function->getType() );
   }
   #endif

};

// ---


struct CppiaExprWithValue : public CppiaDynamicExpr
{
   Dynamic     value;

   CppiaExprWithValue(const CppiaExpr *inSrc=0) : CppiaDynamicExpr(inSrc)
   {
      value.mPtr = 0;
   }

   hx::Object *runObject(CppiaCtx *ctx) { return value.mPtr; }
   void mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(value); }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(value); }
#endif

   const char *getName() { return "CppiaExprWithValue"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      inModule.markable.push_back(this);
      return this;
   }

   void runVoid(CppiaCtx *ctx) { runObject(ctx); }

};

// ---


static int idx = 0;

struct CallDynamicFunction : public CppiaExprWithValue
{
   Expressions args;

   CallDynamicFunction(CppiaModule &inModule, const CppiaExpr *inSrc,
                       Dynamic inFunction, Expressions &ioArgs )
      : CppiaExprWithValue(inSrc)
   {
      args.swap(ioArgs);
      value = inFunction;
      inModule.markable.push_back(this);
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      LinkExpressions(args,inModule);
      return CppiaExprWithValue::link(inModule);
   }

   const char *getName() { return "CallDynamicFunction"; }
   ExprType getType() { return etObject; }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      int n = args.size();
      switch(n)
      {
         case 0:
            return value->__run().mPtr;
         case 1:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               BCR_CHECK;
               return value->__run(arg0).mPtr;
            }
         case 2:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg1( args[1]->runObject(ctx) );
               BCR_CHECK;
               return value->__run(arg0,arg1).mPtr;
            }
         case 3:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg1( args[1]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg2( args[2]->runObject(ctx) );
               BCR_CHECK;
               return value->__run(arg0,arg1,arg2).mPtr;
            }
         case 4:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg1( args[1]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg2( args[2]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg3( args[3]->runObject(ctx) );
               BCR_CHECK;
               return value->__run(arg0,arg1,arg2,arg3).mPtr;
            }
         case 5:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg1( args[1]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg2( args[2]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg3( args[3]->runObject(ctx) );
               BCR_CHECK;
               Dynamic arg4( args[4]->runObject(ctx) );
               BCR_CHECK;
               return value->__run(arg0,arg1,arg2,arg3,arg4).mPtr;
            }
      }

      Array<Dynamic> argVals = Array_obj<Dynamic>::__new(n,n);
      for(int a=0;a<n;a++)
      {
         argVals[a] = Dynamic( args[a]->runObject(ctx) );
         BCR_CHECK;
      }
      return value->__Run(argVals).mPtr;
   }

   int runInt(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      return result ? result->__ToInt() : 0;
   }
   Float  runFloat(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      return result ? result->__ToDouble() : 0;
   }
   String runString(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      BCR_CHECK;
      return result ? result->toString() : String();
   }
};

struct SetExpr : public CppiaExpr
{
   CppiaExpr *lvalue;
   CppiaExpr *value;
   AssignOp op;

   SetExpr(CppiaStream &stream,AssignOp inOp)
   {
      op = inOp;
      lvalue = createCppiaExpr(stream);
      value = createCppiaExpr(stream);
   }

   const char *getName() { return "SetExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      lvalue = lvalue->link(inModule);
      value = value->link(inModule);
      CppiaExpr *result = lvalue->makeSetter(op,value);
      if (!result)
      {
         printf("Could not makeSetter.\n");
         inModule.where(lvalue);
         throw "Bad Set expr";
      }
      delete this;
      return result;
   }

};

#if (HXCPP_API_LEVEL < 330)
class CppiaInterface : public hx::Interface
{
   typedef CppiaInterface __ME;
   typedef hx::Interface super;
   HX_DEFINE_SCRIPTABLE_INTERFACE
};
#endif

enum CastOp
{
   castNOP,
   castDynamic,
   castDataArray,
   castDynArray,
   castInt,
   castBool,
};

struct CastExpr : public CppiaDynamicExpr
{
   CppiaExpr *value;
   CastOp  op;
   int     typeId;
   ArrayType arrayType;

   CastExpr(CppiaStream &stream,CastOp inOp)
   {
      op = inOp;
      typeId = 0;
      if (op==castDataArray)
         typeId = stream.getInt();

      value = createCppiaExpr(stream);
   }
   ExprType getType() { return op==castInt ? etInt : etObject; }

   template<typename T>
   hx::Object *convert(hx::Object *obj)
   {
      Array_obj<T> *alreadyGood = dynamic_cast<Array_obj<T> *>(obj);
      if (alreadyGood)
         return alreadyGood;
      #if (HXCPP_API_LEVEL>=330)
      cpp::VirtualArray_obj *varray = dynamic_cast<cpp::VirtualArray_obj *>(obj);
      if (varray)
      {
         return Array<T>( cpp::VirtualArray(varray) ).mPtr;
      }
      #endif
      int n = obj->__length();
      Array<T> result = Array_obj<T>::__new(n,n);
      for(int i=0;i<n;i++)
         result[i] = obj->__GetItem(i);
      return result.mPtr;
   }

   int runInt(CppiaCtx *ctx)
   {
      return value->runInt(ctx);
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (op==castInt)
         return Dynamic(value->runInt(ctx)).mPtr;
      else if (op==castBool)
         return Dynamic((bool)value->runInt(ctx)).mPtr;

      hx::Object *obj = value->runObject(ctx);
      if (!obj)
         return 0;
      if (op==castDynamic)
         return obj->__GetRealObject();

      switch(arrayType)
      {
         case arrBool:         return convert<bool>(obj);
         case arrUnsignedChar: return convert<unsigned char>(obj);
         case arrInt:          return convert<int>(obj);
         case arrFloat:        return convert<Float>(obj);
         case arrString:       return convert<String>(obj);
         #if (HXCPP_API_LEVEL>=330)
         case arrAny:
         {
            ArrayBase *base = dynamic_cast<ArrayBase *>(obj);
            if (base)
               return new cpp::VirtualArray_obj(base);
            return dynamic_cast<cpp::VirtualArray_obj *>(obj);
         }
         case arrObject:       return convert<Dynamic>(obj);
         #else
         case arrAny:          return convert<Dynamic>(obj);
         case arrObject:       return obj;
         #endif
         case arrNotArray:     throw "Bad cast";
      }
      return 0;
   }

   const char *getName() { return "CastExpr"; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      value = value->link(inModule);

      if (op==castNOP)
      {
         delete this;
         return value;
      }

      if (op==castDynamic || op==castInt || op==castBool )
      {
         return this;
         //CppiaExpr *replace = value;
         //delete this;
         //return replace;
      }
      if (op==castDataArray)
      {
         TypeData *t = inModule.types[typeId];
         arrayType = t->arrayType;
         if (arrayType==arrNotArray)
         {
            printf("Cast to %d, %s\n", typeId, t->name.__s);
            throw "Data cast to non-array";
         }
      }
      else
         arrayType = arrObject;
      return this;
   }
};


struct ToInterface : public CppiaDynamicExpr
{
   int       fromTypeId;
   int       toTypeId;
   CppiaExpr *value;
   bool      useNative;
   bool      array;
   HaxeNativeInterface *interfaceInfo;
   void      **cppiaVTable;
   TypeData *toType;


   ToInterface(CppiaStream &stream,bool inArray)
   {
      toTypeId = stream.getInt();
      array = inArray;
      fromTypeId = array ? 0 : stream.getInt();
      value = createCppiaExpr(stream);
      interfaceInfo = 0;
      useNative = false;
      cppiaVTable = 0;
      toType = 0;
   }

   const char *getName() { return array ? "ToInterfaceArray" : "ToInterface"; }

   #if (HXCPP_API_LEVEL >= 330)
   CppiaExpr *link(CppiaModule &inModule)
   {
      DBGLOG("Api 330 - no cast required\n");
      CppiaExpr *linked = value->link(inModule);
      delete this;
      return linked;
   }
   hx::Object *runObject(CppiaCtx *ctx) { return 0; }

   #else
   CppiaExpr *link(CppiaModule &inModule)
   {
      toType = inModule.types[toTypeId];
      TypeData *fromType = fromTypeId ? inModule.types[fromTypeId] : 0;

      if (toType->interfaceBase)
      {
         interfaceInfo = toType->interfaceBase;
         if (!fromType)
         {
            useNative = true;
         }
         else if (!fromType->cppiaClass)
         {
            DBGLOG("native -> native\n");
            useNative = true;
         }
         else
         {
            DBGLOG("cppia class, native interface\n");
            cppiaVTable = fromType->cppiaClass->getInterfaceVTable(toType->interfaceBase->name);
         }
         value = value->link(inModule);
         return this;
      }


      DBGLOG("cppia class, cppia interface - no cast required\n");

      CppiaExpr *linked = value->link(inModule);
      delete this;
      return linked;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *obj = value->runObject(ctx);
      if (!obj)
         return 0;
      if (obj)
         obj = obj->__GetRealObject();

      if (cppiaVTable)
      {
         if (array)
         {
            CPPIA_CHECK(obj);
            int n = obj->__length();
            Array<Dynamic> result = Array_obj<Dynamic>::__new(n,n);
            for(int i=0;i<n;i++)
               result[i] = interfaceInfo->factory(cppiaVTable,obj->__GetItem(i)->__GetRealObject());
            return result.mPtr;
         }
         return interfaceInfo->factory(cppiaVTable,obj);
      }
      hx::Object *result = obj->__ToInterface(*interfaceInfo->mType);
      return result;
   }
   #endif
};

struct NewExpr : public CppiaDynamicExpr
{
   int classId;
   TypeData *type;
   Expressions args;
	hx::ConstructArgsFunc constructor;


   NewExpr(CppiaStream &stream)
   {
      classId = stream.getInt();
      constructor = 0;
      ReadExpressions(args,stream);
   }

   const char *getName() { return "NewExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      type = inModule.types[classId];
      if (!type->cppiaClass && type->haxeClass.mPtr)
         constructor = type->haxeClass.mPtr->mConstructArgs;

      LinkExpressions(args,inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (type->arrayType)
      {
         int size = 0;
         if (args.size()==1)
            size = args[0]->runInt(ctx);
         switch(type->arrayType)
         {
            case arrBool:
               return Array_obj<bool>::__new(size,size).mPtr;
            case arrUnsignedChar:
               return Array_obj<unsigned char>::__new(size,size).mPtr;
            case arrInt:
               return Array_obj<int>::__new(size,size).mPtr;
            case arrFloat:
               return Array_obj<Float>::__new(size,size).mPtr;
            case arrString:
               return Array_obj<String>::__new(size,size).mPtr;
            case arrAny:
               #if (HXCPP_API_LEVEL>=330)
               return cpp::VirtualArray_obj::__new(size,size).mPtr;
               #else
               // Fallthrough
               #endif
            case arrObject:
               return Array_obj<Dynamic>::__new(size,size).mPtr;
            default:
               return 0;
         }
      }
      else if (constructor)
      {
         int n = args.size();
         Array< Dynamic > argList = Array_obj<Dynamic>::__new(n,n);
         for(int a=0;a<n;a++)
            argList[a].mPtr = args[a]->runObject(ctx);

          return constructor(argList).mPtr;
      }
      else
      {
         return type->cppiaClass->createInstance(ctx,args);
      }

      printf("Can't create non haxe type\n");
      return 0;
   }
   
};

template<typename T>
inline void SetVal(null &out, const T &value) {  }
template<typename T>
inline void SetVal(int &out, const T &value) { out = value; }
inline void SetVal(int &out, const String &value) { out = 0; }
template<typename T>
inline void SetVal(Float &out, const T &value) { out = value; }
inline void SetVal(Float &out, const String &value) { out = 0; }
template<typename T>
inline void SetVal(String &out, const T &value) { out = String(value); }
template<typename T>
inline void SetVal(Dynamic &out, const T &value) { out = value; }

//template<typname RETURN>
struct CallHaxe : public CppiaExpr
{
   Expressions args;
   CppiaExpr *thisExpr;
   ScriptFunction function;
   ExprType returnType;

   CallHaxe(CppiaExpr *inSrc,ScriptFunction inFunction, CppiaExpr *inThis, Expressions &ioArgs )
       : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      thisExpr = inThis;
      function = inFunction;
   }
   ExprType getType() { return returnType; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (strlen(function.signature) != args.size()+1)
         throw "CallHaxe: Invalid arg count";
      for(int i=0;i<args.size()+1;i++)
      {
         switch(function.signature[i])
         {
            case sigInt: case sigFloat: case sigString: case sigObject:
               break; // Ok
            case sigVoid: 
               if (i==0) // return void ok
                  break;
               // fallthough
            default:
               throw "Bad haxe signature";
         }
      }
      switch(function.signature[0])
      {
         case sigInt: returnType = etInt; break;
         case sigFloat: returnType = etFloat; break;
         case sigString: returnType = etString; break;
         case sigObject: returnType = etObject; break;
         case sigVoid: returnType = etVoid; break;
         default: ;
      }

      if (thisExpr)
         thisExpr = thisExpr->link(inModule);
      LinkExpressions(args,inModule);

      return this;
   }

   const char *getName() { return "CallHaxe"; }

   template<typename T>
   void run(CppiaCtx *ctx,T &outValue)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->pushObject(thisExpr ? thisExpr->runObject(ctx) : ctx->getThis(false));

      const char *s = function.signature+1;
      for(int a=0;a<args.size();a++)
      {
         CppiaExpr *arg = args[a];
         switch(*s++)
         {
            case sigInt: ctx->pushInt( arg->runInt(ctx) ); break;
            case sigFloat: ctx->pushFloat( arg->runFloat(ctx) ); break;
            case sigString: ctx->pushString( arg->runString(ctx) ); break;
            case sigObject: ctx->pushObject( arg->runObject(ctx) ); break;
            default: ;// huh?
         }
      }

      AutoStack a(ctx,pointer);
      function.execute(ctx);

      #ifdef DEBUG_RETURN_TYPE
      gLastRet = returnType;
      #endif
      if (sizeof(outValue)>0)
      {
         if (returnType == etInt)
            SetVal(outValue,ctx->getInt());
         else if (returnType == etFloat)
            SetVal(outValue,ctx->getFloat());
         else if (returnType == etString)
            SetVal(outValue,ctx->getString());
         else if (returnType == etObject)
            SetVal(outValue,ctx->getObject());
      }
   }

   void runVoid(CppiaCtx *ctx)
   {
      null val;
      run(ctx,val);
   }
   int runInt(CppiaCtx *ctx)
   {
      int val;
      run(ctx,val);
      return val;
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float val;
      run(ctx,val);
      return val;
   }
   String runString(CppiaCtx *ctx)
   {
      String val;
      run(ctx,val);
      return val;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      Dynamic val;
      run(ctx,val);
      return val.mPtr;
   }
};

struct CallStatic : public CppiaExpr
{
   int classId;
   int fieldId;
   Expressions args;
  
   CallStatic(CppiaStream &stream)
   {
      classId = stream.getInt();
      fieldId = stream.getInt();
      ReadExpressions(args,stream);
   }

   const char *getName() { return "CallStatic"; }
   CppiaExpr *link(CppiaModule &inModule)
   {

      TypeData *type = inModule.types[classId];
      String field = inModule.strings[fieldId];

      CppiaExpr *replace = 0;
      if (type->cppiaClass)
      {
         ScriptCallable *func = (ScriptCallable *)type->cppiaClass->findFunction(true,fieldId);
         if (!func)
         {
            printf("Could not find static function %s in %s\n", field.__s, type->name.__s);
         }
         else
         {
            replace = new CallFunExpr( this, 0, func, args );
         }
      }

      if (!replace && type->haxeClass.mPtr)
      {
         //const StaticInfo *info = type->haxeClass->GetStaticStorage(field);
         //printf("INFO %s -> %p\n", field.__s,  info);
         // TODO - create proper glue for static functions
         Dynamic func = type->haxeClass.mPtr->__Field( field, HX_PROP_NEVER );
         if (func.mPtr)
            replace = new CallDynamicFunction(inModule, this, func, args );
      }

      // TODO - optimise...
      if (!replace && type->name==HX_CSTRING("String") && field==HX_CSTRING("fromCharCode"))
         replace = new CallDynamicFunction(inModule, this, String::fromCharCode_dyn(), args );
         

      //printf(" static call to %s::%s (%d)\n", type->name.__s, field.__s, type->cppiaClass!=0);
      if (replace)
      {
         delete this;
         replace->link(inModule);
         return replace;
      }

      printf("Unknown static call to %s::%s (%d)\n", type->name.__s, field.__s, type->cppiaClass!=0);
      inModule.where(this);
      throw "Bad link";
      return this;
   }
};



struct CallGetIndex : public CppiaIntExpr
{
   CppiaExpr   *thisExpr;
  
   CallGetIndex(CppiaExpr *inSrc, CppiaExpr *inThis) : CppiaIntExpr(inSrc)
   {
      thisExpr = inThis;
   }

   const char *getName() { return "__Index"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      thisExpr = thisExpr->link(inModule);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      hx::Object *obj = thisExpr->runObject(ctx);
      CPPIA_CHECK(obj);
      #if (HXCPP_API_LEVEL>=330)
      return static_cast<EnumBase_obj *>(obj)->_hx_getIndex();
      #else
      return obj->__Index();
      #endif
   }
};


struct CallSetField : public CppiaDynamicExpr
{
   CppiaExpr   *thisExpr;
   CppiaExpr   *nameExpr;
   CppiaExpr   *valueExpr;
   CppiaExpr   *isPropExpr;
  
   CallSetField(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inName, CppiaExpr *inValue, CppiaExpr *inProp) :
      CppiaDynamicExpr(inSrc)
   {
      thisExpr = inThis;
      nameExpr = inName;
      valueExpr = inValue;
      isPropExpr = inProp;
   }

   const char *getName() { return "__SetField"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      thisExpr = thisExpr->link(inModule);
      nameExpr = nameExpr->link(inModule);
      valueExpr = valueExpr->link(inModule);
      isPropExpr = isPropExpr->link(inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *obj = thisExpr->runObject(ctx);
      CPPIA_CHECK(obj);
      String name = nameExpr->runString(ctx);
      hx::Object *value = valueExpr->runObject(ctx);
      int isProp = isPropExpr->runInt(ctx);
      return Dynamic(obj->__SetField(name, Dynamic(value), (hx::PropertyAccess)isProp)).mPtr;
   }
};


struct CallGetField : public CppiaDynamicExpr
{
   CppiaExpr   *thisExpr;
   CppiaExpr   *nameExpr;
   CppiaExpr   *isPropExpr;
  
   CallGetField(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inName, CppiaExpr *inProp) : CppiaDynamicExpr(inSrc)
   {
      thisExpr = inThis;
      nameExpr = inName;
      isPropExpr = inProp;
   }

   const char *getName() { return "__Field"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      thisExpr = thisExpr->link(inModule);
      nameExpr = nameExpr->link(inModule);
      isPropExpr = isPropExpr->link(inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *obj = thisExpr->runObject(ctx);
      CPPIA_CHECK(obj);
      String name = nameExpr->runString(ctx);
      int isProp = isPropExpr->runInt(ctx);
      #if (HXCPP_API_LEVEL>=330)
      return obj->__Field(name,(hx::PropertyAccess)isProp).asObject();
      #else
      return obj->__Field(name,(hx::PropertyAccess)isProp).mPtr;
      #endif
   }
};





struct CallMemberVTable : public CppiaExpr
{
   Expressions args;
   CppiaExpr   *thisExpr;
   int         slot;
   ExprType    returnType;
   bool isInterfaceCall;

   CallMemberVTable(CppiaExpr *inSrc, CppiaExpr *inThis, int inVTableSlot, ExprType inReturnType,
             bool inIsInterfaceCall,Expressions &ioArgs)
      : CppiaExpr(inSrc), returnType(inReturnType)
   {
      args.swap(ioArgs);
      slot = inVTableSlot;
      thisExpr = inThis;
      isInterfaceCall = inIsInterfaceCall;
   }
   const char *getName() { return "CallMemberVTable"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (thisExpr)
         thisExpr = thisExpr->link(inModule);
      LinkExpressions(args,inModule);
      return this;
   }
   ExprType getType() { return returnType; }

   #define CALL_VTABLE_SETUP \
      hx::Object *thisVal = thisExpr ? thisExpr->runObject(ctx) : ctx->getThis(); \
      CPPIA_CHECK(thisVal); \
      ScriptCallable **vtable = (ScriptCallable **)thisVal->__GetScriptVTable(); \
      unsigned char *pointer = ctx->pointer; \
      vtable[slot]->pushArgs(ctx, thisVal, args); \
      /* TODO */; \
      AutoStack save(ctx,pointer);

   void runVoid(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      ctx->runVoid(vtable[slot]);
   }
   int runInt(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertInt(ctx, returnType, vtable[slot]); 
   }
 
   Float runFloat(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertFloat(ctx, returnType, vtable[slot]); 
   }
   String runString(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertString(ctx, returnType, vtable[slot]); 
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertObject(ctx, returnType, vtable[slot]); 
   }


};


enum MemberCallType
{
   callObject,
   callThis,
   callSuperNew,
   callSuper,
};


struct ThisExpr : public CppiaDynamicExpr
{
   ThisExpr(CppiaStream &stream)
   {
   }
   hx::Object *runObject(CppiaCtx *ctx) { return ctx->getThis(); }
};


struct ClassOfExpr : public CppiaExprWithValue
{
   int typeId;

   ClassOfExpr(CppiaStream &stream)
   {
      typeId = stream.getInt();
   }
   const char *getName() { return "ClassOfExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[typeId];
      if (type->cppiaClass)
         value.mPtr = type->cppiaClass->mClass.mPtr;
      else
         value.mPtr = type->haxeClass.mPtr;

      return CppiaExprWithValue::link(inModule);
   }
};


struct CallGlobal : public CppiaExpr
{
   int fieldId;
   Expressions args;
  
   CallGlobal(CppiaStream &stream)
   {
      fieldId = stream.getInt();
      int n = stream.getInt();
      ReadExpressions(args,stream,n);
   }

   const char *getName() { return "CallGlobal"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      String name = inModule.strings[fieldId];
      LinkExpressions(args,inModule);
      return createGlobalBuiltin(this, name, args );
   }
};

void callDynamic(CppiaCtx *ctx, hx::Object *inFunction, int inArgs)
{
   // ctx.pointer points to end-of-args
   hx::Object **base = ((hx::Object **)(ctx->pointer) ) - inArgs;
   try
   {
      switch(inArgs)
      {
         case 0:
            base[0] = inFunction->__run().mPtr;
            break;
         case 1:
            base[0] = inFunction->__run(base[0]).mPtr;
            break;
         case 2:
            base[0] = inFunction->__run(base[0],base[1]).mPtr;
            break;
         case 3:
            base[0] = inFunction->__run(base[0],base[1],base[2]).mPtr;
            break;
         case 4:
            base[0] = inFunction->__run(base[0],base[1],base[2],base[3]).mPtr;
            break;
         case 5:
            base[0] = inFunction->__run(base[0],base[1],base[2],base[3],base[4]).mPtr;
            break;
         default:
            {
            Array<Dynamic> argArray = Array_obj<Dynamic>::__new(inArgs,inArgs);
            for(int s=0;s<inArgs;s++)
               argArray[s] = base[s];
            base[0] = inFunction->__Run(argArray).mPtr;
            }
      }
   }
   catch(Dynamic e)
   {
      ctx->exception = e.mPtr;
   }
   ctx->pointer = (unsigned char *)base;
}



struct FieldByName : public CppiaDynamicExpr
{
   CppiaExpr   *object;
   String      name;
   CppiaExpr   *value;
   AssignOp    assign;
   CrementOp   crement;
   hx::Class       staticClass;

   
   FieldByName(CppiaExpr *inSrc, CppiaExpr *inObject, hx::Class inStaticClass,
               String inName, AssignOp inAssign, CrementOp inCrement, CppiaExpr *inValue)
      : CppiaDynamicExpr(inSrc)
   {
      object = inObject;
      staticClass = inStaticClass;
      name = inName;
      assign = inAssign;
      crement = inCrement;
      value = inValue;
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      if (value)
         value = value->link(inModule);
      if (object)
         object = object->link(inModule);
      inModule.markable.push_back(this);
      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *obj = object ? object->runObject(ctx) : staticClass.mPtr ? staticClass.mPtr : ctx->getThis(false);
      BCR_CHECK;
      CPPIA_CHECK(obj);

      if (crement==coNone && assign==aoNone)
         return Dynamic(obj->__Field(name,HX_PROP_DYNAMIC)).mPtr;

      if (crement!=coNone)
      {
         Dynamic val0 = obj->__Field(name,HX_PROP_DYNAMIC);
         BCR_CHECK;
         Dynamic val1 = val0 + (crement<=coPostInc ? 1 : -1);
         obj->__SetField(name, val1,HX_PROP_DYNAMIC);
         BCR_CHECK;
         return crement & coPostInc ? val0.mPtr : val1.mPtr;
      }
      if (assign == aoSet)
      {
         hx::Object *val = value->runObject(ctx);
         BCR_CHECK;
         return Dynamic(obj->__SetField(name,val,HX_PROP_DYNAMIC)).mPtr;
      }

      Dynamic val0 = obj->__Field(name,HX_PROP_DYNAMIC);
      BCR_CHECK;
      Dynamic val1;

      switch(assign)
      {
         case aoAdd: val1 = val0 + Dynamic(value->runObject(ctx)); break;
         case aoMult: val1 = val0 * value->runFloat(ctx); break;
         case aoDiv: val1 = val0 / value->runFloat(ctx); break;
         case aoSub: val1 = val0 - value->runFloat(ctx); break;
         case aoAnd: val1 = (int)val0 & value->runInt(ctx); break;
         case aoOr: val1 = (int)val0 | value->runInt(ctx); break;
         case aoXOr: val1 = (int)val0 ^ value->runInt(ctx); break;
         case aoShl: val1 = (int)val0 << value->runInt(ctx); break;
         case aoShr: val1 = (int)val0 >> value->runInt(ctx); break;
         case aoUShr: val1 = hx::UShr((int)val0,value->runInt(ctx)); break;
         case aoMod: val1 = hx::Mod(val0 ,value->runFloat(ctx)); break;
         default: ;
      }
      BCR_CHECK;
      obj->__SetField(name,val1,HX_PROP_DYNAMIC);
      return val1.mPtr;
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(name);
      HX_MARK_MEMBER(staticClass);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(name);
      HX_VISIT_MEMBER(staticClass);
   }
#endif

};

struct GetFieldByName : public CppiaDynamicExpr
{
   int         nameId;
   int         classId;
   int         vtableSlot;
   CppiaExpr   *object;
   String      name;
   bool        isInterface;
   bool        isStatic;
   hx::Class       staticClass;
  
   GetFieldByName(CppiaStream &stream,bool isThisObject,bool inIsStatic=false)
   {
      classId = stream.getInt();
      nameId = stream.getInt();
      isStatic = inIsStatic;
      object = (isThisObject||isStatic) ? 0 : createCppiaExpr(stream);
      name.__s = 0;
      isInterface = false;
      vtableSlot = -1;
   }
   GetFieldByName(const CppiaExpr *inSrc, int inNameId, CppiaExpr *inObject)
      : CppiaDynamicExpr(inSrc)
   {
      classId = 0;
      nameId = inNameId;
      object = inObject;
      isInterface = false;
      name.__s = 0;
      vtableSlot = -1;
   }
   const char *getName() { return "GetFieldByName"; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      if (object)
         object = object->link(inModule);
      TypeData *type = inModule.types[classId];
      if (!isStatic && type->cppiaClass)
      {
         vtableSlot  = type->cppiaClass->findFunctionSlot(nameId);
         isInterface = type->cppiaClass->isInterface;
         name = inModule.strings[nameId];
      }
      else if (isStatic)
      {
         if (type->cppiaClass)
         {
            CppiaVar *var = type->cppiaClass->findVar(true,nameId);
            if (var)
            {
               CppiaExpr *replace = var->createAccess(this);
               if (!replace)
                  throw "Bad replace";
               replace->link(inModule);
               delete this;
               return replace;
            }

            CppiaExpr *func = type->cppiaClass->findFunction(true,nameId);
            if (func)
            {
               CppiaExprWithValue *replace = new CppiaExprWithValue(this);
               replace->link(inModule);
               replace->value = createMemberClosure(0, ((ScriptCallable *)func));
               delete this;
               return replace;
            }
         }

         staticClass = type->haxeClass;
         if (!staticClass.mPtr)
         {
            inModule.where(this);
            if (type->cppiaClass)
               type->cppiaClass->dump();
            printf("Could not link static %s::%s (%d)\n", type->name.c_str(), inModule.strings[nameId].__s, nameId );
            throw "Bad link";
         }
         name = inModule.strings[nameId];
         const StaticInfo *info = staticClass->GetStaticStorage(name);
         if (info)
         {
            CppiaExpr *replace = createStaticAccess(this, info->type, info->address);
            replace->link(inModule);
            delete this;
            return replace;
         }
      }

      // Use runtime lookup...
      if (vtableSlot==-1 || staticClass.mPtr)
      {
         name = inModule.strings[nameId];
         inModule.markable.push_back(this);
      }
      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *instance = object ? object->runObject(ctx) : isStatic ? staticClass.mPtr : ctx->getThis(false);
      BCR_CHECK;
      CPPIA_CHECK(instance);
      if (vtableSlot!=-1)
      {
         //if (isInterface)
         //   instance = instance->__GetRealObject();
 
         ScriptCallable **vtable = (ScriptCallable **)instance->__GetScriptVTable();
         ScriptCallable *func = vtable[vtableSlot];
         if (func==0)
         {
            printf("Could not find vtable entry %s intf=%d (%d)\n", name.__s, isInterface, vtableSlot);
            return 0;
         }
         return new (sizeof(hx::Object *)) CppiaClosure(instance, func);
      }
      return Dynamic(instance->__Field(name,HX_PROP_DYNAMIC)).mPtr;
   }
  
   CppiaExpr   *makeSetter(AssignOp inOp,CppiaExpr *inValue)
   {
      // delete this - remove markable?
      return new FieldByName(this, object, staticClass, name, inOp, coNone, inValue);
   }

   CppiaExpr   *makeCrement(CrementOp inOp)
   {
      // delete this - remove markable?
      return new FieldByName(this, object, staticClass, name, aoNone, inOp, 0);
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(name);
      HX_MARK_MEMBER(staticClass);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(name);
      HX_VISIT_MEMBER(staticClass);
   }
#endif
};



struct Call : public CppiaDynamicExpr
{
   Expressions args;
   CppiaExpr   *func;
  
   Call(CppiaStream &stream)
   {
      int argCount = stream.getInt();
      func = createCppiaExpr(stream);
      ReadExpressions(args,stream,argCount);
   }

   Call( CppiaExpr *inSrc, int inNameId, CppiaExpr *inObject, Expressions &inArgs ) :
        CppiaDynamicExpr(inSrc)
   {
      std::swap(args, inArgs);

      func = new GetFieldByName(this, inNameId, inObject);
   }


   const char *getName() { return "Call"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      func = func->link(inModule);
      LinkExpressions(args,inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *funcVal = func->runObject(ctx);
      CPPIA_CHECK_FUNC(funcVal);
      int size = args.size();
      switch(size)
      {
         case 0:
           return funcVal->__run().mPtr;

         case 1:
           {
           Dynamic a0 =  args[0]->runObject(ctx);
           BCR_CHECK;
           return funcVal->__run(a0).mPtr;
           }

         case 2:
           {
           Dynamic a0 = args[0]->runObject(ctx);
           BCR_CHECK;
           Dynamic a1 = args[1]->runObject(ctx);
           BCR_CHECK;
           return funcVal->__run(a0,a1).mPtr;
           }
         case 3:
           {
           Dynamic a0 = args[0]->runObject(ctx);
           BCR_CHECK;
           Dynamic a1 = args[1]->runObject(ctx);
           BCR_CHECK;
           Dynamic a2 = args[2]->runObject(ctx);
           BCR_CHECK;
           return funcVal->__run(a0,a1,a2).mPtr;
           }
         case 4:
           {
           Dynamic a0 = args[0]->runObject(ctx);
           BCR_CHECK;
           Dynamic a1 = args[1]->runObject(ctx);
           BCR_CHECK;
           Dynamic a2 = args[2]->runObject(ctx);
           BCR_CHECK;
           Dynamic a3 = args[3]->runObject(ctx);
           BCR_CHECK;
           return funcVal->__run(a0,a1,a2,a3).mPtr;
           }
         case 5:
           {
           Dynamic a0 = args[0]->runObject(ctx);
           BCR_CHECK;
           Dynamic a1 = args[1]->runObject(ctx);
           BCR_CHECK;
           Dynamic a2 = args[2]->runObject(ctx);
           BCR_CHECK;
           Dynamic a3 = args[3]->runObject(ctx);
           BCR_CHECK;
           Dynamic a4 = args[4]->runObject(ctx);
           BCR_CHECK;
           return funcVal->__run(a0,a1,a2,a3,a4).mPtr;
           }


         default:
            Array<Dynamic> argArray = Array_obj<Dynamic>::__new(size,size);
            for(int s=0;s<size;s++)
            {
               argArray[s] = args[s]->runObject(ctx);
               BCR_CHECK;
            }

            return funcVal->__Run(argArray).mPtr;
      }
      return 0;
   }

   #ifdef CPPIA_JIT
   void preGen(CppiaCompiler &compiler)
   {
      AllocTemp frame(compiler);
      for(int a=0;a<args.size();a++)
          args[a]->preGen(compiler);
   }

   void genObject(CppiaCompiler &compiler, const Addr &inDest)
   {
      func->genCode(compiler, TempReg(), etObject );

      AllocTemp pointerSave(compiler);
      CtxMemberVal pointer(offsetof(CppiaCtx,pointer));
      compiler.move(pointerSave, pointer);

 


      // TODO - shortcut for script->script
      for(int a=0;a<args.size();a++)
      {
         args[a]->genCode(compiler, Reg(0), etObject);
         compiler.move( Reg(1), pointer );
         compiler.move( StarAddr(Reg(1)), Reg(0) );
         compiler.add( pointer, Reg(1), ConstValue( sizeof(void *) ) );
      }

      compiler.move(Reg(0), CtxReg());
      compiler.move(Reg(1), TempReg());
      compiler.move(Reg(2), ConstValue( args.size() ));
      compiler.call( callDynamic, 3 );

      if (compiler.exceptionHandler)
      {
         sljit_jump *notZero = compiler.ifNotZero( CtxMemberVal(offsetof(CppiaCtx,exception)) );
         compiler.exceptionHandler->push_back(notZero);
      }
      else
      {
         sljit_jump *isZero = compiler.ifZero( CtxMemberVal(offsetof(CppiaCtx,exception)) );
         compiler.ret( );
         compiler.jumpHere(isZero);
      }

      // Result is at pointer
      if (inDest!=pointer)
         compiler.move( inDest, pointer );
   }
   #endif
};

struct CallMember : public CppiaExpr
{
   int classId;
   int fieldId;
   CppiaExpr *thisExpr;
   Expressions args;
   bool    callSuperField;
  
   CallMember(CppiaStream &stream,MemberCallType inCall)
   {
      classId = stream.getInt();
      fieldId = inCall==callSuperNew ? 0 : stream.getInt();
      //printf("fieldId = %d (%s)\n",fieldId,stream.module->strings[ fieldId ].__s);
      int n = stream.getInt();
      thisExpr = inCall==callObject ? createCppiaExpr(stream) : 0;
      callSuperField = inCall==callSuper;
      ReadExpressions(args,stream,n);
   }

   CppiaExpr *linkSuperCall(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[classId];

      // TODO - callSuperNew

      if (type->cppiaClass)
      {
         //printf("Using cppia super %p %p\n", type->cppiaClass->newFunc, type->cppiaClass->newFunc->funExpr);
         CppiaExpr *replace = new CallFunExpr( this, 0, (ScriptCallable*)type->cppiaClass->newFunc->funExpr, args );
         replace->link(inModule);
         delete this;
         return replace;
      }
      //printf("Using haxe super\n");
      HaxeNativeClass *superReg = HaxeNativeClass::findClass(type->name.__s);
      if (!superReg)
      {
         printf("No class registered for %s\n", type->name.__s);
         throw "Unknown super call";
      }
      if (!superReg->construct.execute)
      {
         //printf("Call super - nothing to do...\n");
         CppiaExpr *replace = new CppiaExpr(this);
         delete this;
         return replace;
      }
      CppiaExpr *replace = new CallHaxe( this, superReg->construct,0, args );
      replace->link(inModule);
      delete this;
      return replace;
   }



   const char *getName() { return "CallMember"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (fieldId==0)
         return linkSuperCall(inModule);

      TypeData *type = inModule.types[classId];
      String field = inModule.strings[fieldId];

      //printf("  linking call %s::%s\n", type->name.__s, field.__s);

      CppiaExpr *replace = 0;
      
      if (type->arrayType)
      {
         replace = createArrayBuiltin(this, type->arrayType, thisExpr, field, args);
         if (!replace)
         {
            if (field!=HX_CSTRING("__SetField") && field!=HX_CSTRING("__Field") && field!=HX_CSTRING("__Index"))
            {
               printf("Bad array field '%s'\n", field.__s);
               inModule.where(this);
               throw "Unknown array field";
            }
         }
      }


      if (!replace && type->cppiaClass)
      {
         if (callSuperField)
         {
            // Bind now ...
            ScriptCallable *func = (ScriptCallable *)type->cppiaClass->findFunction(false, fieldId) ;
            if (func)
               replace = new CallFunExpr( this, thisExpr, func, args );
         }
         else
         {
            int vtableSlot = type->cppiaClass->findFunctionSlot(fieldId);
            //printf("   vslot %d\n", vtableSlot);
            if (vtableSlot!=-1)
            {
               ExprType returnType = type->cppiaClass->findFunctionType(inModule,fieldId);
               replace = new CallMemberVTable( this, thisExpr, vtableSlot, returnType, type->cppiaClass->isInterface, args );
            }
         }
      }
      if (!replace && type->haxeBase)
      {
         ScriptFunction func = type->haxeBase->findFunction(field.__s);
         if (func.signature)
         {
            //printf(" found function %s\n", func.signature );
            replace = new CallHaxe( this, func, thisExpr, args );
         }
      }

      if (!replace && type->interfaceBase)
      {
         ScriptFunction func = type->interfaceBase->findFunction(field.__s);
         if (func.signature)
         {
            //printf(" found function %s\n", func.signature );
            replace = new CallHaxe( this, func, thisExpr, args );
         }
      }

      if (!replace && type->name==HX_CSTRING("String"))
      {
         replace = createStringBuiltin(this, thisExpr, field, args);
      }

      if (!replace && field==HX_CSTRING("__Index"))
         replace = new CallGetIndex(this, thisExpr);
      if (!replace && field==HX_CSTRING("__SetField") && args.size()==3)
         replace = new CallSetField(this, thisExpr, args[0], args[1], args[2]);
      if (!replace && field==HX_CSTRING("__Field") && args.size()==2)
         replace = new CallGetField(this, thisExpr, args[0], args[1]);


      if (replace)
      {
         replace->link(inModule);
         delete this;
         return replace;
      }

      if (type->name != HX_CSTRING("Dynamic"))
      {
         printf("   CallMember %s (%p %p) '%s' fallback\n", type->name.__s, type->haxeClass.mPtr, type->cppiaClass, field.__s);
      }

      {
         replace = new Call( this, fieldId, thisExpr, args );
         replace->link(inModule);
         delete this;
         return replace;
      }

      /*
      printf("Could not link %s::%s\n", type->name.c_str(), field.__s );
      printf("%p %p\n", type->cppiaClass, type->haxeBase);
      if (type->cppiaClass)
         type->cppiaClass->dump();
      if (type->haxeBase)
         type->haxeBase->dump();
      inModule.where(this);
      throw "Bad linkage";

      return this;
      */
   }
};




template<typename T, int REFMODE> 
struct MemReference : public CppiaExpr
{
   int  offset;
   T *pointer;
   CppiaExpr *object;

   #define CHECKVAL \
      if (REFMODE==locObj) CPPIA_CHECK(object);

   #define MEMGETVAL \
     *(T *)( \
         ( REFMODE==locObj      ?(char *)object->runObject(ctx) : \
           REFMODE==locAbsolute ?(char *)pointer : \
           REFMODE==locThis     ?(char *)ctx->getThis() : \
                                 (char *)ctx->frame \
         ) + offset )

   MemReference(const CppiaExpr *inSrc, int inOffset, CppiaExpr *inExpr=0)
      : CppiaExpr(inSrc)
   {
      object = inExpr;
      offset = inOffset;
      pointer = 0;
   }
   MemReference(CppiaExpr *inSrc, T *inPointer)
      : CppiaExpr(inSrc)
   {
      object = 0;
      offset = 0;
      pointer = inPointer;
   }
 
   ExprType getType()
   {
      return (ExprType) ExprTypeOf<T>::value;
   }
   const char *getName() { return "MemReference"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (object)
         object = object->link(inModule);
      if (REFMODE==locAbsolute) // Only for string/object?
         inModule.markable.push_back(this);
 
      return this;
   }

   void mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER( *pointer ); }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER( *pointer ); }
#endif


   void        runVoid(CppiaCtx *ctx) { }
   int runInt(CppiaCtx *ctx)
   {
      CHECKVAL;
      return ValToInt( MEMGETVAL );
   }
   Float       runFloat(CppiaCtx *ctx)
   {
      CHECKVAL;
      return ValToFloat( MEMGETVAL );
   }
   ::String    runString(CppiaCtx *ctx) {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToString(t);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      CHECKVAL;
      return Dynamic( MEMGETVAL ).mPtr;
   }

   CppiaExpr  *makeSetter(AssignOp op,CppiaExpr *value);
   CppiaExpr  *makeCrement(CrementOp inOp);
};



/*
CppiaExpr *createStaticAccess(CppiaExpr *inSrc,ExprType inType, void *inPtr)
{
   switch(inType)
   {
      case etInt : return new MemReference<int,locAbsolute>(inSrc, (int *)inPtr );
      case etFloat : return new MemReference<Float,locAbsolute>(inSrc, (Float *)inPtr );
      case etString : return new MemReference<String,locAbsolute>(inSrc, (String *)inPtr );
      case etObject : return new MemReference<hx::Object *,locAbsolute>(inSrc, (hx::Object **)inPtr );
      default:
         return 0;
   }
}
*/

CppiaExpr *createStaticAccess(CppiaExpr *inSrc,FieldStorage inType, void *inPtr)
{
   switch(inType)
   {
      case fsBool : return new MemReference<bool,locAbsolute>(inSrc, (bool *)inPtr );
      case fsInt : return new MemReference<int,locAbsolute>(inSrc, (int *)inPtr );
      case fsByte : return new MemReference<unsigned char,locAbsolute>(inSrc, (unsigned char *)inPtr );
      case fsFloat : return new MemReference<Float,locAbsolute>(inSrc, (Float *)inPtr );
      case fsString : return new MemReference<String,locAbsolute>(inSrc, (String *)inPtr );
      case fsObject : return new MemReference<hx::Object *,locAbsolute>(inSrc, (hx::Object **)inPtr );
      default:
         return 0;
   }
}




template<typename T, int REFMODE, typename Assign> 
struct MemReferenceSetter : public CppiaExpr
{
   int offset;
   T         *pointer;
   CppiaExpr *object;
   CppiaExpr *value;

   MemReferenceSetter(MemReference<T,REFMODE> *inSrc, CppiaExpr *inValue) : CppiaExpr(inSrc)
   {
      offset = inSrc->offset;
      object = inSrc->object;
      pointer = inSrc->pointer;
      value = inValue;
   }
   ExprType getType()
   {
      return (ExprType) ExprTypeOf<T>::value;
   }

   void runVoid(CppiaCtx *ctx)
   {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_VCHECK;
      Assign::run( t, ctx, value);
   }
   int runInt(CppiaCtx *ctx)
   {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToInt( Assign::run(t,ctx, value ) );
   }
   Float runFloat(CppiaCtx *ctx)
   {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToFloat( Assign::run(t,ctx, value) );
   }
   ::String runString(CppiaCtx *ctx)
   {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToString( Assign::run(t,ctx, value) );
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return Dynamic( Assign::run(t,ctx,value) ).mPtr;
   }

   void mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER( *pointer ); }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER( *pointer ); }
#endif

   CppiaExpr *link(CppiaModule &inModule)
   {
      if (REFMODE==locAbsolute) // Only for string/object?
         inModule.markable.push_back(this);
      return this;
   }


};


template<typename T, int REFMODE> 
CppiaExpr *MemReference<T,REFMODE>::makeSetter(AssignOp op,CppiaExpr *value)
{
   switch(op)
   {
      case aoSet:
         return new MemReferenceSetter<T,REFMODE,AssignSet>(this,value);
      case aoAdd:
         return new MemReferenceSetter<T,REFMODE,AssignAdd>(this,value);
      case aoMult:
         return new MemReferenceSetter<T,REFMODE,AssignMult>(this,value);
      case aoDiv:
         return new MemReferenceSetter<T,REFMODE,AssignDiv>(this,value);
      case aoSub:
         return new MemReferenceSetter<T,REFMODE,AssignSub>(this,value);
      case aoAnd:
         return new MemReferenceSetter<T,REFMODE,AssignAnd>(this,value);
      case aoOr:
         return new MemReferenceSetter<T,REFMODE,AssignOr>(this,value);
      case aoXOr:
         return new MemReferenceSetter<T,REFMODE,AssignXOr>(this,value);
      case aoShl:
         return new MemReferenceSetter<T,REFMODE,AssignShl>(this,value);
      case aoShr:
         return new MemReferenceSetter<T,REFMODE,AssignShr>(this,value);
      case aoUShr:
         return new MemReferenceSetter<T,REFMODE,AssignUShr>(this,value);
      case aoMod:
         return new MemReferenceSetter<T,REFMODE,AssignMod>(this,value);
      default: ;
   }
   throw "Bad assign op";
   return 0;
}



template<typename T, int REFMODE,typename CREMENT> 
struct MemReferenceCrement : public CppiaExpr
{
   int offset;
   T   *pointer;
   CppiaExpr *object;

   MemReferenceCrement(MemReference<T,REFMODE> *inSrc) : CppiaExpr(inSrc)
   {
      offset = inSrc->offset;
      object = inSrc->object;
      pointer = inSrc->pointer;
   }
   ExprType getType()
   {
      return (ExprType) ExprTypeOf<T>::value;
   }

   void        runVoid(CppiaCtx *ctx) {
      CHECKVAL;
       CREMENT::run( MEMGETVAL );
   }
   int runInt(CppiaCtx *ctx) {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToInt( CREMENT::run(t) );
   }
   Float       runFloat(CppiaCtx *ctx) {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToFloat( CREMENT::run(t));
   }
   ::String    runString(CppiaCtx *ctx) {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return ValToString( CREMENT::run(t) );
   }

   hx::Object *runObject(CppiaCtx *ctx) {
      CHECKVAL;
      T &t = MEMGETVAL;
      BCR_CHECK;
      return Dynamic( CREMENT::run(t) ).mPtr;
   }


   void mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER( *pointer ); }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER( *pointer ); }
#endif

   CppiaExpr *link(CppiaModule &inModule)
   {
      if (REFMODE==locAbsolute) // Only for string/object?
         inModule.markable.push_back(this);
      return this;
   }


};



template<typename T, int REFMODE> 
CppiaExpr *MemReference<T,REFMODE>::makeCrement(CrementOp inOp)
{
   switch(inOp)
   {
      case coPreInc:
         return new MemReferenceCrement<T,REFMODE,CrementPreInc>(this);
      case coPostInc:
         return new MemReferenceCrement<T,REFMODE,CrementPostInc>(this);
      case coPreDec:
         return new MemReferenceCrement<T,REFMODE,CrementPreDec>(this);
      case coPostDec:
         return new MemReferenceCrement<T,REFMODE,CrementPostDec>(this);
      default:
         break;
   }
   throw "bad crement op";
   return 0;
}


struct MemStackFloatSetter : public CppiaExpr
{
   int offset;
   CppiaExpr *value;
   AssignOp op;

   MemStackFloatSetter(const CppiaExpr *inSrc, int inOffset, AssignOp inOp, CppiaExpr *inValue)
      : CppiaExpr(inSrc), offset(inOffset), op(inOp), value(inValue){ } 
   ExprType getType() { return etFloat; }

   inline Float doRun(CppiaCtx *ctx)
   {
      Float v = value->runFloat(ctx);
      BCR_CHECK;

      void *ptr = (char*)ctx->frame + offset;
      switch(op)
      {
         case aoAdd: v = GetFloatAligned(ptr) + v; break;
         case aoMult: v = GetFloatAligned(ptr) * v; break;
         case aoDiv: v = GetFloatAligned(ptr) / v; break;
         case aoSub: v = GetFloatAligned(ptr) - v; break;
         case aoAnd: v = GetFloatAligned(ptr) && v; break;
         case aoOr: v = GetFloatAligned(ptr) || v; break;
         case aoMod: v = hx::DoubleMod( GetFloatAligned(ptr),  v); break;
         default: ;
      }
      SetFloatAligned(ptr, v);
      return v;
   }


   void        runVoid(CppiaCtx *ctx) { doRun(ctx); }
   int runInt(CppiaCtx *ctx) { return doRun(ctx); }
   Float       runFloat(CppiaCtx *ctx) { return doRun(ctx); }
   ::String    runString(CppiaCtx *ctx) { return ValToString(doRun(ctx)); }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(doRun(ctx)).mPtr; }
};


struct MemStackFloatCrement : public CppiaExpr
{
   int offset;
   CrementOp op;

   MemStackFloatCrement(const CppiaExpr *inSrc, int inOffset, CrementOp inOp)
      : CppiaExpr(inSrc), offset(inOffset), op(inOp) { } 
   ExprType getType() { return etFloat; }

   inline Float doRun(CppiaCtx *ctx)
   {
      void *ptr = (char*)ctx->frame + offset;
      Float v = GetFloatAligned(ptr);
      switch(op)
      {
         case coPreInc: SetFloatAligned(ptr,++v); break;
         case coPostInc: SetFloatAligned(ptr,v+1); break;
         case coPreDec: SetFloatAligned(ptr,--v); break;
         case coPostDec: SetFloatAligned(ptr,v-1); break;
         default: ;
      }
      return v;
   }

   void        runVoid(CppiaCtx *ctx) { doRun(ctx); }
   int runInt(CppiaCtx *ctx) { return doRun(ctx); }
   Float       runFloat(CppiaCtx *ctx) { return doRun(ctx); }
   ::String    runString(CppiaCtx *ctx) { return ValToString(doRun(ctx)); }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(doRun(ctx)).mPtr; }
};



struct MemStackFloatReference : public CppiaExpr
{
   int  offset;

   MemStackFloatReference(const CppiaExpr *inSrc, int inOffset)
      : CppiaExpr(inSrc), offset(inOffset) { }

   ExprType getType() { return etFloat; }
   const char *getName() { return "MemStackFloatReference"; }
   CppiaExpr *link(CppiaModule &inModule) { return this; }

   void        runVoid(CppiaCtx *ctx) { }
   int runInt(CppiaCtx *ctx) { return GetFloatAligned( ((char *)ctx->frame) + offset ); }
   Float  runFloat(CppiaCtx *ctx) { return GetFloatAligned( ((char *)ctx->frame) + offset ); }
   ::String    runString(CppiaCtx *ctx) { return ValToString( GetFloatAligned( ((char *)ctx->frame) + offset )); }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      return Dynamic( GetFloatAligned( ((char *)ctx->frame) + offset ) ).mPtr;
   }

   CppiaExpr  *makeSetter(AssignOp op,CppiaExpr *value)
   {
      return new MemStackFloatSetter(this, offset, op, value);
   }
   CppiaExpr  *makeCrement(CrementOp inOp)
   {
      return new MemStackFloatCrement(this, offset, inOp);
   }
};


#if (HXCPP_API_LEVEL>=330)
struct VirtualArrayLength : public CppiaIntExpr
{
   CppiaExpr   *thisExpr;
  
   VirtualArrayLength(CppiaExpr *inSrc, CppiaExpr *inThis) : CppiaIntExpr(inSrc)
   {
      thisExpr = inThis;
   }

   const char *getName() { return "length"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      thisExpr = thisExpr->link(inModule);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      cpp::VirtualArray_obj *obj = (cpp::VirtualArray_obj *)thisExpr->runObject(ctx);
      BCR_CHECK;
      CPPIA_CHECK(obj);
      return obj->get_length();
   }
};
#endif


struct GetFieldByLinkage : public CppiaExpr
{
   int         fieldId;
   int         typeId;
   CppiaExpr   *object;
  
   GetFieldByLinkage(CppiaStream &stream,bool inThisObject)
   {
      typeId = stream.getInt();
      fieldId = stream.getInt();
      object = inThisObject ? 0 : createCppiaExpr(stream);
   }

   const char *getName() { return "GetFieldByLinkage"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[typeId];
      String field = inModule.strings[fieldId];

      int offset = 0;
      CppiaExpr *replace = 0;
      FieldStorage storeType = fsUnknown;

      if (type->haxeClass.mPtr)
      {
         const StorageInfo *store = type->haxeClass.mPtr->GetMemberStorage(field);
         if (store)
         {
            offset = store->offset;
            storeType = store->type;
            DBGLOG(" found haxe var %s::%s = %d (%d)\n", type->haxeClass->mName.__s, field.__s, offset, storeType);
         }
      }

      if (!offset && type->cppiaClass)
      {
         CppiaVar *var = type->cppiaClass->findVar(false, fieldId);
         if (var)
         {
            offset = var->offset;
            storeType = var->storeType;
            DBGLOG(" found script var %s = %d (%d)\n", field.__s, offset, storeType);
         }
      }

      if (offset)
      {
         switch(storeType)
         {
            case fsBool:
               replace = object ?
                     (CppiaExpr*)new MemReference<bool,locObj>(this,offset,object):
                     (CppiaExpr*)new MemReference<bool,locThis>(this,offset);
                  break;
            case fsInt:
               replace = object ?
                     (CppiaExpr*)new MemReference<int,locObj>(this,offset,object):
                     (CppiaExpr*)new MemReference<int,locThis>(this,offset);
                  break;
            case fsFloat:
               replace = object ?
                     (CppiaExpr*)new MemReference<Float,locObj>(this,offset,object):
                     (CppiaExpr*)new MemReference<Float,locThis>(this,offset);
                  break;
            case fsString:
               replace = object ?
                     (CppiaExpr*)new MemReference<String,locObj>(this,offset,object):
                     (CppiaExpr*)new MemReference<String,locThis>(this,offset);
                  break;
            case fsObject:
               replace = object ?
                     (CppiaExpr*)new MemReference<hx::Object *,locObj>(this,offset,object):
                     (CppiaExpr*)new MemReference<hx::Object *,locThis>(this,offset);
                  break;
            case fsByte:
            case fsUnknown:
                printf("TODO - byte/unkown GetFieldByLinkage\n");
                ;// todo
         }
      }

      if (!replace && type->arrayType!=arrNotArray && field==HX_CSTRING("length"))
      {
         #if (HXCPP_API_LEVEL>=330)
         if (type->arrayType==arrAny)
         {
            replace = new VirtualArrayLength(this,object);
         }
         else
         #endif
         {
         int offset = (int) offsetof( Array_obj<int>, length );
         replace = object ?
             (CppiaExpr*)new MemReference<int,locObj>(this,offset,object):
             (CppiaExpr*)new MemReference<int,locThis>(this,offset);
         }
      }

      if (!replace && type->name==HX_CSTRING("String") && field==HX_CSTRING("length"))
      {
         int offset = (int) offsetof( Array_obj<Int>, length );
         replace = object ?
             (CppiaExpr*)new MemReference<int,locObj>(this,offset,object):
             (CppiaExpr*)new MemReference<int,locThis>(this,offset);
      }



      if (replace)
      {
         replace = replace->link(inModule);
         delete this;
         return replace;
      }

      // It is ok for interfaces to look up members by name - and variables that turn
      //  out to actaully be Dynamic (eg template types)
      if (!type->isInterface && type->name!=HX_CSTRING("Dynamic") )
      {
         printf("   GetFieldByLinkage %s (%p %p %p) '%s' fallback\n", type->name.__s, object, type->haxeClass.mPtr, type->cppiaClass, field.__s);
         if (type->cppiaClass)
            type->cppiaClass->dump();
         else
           printf(" - is Native class\n");
      }

      CppiaExpr *result = new GetFieldByName(this, fieldId, object);
      result = result->link(inModule);
      delete this;
      return result;
   }
};




struct StringVal : public CppiaExprWithValue
{
   int stringId;
   String strVal;

   StringVal(int inId) : stringId(inId)
   {
   }

   ExprType getType() { return etString; }

   const char *getName() { return "StringVal"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      strVal = inModule.strings[stringId];
      //printf("Linked %d -> %s\n", stringId, strVal.__s);
      return CppiaExprWithValue::link(inModule);
   }
   ::String    runString(CppiaCtx *ctx)
   {
      return strVal;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (!value.mPtr)
         value = strVal;
      return value.mPtr;
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(value);
      HX_MARK_MEMBER(strVal);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(value);
      HX_VISIT_MEMBER(strVal);
   }
#endif
};


template<typename T>
struct DataVal : public CppiaExprWithValue
{
   T data;

   DataVal(T inVal) : data(inVal)
   {
   }

   ExprType getType() { return (ExprType)ExprTypeOf<T>::value; }

   void        runVoid(CppiaCtx *ctx) {  }
   int runInt(CppiaCtx *ctx) { return ValToInt(data); }
   Float       runFloat(CppiaCtx *ctx) { return ValToFloat(data); }
   ::String    runString(CppiaCtx *ctx) { return ValToString(data); }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (!value.mPtr)
         value = Dynamic(data);
      return value.mPtr;
   }
};



struct NullVal : public CppiaExpr
{
   NullVal() { }
   ExprType getType() { return etObject; }

   void        runVoid(CppiaCtx *ctx) {  }
   int runInt(CppiaCtx *ctx) { return 0; }
   Float       runFloat(CppiaCtx *ctx) { return 0.0; }
   ::String    runString(CppiaCtx *ctx) { return null(); }
   hx::Object  *runObject(CppiaCtx *ctx) { return 0; }

};


struct PosInfo : public CppiaExprWithValue
{
   int fileId;
   int line;
   int classId;
   int methodId;

   PosInfo(CppiaStream &stream)
   {
      fileId = stream.getInt();
      line = stream.getInt();
      classId = stream.getInt();
      methodId = stream.getInt();
   }
   const char *getName() { return "PosInfo"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      String clazz = inModule.strings[classId];
      String file = inModule.strings[fileId];
      String method = inModule.strings[methodId];
      value = hx::SourceInfo(file,line,clazz,method);
      return CppiaExprWithValue::link(inModule);
   }
};


struct ObjectDef : public CppiaDynamicExpr
{
   int fieldCount;
   std::vector<int> stringIds;
   CppiaModule *data;
   ArrayType arrayType;
   Expressions values;

   ObjectDef(CppiaStream &stream)
   {
      fieldCount = stream.getInt();
      data = 0;
      stringIds.resize(fieldCount);
      for(int i=0;i<fieldCount;i++)
         stringIds[i] = stream.getInt();
      ReadExpressions(values,stream,fieldCount);
   }

   const char *getName() { return "ObjectDef"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      data = &inModule;
      LinkExpressions(values,inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Anon result = hx::Anon_obj::Create();
      for(int i=0;i<fieldCount;i++)
      {
         result->Add(data->strings[stringIds[i]], values[i]->runObject(ctx), false );
         BCR_CHECK;
      }
      return result.mPtr;
   }
};

struct ArrayDef : public CppiaDynamicExpr
{
   int classId;
   Expressions items;
   ArrayType arrayType;

   ArrayDef(CppiaStream &stream)
   {
      classId = stream.getInt();
      ReadExpressions(items,stream);
   }


   const char *getName() { return "ArrayDef"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[classId];
      arrayType = type->arrayType;
      if (!arrayType)
      {
         printf("ArrayDef of non array-type %s\n", type->name.__s);
         throw "Bad ArrayDef";
      }
      LinkExpressions(items,inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      int n = items.size();
      switch(arrayType)
      {
         case arrBool:
            { 
            Array<bool> result = Array_obj<bool>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result[i] = items[i]->runInt(ctx)!=0;
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrUnsignedChar:
            { 
            Array<unsigned char> result = Array_obj<unsigned char>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result[i] = items[i]->runInt(ctx);
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrInt:
            { 
            Array<int> result = Array_obj<int>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result[i] = items[i]->runInt(ctx);
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrFloat:
            { 
            Array<Float> result = Array_obj<Float>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result[i] = items[i]->runFloat(ctx);
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrString:
            { 
            Array<String> result = Array_obj<String>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result[i] = items[i]->runString(ctx);
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrAny:
            #if (HXCPP_API_LEVEL>=330)
            {
            cpp::VirtualArray result = cpp::VirtualArray_obj::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result->init(i,Dynamic(items[i]->runObject(ctx)));
               BCR_CHECK;
            }
            return result.mPtr;
            }
            #else
            // Fallthough...
            #endif
         case arrObject:
            { 
            Array<Dynamic> result = Array_obj<Dynamic>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result[i] = items[i]->runObject(ctx);
               BCR_CHECK;
            }
            return result.mPtr;
            }
         default:
            return 0;
      }
      return 0;
   }
};

struct DynamicArrayI : public CppiaDynamicExpr
{
   CppiaExpr *object;
   CppiaExpr *index;
   CppiaExpr *value;
   AssignOp  assign;
   CrementOp crement;

   DynamicArrayI(CppiaExpr *inSrc, CppiaExpr *inObj, CppiaExpr *inI)
     : CppiaDynamicExpr(inSrc)
   {
      object = inObj;
      index = inI;
      value = 0;
      assign = aoNone;
      crement = coNone;
   }
   CppiaExpr *link(CppiaModule &inModule)
   {
      object = object->link(inModule);
      index = index->link(inModule);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *obj = object->runObject(ctx);
      BCR_CHECK;
      CPPIA_CHECK(obj);
      int i = index->runInt(ctx);
      BCR_CHECK;
      if (crement==coNone && assign==aoNone)
      {
         return obj->__GetItem(i).mPtr;
      }

      if (crement!=coNone)
      {
         Dynamic val0 = obj->__GetItem(i);
         Dynamic val1 = val0 + (crement<=coPostInc ? 1 : -1);
         obj->__SetItem(i, val1);
         return crement & coPostInc ? val0.mPtr : val1.mPtr;
      }
      if (assign == aoSet)
      {
         hx::Object *val = value->runObject(ctx);
         BCR_CHECK;
         return obj->__SetItem(i, val).mPtr;
      }

      Dynamic val0 = obj->__GetItem(i);
      Dynamic val1;

      switch(assign)
      {
         case aoAdd: val1 = val0 + Dynamic(value->runObject(ctx)); break;
         case aoMult: val1 = val0 * value->runFloat(ctx); break;
         case aoDiv: val1 = val0 / value->runFloat(ctx); break;
         case aoSub: val1 = val0 - value->runFloat(ctx); break;
         case aoAnd: val1 = (int)val0 & value->runInt(ctx); break;
         case aoOr: val1 = (int)val0 | value->runInt(ctx); break;
         case aoXOr: val1 = (int)val0 ^ value->runInt(ctx); break;
         case aoShl: val1 = (int)val0 << value->runInt(ctx); break;
         case aoShr: val1 = (int)val0 >> value->runInt(ctx); break;
         case aoUShr: val1 = hx::UShr((int)val0,value->runInt(ctx)); break;
         case aoMod: val1 = hx::Mod(val0 ,value->runFloat(ctx)); break;
         default: ;
      }
      BCR_CHECK;
      obj->__SetItem(i,val1);
      return val1.mPtr;
   }
   CppiaExpr  *makeSetter(AssignOp op,CppiaExpr *inValue)
   {
      assign = op;
      value = inValue;
      return this;
   }
   CppiaExpr  *makeCrement(CrementOp inOp)
   {
      crement = inOp;
      return this;
   }
};



struct ArrayAccessI : public CppiaDynamicExpr
{
   CppiaExpr *object;
   CppiaExpr *index;
   CppiaExpr *value;

   int            classId;
   ScriptFunction __get;
   ScriptFunction __set;
   ExprType  accessType;

   AssignOp  assign;
   CrementOp crement;

   ArrayAccessI(CppiaExpr *inSrc, int inClassId, CppiaExpr *inObj, CppiaExpr *inI)
     : CppiaDynamicExpr(inSrc)
   {
      object = inObj;
      index = inI;
      value = 0;
      assign = aoNone;
      crement = coNone;
      classId = inClassId;
      __get = 0;
      __set = 0;
   }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (object)
         object = object->link(inModule);


      index = index->link(inModule);

      TypeData *type = inModule.types[classId];
      // TODO - not always cppiaClass
      if (type->cppiaClass)
         throw "ArrayAccess interface can't be implemented in cppia";

      __get = type->haxeBase->findFunction("__get");
      if (!__get.execute)
      {
         printf("Class %s missing __get\n", type->name.__s);
         throw "Bad array access - __get";
      }
      __set = type->haxeBase->findFunction("__set");
      if (!__set.execute)
      {
         printf("Class %s missing __set\n", type->name.__s);
         throw "Bad array access - __set";
      }

      if (__get.signature[1]!='i' || __set.signature[1]!='i')
         throw "__get/__set should take an int";

      if (__get.signature[0]!=__set.signature[2])
         throw "Mismatch __get/__set array access";

      switch(__get.signature[0])
      {
         case sigInt: accessType = etInt; break;
         case sigFloat: accessType = etFloat; break;
         case sigString: accessType = etString; break;
         case sigObject: accessType = etObject; break;
         default: throw "Bad signature on __get";
      }

      if (value)
         value = value->link(inModule);

      DBGLOG("Created ARRAYI wrapper %s %d %s / %s\n", type->name.__s, accessType, __get.signature, __set.signature);

      return this;
   }

   template<typename T>
   void setResult(CppiaCtx *ctx,T &outValue)
   {
      #ifdef DEBUG_RETURN_TYPE
      gLastRet = accessType;
      #endif
      if (sizeof(outValue)>0)
      {
         if (accessType == etInt)
            SetVal(outValue,ctx->getInt());
         else if (accessType == etFloat)
            SetVal(outValue,ctx->getFloat());
         else if (accessType == etString)
            SetVal(outValue,ctx->getString());
         else if (accessType == etObject)
            SetVal(outValue,ctx->getObject());
      }
   }

   
   template<typename T>
   void run(CppiaCtx *ctx,T &outValue)
   {
      unsigned char *pointer = ctx->pointer;
      unsigned char *frame = ctx->frame;

      hx::Object *obj = object ? object->runObject(ctx) : ctx->getThis(false);

      BCR_VCHECK;
      CPPIA_CHECK(obj);
      int i = index->runInt(ctx);
      BCR_VCHECK;

      if (crement==coNone && (assign==aoNone || assign==aoSet) )
      {
         unsigned char *pointer = ctx->pointer;

         if (assign==aoSet)
         {
            if (accessType == etInt)
            {
               int val = value->runInt(ctx);
               BCR_VCHECK;
               ctx->pushObject(obj);
               ctx->pushInt(i);
               ctx->pushInt(val);
            }
            else if (accessType == etFloat)
            {
               Float val = value->runFloat(ctx);
               BCR_VCHECK;
               ctx->pushObject(obj);
               ctx->pushInt(i);
               ctx->pushFloat(val);
            }
            else if (accessType == etString)
            {
               String val = value->runString(ctx);
               BCR_VCHECK;
               ctx->pushObject(obj);
               ctx->pushInt(i);
               ctx->pushString(val);
            }
            else if (accessType == etObject)
            {
               hx::Object *val = value->runObject(ctx);
               BCR_VCHECK;
               ctx->pushObject(obj);
               ctx->pushInt(i);
               ctx->pushObject(val);
            }

            AutoStack a(ctx,pointer);
            __set.execute(ctx);
            BCR_VCHECK;
            setResult(ctx,outValue);
         }
         else
         {
            ctx->pushObject(obj);
            ctx->pushInt(i);

            AutoStack a(ctx,pointer);
            BCR_VCHECK;
            setResult(ctx,outValue);
         }

         return;
      }


   #if 0
      if (crement!=coNone)
      {
         Dynamic val0 = obj->__GetItem(i);
         Dynamic val1 = val0 + (crement<=coPostInc ? 1 : -1);
         obj->__SetItem(i, val1);
         return crement & coPostInc ? val0.mPtr : val1.mPtr;
      }
      if (assign == aoSet)
      {
         hx::Object *val = value->runObject(ctx);
         BCR_CHECK;
         return obj->__SetItem(i, val).mPtr;
      }

      Dynamic val0 = obj->__GetItem(i);
      Dynamic val1;

      switch(assign)
      {
         case aoAdd: val1 = val0 + Dynamic(value->runObject(ctx)); break;
         case aoMult: val1 = val0 * value->runFloat(ctx); break;
         case aoDiv: val1 = val0 / value->runFloat(ctx); break;
         case aoSub: val1 = val0 - value->runFloat(ctx); break;
         case aoAnd: val1 = (int)val0 & value->runInt(ctx); break;
         case aoOr: val1 = (int)val0 | value->runInt(ctx); break;
         case aoXOr: val1 = (int)val0 ^ value->runInt(ctx); break;
         case aoShl: val1 = (int)val0 << value->runInt(ctx); break;
         case aoShr: val1 = (int)val0 >> value->runInt(ctx); break;
         case aoUShr: val1 = hx::UShr((int)val0,value->runInt(ctx)); break;
         case aoMod: val1 = hx::Mod(val0 ,value->runFloat(ctx)); break;
         default: ;
      }
      BCR_CHECK;
      obj->__SetItem(i,val1);
      return val1.mPtr;
   #endif
   }

   CppiaExpr  *makeSetter(AssignOp op,CppiaExpr *inValue)
   {
      if (op!=aoSet)
         throw "TODO - arrayAccess undefined setter";
      assign = op;
      value = inValue;
      return this;
   }
   CppiaExpr  *makeCrement(CrementOp inOp)
   {
      throw "TODO - arrayAccess makeCrement";
      crement = inOp;
      return this;
   }

   void runVoid(CppiaCtx *ctx)
      { null val; run(ctx,val); BCR_VCHECK; }

   int runInt(CppiaCtx *ctx)
      { int val; run(ctx,val); BCR_CHECK; return val; }

   Float runFloat(CppiaCtx *ctx)
      { Float val; run(ctx,val); BCR_CHECK; return val; }

   ::String    runString(CppiaCtx *ctx)
      { String val; run(ctx,val); BCR_CHECK; return val; }

   hx::Object *runObject(CppiaCtx *ctx)
      { Dynamic val; run(ctx,val); BCR_CHECK; return val.mPtr; }
};


struct ArrayIExpr : public CppiaExpr
{
   int       classId;
   CppiaExpr *thisExpr;
   CppiaExpr *iExpr;
   CppiaExpr *value;
   CrementOp crementOp;
   AssignOp  assignOp;


   ArrayIExpr(CppiaStream &stream)
   {
      classId = stream.getInt();
      thisExpr = createCppiaExpr(stream);
      iExpr = createCppiaExpr(stream);
      value = 0;
      crementOp = coNone;
      assignOp = aoNone;
   }

   const char *getName() { return "ArrayIExpr"; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[classId];

      CppiaExpr *replace = 0;

      if (type->name==HX_CSTRING("Dynamic"))
      {
         replace = new DynamicArrayI(this, thisExpr, iExpr);
      }
      else
      {
         if (!type->arrayType)
         {
            replace = new ArrayAccessI(this,classId, thisExpr, iExpr);
         }
         else
         {
            Expressions val;
            val.push_back(iExpr);
            replace = createArrayBuiltin(this, type->arrayType, thisExpr, HX_CSTRING("__get"), val);
         }
      }
      replace = replace->link(inModule);

      delete this;
      return replace;
   }

};


struct EnumIExpr : public CppiaDynamicExpr
{
   CppiaExpr *object;
   int       index;
   int       classId;


   EnumIExpr(CppiaStream &stream)
   {
      classId = stream.getInt();
      index = stream.getInt();
      object = createCppiaExpr(stream);
   }

   const char *getName() { return "EnumIExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      object = object->link(inModule);
      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *obj = object->runObject(ctx);
      BCR_CHECK;
      #if (HXCPP_API_LEVEL>=330)
      return static_cast<EnumBase_obj *>(obj)->_hx_getParamI(index).mPtr;
      #else
      return obj->__EnumParams()[index].mPtr;
      #endif
   }

};


struct VarDecl : public CppiaVoidExpr
{
   CppiaStackVar var;
   CppiaExpr *init;

   VarDecl(CppiaStream &stream,bool inInit)
   {
      var.fromStream(stream);
      init = 0;
      if (inInit)
      {
         int t = stream.getInt();
         init = createCppiaExpr(stream);
         init->haxeTypeId = t;
      }
   }

   void runVoid(CppiaCtx *ctx)
   {
      if (init)
      {
         // TODO  - store type
         switch(var.expressionType)
         {
            case etInt: *(int *)(ctx->frame+var.stackPos) = init->runInt(ctx); break;
            case etFloat: SetFloatAligned(ctx->frame+var.stackPos,init->runFloat(ctx)); break;
            case etString: *(String *)(ctx->frame+var.stackPos) = init->runString(ctx); break;
            case etObject: *(hx::Object **)(ctx->frame+var.stackPos) = init->runObject(ctx); break;
            case etVoid:
            case etNull:
               break;
          }
      }
   }

   const char *getName() { return "VarDecl"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      var.link(inModule);
      init = init ? init->link(inModule) : 0;
      return this;
   }
};

struct TVars : public CppiaVoidExpr
{
   Expressions vars;

   TVars(CppiaStream &stream)
   {
      int count = stream.getInt();
      vars.resize(count);
      for(int i=0;i<count;i++)
      {
         std::string tok = stream.getToken();
         if (tok=="VARDECL")
            vars[i] = new VarDecl(stream,false);
         else if (tok=="VARDECLI")
            vars[i] = new VarDecl(stream,true);
         else
            throw "Bad var decl";
      }
   }
   CppiaExpr *link(CppiaModule &inModule)
   {
      LinkExpressions(vars,inModule);
      return this;
   }
   
   void runVoid(CppiaCtx *ctx)
   {
      CppiaExpr **v = &vars[0];
      CppiaExpr **end = v + vars.size();
      for(;v<end && !ctx->breakContReturn;v++)
         (*v)->runVoid(ctx);
   }
};

struct ForExpr : public CppiaVoidExpr
{
   CppiaStackVar var;
   CppiaExpr *init;
   CppiaExpr *loop;

 
   ForExpr(CppiaStream &stream)
   {
      var.fromStream(stream);
      init = createCppiaExpr(stream);
      loop = createCppiaExpr(stream);
   }

   const char *getName() { return "ForExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      var.link(inModule);
      init = init->link(inModule);
      loop = loop->link(inModule);
      return this;
   }

   void runVoid(CppiaCtx *ctx)
   {
      hx::Object *iterator = init->runObject(ctx);
      CPPIA_CHECK(iterator);
      BCR_VCHECK;
      Dynamic  hasNext = iterator->__Field(HX_CSTRING("hasNext"),HX_PROP_DYNAMIC);
      BCR_VCHECK;
      CPPIA_CHECK(hasNext.mPtr);
      Dynamic  getNext = iterator->__Field(HX_CSTRING("next"),HX_PROP_DYNAMIC);
      BCR_VCHECK;
      CPPIA_CHECK(getNext.mPtr);

      while(hasNext())
      {
         var.set(ctx,getNext());
         loop->runVoid(ctx);

         if (ctx->breakContReturn)
         {
            if (ctx->breakContReturn & (bcrBreak|bcrReturn))
               break;
            // clear Continue
            ctx->breakContReturn = 0;
         }
      }
   }
};

struct WhileExpr : public CppiaVoidExpr
{
   bool      isWhileDo;
   CppiaExpr *condition;
   CppiaExpr *loop;

 
   WhileExpr(CppiaStream &stream)
   {
      isWhileDo = stream.getInt();
      condition = createCppiaExpr(stream);
      loop = createCppiaExpr(stream);
   }

   const char *getName() { return "WhileExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      condition = condition->link(inModule);
      loop = loop->link(inModule);
      return this;
   }

   void runVoid(CppiaCtx *ctx)
   {
      if (isWhileDo && !condition->runInt(ctx))
         return;

      BCR_VCHECK;

      while(true)
      {
         loop->runVoid(ctx);

         if (ctx->breakContReturn)
         {
            if (ctx->breakContReturn & (bcrBreak|bcrReturn))
               break;
            // clear Continue
            ctx->breakContReturn = 0;
         }

         if (!condition->runInt(ctx) || ctx->breakContReturn)
            break;
      }
      ctx->breakContReturn &= ~bcrLoop;
   }
};

struct SwitchExpr : public CppiaExpr
{
   struct Case
   {
      Expressions conditions;
      CppiaExpr   *body;
   };
   int       caseCount;
   CppiaExpr *condition;
   std::vector<Case> cases;
   CppiaExpr *defaultCase;

 
   SwitchExpr(CppiaStream &stream)
   {
      caseCount = stream.getInt();
      bool hasDefault = stream.getInt();
      condition = createCppiaExpr(stream);
      cases.resize(caseCount);
      for(int i=0;i<caseCount;i++)
      {
         int count = stream.getInt();
         ReadExpressions(cases[i].conditions,stream,count);
         cases[i].body = createCppiaExpr(stream);
      }
      defaultCase = hasDefault ? createCppiaExpr(stream) : 0;
   }

   const char *getName() { return "SwitchExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      condition = condition->link(inModule);
      for(int i=0;i<caseCount;i++)
      {
         LinkExpressions(cases[i].conditions,inModule);
         cases[i].body = cases[i].body->link(inModule);
      }
      if (defaultCase)
         defaultCase = defaultCase->link(inModule);
      return this;
   }

   template<typename T>
   CppiaExpr *TGetBody(CppiaCtx *ctx)
   {
      T value;
      runValue(value,ctx,condition);
      for(int i=0;i<caseCount;i++)
      {
         Case &c = cases[i];
         for(int j=0;j<c.conditions.size();j++)
         {
            T caseVal;
            runValue(caseVal,ctx,c.conditions[j]);
            if (value==caseVal)
               return c.body;
         }
      }
      return defaultCase;
   }


   CppiaExpr *getBody(CppiaCtx *ctx)
   {
      switch(condition->getType())
      {
         case etInt :
             // todo - int/map ?
             return TGetBody<int>(ctx);
         case etString : return TGetBody<String>(ctx);
         case etFloat : return TGetBody<Float>(ctx);
         // Enum
         case etObject : return TGetBody<Dynamic>(ctx);
         default: ;
      }

     return defaultCase;
   }

   void runVoid(CppiaCtx *ctx)
   {
      CppiaExpr *body = getBody(ctx);
      BCR_VCHECK;
      if (body)
         body->runVoid(ctx);
   }
   int runInt(CppiaCtx *ctx)
   {
      CppiaExpr *body = getBody(ctx);
      BCR_CHECK;
      if (body)
         return body->runInt(ctx);
      return 0;
   }

   Float runFloat(CppiaCtx *ctx)
   {
      CppiaExpr *body = getBody(ctx);
      BCR_CHECK;
      if (body)
         return body->runFloat(ctx);
      return 0;
   }

   ::String    runString(CppiaCtx *ctx)
    {
      CppiaExpr *body = getBody(ctx);
      BCR_CHECK;
      if (body)
         return body->runString(ctx);
      return String();
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      CppiaExpr *body = getBody(ctx);
      BCR_CHECK;
      if (body)
         return body->runObject(ctx);
      return 0;
   }

};


struct TryExpr : public CppiaVoidExpr
{
   struct Catch
   {
      CppiaStackVar var;
      TypeData    *type;
      CppiaExpr   *body;
   };
   int       catchCount;
   CppiaExpr *body;
   std::vector<Catch> catches;

 
   TryExpr(CppiaStream &stream)
   {
      catchCount = stream.getInt();
      body = createCppiaExpr(stream);
      catches.resize(catchCount);
      for(int i=0;i<catchCount;i++)
      {
         catches[i].var.fromStream(stream);
         catches[i].body = createCppiaExpr(stream);
      }
   }

   const char *getName() { return "TryExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      body = body->link(inModule);
      for(int i=0;i<catchCount;i++)
      {
         Catch &c = catches[i];
         c.var.link(inModule);
         c.type = inModule.types[c.var.typeId];
         c.body = c.body->link(inModule);
      }
      return this;
   }
   // TODO - return types...
   void runVoid(CppiaCtx *ctx)
   {
      try
      {
         body->runVoid(ctx);
         BCR_VCHECK;
      }
      catch(Dynamic caught)
      {
         //Class cls = caught.mPtr ? caught->__GetClass() : 0;
         for(int i=0;i<catchCount;i++)
         {
            Catch &c = catches[i];
            if ( c.type->isClassOf(caught) )
            {
               HX_STACK_BEGIN_CATCH
               c.var.set(ctx,caught);
               c.body->runVoid(ctx);
               return;
            }
         }
         HX_STACK_DO_THROW(caught);
      }
   }
};


struct VarRef : public CppiaExpr
{
   int varId;
   CppiaStackVar *var;
   ExprType type;
   FieldStorage store;
   int      stackPos;

   VarRef(CppiaStream &stream)
   {
      varId = stream.getInt();
      var = 0;
      type = etVoid;
      stackPos = 0;
      store = fsUnknown;
   }

   ExprType getType() { return type; }

   const char *getName() { return "VarRef"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      var = inModule.layout->findVar(varId);
      if (!var)
      {
         // link to cppia static...

         printf("Could not link var %d %s.\n", varId, inModule.strings[ getStackVarNameId(varId) ].__s );
         inModule.layout->dump(inModule.strings,"");
         inModule.where(this);
         throw "Unknown variable";
      }

      type = var->expressionType;
      stackPos = var->stackPos;
      store = var->storeType;

      CppiaExpr *replace = 0;
      switch(store)
      {
         case fsBool:
             replace = new MemReference<bool,locStack>(this,var->stackPos);
             break;
         case fsByte:
         case fsInt:
               replace = new MemReference<int,locStack>(this,var->stackPos);
            break;
         case fsFloat:
            #ifdef HXCPP_ALIGN_FLOAT
            replace = new MemStackFloatReference(this,var->stackPos);
            #else
            replace = new MemReference<Float,locStack>(this,var->stackPos);
            #endif
            break;
         case fsString:
            replace = new MemReference<String,locStack>(this,var->stackPos);
            break;
         case fsObject:
            replace = new MemReference<hx::Object *,locStack>(this,var->stackPos);
            break;
         default: ;
      }
      if (replace)
      {
         replace->link(inModule);
         delete this;
         return replace;
      }

      return this;
   }
};


/*
struct JumpBreak : public CppiaVoidExpr
{
   JumpBreak() {  }
   void runVoid(CppiaCtx *ctx)
   {
      ctx->breakJump();
   }
   const char *getName() { return "JumpBreak"; }

};

template<typename T>
struct ThrowType : public CppiaVoidExpr
{
   T* value;

   ThrowType(T *inValue=0) { value = inValue; }
   void runVoid(CppiaCtx *ctx)
   {
      throw value;
   }
   const char *getName() { return "Break/Continue"; }

};
*/

struct FlagBreak : public CppiaVoidExpr
{
   int flag;

   FlagBreak(int inFlag) : flag(inFlag) {  }
   void runVoid(CppiaCtx *ctx)
   {
      ctx->breakContReturn |= flag;
   }
   const char *getName() { return flag==bcrBreak ? "Break" : "Continue"; }

};




struct RetVal : public CppiaVoidExpr
{
   bool      retVal;
   CppiaExpr *value;
   ExprType  returnType;

   RetVal(CppiaStream &stream,bool inRetVal)
   {
      retVal = inRetVal;
      if (retVal)
      {
         int t = stream.getInt();
         value = createCppiaExpr(stream);
         value->haxeTypeId = t;
      }
      else
         value = 0;
   }

   const char *getName() { return "RetVal"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (value)
      {
         value = value->link(inModule);
         returnType = inModule.layout->returnType;
      }
      else
         returnType = etNull;
      return this;
   }

   void runVoid(CppiaCtx *ctx)
   {
      #ifdef DEBUG_RETURN_TYPE
      gLastRet = returnType;
      #endif
      switch(returnType)
      {
         case etInt:
            ctx->returnInt( value->runInt(ctx) );
            break;
         case etFloat:
            ctx->returnFloat( value->runFloat(ctx) );
            break;
         case etString:
            ctx->returnString( value->runString(ctx) );
            break;
         case etObject:
            ctx->returnObject( value->runObject(ctx) );
            break;
         default:
            if (value)
               value->runVoid(ctx);
      }
      #ifdef SJLJ_RETURN
      ctx->longJump();
      #else
      ctx->returnFlag();
      #endif
   }
};



struct BinOp : public CppiaExpr
{
   CppiaExpr *left;
   CppiaExpr *right;
   ExprType type;

   BinOp(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
      right = createCppiaExpr(stream);
      type = etFloat;
   }

   ExprType getType() { return type; }

   const char *getName() { return "BinOp"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      left = left->link(inModule);
      right = right->link(inModule);

      if (left->getType()==etInt && right->getType()==etInt)
         type = etInt;
      else
         type = etFloat;
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (type==etInt)
         return Dynamic(runInt(ctx)).mPtr;
      return Dynamic(runFloat(ctx)).mPtr;
   }
   String runString(CppiaCtx *ctx)
   {
      if (type==etInt)
         return String(runInt(ctx));
      return String(runFloat(ctx));
   }
};


#define ARITH_OP(name,OP) \
struct name : public BinOp \
{ \
   name(CppiaStream &stream) : BinOp(stream) { } \
 \
   int runInt(CppiaCtx *ctx) \
   { \
      int lval = left->runInt(ctx); \
      BCR_CHECK; \
      return lval OP right->runInt(ctx); \
   } \
   Float runFloat(CppiaCtx *ctx) \
   { \
      Float lval = left->runFloat(ctx); \
      BCR_CHECK; \
      return lval OP right->runFloat(ctx); \
   } \
};

struct OpDiv : public BinOp
{
   OpDiv(CppiaStream &stream) : BinOp(stream) { }

   ExprType getType() { return etFloat; }
   int runInt(CppiaCtx *ctx)
   {
      // Int division - if you use 'cast' - do as float then cast to int
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return lval / right->runFloat(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return lval / right->runFloat(ctx);
   }
};


ARITH_OP(OpMult,*)
ARITH_OP(OpSub,-)


struct ThrowExpr : public CppiaVoidExpr
{
   CppiaExpr *value;
   ThrowExpr(CppiaStream &stream)
   {
      value = createCppiaExpr(stream);
   }
   const char *getName() { return "ThrowExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      value = value->link(inModule);
      return this;
   }
   void runVoid(CppiaCtx *ctx)
   {
      throw Dynamic( value->runObject(ctx) );
   }
};

struct OpNot : public CppiaBoolExpr
{
   CppiaExpr *value;
   OpNot(CppiaStream &stream)
   {
      value = createCppiaExpr(stream);
   }
   CppiaExpr *link(CppiaModule &inModule)
   {
      value = value->link(inModule);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      return ! value->runInt(ctx);
   }
};

struct OpAnd : public CppiaBoolExpr
{
   CppiaExpr *left;
   CppiaExpr *right;
   OpAnd(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
      right = createCppiaExpr(stream);
   }
   CppiaExpr *link(CppiaModule &inModule)
   {
      left = left->link(inModule);
      right = right->link(inModule);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      int l =  left->runInt(ctx);
      BCR_CHECK;
      return l && right->runInt(ctx);
   }
};


struct OpOr : public OpAnd
{
   OpOr(CppiaStream &stream) : OpAnd(stream) { }
   int runInt(CppiaCtx *ctx)
   {
      int l =  left->runInt(ctx);
      BCR_CHECK;
      return l || right->runInt(ctx);
   }
};




struct BitNot : public CppiaIntExpr
{
   CppiaExpr *left;
   BitNot(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
   }
   CppiaExpr *link(CppiaModule &inModule)
   {
      left = left->link(inModule);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      return ~left->runInt(ctx);
   }
};

struct BitAnd : public CppiaIntExpr
{
   CppiaExpr *left;
   CppiaExpr *right;
   BitAnd(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
      right = createCppiaExpr(stream);
   }
   const char *getName() { return "BitAnd"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      left = left->link(inModule);
      right = right->link(inModule);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return l & right->runInt(ctx);
   }
};

struct BitOr : public BitAnd
{
   BitOr(CppiaStream &stream) : BitAnd(stream) { }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return l | right->runInt(ctx);
   }
};


struct BitXOr : public BitAnd
{
   BitXOr(CppiaStream &stream) : BitAnd(stream) { }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return l ^ right->runInt(ctx);
   }
};


struct BitUSR : public BitAnd
{
   BitUSR(CppiaStream &stream) : BitAnd(stream) { }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return  hx::UShr(l , right->runInt(ctx));
   }
};


struct BitShiftR : public BitAnd
{
   BitShiftR(CppiaStream &stream) : BitAnd(stream) { }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return  l >> right->runInt(ctx);
   }
};


struct BitShiftL : public BitAnd
{
   BitShiftL(CppiaStream &stream) : BitAnd(stream) { }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return  l << right->runInt(ctx);
   }
};



template<bool AS_DYNAMIC>
struct SpecialAdd : public CppiaExpr
{
   CppiaExpr *left;
   CppiaExpr *right;

   SpecialAdd(const CppiaExpr *inSrc, CppiaExpr *inLeft, CppiaExpr *inRight)
      : CppiaExpr(inSrc)
   {
      left = inLeft;
      right = inRight;
   }
   void runVoid(CppiaCtx *ctx)
   {
      left->runVoid(ctx);
      right->runVoid(ctx);
   }
   String runString(CppiaCtx *ctx)
   {
      if (AS_DYNAMIC)
      {
         Dynamic lval = left->runObject(ctx);
         BCR_CHECK;
         return lval + Dynamic(right->runObject(ctx));
      }
      else
      {
         String lval = left->runString(ctx);
         BCR_CHECK;
         return lval + right->runString(ctx);
      }
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (AS_DYNAMIC)
      {
         Dynamic lval = left->runObject(ctx);
         BCR_CHECK;
         return (lval + Dynamic(right->runObject(ctx))).mPtr;
      }
 
      return Dynamic(runString(ctx)).mPtr;
   }
   int runInt(CppiaCtx *ctx)
   {
      if (AS_DYNAMIC)
      {
         Dynamic lval = left->runObject(ctx);
         BCR_CHECK;
         return (lval + Dynamic(right->runObject(ctx)))->__ToInt();
      }
 
      left->runVoid(ctx);
      BCR_CHECK;
      right->runVoid(ctx);
      return 0;
   }
   Float runFloat(CppiaCtx *ctx)
   {
      if (AS_DYNAMIC)
      {
         Dynamic lval = left->runObject(ctx);
         BCR_CHECK;
         return (lval + Dynamic(right->runObject(ctx)))->__ToDouble();
      }
 
      left->runVoid(ctx);
      BCR_CHECK;
      right->runVoid(ctx);
      return 0;
   }
};

struct OpNeg : public CppiaExpr
{
   CppiaExpr *value;
   ExprType type;
   OpNeg(CppiaStream &stream)
   {
      value = createCppiaExpr(stream);
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      value = value->link(inModule);
      type = value->getType()==etInt ? etInt : etFloat;
      return this;
   }

   ExprType getType() { return type; }

   void runVoid(CppiaCtx *ctx) { value->runVoid(ctx); }
   int runInt(CppiaCtx *ctx)
   {
      return - value->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      return - value->runFloat(ctx);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      return Dynamic(- value->runFloat(ctx)).mPtr;
   }
};

struct OpAdd : public BinOp
{
   OpAdd(CppiaStream &stream) : BinOp(stream)
   {
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      BinOp::link(inModule);

      if (left->getType()==etString || right->getType()==etString )
      {
         CppiaExpr *result = new SpecialAdd<false>(this,left,right);
         delete this;
         return result;
      }
      else if ( !isNumeric(left->getType()) || !isNumeric(right->getType()) )
      {
         CppiaExpr *result = new SpecialAdd<true>(this,left,right);
         delete this;
         return result;
      }

      return this;
   }

   void runVoid(CppiaCtx *ctx)
   {
      left->runVoid(ctx);
      BCR_VCHECK;
      right->runVoid(ctx);
   }
   int runInt(CppiaCtx *ctx)
   {
      int lval = left->runInt(ctx);
      BCR_CHECK;
      return lval + right->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return lval + right->runFloat(ctx);
   }
};


struct OpMod : public BinOp
{
   OpMod(CppiaStream &stream) : BinOp(stream)
   {
   }


   int runInt(CppiaCtx *ctx)
   {
      int lval = left->runInt(ctx);
      BCR_CHECK;
      return lval % right->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return hx::DoubleMod(lval,right->runFloat(ctx));
   }
};


struct EnumField : public CppiaDynamicExpr
{
   int                  enumId;
   int                  fieldId;
   CppiaEnumConstructor *value;
   Expressions          args;

   // Mark class?
   String               enumName;
   hx::Class                enumClass;

   EnumField(CppiaStream &stream,bool inWithArgs)
   {
      enumId = stream.getInt();
      fieldId = stream.getInt();
      value= 0;
      if (inWithArgs)
      {
         int argCount = stream.getInt();
         for(int a=0;a<argCount;a++)
            args.push_back( createCppiaExpr(stream) );
      }
   }

   const char *getName() { return "EnumField"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[enumId];
      if (type->cppiaClass)
      {
         if (!type->cppiaClass->isEnum)
            throw "Field of non-enum";
         value = type->cppiaClass->findEnum(fieldId);
      }
      else
      {
         enumClass = hx::Class_obj::Resolve(type->name);
         if (!enumClass.mPtr)
         {
            printf("Could not find enum %s\n", type->name.__s );
            throw "Bad enum";
         }
         enumName = inModule.strings[fieldId];
         inModule.markable.push_back(this);
      }

      LinkExpressions(args,inModule);
      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      int s = args.size();
      if (s==0)
         return value ? value->value.mPtr : enumClass->ConstructEnum(enumName,null()).mPtr;

      Array<Dynamic> dynArgs = Array_obj<Dynamic>::__new(s,s);
      for(int a=0;a<s;a++)
      {
         dynArgs[a] = args[a]->runObject(ctx);
         BCR_CHECK;
      }

      return value ? value->create(dynArgs) : enumClass->ConstructEnum(enumName,dynArgs).mPtr;
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(enumName);
      HX_MARK_MEMBER(enumClass);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(enumName);
      HX_VISIT_MEMBER(enumClass);
   }
#endif
};

struct CrementExpr : public CppiaExpr 
{
   CrementOp op;
   CppiaExpr *lvalue;

   CrementExpr(CppiaStream &stream,CrementOp inOp)
   {
      op = inOp;
      lvalue = createCppiaExpr(stream);
   }
   const char *getName() { return "CrementExpr"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      lvalue = lvalue->link(inModule);
      CppiaExpr *replace = lvalue->makeCrement( op );
      if (!replace)
      {
         printf("Could not create increment operator\n");
         inModule.where(lvalue);
         throw "Bad increment";
      }
      replace->link(inModule);
      delete this;
      return replace;
   }
};

struct OpCompareBase : public CppiaBoolExpr 
{
   enum CompareType { compFloat, compInt, compString, compDynamic };

   CppiaExpr *left;
   CppiaExpr *right;
   CompareType compareType;

   OpCompareBase(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
      right = createCppiaExpr(stream);
      compareType = compDynamic;
   }

   const char *getName() { return "OpCompare"; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      left = left->link(inModule);
      right = right->link(inModule);
      ExprType t1 = left->getType();
      ExprType t2 = right->getType();

      if (isNumeric(t1) && isNumeric(t2))
      {
         compareType = t1==etInt && t2==etInt ? compInt : compFloat;
      }
      else if (t1==etString || t2==etString)
      {
         compareType = compString;
      }
      else
         compareType = compDynamic;

      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      bool result = runInt(ctx);
      return Dynamic(result).mPtr;
   }
};

template<typename COMPARE>
struct OpCompare : public OpCompareBase 
{
   COMPARE compare;

   OpCompare(CppiaStream &stream)
      : OpCompareBase(stream) { }

   int runInt(CppiaCtx *ctx)
   {
      switch(compareType)
      {
         case compFloat:
         {
            Float leftVal = left->runFloat(ctx);
            BCR_CHECK;
            Float rightVal = right->runFloat(ctx);
            return compare.test(leftVal,rightVal);
         }
         case compInt:
         {
            int leftVal = left->runInt(ctx);
            BCR_CHECK;
            int rightVal = right->runInt(ctx);
            return compare.test(leftVal,rightVal);
         }
         case compString:
         {
            String leftVal = left->runString(ctx);
            BCR_CHECK;
            String rightVal = right->runString(ctx);
            return compare.test(leftVal,rightVal);
         }
         case compDynamic:
         {
            Dynamic leftVal(left->runObject(ctx));
            BCR_CHECK;
            Dynamic rightVal(right->runObject(ctx));
            return compare.test(leftVal,rightVal);
         }
      }

      return 0;
   }
};


#define DEFINE_COMPARE_OP(name,OP) \
struct name \
{ \
   template<typename T> \
   inline bool test(const T &left, const T&right) \
   { \
      return left OP right; \
   } \
};

DEFINE_COMPARE_OP(CompareLess,<);
DEFINE_COMPARE_OP(CompareLessEq,<=);
DEFINE_COMPARE_OP(CompareGreater,>);
DEFINE_COMPARE_OP(CompareGreaterEq,>=);
DEFINE_COMPARE_OP(CompareEqual,==);
DEFINE_COMPARE_OP(CompareNotEqual,!=);



CppiaExpr *createCppiaExpr(CppiaStream &stream)
{
   int fileId = stream.getFileId();
   int line = stream.getLineId();
   std::string tok = stream.getToken();
   //DBGLOG(" expr %s\n", tok.c_str() );

   CppiaExpr *result = 0;
   if (tok=="FUN")
      result = new ScriptCallable(stream);
   else if (tok=="BLOCK")
      result = new BlockExpr(stream);
   else if (tok=="IFELSE")
      result = new IfElseExpr(stream);
   else if (tok=="IF")
      result = new IfExpr(stream);
   else if (tok=="ISNULL")
      result = new CppiaIsNull(stream);
   else if (tok=="NOTNULL")
      result = new CppiaIsNotNull(stream);
   else if (tok=="CALLSTATIC")
      result = new CallStatic(stream);
   else if (tok=="CALLTHIS")
      result = new CallMember(stream,callThis);
   else if (tok=="CALLSUPERNEW")
      result = new CallMember(stream,callSuperNew);
   else if (tok=="CALLSUPER")
      result = new CallMember(stream,callSuper);
   else if (tok=="CALLMEMBER")
      result = new CallMember(stream,callObject);
   else if (tok=="CALLGLOBAL")
      result = new CallGlobal(stream);
   else if (tok=="true")
      result = new DataVal<bool>(true);
   else if (tok=="false")
      result = new DataVal<bool>(false);
   else if (tok=="s")
      result = new StringVal(stream.getInt());
   else if (tok=="f")
      result = new DataVal<Float>(atof( stream.module->strings[stream.getInt()].__s ));
   else if (tok=="i")
      result = new DataVal<int>(stream.getInt());
   else if (tok=="POSINFO")
      result = new PosInfo(stream);
   else if (tok=="VAR")
      result = new VarRef(stream);
   else if (tok=="WHILE")
      result = new WhileExpr(stream);
   else if (tok=="FOR")
      result = new ForExpr(stream);
   else if (tok=="RETVAL")
      result = new RetVal(stream,true);
   else if (tok=="RETURN")
      result = new RetVal(stream,false);
   else if (tok=="THROW")
      result = new ThrowExpr(stream);
   else if (tok=="CALL")
      result = new Call(stream);
   else if (tok=="FLINK")
      result = new GetFieldByLinkage(stream,false);
   else if (tok=="THIS")
      result = new ThisExpr(stream);
   else if (tok=="CLASSOF")
      result = new ClassOfExpr(stream);
   else if (tok=="FTHISINST")
      result = new GetFieldByLinkage(stream,true);
   else if (tok=="FNAME")
      result = new GetFieldByName(stream,false);
   else if (tok=="FSTATIC")
      result = new GetFieldByName(stream,false,true);
   else if (tok=="FTHISNAME")
      result = new GetFieldByName(stream,true);
   else if (tok=="FENUM")
      result = new EnumField(stream,false);
   else if (tok=="CREATEENUM")
      result = new EnumField(stream,true);
   else if (tok=="NULL")
      result = new NullVal();
   else if (tok=="TVARS")
      result = new TVars(stream);
   else if (tok=="NEW")
      result = new NewExpr(stream);
   else if (tok=="ARRAYI")
      result = new ArrayIExpr(stream);
   else if (tok=="ENUMI")
      result = new EnumIExpr(stream);
   else if (tok=="ADEF")
      result = new ArrayDef(stream);
   else if (tok=="OBJDEF")
      result = new ObjectDef(stream);
   else if (tok=="TOINTERFACE")
      result = new ToInterface(stream,false);
   else if (tok=="TOINTERFACEARRAY")
      result = new ToInterface(stream,true);
   else if (tok=="CAST")
      result = new CastExpr(stream,castDynamic);
   else if (tok=="NOCAST")
      result = new CastExpr(stream,castNOP);
   else if (tok=="CASTINT")
      result = new CastExpr(stream,castInt);
   else if (tok=="CASTBOOL")
      result = new CastExpr(stream,castBool);
   else if (tok=="TODYNARRAY")
      result = new CastExpr(stream,castDynArray);
   else if (tok=="TODATAARRAY")
      result = new CastExpr(stream,castDataArray);
   else if (tok=="SWITCH")
      result = new SwitchExpr(stream);
   else if (tok=="TRY")
      result = new TryExpr(stream);
   else if (tok=="BREAK")
      result = new FlagBreak(bcrBreak);
   else if (tok=="CONTINUE")
      result = new FlagBreak(bcrContinue);
   // Uniops..
   else if (tok=="NEG")
      result = new OpNeg(stream);
   else if (tok=="++")
      result = new CrementExpr(stream,coPreInc);
   else if (tok=="+++")
      result = new CrementExpr(stream,coPostInc);
   else if (tok=="--")
      result = new CrementExpr(stream,coPreDec);
   else if (tok=="---")
      result = new CrementExpr(stream,coPostDec);
   // Arithmetic
   else if (tok=="+")
      result = new OpAdd(stream);
   else if (tok=="*")
      result = new OpMult(stream);
   else if (tok=="/")
      result = new OpDiv(stream);
   else if (tok=="-")
      result = new OpSub(stream);
   else if (tok=="&")
      result = new BitAnd(stream);
   else if (tok=="|")
      result = new BitOr(stream);
   else if (tok=="^")
      result = new BitXOr(stream);
   else if (tok=="<<")
      result = new BitShiftL(stream);
   else if (tok==">>")
      result = new BitShiftR(stream);
   else if (tok==">>>")
      result = new BitUSR(stream);
   else if (tok=="%")
      result = new OpMod(stream);
   // Arithmetic =
   else if (tok=="SET")
      result = new SetExpr(stream,aoSet);
   else if (tok=="+=")
      result = new SetExpr(stream,aoAdd);
   else if (tok=="*=")
      result = new SetExpr(stream,aoMult);
   else if (tok=="/=")
      result = new SetExpr(stream,aoDiv);
   else if (tok=="-=")
      result = new SetExpr(stream,aoSub);
   else if (tok=="&=")
      result = new SetExpr(stream,aoAnd);
   else if (tok=="|=")
      result = new SetExpr(stream,aoOr);
   else if (tok=="^=")
      result = new SetExpr(stream,aoXOr);
   else if (tok=="<<=")
      result = new SetExpr(stream,aoShl);
   else if (tok==">>=")
      result = new SetExpr(stream,aoShr);
   else if (tok==">>>=")
      result = new SetExpr(stream,aoUShr);
   else if (tok=="%=")
      result = new SetExpr(stream,aoMod);
   // Comparison
   else if (tok=="<")
      result = new OpCompare<CompareLess>(stream);
   else if (tok=="<=")
      result = new OpCompare<CompareLessEq>(stream);
   else if (tok==">")
      result = new OpCompare<CompareGreater>(stream);
   else if (tok==">=")
      result = new OpCompare<CompareGreaterEq>(stream);
   else if (tok=="==")
      result = new OpCompare<CompareEqual>(stream);
   else if (tok=="!=")
      result = new OpCompare<CompareNotEqual>(stream);
   // Logical
   else if (tok=="!")
      result = new OpNot(stream);
   else if (tok=="&&")
      result = new OpAnd(stream);
   else if (tok=="||")
      result = new OpOr(stream);
   else if (tok=="~")
      result = new BitNot(stream);

   if (!result)
      throw "invalid expression";

   stream.module->setDebug(result, fileId, line);


   return result;
}

// --- TypeData -------------------------

void TypeData::link(CppiaModule &inModule)
{
   if (linked)
      return;

   linked = true;

   TypeData *cppiaSuperType = 0;
   if (cppiaClass && cppiaClass->superId)
   {
     cppiaSuperType = inModule.types[cppiaClass->superId];
     cppiaSuperType->link(inModule);
   }

   if (name.length>0)
   {
      haxeClass = hx::Class_obj::Resolve(name);
      int scriptId = getScriptId(haxeClass);
      if (scriptId>0 && scriptId!=inModule.scriptId)
      {
         DBGLOG("Reference to old script %s - ignoring\n", haxeClass.mPtr->mName.__s);
         haxeClass.mPtr = 0;
      }
      DBGLOG("Link %s, haxe=%s\n", name.__s, haxeClass.mPtr ? haxeClass.mPtr->mName.__s : "?" );
      if (!haxeClass.mPtr && !cppiaClass && name==HX_CSTRING("int"))
      {
         name = HX_CSTRING("Int");
         haxeClass = hx::Class_obj::Resolve(name);
      }
      if (!haxeClass.mPtr && !cppiaClass && name==HX_CSTRING("bool"))
      {
         name = HX_CSTRING("Bool");
         haxeClass = hx::Class_obj::Resolve(name);
      }
      if (!haxeClass.mPtr && !cppiaClass && (name==HX_CSTRING("float") || name==HX_CSTRING("double")))
      {
         name = HX_CSTRING("Float");
         haxeClass = hx::Class_obj::Resolve(name);
      }

      DBGLOG(" link type '%s' %s ", name.__s, haxeClass.mPtr ? "native" : "script" );
      interfaceBase = HaxeNativeInterface::findInterface(name.__s);
      isInterface = interfaceBase || (cppiaClass && cppiaClass->isInterface);

      if (!haxeClass.mPtr && name.substr(0,6)==HX_CSTRING("Array.") || name==HX_CSTRING("Array") )
      {
         haxeClass = hx::Class_obj::Resolve(HX_CSTRING("Array"));
         if (name.length==5)
            arrayType = arrAny;
         else
         {
            String t = name.substr(6,null());

            if (t==HX_CSTRING("int"))
               arrayType = arrInt;
            else if (t==HX_CSTRING("bool"))
               arrayType = arrBool;
            else if (t==HX_CSTRING("Float"))
               arrayType = arrFloat;
            else if (t==HX_CSTRING("String"))
               arrayType = arrString;
            else if (t==HX_CSTRING("unsigned char"))
               arrayType = arrUnsignedChar;
            else if (t==HX_CSTRING("Any"))
               arrayType = arrAny;
            else if (t==HX_CSTRING("Object"))
               arrayType = arrObject;
            else
               throw "Unknown array type";
         }

         DBGLOG("array type %d\n",arrayType);
      }
      else if (!haxeClass.mPtr && cppiaSuperType)
      {
         haxeBase = cppiaSuperType->haxeBase;
         haxeClass.mPtr = cppiaSuperType->haxeClass.mPtr;
         DBGLOG("extends %s\n", cppiaSuperType->name.__s);
      }
      else if (!haxeClass.mPtr)
      {
         haxeBase = isInterface ? 0 : HaxeNativeClass::hxObject();

         /*
         if (!isInterface && (*sScriptRegistered)[name.__s])
         {
            printf("Base class %s\n", name.__s);
            (*sScriptRegistered)[name.__s]->dump();
            throw "New class, but with existing def";
         }
         */
         DBGLOG(isInterface ? "interface base\n" : "base\n");
      }
      else
      {
         haxeBase = HaxeNativeClass::findClass(name.__s);
         if (!haxeBase)
         {
            if (isInterface)
            {
               DBGLOG("interface base\n");
               haxeBase = 0;
            }
            else
            {
               DBGLOG("assumed base (todo - register)\n");
               haxeBase = HaxeNativeClass::hxObject();
            }
         }
         else
         {
            DBGLOG("\n");
         }
      }

      if (name==HX_CSTRING("Void"))
         expressionType = etVoid;
      else if (name==HX_CSTRING("Null"))
         expressionType = etNull;
      else if (name==HX_CSTRING("String"))
         expressionType = etString;
      else if (name==HX_CSTRING("Float"))
         expressionType = etFloat;
      else if (name==HX_CSTRING("Int") || name==HX_CSTRING("Bool"))
         expressionType = etInt;
      else
         expressionType = etObject;
   }
   else
   {
      haxeBase = HaxeNativeClass::hxObject();
      haxeClass = hx::Class_obj::Resolve(HX_CSTRING("Dynamic"));
   }
}

// --- CppiaModule -------------------------

CppiaModule::~CppiaModule()
{
   delete main;
   for(int i=0;i<classes.size();i++)
      delete classes[i];
}

void CppiaModule::link()
{
   DBGLOG("Resolve registered - super\n");
   HaxeNativeClass::link();
   
   DBGLOG("Resolve typeIds\n");
   for(int t=0;t<types.size();t++)
      types[t]->link(*this);

   DBGLOG("Resolve inherited atributes\n");
   for(int i=0;i<classes.size();i++)
   {
      classes[i]->linkTypes();
   }

   for(int i=0;i<classes.size();i++)
   {
      linkingClass = classes[i];
      classes[i]->link();
   }
   linkingClass = 0;

   if (main)
      main = (ScriptCallable *)main->link(*this);
}

#ifdef CPPIA_JIT
void CppiaModule::compile()
{
   for(int i=0;i<classes.size();i++)
      classes[i]->compile();

   if (main)
      main->compile();
}
#endif


/*
CppiaClassInfo *CppiaModule::findClass(String inName)
{
   for(int i=0;i<classes.size();i++)
      if (strings[classes[i]->nameId] == inName)
         return classes[i];
   return 0;
}
*/


void CppiaModule::mark(hx::MarkContext *__inCtx)
{
   DBGLOG(" --- MARK --- \n");
   HX_MARK_MEMBER(strings);
   for(int i=0;i<types.size();i++)
   {
      if (types[i]) /* May be partially constructed */
         types[i]->mark(__inCtx);
   }
   for(int i=0;i<markable.size();i++)
   {
      markable[i]->mark(__inCtx);
   }
   for(int i=0;i<classes.size();i++)
      if (classes[i])
      {
         classes[i]->mark(__inCtx);
      }
}

#ifdef HXCPP_VISIT_ALLOCS
void CppiaModule::visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(strings);
   for(int i=0;i<types.size();i++)
      types[i]->visit(__inCtx);
   for(int i=0;i<markable.size();i++)
      markable[i]->visit(__inCtx);
   for(int i=0;i<classes.size();i++)
      if (classes[i])
         classes[i]->visit(__inCtx);
}
#endif

// TODO  - more than one
//static hx::Object *currentCppia = 0;

std::vector<hx::Resource> scriptResources;


// Cppia Object - manage
class CppiaObject : public hx::Object
{
public:
   CppiaModule *data;
   CppiaObject(CppiaModule *inModule)
   {
      data = inModule;
      GCSetFinalizer( this, onRelease );
   }
   static void onRelease(hx::Object *inObj)
   {
      delete ((CppiaObject *)inObj)->data;
   }
   void __Mark(hx::MarkContext *ctx) { data->mark(ctx); }
#ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *ctx) { data->visit(ctx); }
#endif
};





bool LoadCppia(String inValue)
{
   CppiaModule   *cppiaPtr = new CppiaModule();
   hx::Object **ptrPtr = new hx::Object*[1];
   *ptrPtr = new CppiaObject(cppiaPtr); 
   GCAddRoot(ptrPtr);


   CppiaModule   &cppia = *cppiaPtr;
   CppiaStream stream(cppiaPtr,inValue.__s, inValue.length);

   String error;
   try
   {
      std::string tok = stream.getAsciiToken();
      if (tok!="CPPIA" && tok!="CPPIB")
         throw "Bad magic";

      stream.setBinary(tok=="CPPIB");

      int stringCount = stream.getAsciiInt();
      cppia.cStrings.resize(stringCount);
      for(int s=0;s<stringCount;s++)
      {
         cppia.strings[s] = stream.readString();
         cppia.cStrings[s] = std::string(cppia.strings[s].__s,cppia.strings[s].length);
      }

      int typeCount = stream.getAsciiInt();
      cppia.types.resize(typeCount);
      DBGLOG("Type count : %d\n", typeCount);
      for(int t=0;t<typeCount;t++)
         cppia.types[t] = new TypeData(stream.readString());

      int classCount = stream.getAsciiInt();
      DBGLOG("Class count : %d\n", classCount);

      if (stream.binary)
      {
         int newLine = stream.getByte();
         if (newLine!='\n')
            throw "Missing new-line after class count";
      }

      cppia.classes.reserve(classCount);
      for(int c=0;c<classCount;c++)
      {
         CppiaClassInfo *info = new CppiaClassInfo(cppia);
         if (info->load(stream))
            cppia.classes.push_back(info);
      }

      tok = stream.getToken();
      if (tok=="MAIN")
      {
         DBGLOG("Main...\n");
         cppia.main = new ScriptCallable(createCppiaExpr(stream));
      }
      else if (tok!="NOMAIN")
         throw "no main specified";

      tok = stream.getToken();
      if (tok=="RESOURCES")
      {
         int count = stream.getInt( );
         scriptResources.resize(count+1);
         for(int r=0;r<count;r++)
         {
            tok = stream.getToken();
            if (tok!="RESO")
               throw "no reso tag";
            
            scriptResources[r].mName = cppia.strings[stream.getInt()];
            scriptResources[r].mDataLength = stream.getInt();
         }
         if (!stream.binary)
            stream.skipChar();

         for(int r=0;r<count;r++)
         {
            int len = scriptResources[r].mDataLength;
            unsigned char *buffer = (unsigned char *)malloc(len+5);
            *(int *)buffer = 0xffffffff;
            buffer[len+5-1] = '\0';
            stream.readBytes(buffer+4, len);
            scriptResources[r].mData = buffer + 4;
         }
         scriptResources[count].mDataLength = 0;
         scriptResources[count].mData = 0;
         scriptResources[count].mName = String();
         
         RegisterResources(&scriptResources[0]);
      }
      else
         throw "no resources tag";


   }
   catch(const char *errorString)
   {
      error = HX_CSTRING("Error reading file ") + String(errorString) + 
                HX_CSTRING(", line ") + String(stream.line) + HX_CSTRING(", char ") + 
                   String(stream.pos);
   }

   if (!error.__s)
      try
      {
         DBGLOG("Link...\n");
         cppia.link();
      }
      catch(const char *errorString)
      {
         error = String(errorString);
      }

   #ifdef CPPIA_JIT
   if (!error.__s)
      try
      {
         DBGLOG("Compile...\n");
         cppia.compile();
      }
      catch(const char *errorString)
      {
         error = String(errorString);
      }
   #endif


   if (!error.__s) try
   {
      //__hxcpp_enable(false);
      DBGLOG("--- Run --------------------------------------------\n");

      CppiaCtx *ctx = CppiaCtx::getCurrent();
      cppia.boot(ctx);
      if (cppia.main)
      {
         ctx->runVoid(cppia.main);
         //printf("Result %s.\n", cppia.main->runString(&ctx).__s);
      }
      return true;
   }
   catch(const char *errorString)
   {
      error = String(errorString);
   }

   if (error.__s)
      hx::Throw(error);

   return false;
}

::String ScriptableToString(void *inClass)
{
   CppiaClassInfo *info = (CppiaClassInfo *)inClass;
   return info->mClass->toString();
}

int ScriptableGetType(void *inClass)
{
   CppiaClassInfo *info = (CppiaClassInfo *)inClass;
   return info->__GetType();
}


hx::Class ScriptableGetClass(void *inClass)
{
   CppiaClassInfo *info = (CppiaClassInfo *)inClass;
   return info->mClass;
}


// Called by haxe generated code ...
void ScriptableMark(void *inClass, hx::Object *inThis, hx::MarkContext *__inCtx)
{
   ((CppiaClassInfo *)inClass)->markInstance(inThis, __inCtx);
}

#ifdef HXCPP_VISIT_ALLOCS
void ScriptableVisit(void *inClass, hx::Object *inThis, hx::VisitContext *__inCtx)
{
   ((CppiaClassInfo *)inClass)->visitInstance(inThis, __inCtx);
}
#endif

bool ScriptableField(hx::Object *inObj, const ::String &inName,hx::PropertyAccess  inCallProp,Dynamic &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->getField(inObj,inName,inCallProp,outResult);
}

bool ScriptableField(hx::Object *inObj, int inName,hx::PropertyAccess  inCallProp,Float &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   Dynamic result;
   if ( ((CppiaClassInfo *)vtable[-1])->getField(inObj,__hxcpp_field_from_id(inName),inCallProp,result) )
   {
      //CPPIA_CHECK(result);
      if (result.mPtr)
         outResult = result->__ToDouble();
      else
         outResult = 0;
      return true;
   }
   return false;
}

bool ScriptableField(hx::Object *inObj, int inName,hx::PropertyAccess  inCallProp,Dynamic &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->getField(inObj,__hxcpp_field_from_id(inName),inCallProp,outResult);
}

void ScriptableGetFields(hx::Object *inObject, Array< ::String> &outFields)
{
   void **vtable = inObject->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->GetInstanceFields(inObject,outFields);
}

bool ScriptableSetField(hx::Object *inObj, const ::String &inName, Dynamic inValue,hx::PropertyAccess  inCallProp, Dynamic &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->setField(inObj,inName,inValue,inCallProp,outResult);
}

#if (HXCPP_API_LEVEL >= 330)
void *hx::Object::_hx_getInterface(int inId)
{
   void **vtable = __GetScriptVTable();
   if (!vtable)
      return 0;
   CppiaClassInfo *info = ((CppiaClassInfo *)vtable[-1]);
   void *result = info->interfaceScriptTables[inId];
   return info->interfaceScriptTables[inId];
}
#endif




};


void __scriptable_load_cppia(String inCode)
{
   hx::LoadCppia(inCode);
}


