   #include <hxcpp.h>
#include "Cppia.h"
#include "CppiaStream.h"

namespace hx
{


static int sTypeSize[] = { 0, 0, sizeof(hx::Object *), sizeof(String), sizeof(Float), sizeof(int) };

// --- ScriptCallable ----------------------------------------------------

String sInvalidArgCount = HX_CSTRING("Invalid arguement count");


#ifdef CPPIA_JIT
void SLJIT_CALL argToInt(CppiaCtx *ctx) { ctx->pushInt( (* (hx::Object **)(ctx->pointer))->__ToInt() ); }
void SLJIT_CALL argToDouble(CppiaCtx *ctx) { ctx->pushFloat( (* (hx::Object **)(ctx->pointer))->__ToDouble() ); }
void SLJIT_CALL argToString(CppiaCtx *ctx) { ctx->pushString( (* (hx::Object **)(ctx->pointer))->__ToString() ); }
#endif





ScriptCallable::ScriptCallable(CppiaStream &stream)
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
   hasDefaults = false;
   #ifdef CPPIA_JIT
   compiled = 0;
   #endif
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


ScriptCallable::ScriptCallable(CppiaExpr *inBody) : CppiaDynamicExpr(inBody)
{
   returnTypeId = 0;
   returnType = etVoid;
   argCount = 0;
   stackSize = 0;
   captureSize = 0;
   hasDefaults = false;
   body = inBody;
   data = 0;
   #ifdef CPPIA_JIT
   compiled = 0;
   #endif
}


class RelayExpr : public CppiaExpr
{
   StackExecute execute;
   public:
      RelayExpr( StackExecute inExecute )
      {
         execute = inExecute;
      }

   void runVoid(CppiaCtx *ctx)
   {
      execute(ctx);
   }
};

ScriptCallable::ScriptCallable(CppiaModule &inModule,ScriptNamedFunction *inFunction)
{
   returnTypeId = 0;
   returnType = etVoid;

   argCount = 0;
   stackSize = 0;
   captureSize = 0;
   hasDefaults = false;
   //body = inBody;
   data = 0;

   const char *signature = inFunction->signature;
   int argCount = strlen(signature)-1;
   if (argCount<0)
      throw "ScriptNamedFunction: Invalid arg count";

   args.resize(argCount);
   hasDefault.resize(argCount);
   initVals.resize(argCount);
   for(int i=0;i<args.size();i++)
   {
      CppiaStackVar &arg = args[i];
      arg.nameId = 0;
      switch(signature[i+1])
      {
         case sigBool:
         case sigInt:
            arg.argType = etInt;
            arg.storeType = fsInt;
            break;
         case sigFloat:
            arg.argType = etFloat;
            arg.storeType = fsFloat;
            break;
         case sigString:
            arg.argType = etString;
            hasDefault[i] = true;
            arg.storeType = fsString;
            break;
         case sigObject:
            arg.argType = etObject;
            hasDefault[i] = true;
            arg.storeType = fsObject;
            break;
         default:
            throw "Bad haxe signature";
      }
   }
   body = new RelayExpr(inFunction->execute);

   #ifdef CPPIA_JIT
   // magically already compiled for us
   compiled = inFunction->execute;
   #endif
}


ScriptCallable::~ScriptCallable()
{
   #ifdef CPPIA_JIT
   if (compiled)
      CppiaCompiler::freeCompiled(compiled);
   #endif
}

CppiaExpr *ScriptCallable::link(CppiaModule &inModule)
{
   StackLayout *oldLayout = inModule.layout;
   StackLayout layout(oldLayout);
   inModule.layout = &layout;
   data = &inModule;

   returnType = inModule.types[ returnTypeId ]->expressionType;
   layout.returnType = returnType;

   for(int a=0;a<args.size();a++)
      args[a].link(inModule, hasDefault[a]);

   for(int a=0;a<initVals.size();a++)
      if (hasDefault[a])
      {
         args[a].linkDefault();
         hasDefaults = true;
      }

   body = body->link(inModule);

   captureVars.swap(layout.captureVars);
   captureSize = layout.captureSize;

   #ifdef HXCPP_STACK_SCRIPTABLE
   std::swap(varMap,layout.varMap);
   position.scriptCallable = this;
   #endif

   stackSize = layout.size;
   inModule.layout = oldLayout;

   position.className = className;
   position.functionName = functionName;
   position.fileName = filename;
   position.fullName = filename;

   #ifdef HXCPP_DEBUGGER
   position.fileHash = Hash(0,filename);
   int hash = Hash(0,className);
   hash = Hash(hash,".");
   position.classFuncHash = Hash(hash,functionName);
   #endif

   #ifdef CPPIA_JIT
   //printf("Compile?\n");
   #endif

   return this;
}

#ifdef HXCPP_STACK_SCRIPTABLE
void ScriptCallable::getScriptableVariables(unsigned char *inStack, Array<Dynamic> outNames)
{
   unsigned char *frame = inStack - stackSize;

   hx::Object *thizz = *(hx::Object **)frame;
   if (thizz)
      outNames->push(HX_CSTRING("this"));

   for(CppiaStackVarMap::iterator i=varMap.begin();i!=varMap.end();++i)
   {
      CppiaStackVar *var = i->second;
      outNames->push(var->module->strings[ var->nameId] );
   }
}


bool ScriptCallable::getScriptableValue(unsigned char *inStack, String inName, ::Dynamic &outValue)
{
   unsigned char *frame = inStack - stackSize;
   if (inName == HX_CSTRING("this"))
   {
      outValue = *(hx::Object **)frame;
      return true;
   }


   for(CppiaStackVarMap::iterator i=varMap.begin();i!=varMap.end();++i)
   {
      CppiaStackVar *var = i->second;
      if ( var->module->strings[ var->nameId]==inName)
      {
         outValue = var->getInFrame(frame);
         return true;
      }
   }

   return false;
}


bool ScriptCallable::setScriptableValue(unsigned char *inStack, String inName, ::Dynamic inValue)
{
   unsigned char *frame = inStack - stackSize;
   for(CppiaStackVarMap::iterator i=varMap.begin();i!=varMap.end();++i)
   {
      CppiaStackVar *var = i->second;
      if ( var->module->strings[ var->nameId]==inName)
      {
         var->setInFrame(frame,inValue);
         return true;
      }
   }

   return false;
}


#endif


int ScriptCallable::Hash(int value, const char *inString)
{
   if (inString)
      while(*inString)
         value = value*223 + *inString++;
   return value;
}


#ifdef CPPIA_JIT


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



void ScriptCallable::genArgs(CppiaCompiler *compiler, CppiaExpr *inThis, Expressions &inArgs, const JitVal &inThisVal)
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


   int framePos = compiler->getCurrentFrameSize();
   // Push this ...
   if (inThis)
   {
      inThis->genCode(compiler, JitFramePos(framePos), etObject);
   }
   else if (inThisVal.valid())
   {
      compiler->move( JitFramePos(framePos), inThisVal );
   }
   else
   {
      compiler->move( JitFramePos(framePos), JitVal( (void *)0 ));
   }
   compiler->addFrame(etObject);


   for(int a=0;a<argCount;a++)
   {
      CppiaStackVar &var = args[a];
      if (a>=inCount)
      {
         int framePos = compiler->getCurrentFrameSize();
         switch(var.argType)
         {
            case etInt:
               compiler->move( JitFramePos(framePos).as(jtInt), 0 );
               break;
            case etObject:
               compiler->move( JitFramePos(framePos).as(jtPointer), (void *)0 );
               break;
            case etFloat:
               compiler->move( JitFramePos(framePos).as(jtInt), 0 );
               compiler->move( JitFramePos(framePos).as(jtInt) + 4, 0 );
               break;
            case etString:
               compiler->move( JitFramePos(framePos).as(jtInt), 0 );
               compiler->move( JitFramePos(framePos).as(jtPointer) + StringOffset::Ptr, (void *)0 );
               break;
            default: ;
         }
         compiler->addFrame(var.argType);
      }
      else
      {
         int framePos = compiler->getCurrentFrameSize();
         inArgs[a]->genCode(compiler, JitFramePos(framePos).as( getJitType(var.argType) ), var.argType);
      }
      compiler->addFrame(var.argType);
   }
}


void ScriptCallable::genDefaults(CppiaCompiler *compiler)
{
   if (hasDefaults)
   {
      for(int i=0;i<args.size();i++)
         if (hasDefault[i])
            args[i].genDefault(compiler, initVals[i]);
   }
}


#endif


void ScriptCallable::pushArgs(CppiaCtx *ctx, hx::Object *inThis, Expressions &inArgs)
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
      if (a>=inCount)
      {
         pushDefault(ctx,a);
      }
      else
      {
         switch(var.argType)
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



void ScriptCallable::pushArgsDynamic(CppiaCtx *ctx, hx::Object *inThis, Array<Dynamic> &inArgs)
{
   BCR_VCHECK;

   int inCount = inArgs==null() ? 0 : inArgs->length;

   ctx->push( inThis );

   for(int a=0;a<argCount;a++)
   {
      CppiaStackVar &var = args[a];

      if (a<inCount && (inArgs[a].mPtr || !hasDefault[a]))
      {
         switch(var.argType)
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
         pushDefault(ctx,a);
      }
   }
}


// Return the closure
hx::Object *ScriptCallable::runObject(CppiaCtx *ctx)
{
   return createClosure(ctx,this);
}

const char *ScriptCallable::getName() { return "ScriptCallable"; }
String ScriptCallable::runString(CppiaCtx *ctx) { return HX_CSTRING("#function"); }

void ScriptCallable::runVoid(CppiaCtx *ctx)
{
}


#ifdef CPPIA_JIT
struct AutoFrame
{
   StackContext *ctx;
   unsigned char *frame;

   AutoFrame(StackContext *inCtx) : ctx(inCtx)
   {
      frame = ctx->frame;
   }
   ~AutoFrame()
   {
      ctx->frame = frame;
   }
};
#endif



// Run the actual function
void ScriptCallable::runFunction(CppiaCtx *ctx)
{

   #ifdef CPPIA_JIT
   if (compiled)
   {
      AutoFrame frame(ctx);
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

      if (hasDefaults)
      {
         for(int i=0;i<args.size();i++)
            if (hasDefault[i])
               args[i].setDefault(ctx, initVals[i]);
      }

      CPPIA_STACK_FRAME(this);
      CPPIA_STACK_LINE(this);
      body->runVoid(ctx);
   }
}



// Run the actual function - like runFunction, but stack may already contain values
void ScriptCallable::runFunctionClosure(CppiaCtx *ctx)
{
   #ifdef CPPIA_JIT
   if (compiled)
   {
      AutoFrame frame(ctx);
      //printf("Running compiled code...\n");
      compiled(ctx);
      //printf("Done.\n");
   }
   else
   #endif
   {
      if (hasDefaults)
      {
         for(int i=0;i<args.size();i++)
            if (hasDefault[i])
               args[i].setDefault(ctx, initVals[i]);
      }

      CPPIA_STACK_FRAME(this);
      CPPIA_STACK_LINE(this);
      body->runVoid(ctx);
   }
}


void ScriptCallable::addStackVarsSpace(CppiaCtx *ctx)
{
   if (stackSize)
   {
      memset(ctx->pointer, 0 , stackSize );
      ctx->pointer += stackSize;
   }
}


#ifdef CPPIA_JIT
void ScriptCallable::genCode(CppiaCompiler *compiler,const JitVal &inDest,ExprType destType)
{
   compiler->move(sJitCtxFrame, sJitFrame);
   compiler->callNative( (void *)createClosure, sJitCtx, (void *)this );
   compiler->convertReturnReg(etObject, inDest, destType);
}
#endif


bool ScriptCallable::pushDefault(CppiaCtx *ctx,int arg)
{
   switch(args[arg].argType)
   {
      case etInt:
         ctx->pushInt(0);
         break;
      case etFloat:
         ctx->pushFloat(0);
         break;
      case etString:
         ctx->pushString( String() );
         break;
      default:
         ctx->pushObject(null());
   }
   return true;
}

void ScriptCallable::addExtraDefaults(CppiaCtx *ctx,int inHave)
{
   if (inHave>argCount)
      return;

   for(int a=inHave;a<argCount;a++)
      pushDefault(ctx,a);
}


#ifdef CPPIA_JIT

#ifdef HXCPP_STACK_TRACE
static void SLJIT_CALL pushFrame(StackContext *inCtx, StackFrame *inFrame )
{
   inCtx->pushFrame(inFrame);
}

static void SLJIT_CALL popFrame(StackContext *inCtx, StackFrame *inFrame)
{
   inCtx->popFrame(inFrame);
}

static void onReturn( CppiaCompiler *inCompiler, int stackSize )
{
   inCompiler->add(sJitArg1.as(jtPointer), sJitFrame.as(jtPointer), stackSize );
   inCompiler->callNative( (void *)popFrame, sJitCtx.as(jtPointer), sJitArg1.as(jtPointer));
}
#endif


void ScriptCallable::compile()
{
   if (!compiled && body)
   {
      int size = stackSize;
      #ifdef HXCPP_STACK_TRACE
      size += sizeof(StackFrame);
      #endif
      CppiaCompiler *compiler = CppiaCompiler::create(size);

      // First pass calculates size...
      genDefaults(compiler);

      #ifdef HXCPP_STACK_TRACE
      compiler->move(sJitFrame.star(jtPointer)+(offsetof(StackFrame,position) + stackSize),  (void *)&position );
      compiler->add(sJitArg1.as(jtPointer), sJitFrame.as(jtPointer), stackSize );
      compiler->callNative( (void *)pushFrame, sJitCtx.as(jtPointer), sJitArg1.as(jtPointer) );

      compiler->setOnReturn( onReturn, stackSize );
         #ifdef HXCPP_STACK_LINE
         compiler->setLineOffset( stackSize + offsetof(StackFrame,lineNumber) ); 
         #endif
      #endif

      body->genCode(compiler);

      compiler->beginGeneration(1);

      // Second pass does the job
      genDefaults(compiler);

      #ifdef HXCPP_STACK_TRACE
      compiler->move(sJitFrame.star(jtPointer)+(offsetof(StackFrame,position) + stackSize),  (void *)&position );
      compiler->add(sJitArg1.as(jtPointer), sJitFrame.as(jtPointer), stackSize );
      compiler->callNative( (void *)pushFrame, sJitCtx.as(jtPointer), sJitArg1.as(jtPointer) );
      #endif

      body->genCode(compiler);

      compiled = compiler->finishGeneration();

      delete compiler;
   }
}
#endif

// --- CppiaClosure ----



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
      // this pointer...
      *(hx::Object **)ctx->frame =  *(hx::Object **)base;

      for(int i=0;i<function->captureVars.size();i++)
      {
         CppiaStackVar *var = function->captureVars[i];
         int size = sTypeSize[var->expressionType];
         memcpy( ctx->frame+var->stackPos, base + var->capturePos, size);
      }

      #ifdef CPPIA_JIT
      if (function->compiled)
      {
         {
         AutoFrame frame(ctx);
         function->compiled(ctx);
         }
         if (!ctx->exception)
         {
            switch(function->returnType)
            {
               case etFloat:
                  return ctx->getFloat();
               case etInt:
                  return ctx->getInt();
               case etString:
                  return ctx->getString();
               case etObject:
                  return ctx->getObject();
               default: ;
            }
         }
         return null();
      }
      //printf("Not compiled %d!\n", function->captureVars.size());
      //todo - compiled?
      #endif

      // TODO - stack var space added twice?
      function->runFunctionClosure(ctx);
      ctx->breakContReturn = 0;

      switch(function->returnType)
      {
         case etFloat:
            return ctx->getFloat( );
         case etInt:
            return ctx->getInt( );
         case etString:
            return ctx->getString( );
         case etObject:
            return ctx->getObject( );
         default: break;
      }

      return null();
   }

   void pushArgDynamic(CppiaCtx *ctx, int a, Dynamic inValue)
   {
      // Developer has used dynamic to call a closure with wrong # parameters
      if (a>=function->args.size())
         return;

      if (!inValue.mPtr)
      {
         function->pushDefault(ctx,a);
         return;
      }

      switch(function->args[a].argType)
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
            {
               hx::Object *value = inValue.mPtr;
               if (value)
               {
                  ArrayType want = function->args[a].type->arrayType;
                  if (want!=arrNotArray)
                     value = DynamicToArrayType(value, want);
               }
               ctx->pushObject(value);
            }
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
         pushArgDynamic(ctx,a,inArgs[a]);

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
      pushArgDynamic(ctx,0,a);
      return doRun(ctx,1);
   }
   Dynamic __run(D a,D b)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      return doRun(ctx,2);
   }
   Dynamic __run(D a,D b,D c)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      pushArgDynamic(ctx,2,c);
      return doRun(ctx,3);
   }
   Dynamic __run(D a,D b,D c,D d)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      pushArgDynamic(ctx,2,c);
      pushArgDynamic(ctx,3,d);
      return doRun(ctx,4);

   }
   Dynamic __run(D a,D b,D c,D d,D e)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      pushArgDynamic(ctx,2,c);
      pushArgDynamic(ctx,3,d);
      pushArgDynamic(ctx,4,e);
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

hx::Object * CPPIA_CALL createClosure(CppiaCtx *ctx, ScriptCallable *inFunction)
{
   return new (inFunction->captureSize) CppiaClosure(ctx,inFunction);
}

hx::Object *createMemberClosure(hx::Object *inThis, ScriptCallable *inFunction)
{
   return new (sizeof(hx::Object *)) CppiaClosure(inThis,inFunction);
}






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
   name = cppia.strings[ nameId ].utf8_str();
   stream.module->creatingFunction = name.c_str();
   returnType = stream.getInt();
   argCount = stream.getInt();
   DBGLOG("  Function %s(%d) : %s %s%s\n", name.c_str(), argCount, cppia.typeStr(returnType), isStatic?"static":"instance", isDynamic ? " DYNAMIC": "");
   args.resize(argCount);
   for(int a=0;a<argCount;a++)
   {
      ArgInfo &arg = args[a];
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



}
