#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <hx/Unordered.h>
#include <stdio.h>
#include <vector>
#include <string>

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

bool gEnableJit = false;


#ifdef DEBUG_RETURN_TYPE
int gLastRet = etVoid;
#endif

static bool isNumeric(ExprType t) { return t==etInt || t==etFloat; }


void cppiaClassMark(CppiaClassInfo *inClass,hx::MarkContext *__inCtx);
void cppiaClassVisit(CppiaClassInfo *inClass,hx::VisitContext *__inCtx);
int getScriptId(hx::Class inClass);

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

void CppiaTrap( )
{
    // Good when using gdb, and to collect a core ...
    #if __has_builtin(__builtin_trap)
    __builtin_trap();
    #else
    (* (int *) 0) = 0;
    #endif
}



// --- StackLayout ---



// 'this' pointer is in slot 0 and captureSize(0) ...
StackLayout::StackLayout(StackLayout *inParent) :
   size( sizeof(void *) ), captureSize(sizeof(void *)), parent(inParent)
{
}


void StackLayout::dump(Array<String> &inStrings, std::string inIndent)
{
   CPPIALOG("%sCapture:\n",inIndent.c_str());
   for(int i=0;i<captureVars.size();i++)
      CPPIALOG("%s %s\n", inIndent.c_str(), inStrings[captureVars[i]->nameId].out_str() );
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
void CppiaExpr::genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
{
   compiler->trace(getName());
}

JumpId CppiaExpr::genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
{
   genCode(compiler,sJitTemp0.as(jtInt), etInt);
   // inReverse = false -> jump if not 0
   // inReverse = true  -> jump if zero
   return compiler->compare(inReverse ? cmpI_EQUAL : cmpI_NOT_EQUAL, sJitTemp0, (int)0, inLabel);
}

#endif




// --- CppiaDynamicExpr ----------------------------------------
// Delegates to 'runObject'



int  CppiaDynamicExpr::runInt(CppiaCtx *ctx)
{
   hx::Object *obj = runObject(ctx);
   return ValToInt(obj);
}
Float CppiaDynamicExpr::runFloat(CppiaCtx *ctx)
{
   return ValToFloat(runObject(ctx));
}
::String CppiaDynamicExpr::runString(CppiaCtx *ctx)
{
  hx::Object *result = runObject(ctx);
  BCR_CHECK;
  return result ? result->toString() : String();
}

void  CppiaDynamicExpr::runVoid(CppiaCtx *ctx)
{
   runObject(ctx);
}


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
   bool isBoolInt() { return true; }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JumpId notCondition = genCompare(compiler, true, 0);

      if (destType==etObject)
         compiler->move(inDest, (void *)Dynamic(true).mPtr);
      else
         compiler->move(inDest, (int)1);

      JumpId notDone = compiler->jump();
      compiler->comeFrom(notCondition);
      if (destType==etObject)
         compiler->move(inDest, (void *)Dynamic(false).mPtr);
      else
         compiler->move(inDest, (int)0);

      compiler->comeFrom(notDone);
   }
   #endif
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



void LinkExpressions(Expressions &ioExpressions, CppiaModule &data)
{
   for(int i=0;i<ioExpressions.size();i++)
      ioExpressions[i] = ioExpressions[i]->link(data);
}


CppiaExpr *convertToFunction(CppiaExpr *inExpr);






bool TypeData::isClassOf(Dynamic inInstance)
{
   if (cppiaClass)
      return cppiaClass->getClass()->VCanCast(inInstance.mPtr);
   else if (haxeClass.mPtr)
      return __instanceof(inInstance,haxeClass) || isDynamic;

   return false;
}



CppiaExpr *convertToFunction(CppiaExpr *inExpr) { return new ScriptCallable(inExpr); }


template<typename T>
static hx::Object *convert(hx::Object *obj)
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


hx::Object *DynamicToArrayType(hx::Object *obj, ArrayType arrayType)
{
   switch(arrayType)
   {
      case arrBool:         return convert<bool>(obj);
      case arrUnsignedChar: return convert<unsigned char>(obj);
      case arrInt:          return convert<int>(obj);
      case arrFloat:        return convert<Float>(obj);
      case arrFloat32:      return convert<float>(obj);
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
   return runContextConvertObject(ctx, inFunExpr->getReturnType(), inFunExpr );
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
      CPPIA_STACK_LINE(this);
      body->runVoid(ctx);
   }
   int runInt(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      CPPIA_STACK_LINE(this);
      return body->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      CPPIA_STACK_LINE(this);
      return body->runFloat(ctx);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->push( ctx->getThis(false) );
      AutoStack save(ctx,pointer);
      addStackVarsSpace(ctx);
      CPPIA_STACK_FRAME(this);
      CPPIA_STACK_LINE(this);
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
         return blockFunc->link(data);
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
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      int n = expressions.size();
      int lineOffset = compiler->getLineOffset();
      //compiler->trace(filename);
      for(int i=0;i<n;i++)
      {
         if (lineOffset)
            compiler->move( sJitFrame.star(etInt)+lineOffset, expressions[i]->line );
         //compiler->traceInt("line",expressions[i]->line);
         if (i<n-1)
            expressions[i]->genCode(compiler);
         else
            expressions[i]->genCode(compiler, inDest, destType);
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JumpId ifNot = condition->genCompare(compiler,true);
      doIf->genCode(compiler,inDest,destType);
      JumpId doneIf = compiler->jump();

      compiler->comeFrom(ifNot);
      doElse->genCode(compiler,inDest,destType);

      compiler->comeFrom(doneIf);
   }
   #endif
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JumpId ifNot = condition->genCompare(compiler,true);
      doIf->genCode(compiler,inDest,destType);
      compiler->comeFrom(ifNot);
   }
   #endif
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
      if (condition->getType()==etString)
         return condition->runString(ctx)==null();
      else
         return condition->runObject(ctx)==0;
   }

   #ifdef CPPIA_JIT
   JumpId genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
   {
      if (condition->getType()==etString)
      {
         JitTemp val(compiler,jtString);
         condition->genCode(compiler, val, etString);
         return compiler->compare(inReverse ? cmpP_NOT_EQUAL : cmpP_EQUAL, val.as(jtPointer) + StringOffset::Ptr, (void *)0, inLabel);
      }
      else
      {
         condition->genCode(compiler, sJitTemp0, etObject);
         // inReverse = false -> jump if not 0
         // inReverse = true  -> jump if zero
         return compiler->compare(inReverse ? cmpP_NOT_EQUAL : cmpP_EQUAL, sJitTemp0.as(jtPointer), (void *)0, inLabel);
      }
   }
   #endif

};


struct CppiaIsNotNull : public CppiaBoolExpr
{
   CppiaExpr *condition;

   CppiaIsNotNull(CppiaStream &stream) { condition = createCppiaExpr(stream); }

   const char *getName() { return "IsNotNull"; }
   CppiaExpr *link(CppiaModule &inModule) { condition = condition->link(inModule); return this; }

   int runInt(CppiaCtx *ctx)
   {
      if (condition->getType()==etString)
         return condition->runString(ctx)!=null();
      else
         return condition->runObject(ctx)!=0;
   }


   #ifdef CPPIA_JIT
   JumpId genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
   {
      if (condition->getType()==etString)
      {
         JitTemp val(compiler,jtString);
         condition->genCode(compiler, val, etString);
         return compiler->compare(inReverse ? cmpP_EQUAL : cmpP_NOT_EQUAL, val.as(jtPointer) + StringOffset::Ptr, (void *)0, inLabel);
      }
      else
      {
         condition->genCode(compiler, sJitTemp0, etObject);
      // inReverse = false -> jump if not 0
      // inReverse = true  -> jump if zero
         return compiler->compare(inReverse ? cmpP_EQUAL : cmpP_NOT_EQUAL, sJitTemp0.as(jtPointer), (void *)0, inLabel);
      }
   }
   #endif

};


#ifdef CPPIA_JIT
void genFunctionResult(CppiaCompiler *compiler,const JitVal &inDest, ExprType destType, ExprType returnType, bool isBoolReturn)
{
      compiler->checkException();

      // result is at 'framePos'
      if (isBoolReturn && (destType==etObject || destType==etString))
      {
         JumpId isZero = compiler->compare(cmpI_EQUAL,JitFramePos(compiler->getCurrentFrameSize()).as(jtInt),(int)0);
         if (destType==etObject)
            compiler->move(inDest.as(jtPointer),(void *)Dynamic(true).mPtr);
         else
         {
            compiler->move(inDest.as(jtInt),String(true).length);
            compiler->move(inDest.as(jtPointer)+StringOffset::Ptr,(void *)String(true).raw_ptr());
         }
         JumpId done = compiler->jump();

         compiler->comeFrom(isZero);

         if (destType==etObject)
            compiler->move(inDest.as(jtPointer),(void *)Dynamic(false).mPtr);
         else
         {
            compiler->move(inDest.as(jtInt),String(false).length);
            compiler->move(inDest.as(jtPointer)+StringOffset::Ptr,(void *)String(false).raw_ptr());
         }

         compiler->comeFrom(done);
      }
      else
      {
         compiler->convertResult( returnType, inDest, destType );
      }
}

void genFunctionCall(ScriptCallable *function, CppiaCompiler *compiler,
                     const JitVal &inDest, ExprType destType, bool isBoolReturn, ExprType returnType,
                     CppiaExpr *thisExpr,  Expressions &args, const JitVal &inThisVal )
{
      int framePos = compiler->getCurrentFrameSize();

      // Push args...
      function->genArgs(compiler,thisExpr,args, inThisVal);

      compiler->restoreFrameSize(framePos);

      // Store new frame in context ...
      compiler->add( sJitCtxFrame, sJitFrame, JitVal(framePos) );

      // Compiled yet
      if (function->compiled)
      {
         compiler->call( JitVal( (void *)(function->compiled)), sJitCtx );
      }
      else
      {
         // Compiled later
         compiler->move( sJitTemp1, JitVal((void *)&function->compiled) );
         compiler->call( sJitTemp1.star(), sJitCtx );
      }
      compiler->setMaxPointer();

      genFunctionResult(compiler, inDest, destType, returnType, isBoolReturn);

}
#endif




struct CallFunExpr : public CppiaExpr
{
   Expressions args;
   CppiaExpr   *thisExpr;
   ScriptCallable     *function;
   ExprType    returnType;
   bool        isBoolReturn;
   bool        isThisCall;

   CallFunExpr(const CppiaExpr *inSrc, CppiaExpr *inThisExpr, ScriptCallable *inFunction, Expressions &ioArgs, bool inThisCall )
      : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      function = inFunction;
      thisExpr = inThisExpr;
      returnType = etVoid;
      isBoolReturn = false;
      isThisCall = inThisCall;
   }

   CppiaExpr *link(CppiaModule &inModule)
   {
      LinkExpressions(args,inModule);
      // Should already be linked
      //function = (ScriptCallable *)function->link(inModule);
      if (thisExpr)
         thisExpr = thisExpr->link(inModule);
      returnType = inModule.types[ function->returnTypeId ]->expressionType;
      isBoolReturn = inModule.types[ function->returnTypeId ]->haxeClass==ClassOf<bool>();
      return this;
   }

   const char *getName() { return "CallFunExpr"; }
   ExprType getType() { return returnType; }
   bool isBoolInt() { return isBoolReturn; }

   #define CallFunExprVal(ret,name,funcName) \
   ret name(CppiaCtx *ctx) \
   { \
      unsigned char *pointer = ctx->pointer; \
      function->pushArgs(ctx,thisExpr?thisExpr->runObject(ctx):ctx->getThis(false),args); \
      BCR_CHECK; \
      AutoStack save(ctx,pointer); \
      return funcName(ctx, function->getReturnType(), function); \
   }
   CallFunExprVal(int,runInt, runContextConvertInt);
   CallFunExprVal(Float ,runFloat, runContextConvertFloat);
   //CallFunExprVal(hx::Object * ,runObject, runContextConvertObject);
   //CallFunExprVal(String ,runString, runContextConvertString);

   String runString(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      function->pushArgs(ctx,thisExpr?thisExpr->runObject(ctx):ctx->getThis(false),args);
      BCR_CHECK;
      AutoStack save(ctx,pointer);
      if (isBoolReturn)
         return String(ctx->runInt(function) ? true : false );
      return runContextConvertString(ctx, function->getReturnType(), function);
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      function->pushArgs(ctx,thisExpr?thisExpr->runObject(ctx):ctx->getThis(false),args);
      BCR_CHECK;
      AutoStack save(ctx,pointer);
      if (isBoolReturn)
         return Dynamic(ctx->runInt(function) ? true : false ).mPtr;
      return runContextConvertObject(ctx, function->getReturnType(), function);
   }

   void runVoid(CppiaCtx *ctx)
   {
      unsigned char *pointer = ctx->pointer;
      function->pushArgs(ctx,thisExpr?thisExpr->runObject(ctx):ctx->getThis(false),args);
      if (ctx->breakContReturn)
         return;

      AutoStack save(ctx,pointer);
      ctx->runVoid(function);
   }

   #ifdef CPPIA_JIT
   static void SLJIT_CALL callScriptable(CppiaCtx *inCtx, ScriptCallable *inScriptable)
   {
      // compiled?
      CPPIALOG("callScriptable %p(%p) -> %p\n", inCtx, CppiaCtx::getCurrent(), inScriptable );
      CPPIALOG(" name = %s\n", inScriptable->getName());
      inScriptable->runFunction(inCtx);
      CPPIALOG(" Done scipt callable\n");
   }


   // Function Call
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      genFunctionCall(function, compiler, inDest, destType, isBoolReturn, returnType,thisExpr, args,
                      isThisCall ? (JitVal)sJitThis : JitVal());
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType!=etNull && destType!=etVoid)
         compiler->convert( (void *)value.mPtr, etObject,  inDest, destType);
   }
   #endif

};

// ---


#ifdef CPPIA_JIT
void SLJIT_CALL callDynamic(CppiaCtx *ctx, hx::Object *inFunction, int inArgs)
{
   if (!inFunction)
   {
      ctx->exception = Dynamic(HX_CSTRING("Null Function")).mPtr;
      return;
   }
   // ctx.frame points to 0th arg
   hx::Object **base = ((hx::Object **)(ctx->frame) );
   unsigned char *oldPointer = ctx->pointer;
   ctx->pointer = ctx->frame;

   //hx::Object **base = ((hx::Object **)(ctx->pointer) ) - inArgs;
   //ctx->pointer = (unsigned char *)base;

   TRY_NATIVE
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
   CATCH_NATIVE
   ctx->pointer = oldPointer;
}
#endif




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


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      {
      AutoFramePos frame(compiler);

      for(int a=0;a<args.size();a++)
      {
         args[a]->genCode(compiler, JitFramePos(compiler->getCurrentFrameSize(),etObject), etObject);
         compiler->addFrame(etObject);
      }
      }

      compiler->add(sJitCtxFrame, sJitFrame, compiler->getCurrentFrameSize());

      compiler->callNative((void *)callDynamic,sJitCtx, (void *)value.mPtr,(int)args.size());

      compiler->checkException();
      if (destType!=etVoid && destType!=etNull)
         compiler->convertResult( etObject, inDest, destType );

      /*
      if (compiler.exceptionHandler)
      {
         sljit_jump *notZero = compiler.ifNotZero( CtxMemberVal(offsetof(CppiaCtx,exception)) );
         compiler.exceptionHandler->push_back(notZero);
      }
      else
      {
         sljit_jump *isZero = compiler.ifZero( CtxMemberVal(offsetof(CppiaCtx,exception)) );
         compiler.ret( );
         compiler.comeFrom(isZero);
      }

      // Result is at pointer
      if (inDest!=pointer)
         compiler.move( inDest, pointer );
      */
   }
   #endif

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
         CPPIALOG("Could not makeSetter.\n");
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
   castInstance,
   castDataArray,
   castDynArray,
   castInt,
   castBool,
   castFloat,
   castString,
};


struct CastExpr : public CppiaDynamicExpr
{
   CppiaExpr *value;
   CastOp  op;
   int     typeId;
   ArrayType arrayType;
   TypeData *typeData;

   CastExpr(CppiaStream &stream,CastOp inOp)
   {
      op = inOp;
      typeId = 0;
      typeData = 0;
      if (op==castDataArray || op==castInstance)
         typeId = stream.getInt();

      value = createCppiaExpr(stream);
   }
   ExprType getType() { return op==castInt || op==castBool ? etInt :
                               op==castFloat ? etFloat :
                               op==castString ? etString :
                                                etObject; }

   bool isBoolInt() { return op==castBool; }

   int runInt(CppiaCtx *ctx)
   {
      if (op==castBool)
         return (bool)value->runInt(ctx);
      return value->runInt(ctx);
   }

   double runFloat(CppiaCtx *ctx)
   {
      return value->runFloat(ctx);
   }

   String runString(CppiaCtx *ctx)
   {
      switch(op)
      {
         case castInt:
            return String( value->runInt(ctx) );
         case castFloat:
            return String( value->runFloat(ctx) );
         case castBool:
            return String( (bool)value->runInt(ctx) );
         default:
            break;
      }
      hx::Object *o = runObject(ctx);
      return o ? o->toString() : String();
   }


   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (op==castInt)
         return Dynamic(value->runInt(ctx)).mPtr;
      else if (op==castFloat)
         return Dynamic(value->runFloat(ctx)).mPtr;
      else if (op==castBool)
         return Dynamic((bool)value->runInt(ctx)).mPtr;
      else if (op==castString)
         return Dynamic(value->runString(ctx)).mPtr;

      hx::Object *obj = value->runObject(ctx);
      if (op==castInstance)
      {
         if ( !obj || !typeData->isClassOf(obj) )
            hx::BadCast();
         return obj;
      }
      else if (!obj)
         return 0;

      if (op==castDynamic)
      #if (HXCPP_API_LEVEL>=331)
         return obj;
      #else
         return obj->__GetRealObject();
      #endif

      return DynamicToArrayType(obj, arrayType);
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

      if (op==castInstance)
      {
         typeData = inModule.types[typeId];
         if (typeData->isFloat)
            op=castFloat;
         else if (typeData->name==HX_CSTRING("Int"))
            op=castInt;
         else if (typeData->name==HX_CSTRING("Bool"))
            op=castBool;
         else if (typeData->name==HX_CSTRING("String"))
            op=castString;
         else
            return this;
      }

      if (op==castDynamic || op==castInt || op==castBool || op==castFloat || op==castString )
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
            CPPIALOG("Cast to %d, %s\n", typeId, t->name.out_str());
            throw "Data cast to non-array";
         }
      }
      else if (op==castDynArray)
      {
         arrayType = arrAny;
      }
      else
         arrayType = arrObject;
      return this;
   }

   #ifdef CPPIA_JIT

   static hx::Object * SLJIT_CALL callDynamicToArrayType(hx::Object *inObj, int inToType )
   {
      TRY_NATIVE
      return DynamicToArrayType(inObj, (ArrayType)inToType);
      CATCH_NATIVE
      return 0;
   }


   static hx::Object * SLJIT_CALL safeCast(hx::Object *obj, TypeData *typeData )
   {
      if (!obj || !typeData->isClassOf(obj) )
      {
         CppiaCtx::getCurrent()->exception = HX_INVALID_CAST.mPtr;
         return 0;
      }
 
      return obj;
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      if (destType==etNull || destType==etVoid)
      {
         value->genCode(compiler);
         return;
      }

      if (op==castInt)
      {
         if (destType==etInt)
            value->genCode(compiler, inDest, destType);
         else
         {
            value->genCode(compiler, sJitTemp0, etInt);
            compiler->convert(sJitTemp0, etInt, inDest, destType);
         }
      }
      else if (op==castFloat)
      {
         if (destType==etFloat)
            value->genCode(compiler, inDest, destType);
         else
         {
            value->genCode(compiler, sJitTempF0, etFloat);
            compiler->convert(sJitTempF0, etFloat, inDest, destType);
         }
      }
      else if (op==castString)
      {
         if (destType==etString)
            value->genCode(compiler, inDest, destType);
         else
         {
            JitTemp temp(compiler,jtString);
            value->genCode(compiler, temp, etString);
            compiler->convert(temp, etString, inDest, destType );
         }
      }
      else if (op==castBool)
      {
         value->genCode(compiler, sJitTemp0, etInt);

         JumpId isZero = compiler->compare(cmpI_EQUAL,sJitTemp0.as(jtInt),(int)0);
         if (destType==etInt)
            compiler->move(inDest.as(jtInt),1);
         else if (destType==etObject)
            compiler->move(inDest.as(jtPointer),(void *)Dynamic(true).mPtr);
         else
         {
            compiler->move(inDest.as(jtInt),String(true).length);
            compiler->move(inDest.as(jtPointer)+StringOffset::Ptr,(void *)String(true).raw_ptr());
         }
         JumpId done = compiler->jump();

         compiler->comeFrom(isZero);

         if (destType==etInt)
            compiler->move(inDest.as(jtInt),0);
         else if (destType==etObject)
            compiler->move(inDest,(void *)Dynamic(false).mPtr);
         else
         {
            compiler->move(inDest.as(jtInt),String(false).length);
            compiler->move(inDest.as(jtPointer)+StringOffset::Ptr,(void *)String(false).raw_ptr());
         }
         compiler->comeFrom(done);
      }
      else if (op==castInstance)
      {
         value->genCode(compiler, sJitTemp0.as(jtPointer), etObject);
         compiler->callNative( (void *)safeCast, sJitTemp0, (void *)typeData );
         compiler->checkException();
         compiler->convertReturnReg(etObject, inDest, destType);
      }
      else if (op==castDynamic)
      {
         if (destType==etObject)
            value->genCode(compiler, inDest, destType);
         else if (destType==etString)
         {
            value->genCode(compiler, inDest, etString);
         }
         else
         {
            JitTemp o(compiler,jtPointer);
            value->genCode(compiler, o, etObject);
            compiler->convert(o,etObject, inDest, destType);
         }
      }
      else
      {
         value->genCode(compiler, sJitArg0, etObject);
         JumpId notNull = compiler->compare(cmpP_NOT_ZERO, sJitArg0.as(jtPointer), (void *)0);
         compiler->returnNull(inDest, destType);
         JumpId returnedNull = compiler->jump();

         compiler->comeFrom(notNull);
         compiler->callNative( (void *)callDynamicToArrayType, sJitArg0.as(jtPointer), (int)arrayType );
         compiler->checkException();
         compiler->convertReturnReg(etObject, inDest, destType);
         compiler->comeFrom(returnedNull);
      }
   }
   #endif
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


#ifdef CPPIA_JIT
static void *SLJIT_CALL createArrayBool(int n) { return (Array_obj<bool>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayUChar(int n) { return (Array_obj<unsigned char>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayInt(int n) { return (Array_obj<int>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayFloat(int n) { return (Array_obj<Float>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayFloat32(int n) { return (Array_obj<float>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayString(int n) { return (Array_obj<String>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayObject(int n) { return (Array_obj<Dynamic>::__new(n,n)).mPtr; }
static void *SLJIT_CALL createArrayAny(int n) {
  #if (HXCPP_API_LEVEL>=330)
  return (cpp::VirtualArray_obj::__new(n,n)).mPtr;
  #else
  return (Array_obj<Dynamic>::__new(n,n)).mPtr;
  #endif
}
static void SLJIT_CALL varraySetInt(cpp::VirtualArray_obj *varray, int i, int value) { varray->init(i,value); }
static void SLJIT_CALL varraySetBool(cpp::VirtualArray_obj *varray, int i, int value) { varray->init(i,(bool)value); }
static void SLJIT_CALL varraySetObject(cpp::VirtualArray_obj *varray, int i, hx::Object *value) { varray->init(i,Dynamic(value)); }
static void SLJIT_CALL varraySetString(cpp::VirtualArray_obj *varray, int i, String *value) { varray->init(i,*value); }
static void SLJIT_CALL varraySetFloat(cpp::VirtualArray_obj *varray, int i, double *value) { varray->init(i,*value); }

static void *SLJIT_CALL runConstructor(void *inConstructor,  Array_obj<Dynamic> *inArgs)
{
   TRY_NATIVE
   return (((hx::ConstructArgsFunc)inConstructor)(inArgs)).mPtr;
   CATCH_NATIVE
   return 0;
}
static void * SLJIT_CALL getScriptVTable(hx::Object *inObject)
{
   return inObject->__GetScriptVTable();
}

/*
static void *SLJIT_CALL runCreateInstance(CppiaCtx *ctx, CppiaClassInfo *info,  Array_obj<Dynamic> *inArgs)
{
   TRY_NATIVE
   Array<Dynamic> args(inArgs);
   void *result = info->createInstance(ctx, args);
   return result;
   CATCH_NATIVE
   return 0;
}
*/

static void *SLJIT_CALL allocHaxe(CppiaCtx *inCtx, CppiaClassInfo *inInfo )
{
   Expressions empty;
   return inInfo->createInstance(inCtx,empty,false);
}

#endif


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
            case arrFloat32:
               return Array_obj<float>::__new(size,size).mPtr;
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

      CPPIALOG("Can't create non haxe type\n");
      return 0;
   }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      if (type->arrayType)
      {
         void *func = 0;
         switch(type->arrayType)
         {
            case arrBool:         func = (void *)createArrayBool; break;
            case arrUnsignedChar: func = (void *)createArrayUChar; break;
            case arrInt:func = (void *)createArrayInt; break;
            case arrFloat:func = (void *)createArrayFloat; break;
            case arrFloat32:func = (void *)createArrayFloat32; break;
            case arrString:func = (void *)createArrayString; break;
            case arrAny:func = (void *)createArrayAny; break;
            case arrObject:func = (void *)createArrayObject; break;
            default:
               CPPIALOG("Unknown array creation\n");
               return;
         }

         if (args.size()==1)
         {
            args[0]->genCode(compiler, sJitTemp0, etInt);
            compiler->callNative(func,sJitTemp0.as(jtInt));
         }
         else
            compiler->callNative(func,(int)0);
         compiler->convert(sJitReturnReg, etObject, inDest, destType);
      }
      else if (constructor)
      {
         int n = args.size();
         JitTemp argList(compiler,jtPointer);
         compiler->callNative((void *)createArrayObject,n);
         compiler->move(argList, sJitReturnReg);
         for(int a=0;a<n;a++)
         {
            args[a]->genCode(compiler, sJitTemp2, etObject);

            compiler->move(sJitTemp0, argList);
            compiler->move(sJitTemp1, sJitTemp0.star(jtPointer, ArrayBase::baseOffset()));
            compiler->move(sJitTemp1.star(jtPointer,a*sizeof(void *)), sJitTemp2.as(jtPointer) );
         }

         compiler->callNative( (void *)runConstructor, (void *)constructor, argList );
         compiler->checkException();
         compiler->convert(sJitReturnReg, etObject, inDest, destType);
      }
      else
      {
         CppiaClassInfo *info = type->cppiaClass;
         // return info->createInstance(ctx,args);

         #ifdef HXCPP_GC_NURSERY
         if (info->dynamicFunctions.size()==0)
         {
            int size = info->haxeBase->mDataOffset + info->extraData;
            // sJitCtx = alloc
            // sJitReturnReg = alloc->spaceFirst - will be the result if all goes well
            compiler->move(sJitReturnReg, sJitCtx.star(jtPointer, offsetof(hx::StackContext,spaceFirst) ) );

            // sJitTemp1 = end = spaceFirst + size + sizeof(int)
            compiler->add(sJitTemp1.as(jtPointer), sJitReturnReg.as(jtPointer), (int)(size + sizeof(int) ) );

            JumpId inRange = compiler->compare(cmpP_LESS_EQUAL, sJitTemp1, sJitCtx.star(jtPointer, offsetof(hx::StackContext,spaceOversize) ) );

            // Not in range ...
               compiler->callNative((void *)allocHaxe, sJitCtx, (void *)info );

               JumpId allocCallDone = compiler->jump();
            // In range
               compiler->comeFrom(inRange);

               // alloc->spaceStart = end;
               compiler->move( sJitCtx.star(jtPointer, offsetof(hx::StackContext,spaceFirst)), sJitTemp1.as(jtPointer) );

               // sJitReturnReg = buffer

               // TODO - IMMIX_ALLOC_IS_CONTAINER from classInfo
               // buffer[-1] = size | container

               if (info->containsPointers)
                  compiler->move( sJitReturnReg.star(etInt,-sizeof(int)), (int)( size | IMMIX_ALLOC_IS_CONTAINER) );
               else
                  compiler->move( sJitReturnReg.star(etInt,-sizeof(int)), (int)( size ) );

               // Set class vtable
               compiler->move(sJitReturnReg.star(jtPointer), (void *) info->getHaxeBaseVTable() );

               // Set script vtable
               compiler->move(sJitReturnReg.star(jtPointer, (int)(info->haxeBase->mDataOffset-sizeof(void *))), (void *) info->vtable );

            // join sJitReturnReg contains return buffer
            compiler->comeFrom(allocCallDone);
         }
         else
         #endif
         {
            compiler->callNative((void *)allocHaxe, sJitCtx, (void *)info );
         }

         // Result is in sJitReturnReg
         if (info->newFunc)
         {
            // Leaves result on frame 'this' slot = return position
            genFunctionCall( info->newFunc->funExpr,compiler, JitVal(), etVoid, false, etVoid, 0, args, sJitReturnReg);

            compiler->convertResult(etObject, inDest, destType);
         }
         else
         {
            compiler->convert(sJitReturnReg, etObject, inDest, destType);
         }
      }

   }
   #endif
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
   ScriptNamedFunction function;
   ExprType returnType;
   bool   isStatic;
   bool   isSuper;

   CallHaxe(CppiaExpr *inSrc,ScriptNamedFunction inFunction, CppiaExpr *inThis, Expressions &ioArgs, bool inIsStatic=false, bool inIsSuper = false )
       : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      thisExpr = inThis;
      function = inFunction;
      isStatic = inIsStatic;
      isSuper = inIsSuper;
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
            case sigInt: case sigBool: case sigFloat: case sigString: case sigObject:
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
         case sigBool:
         case sigInt:
            returnType = etInt; break;
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


   bool isBoolInt() { return function.signature[0]==sigBool; }

   const char *getName() { return "CallHaxe"; }

   template<typename T>
   void run(CppiaCtx *ctx,T &outValue)
   {
      unsigned char *pointer = ctx->pointer;
      ctx->pushObject(isStatic ? 0: thisExpr ? thisExpr->runObject(ctx) : ctx->getThis(false));

      const char *s = function.signature+1;
      for(int a=0;a<args.size();a++)
      {
         CppiaExpr *arg = args[a];
         switch(*s++)
         {
            case sigInt:
            case sigBool:
               ctx->pushInt( arg->runInt(ctx) ); break;
            case sigFloat: ctx->pushFloat( arg->runFloat(ctx) ); break;
            case sigString: ctx->pushString( arg->runString(ctx) ); break;
            case sigObject: ctx->pushObject( arg->runObject(ctx) ); break;
            default: ;// huh?
         }
      }

      AutoStack a(ctx,pointer);
      if (isSuper)
         function.superExecute(ctx);
      else
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
      if (isBoolInt())
      {
         int val;
         run(ctx,val);
         return Dynamic( val!=0 ).mPtr;
      }
      else
      {
        Dynamic val;
        run(ctx,val);
        return val.mPtr;
      }
   }

   #ifdef CPPIA_JIT
   static void SLJIT_CALL tryCallHaxe( void (SLJIT_CALL *func)(StackContext *), StackContext *ctx )
   {
      TRY_NATIVE
      func(ctx);
      CATCH_NATIVE
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      int framePos = compiler->getCurrentFrameSize();

      // Push this ...
      if (thisExpr)
         thisExpr->genCode(compiler, JitFramePos(framePos), etObject);
      else if (isStatic)
         compiler->move(JitFramePos(framePos), (void *)0);
      else
         compiler->move(JitFramePos(framePos), sJitThis);
      compiler->addFrame(etObject);


      // Push args...
      const char *s = function.signature+1;
      for(int a=0;a<args.size();a++)
      {
         CppiaExpr *arg = args[a];
         ExprType argType = etNull;
         switch(*s++)
         {
            case sigBool:
            case sigInt:
               argType = etInt; break;
            case sigFloat: argType = etFloat; break;
            case sigString: argType = etString; break;
            case sigObject: argType = etObject; break;
            default: ;// huh?
         }

         if (argType!=etNull)
         {
            args[a]->genCode( compiler, JitFramePos( compiler->getCurrentFrameSize() ).as( getJitType(argType)), argType );
            compiler->addFrame(argType);
         }
      }

      compiler->restoreFrameSize(framePos);
      // Store new frame in context ...
      compiler->add( sJitCtxFrame, sJitFrame.as(jtPointer), JitVal(framePos) );

      void *func = (void *) ( isSuper ? function.superExecute : function.execute);
      compiler->callNative( (void *)tryCallHaxe, JitVal(func), sJitCtx );

      genFunctionResult(compiler, inDest, destType, returnType, isBoolInt());
   }

   #endif
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
            CPPIALOG("Could not find static function %s in %s\n", field.out_str(), type->name.out_str());
         }
         else
         {
            replace = new CallFunExpr( this, 0, func, args, false );
         }
      }

      if (!replace && type->haxeClass.mPtr)
      {
         ScriptNamedFunction func = type->haxeBase->findStaticFunction(field);
         if (func.signature)
         {
            //CPPIALOG(" found function %s\n", func.signature );
            replace = new CallHaxe( this, func, 0, args, true );
         }
         else
         {
            //const StaticInfo *info = type->haxeClass->GetStaticStorage(field);
            //CPPIALOG("INFO %s -> %p\n", field.out_str(),  info);
            // TODO - create proper glue for static functions
            Dynamic func = type->haxeClass.mPtr->__Field( field, HX_PROP_NEVER );
            if (func.mPtr)
            {
               replace = new CallDynamicFunction(inModule, this, func, args );
            }
         }
      }

      // TODO - optimise...
      if (!replace && type->name==HX_CSTRING("String") && field==HX_CSTRING("fromCharCode"))
         replace = new CallDynamicFunction(inModule, this, String::fromCharCode_dyn(), args );
         

      //CPPIALOG(" static call to %s::%s (%d)\n", type->name.out_str(), field.out_str(), type->cppiaClass!=0);
      if (replace)
      {
         delete this;
         replace->link(inModule);
         return replace;
      }

      CPPIALOG("Unknown static call to %s::%s (%d)\n", type->name.out_str(), field.out_str(), type->cppiaClass!=0);
      inModule.where(this);
      throw "Bad link";
      return this;
   }
};


#ifdef CPPIA_JIT
static hx::Object *nullException = 0;
void genNullReferenceExceptionCheck(CppiaCompiler *compiler, const JitVal &reg)
{
   #ifdef HXCPP_CHECK_POINTER
   if (!nullException)
      nullException = (HX_CSTRING("Null Object Reference")).makePermanentObject();

   JumpId nonNull = compiler->compare(cmpP_NOT_EQUAL, reg.as(jtPointer), (void *)0);
   compiler->move( sJitCtx.star(jtPointer, offsetof(hx::StackContext,exception)), (void *)nullException );
   compiler->addThrow();
   compiler->comeFrom(nonNull);
   #endif
}
#endif



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


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      #if (HXCPP_API_LEVEL<330)
      throw "Enum getIndex not supported by this version of compiled code";
      #endif
      thisExpr->genCode(compiler, sJitTemp0, etObject);
      genNullReferenceExceptionCheck(compiler,sJitTemp0);
      if (destType==etInt)
         compiler->move( inDest, sJitTemp0.star( jtInt, offsetof(hx::EnumBase_obj, index) ) );
      else
      {
         compiler->move( sJitTemp0, sJitTemp0.star( jtInt, offsetof(hx::EnumBase_obj, index) ) );
         compiler->convert( sJitTemp0, etInt, inDest, destType );
      }
   }
   #endif
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


#ifdef CPPIA_JIT
static void SLJIT_CALL fixReturnType( hx::CppiaCtx *inCtx, int actualType)
{
   void *returnPos = inCtx->frame;

   Dynamic result;
   switch(actualType)
   {
      case etInt: result = *(int *)returnPos; break;
      case etFloat: result = *(double *)returnPos; break;
      case etString: result = *(String *)returnPos; break;
   }
   *(hx::Object **)returnPos = result.mPtr;
}
#endif



struct CallMemberVTable : public CppiaExpr
{
   Expressions args;
   CppiaExpr   *thisExpr;
   int         slot;
   int         scriptVTableOffset;
   bool        isInterfaceCall;
   CppiaFunction *funcProto;
   ExprType    returnType;
   bool        checkInterfaceReturnType;
   bool        boolResult;

   CallMemberVTable(CppiaExpr *inSrc, CppiaExpr *inThis,
                    int inVTableSlot,
                    int inScriptVTableOffset,
                    CppiaFunction *inFuncProto,
             bool inIsInterfaceCall,Expressions &ioArgs)
      : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      slot = inVTableSlot;
      thisExpr = inThis;
      isInterfaceCall = inIsInterfaceCall;
      funcProto = inFuncProto;
      returnType = funcProto->cppia.types[funcProto->returnType]->expressionType;
      checkInterfaceReturnType = false;

      scriptVTableOffset = inScriptVTableOffset;
   }
   const char *getName() { return "CallMemberVTable"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      if (thisExpr)
         thisExpr = thisExpr->link(inModule);
      LinkExpressions(args,inModule);

      TypeData *type = inModule.types[funcProto->returnType];
      // These types may have natively been returned as ints or non-objects
      checkInterfaceReturnType = isInterfaceCall && (returnType==etFloat || type->isDynamic);

      boolResult = type->haxeClass==ClassOf<bool>();
      return this;
   }
   ExprType getType() { return returnType; }
   // ScriptCallable **vtable = (ScriptCallable **)thisVal->__GetScriptVTable();

   #define CALL_VTABLE_SETUP \
      hx::Object *thisVal = thisExpr ? thisExpr->runObject(ctx) : ctx->getThis(); \
      CPPIA_CHECK(thisVal); \
      ScriptCallable **vtable = (!isInterfaceCall ? (*(ScriptCallable ***)((char *)thisVal +scriptVTableOffset)) : (ScriptCallable **) thisVal->__GetScriptVTable()); \
      unsigned char *pointer = ctx->pointer; \
      ScriptCallable *func = vtable[slot]; \
      func->pushArgs(ctx, thisVal, args); \
      /* TODO */; \
      AutoStack save(ctx,pointer);

   void runVoid(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      ctx->runVoid(func);
   }
   int runInt(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertInt(ctx, checkInterfaceReturnType ? func->getReturnType() : returnType, func); 
   }
 
   Float runFloat(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertFloat(ctx, checkInterfaceReturnType ? func->getReturnType() : returnType, func); 
   }
   String runString(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertString(ctx, checkInterfaceReturnType ? func->getReturnType() : returnType, func); 
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      CALL_VTABLE_SETUP
      return runContextConvertObject(ctx, checkInterfaceReturnType ? func->getReturnType() : returnType, func); 
   }

   bool isBoolInt() { return boolResult; }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      int framePos = compiler->getCurrentFrameSize();
      if (thisExpr)
      {
         thisExpr->genCode(compiler, JitFramePos(framePos,jtPointer), etObject);
         genNullReferenceExceptionCheck(compiler, JitFramePos(framePos,jtPointer) );
      }
      else
         compiler->move(JitFramePos(framePos,jtPointer), sJitThis);
      compiler->addFrame(etObject);

      for(int i=0;i<funcProto->args.size(); i++)
      {
         ArgInfo &arg = funcProto->args[i];
         ExprType baseType = funcProto->cppia.types[ arg.typeId ]->expressionType;
         ExprType type = arg.optional && baseType!=etString ? etObject : baseType;

         args[i]->genCode( compiler, JitFramePos( compiler->getCurrentFrameSize() ).as( getJitType(type)), type );
         compiler->addFrame(type);
      }

      compiler->restoreFrameSize(framePos);


      JitReg thisVal = thisExpr ? sJitTemp0 : sJitThis;
      if (thisExpr)
         compiler->move( thisVal.as(jtPointer), JitFramePos(framePos,jtPointer) );

      // Store new frame in context ...
      compiler->add( sJitCtxFrame, sJitFrame, JitVal(framePos) );

      TypeData *type = funcProto->cppia.types[funcProto->returnType];
      bool isBoolReturn = type->haxeClass==ClassOf<bool>();

      if (isInterfaceCall)
      {
         //sJitTemp1 = ScriptCallable **vtable = thisVal->__GetScriptVTable();
         compiler->call( (void *)getScriptVTable,thisVal );
         compiler->move( sJitTemp1, sJitReturnReg.as(jtPointer) );
         if (thisExpr)
            compiler->move( thisVal.as(jtPointer), JitFramePos(framePos,jtPointer) );
      }
      else
      {
         //sJitTemp1 = ScriptCallable **vtable = *(ScriptCallable ***)((char *)thisVal +scriptVTableOffset);
         compiler->move(sJitTemp1.as(jtPointer), thisVal.star(jtPointer, scriptVTableOffset) );
      }

      // sJitTemp1 = table[slot]
      compiler->move(sJitTemp1, sJitTemp1.star(jtPointer, slot*sizeof(void *)) );

      // Implementation may return a more general type, with different byte representation
      if (checkInterfaceReturnType)
      {
         JitTemp actualReturnType(compiler,jtInt);
         compiler->move(actualReturnType, sJitTemp1.star(jtInt, offsetof(ScriptCallable,returnType)) );

         // vtable[slot].compiled
         compiler->call(sJitTemp1.star(jtPointer, offsetof(ScriptCallable,compiled)),sJitCtx );
         JumpId isEqual = compiler->compare(cmpI_EQUAL, actualReturnType, (int)returnType);
         // Must be int->Float
         if (returnType==etFloat)
         {
            compiler->move(sJitTemp0.as(jtPointer), sJitCtx.star(jtPointer, offsetof(CppiaCtx,frame)) );
            compiler->convert( sJitTemp0.star(jtInt), etInt, sJitTemp0.star(jtFloat), etFloat );
         }
         else // type -> Dynamic
            compiler->callNative( (void *)fixReturnType, sJitCtx, actualReturnType );

         compiler->comeFrom(isEqual);
      }
      else
      {
         // vtable[slot].compiled
         compiler->call(sJitTemp1.star(jtPointer, offsetof(ScriptCallable,compiled)),sJitCtx );
      }

      genFunctionResult(compiler, inDest, destType, returnType, isBoolReturn);
   }
   #endif
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

   const char *getName() { return "ThisExpr"; }
   hx::Object *runObject(CppiaCtx *ctx) { return ctx->getThis(); }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      compiler->convert(sJitThis, etObject, inDest, destType);
   }
   #endif
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
         value.mPtr = type->cppiaClass->getClass().mPtr;
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


#ifdef CPPIA_JIT
static int SLJIT_CALL getFieldInt( hx::Object *instance, String *name )
{
   TRY_NATIVE
   return instance->__Field(*name, HX_PROP_DYNAMIC);
   CATCH_NATIVE
   return 0;
}

static hx::Object * SLJIT_CALL getFieldObject( hx::Object *instance, String *name )
{
   TRY_NATIVE
   Dynamic ret =  instance->__Field(*name, HX_PROP_DYNAMIC);
   return ret.mPtr;
   CATCH_NATIVE
   return 0;
}

static void SLJIT_CALL getFieldFloat( hx::Object *instance, String *name, double *outValue )
{
   TRY_NATIVE
   *outValue = instance->__Field(*name, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}

static void SLJIT_CALL getFieldString( hx::Object *instance, String *name, String *outValue )
{
   TRY_NATIVE
   *outValue = instance->__Field(*name, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}

static int SLJIT_CALL setFieldInt( hx::Object *instance, String *name, int inValue )
{
   TRY_NATIVE
   instance->__SetField(*name, inValue, HX_PROP_DYNAMIC);
   return inValue;
   CATCH_NATIVE
   return 0;
}
static void SLJIT_CALL setFieldIntV( hx::Object *instance, String *name, int inValue )
{
   TRY_NATIVE
   instance->__SetField(*name, inValue, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}

static hx::Object * SLJIT_CALL setFieldObject( hx::Object *instance, String *name, hx::Object *inValue )
{
   TRY_NATIVE
   instance->__SetField(*name, Dynamic(inValue), HX_PROP_DYNAMIC );
   return inValue;
   CATCH_NATIVE
   return 0;
}

static void SLJIT_CALL setFieldObjectV( hx::Object *instance, String *name, hx::Object *inValue )
{
   TRY_NATIVE
   instance->__SetField(*name, Dynamic(inValue), HX_PROP_DYNAMIC );
   CATCH_NATIVE
}

static void SLJIT_CALL setFieldFloat( hx::Object *instance, String *name, const double *inValue )
{
   TRY_NATIVE
   instance->__SetField(*name, *inValue, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}

static void SLJIT_CALL setFieldString( hx::Object *instance, String *name, const String *inValue )
{
   TRY_NATIVE
   instance->__SetField(*name, *inValue, HX_PROP_DYNAMIC);
   CATCH_NATIVE

}
static void SLJIT_CALL setFieldVariantV( hx::Object *instance, String *name, cpp::Variant *value )
{
   TRY_NATIVE
   instance->__SetField(*name, *value, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}

static cpp::Variant * SLJIT_CALL setFieldVariant( hx::Object *instance, String *name, cpp::Variant *value )
{
   TRY_NATIVE
   instance->__SetField(*name, *value, HX_PROP_DYNAMIC);
   return value;
   CATCH_NATIVE
   return 0;
}


static void SLJIT_CALL getFieldVariant( hx::Object *instance, String *name, cpp::Variant *outValue )
{
   TRY_NATIVE
   *outValue = instance->__Field(*name, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}


static void SLJIT_CALL addVariantObject( cpp::Variant *ioVar, hx::Object *inValue )
{
   TRY_NATIVE
   *ioVar = *ioVar + Dynamic(inValue);
   CATCH_NATIVE
}


static void SLJIT_CALL addVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = *ioVar + inValue;
}


static void SLJIT_CALL addVariantString( cpp::Variant *ioVar, String *inValue )
{
   TRY_NATIVE
   *ioVar = *ioVar + *inValue;
   CATCH_NATIVE
}


static void SLJIT_CALL addVariantFloat( cpp::Variant *ioVar, double *inValue )
{
   *ioVar = *ioVar + *inValue;
}

static void SLJIT_CALL subVariantFloat( cpp::Variant *ioVar, double *inValue )
{
   *ioVar = *ioVar - *inValue;
}

static void SLJIT_CALL divVariantFloat( cpp::Variant *ioVar, double *inValue )
{
   *ioVar = *ioVar / *inValue;
}

static void SLJIT_CALL multVariantFloat( cpp::Variant *ioVar, double *inValue )
{
   *ioVar = *ioVar * *inValue;
}

static void SLJIT_CALL modVariantFloat( cpp::Variant *ioVar, double *inValue )
{
   *ioVar = hx::Mod(*ioVar,*inValue);
}

static void SLJIT_CALL andVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = (int)(*ioVar) & inValue;
}


static void SLJIT_CALL orVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = (int)(*ioVar) | inValue;
}

static void SLJIT_CALL xorVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = (int)(*ioVar) ^ inValue;
}

static void SLJIT_CALL shlVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = (int)(*ioVar) << inValue;
}

static void SLJIT_CALL shrVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = (int)(*ioVar) >> inValue;
}

static void SLJIT_CALL ushrVariantInt( cpp::Variant *ioVar, int inValue )
{
   *ioVar = (unsigned int)(int)(*ioVar) >> inValue;
}


static void SLJIT_CALL incByNameV( hx::Object *inObj, String *inName )
{
   TRY_NATIVE
   inObj->__SetField( *inName, inObj->__Field(*inName, HX_PROP_DYNAMIC) + 1, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}

static void SLJIT_CALL decByNameV( hx::Object *inObj, String *inName )
{
   TRY_NATIVE
   inObj->__SetField( *inName, inObj->__Field(*inName, HX_PROP_DYNAMIC) - 1, HX_PROP_DYNAMIC);
   CATCH_NATIVE
}


static cpp::Variant * SLJIT_CALL preIncByName( hx::Object *inObj, String *inName, cpp::Variant *ioBuf )
{
   TRY_NATIVE
   *ioBuf =  inObj->__Field(*inName, HX_PROP_DYNAMIC) + 1;
   inObj->__SetField( *inName, *ioBuf, HX_PROP_DYNAMIC);
   return ioBuf;
   CATCH_NATIVE
   return 0;
}


static cpp::Variant * SLJIT_CALL postIncByName( hx::Object *inObj, String *inName, cpp::Variant *ioBuf )
{
   TRY_NATIVE
   *ioBuf =  inObj->__Field(*inName, HX_PROP_DYNAMIC);
   inObj->__SetField( *inName, *ioBuf + 1, HX_PROP_DYNAMIC);
   return ioBuf;
   CATCH_NATIVE
   return 0;
}


static cpp::Variant * SLJIT_CALL postDecByName( hx::Object *inObj, String *inName, cpp::Variant *ioBuf )
{
   TRY_NATIVE
   *ioBuf =  inObj->__Field(*inName, HX_PROP_DYNAMIC);
   inObj->__SetField( *inName, *ioBuf - 1, HX_PROP_DYNAMIC);
   return ioBuf;
   CATCH_NATIVE
   return 0;
}


static cpp::Variant * SLJIT_CALL preDecByName( hx::Object *inObj, String *inName, cpp::Variant *ioBuf )
{
   TRY_NATIVE
   *ioBuf =  inObj->__Field(*inName, HX_PROP_DYNAMIC) - 1;
   inObj->__SetField( *inName, *ioBuf, HX_PROP_DYNAMIC);
   return ioBuf;
   CATCH_NATIVE
   return 0;
}




#endif



struct FieldByName : public CppiaDynamicExpr
{
   CppiaExpr   *object;
   String      name;
   CppiaExpr   *value;
   AssignOp    assign;
   CrementOp   crement;
   hx::Class   staticClass;

   
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

   const char *getName() { return "FieldByName"; }
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

   #ifdef CPPIA_JIT
   static int SLJIT_CALL getInt( hx::Object *inSrc, String *inName )
   {
      return inSrc->__Field(*inName,HX_PROP_DYNAMIC);
   }
   static hx::Object * SLJIT_CALL getObject( hx::Object *inSrc, String *inName )
   {
      return Dynamic(inSrc->__Field(*inName,HX_PROP_DYNAMIC)).mPtr;
   }


   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      if (crement==coNone && assign==aoNone)
      {
         CPPIALOG("FieldAccess without set\n");
         return;
      }


      JitTemp objTemp(compiler, jtPointer);
      JitVal  objSrc;

      if (object)
      {
         object->genCode(compiler, objTemp, etObject);
         objSrc = objTemp;
      }
      else if (staticClass.mPtr)
      {
         objSrc = JitVal( (void *)staticClass.mPtr );
      }
      else
      {
         objSrc = sJitThis;
      }


      bool ret = destType==etInt || destType==etString || destType==etObject || destType==etFloat;
      if (crement!=coNone)
      {
         if (!ret)
         {
            if (crement==coPostInc || crement==coPreInc)
               compiler->callNative((void *)incByNameV, objSrc, (void *)&name );
            else
               compiler->callNative((void *)decByNameV, objSrc, (void *)&name );
            compiler->checkException();
         }
         else
         {
            JitTemp gotVal(compiler, etString, sizeof(cpp::Variant));
            void *func = 0;
            switch(crement)
            {
               case coPostInc: func = (void *)postIncByName; break;
               case coPreInc: func = (void *)preIncByName; break;
               case coPostDec: func = (void *)postDecByName; break;
               case coPreDec: func = (void *)preDecByName; break;
               default:
                  CPPIALOG("Error in crement?\n");
                  CppiaTrap();
            }
            compiler->callNative(func, objSrc, (void *)&name, gotVal );
            compiler->checkException();
            compiler->genVariantValueTemp0(0, inDest, destType);
         }

         return;
      }

      if (assign == aoSet)
      {
         switch(value->getType())
         {
            case etObject:
               value->genCode(compiler,sJitArg2.as(jtPointer), etObject);
               compiler->callNative( ret ? (void *)setFieldObject : (void *)setFieldObjectV, objSrc, (void *)&name, sJitArg2.as(jtPointer) );
               compiler->checkException();
               if (ret)
                  compiler->convertReturnReg(etObject, inDest, destType );
               break;
            case etInt:
               value->genCode(compiler,sJitArg2.as(jtInt), etInt);
               compiler->callNative( ret ? (void *)setFieldInt : (void *)setFieldIntV, objSrc, (void *)&name, sJitArg2.as(jtInt) );
               compiler->checkException();
               if (ret)
                  compiler->convertReturnReg(etInt, inDest, destType );
               break;
            case etString:
               {
               JitTemp temp(compiler, jtString);
               value->genCode(compiler,temp, etString);
               compiler->callNative( (void *)setFieldString, objSrc, (void *)&name, temp );
               compiler->checkException();
               if (ret)
                  compiler->convert(temp, etString, inDest, destType );
               }
               break;
            case etFloat:
               {
               JitTemp temp(compiler, jtFloat);
               value->genCode(compiler,temp, etFloat);
               compiler->callNative( (void *)setFieldFloat, objSrc, (void *)&name, temp );
               compiler->checkException();
               if (ret)
                  compiler->convert(temp, etFloat, inDest, destType );
               }
               break;
            default:
               CPPIALOG("Unknown set type %d\n", destType);
               CppiaTrap();
         }
      }
      else
      {
         JitTemp gotVal(compiler, etString, sizeof(cpp::Variant));
         compiler->callNative( (void *)getFieldVariant, objSrc, (void *)&name, gotVal );

         if (assign==aoAdd)
         {
            switch(value->getType())
            {
               case etObject:
                  value->genCode(compiler,sJitArg1.as(jtPointer), etObject);
                  compiler->callNative( (void *)addVariantObject, gotVal, sJitArg1.as(jtPointer) );
                  break;
               case etInt:
                  value->genCode(compiler,sJitArg1.as(jtInt), etInt);
                  compiler->callNative( (void *)addVariantInt, gotVal, sJitArg1.as(jtPointer) );
                  break;
               case etString:
                  {
                  JitTemp sval(compiler, etString);
                  value->genCode(compiler,sval, etString);
                  compiler->callNative( (void *)addVariantString, gotVal, sval);
                  }
                  break;
               case etFloat:
                  {
                  JitTemp fval(compiler, jtFloat);
                  value->genCode(compiler,fval, etFloat);
                  compiler->callNative( (void *)addVariantFloat, gotVal, fval);
                  }
                  break;
               default:
                  CPPIALOG("Unknown add type %d\n", destType);
                  CppiaTrap();
             }
         }
         else if (assign==aoMult || assign==aoDiv || assign==aoMod || assign==aoSub)
         {
            JitTemp fval(compiler,jtFloat);
            value->genCode(compiler, fval, etFloat);
            switch(assign)
            {
               case aoMult: compiler->callNative( (void *)multVariantFloat, gotVal, fval); break;
               case aoDiv: compiler->callNative( (void *)divVariantFloat, gotVal, fval); break;
               case aoMod: compiler->callNative( (void *)modVariantFloat, gotVal, fval); break;
               case aoSub: compiler->callNative( (void *)subVariantFloat, gotVal, fval); break;
               default: ;
            }
         }
         else
         {
            value->genCode(compiler, sJitTemp2, etInt);
            switch(assign)
            {
               case aoAnd: compiler->callNative( (void *)andVariantInt, gotVal, sJitTemp2.as(jtInt)); break;
               case aoOr: compiler->callNative( (void *)orVariantInt, gotVal, sJitTemp2.as(jtInt)); break;
               case aoXOr: compiler->callNative( (void *)xorVariantInt, gotVal, sJitTemp2.as(jtInt)); break;
               case aoShl: compiler->callNative( (void *)shlVariantInt, gotVal, sJitTemp2.as(jtInt)); break;
               case aoShr: compiler->callNative( (void *)shrVariantInt, gotVal, sJitTemp2.as(jtInt)); break;
               case aoUShr: compiler->callNative( (void *)ushrVariantInt, gotVal, sJitTemp2.as(jtInt)); break;
               default: ;
            }
         }

         if (ret)
         {
            compiler->callNative( (void *)setFieldVariant, objSrc, (void *)&name, gotVal );
            compiler->genVariantValueTemp0(0, inDest, destType);
         }
         else
            compiler->callNative( (void *)setFieldVariantV, objSrc, (void *)&name, gotVal );
      }
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
      name.raw_ref() = 0;
      isInterface = false;
      vtableSlot = -1;
   }
   GetFieldByName(const CppiaExpr *inSrc, int inNameId, CppiaExpr *inObject,bool inIsStatic)
      : CppiaDynamicExpr(inSrc)
   {
      classId = 0;
      nameId = inNameId;
      object = inObject;
      isStatic = inIsStatic;
      isInterface = false;
      name.raw_ref() = 0;
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
            CPPIALOG("Could not link static %s::%s (%d)\n", type->name.c_str(), inModule.strings[nameId].out_str(), nameId );
            throw "Bad link";
         }
         name = inModule.strings[nameId];
         const StaticInfo *info = staticClass->GetStaticStorage(name);
         if (info && info->type!=hx::fsUnknown)
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
            CPPIALOG("Could not find vtable entry %s intf=%d (%d)\n", name.out_str(), isInterface, vtableSlot);
            return 0;
         }

         return createMemberClosure(instance, func);
      }

      return Dynamic(instance->__Field(name,HX_PROP_DYNAMIC)).mPtr;
   }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      // TODO - interfaces
      if (object)
      {
         object->genCode(compiler, sJitTemp0.as(jtPointer), etObject);
         genNullReferenceExceptionCheck(compiler, sJitTemp0);
      }
      else if (isStatic)
      {
         compiler->move(sJitTemp0, (void *)&staticClass.mPtr);
         compiler->move(sJitTemp0, sJitTemp0.star() );
      }
      else
      {
         // this...
         compiler->move(sJitTemp0, sJitThis );
      }

      switch(destType)
      {
         case etInt:
            compiler->callNative( (void *)getFieldInt, sJitTemp0.as(jtPointer), (void *)&name);
            compiler->checkException();
            compiler->move(inDest.as(jtInt), sJitReturnReg.as(jtInt));
            break;
         case etFloat:
            if (isMemoryVal(inDest))
            {
               compiler->callNative( (void *)getFieldFloat, sJitTemp0.as(jtPointer),  (void *)&name, inDest.as(jtFloat) );
               compiler->checkException();
            }
            else
            {
               JitTemp floatResult(compiler, jtFloat);
               compiler->callNative( (void *)getFieldFloat, sJitTemp0.as(jtPointer), &name, floatResult );
               compiler->checkException();
               compiler->move(inDest.as(jtFloat), floatResult);
            }
            break;
         case etString:
            compiler->callNative( (void *)getFieldString, sJitTemp0.as(jtPointer),  (void *)&name, inDest.as(jtString) );
            compiler->checkException();
            break;
         case etObject:
            compiler->callNative( (void *)getFieldObject, sJitTemp0.as(jtPointer), (void *)&name);
            compiler->checkException();
            compiler->move(inDest.as(jtPointer), sJitReturnReg.as(jtPointer));
            break;
         default: ;
      }
   }
   #endif
  
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

      func = new GetFieldByName(this, inNameId, inObject, false);
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
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitTemp functionObject(compiler, jtPointer);
      func->genCode(compiler, functionObject, etObject );

      {
      AutoFramePos frame(compiler);

      for(int a=0;a<args.size();a++)
      {
         args[a]->genCode(compiler, JitFramePos(compiler->getCurrentFrameSize(),etObject), etObject);
         compiler->addFrame(etObject);
      }
      // restore frame size
      }

      compiler->add(sJitCtxFrame, sJitFrame, compiler->getCurrentFrameSize());

      compiler->callNative((void *)callDynamic,sJitCtx,functionObject,JitVal( (int)args.size() ) );

      compiler->checkException();
      if (destType!=etVoid)
         compiler->convertResult( etObject, inDest, destType );
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
      //CPPIALOG("fieldId = %d (%s)\n",fieldId,stream.module->strings[ fieldId ].out_str());
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
         //CPPIALOG("Using cppia super %p %p\n", type->cppiaClass->newFunc, type->cppiaClass->newFunc->funExpr);
         CppiaExpr *replace = new CallFunExpr( this, 0, (ScriptCallable*)type->cppiaClass->newFunc->funExpr, args, true );
         replace->link(inModule);
         delete this;
         return replace;
      }
      //CPPIALOG("Using haxe super\n");
      HaxeNativeClass *superReg = HaxeNativeClass::findClass(type->name.utf8_str());
      if (!superReg)
      {
         CPPIALOG("No class registered for %s\n", type->name.out_str());
         throw "Unknown super call";
      }
      if (!superReg->construct.execute)
      {
         //CPPIALOG("Call super - nothing to do...\n");
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

      //CPPIALOG("  linking call %s::%s\n", type->name.out_str(), field.out_str());

      CppiaExpr *replace = 0;
      
      if (type->arrayType)
      {
         replace = createArrayBuiltin(this, type->arrayType, thisExpr, field, args);
         if (!replace)
         {
            if (field!=HX_CSTRING("__SetField") && field!=HX_CSTRING("__Field") && field!=HX_CSTRING("__Index"))
            {
               CPPIALOG("Bad array field '%s'\n", field.out_str());
               inModule.where(this);
               throw "Unknown array field name";
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
               replace = new CallFunExpr( this, thisExpr, func, args, true );
         }
         else
         {
            int vtableSlot = type->cppiaClass->findFunctionSlot(fieldId);

            //CPPIALOG("   vslot %d\n", vtableSlot);
            if (vtableSlot!=-1)
            {
               CppiaFunction *funcProto = type->cppiaClass->findVTableFunction(fieldId);
               int scriptPos = type->cppiaClass->getScriptVTableOffset();
               replace = new CallMemberVTable( this, thisExpr, vtableSlot, scriptPos, funcProto, type->cppiaClass->isInterface, args );
            }
         }

         // Try interface function implemented in host only...
         #if (HXCPP_API_LEVEL >= 330)
         if (!replace)
         {
            std::vector<ScriptNamedFunction *> &nativeInterfaceFuncs = type->cppiaClass->nativeInterfaceFunctions;
            for(int i=0;i<nativeInterfaceFuncs.size();i++)
            {
               if (!strcmp(nativeInterfaceFuncs[i]->name,field.utf8_str()))
               {
                  //CPPIALOG(" found native interface function %s\n", field.out_str() );
                  replace = new CallHaxe( this, *nativeInterfaceFuncs[i], thisExpr, args );
                  break;
               }
            }
         }
         #endif

      }
      if (!replace && type->haxeBase)
      {
         ScriptNamedFunction func = type->haxeBase->findFunction(field.utf8_str());
         if (func.signature)
         {
            if (callSuperField && !func.superExecute)
               CPPIALOG("Warning - calling super host '%s' from cppia can lead to infinte recursion\n", field.utf8_str());

            //CPPIALOG(" found function %s\n", func.signature );
            replace = new CallHaxe( this, func, thisExpr, args, false, callSuperField && func.superExecute);
         }
      }

      if (!replace && type->interfaceBase)
      {
         ScriptNamedFunction func = type->interfaceBase->findFunction(field.utf8_str());
         if (func.signature)
         {
            //CPPIALOG(" found function %s\n", func.signature );
            replace = new CallHaxe( this, func, thisExpr, args );
         }
      }

      if (!replace && type->name==HX_CSTRING("String"))
      {
         replace = createStringBuiltin(this, thisExpr, field, args);
      }

      if (!replace && field==HX_CSTRING("__Index"))
      {
         replace = new CallGetIndex(this, thisExpr);
      }
      if (!replace && field==HX_CSTRING("__SetField") && args.size()==3)
      {
         #ifdef CPPIA_JIT
         throw "Old cppia bytecode not supported by jit (__SetField)";
         #endif
         replace = new CallSetField(this, thisExpr, args[0], args[1], args[2]);
      }
      if (!replace && field==HX_CSTRING("__Field") && args.size()==2)
      {
         #ifdef CPPIA_JIT
         throw "Old cppia bytecode not supported by jit (__Field)";
         #endif
         replace = new CallGetField(this, thisExpr, args[0], args[1]);
      }

      if (!replace && type->haxeBase &&
             (type->name == HX_CSTRING("haxe.ds.IntMap") ||
              type->name == HX_CSTRING("haxe.ds.StringMap") ||
              type->name == HX_CSTRING("haxe.ds.ObjectMap") ) )
      {
         const char *baseFunc = 0;
         if (field.raw_ptr()[0] == 's')
            baseFunc = "set";
         else if (field.raw_ptr()[0] == 'g')
            baseFunc = "get";

         if (baseFunc)
         {
            ScriptFunction func = type->haxeBase->findFunction(baseFunc);
            if (func.signature)
            {
               //CPPIALOG(" replacing %s.%s, signature %s\n", type->name.out_str(), field.out_str(), func.signature);
               replace = new CallHaxe( this, func, thisExpr, args );
            }
         }
      }

      if (replace)
      {
         replace->link(inModule);
         delete this;
         return replace;
      }

      if (!type->isDynamic)
      {
         CPPIALOG("   CallMember %s (%p %p) '%s' fallback\n", type->name.out_str(), type->haxeClass.mPtr, type->cppiaClass, field.out_str());
      }

      {
         replace = new Call( this, fieldId, thisExpr, args );
         replace->link(inModule);
         delete this;
         return replace;
      }

      /*
      CPPIALOG("Could not link %s::%s\n", type->name.c_str(), field.out_str() );
      CPPIALOG("%p %p\n", type->cppiaClass, type->haxeBase);
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





inline hx::Object *CheckNotNull(hx::Object *inPtr)
{
   CPPIA_CHECK(inPtr);
   return inPtr;
}

template<typename T, int REFMODE> 
struct MemReference : public CppiaExpr
{
   int  offset;
   T *pointer;
   CppiaExpr *object;


   #define MEMGETVAL \
     *(T *)( \
         ( REFMODE==locObj      ?(char *)CheckNotNull(object->runObject(ctx)) : \
           REFMODE==locAbsolute ?(char *)pointer : \
           REFMODE==locThis     ?(char *)ctx->getThis() : \
                                 (char *)ctx->frame \
         ) + offset )

   #define MEMGETPTR \
      (T *)( \
         ( REFMODE==locObj      ?(char *)CheckNotNull(object->runObject(ctx)) : \
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
   bool isBoolInt()
   {
      return ExprTypeIsBool<T>::value;
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
      return ValToInt( MEMGETVAL );
   }
   Float       runFloat(CppiaCtx *ctx)
   {
      return ValToFloat( MEMGETVAL );
   }
   ::String    runString(CppiaCtx *ctx) {
      T &t = MEMGETVAL;
      BCR_CHECK;
      if (isBoolInt())
         return ValToString( MEMGETVAL ? true : false );
      return ValToString(t);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (isBoolInt())
         return Dynamic( MEMGETVAL ? true : false ).mPtr;
      return Dynamic( MEMGETVAL ).mPtr;
   }

   #ifdef CPPIA_JIT


   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
     if (REFMODE==locAbsolute)
     {
        compiler->move( sJitTemp0,  JitVal( (void *)pointer ) );
        if (isBoolInt())
        {
           compiler->move( sJitTemp0, sJitTemp0.star(jtByte,0) );
           compiler->convert( sJitTemp0,getType(),inDest, destType, true );
        }
        else
           compiler->convert( sJitTemp0.star(jtPointer,0), getType(),inDest, destType );
     }
     else if (REFMODE==locObj)
     {
        object->genCode( compiler, sJitTemp2, etObject );
        genNullReferenceExceptionCheck(compiler, sJitTemp2);

        if (isBoolInt())
        {
           compiler->move( sJitTemp2, sJitTemp2.star(jtByte,offset) );
           compiler->convert( sJitTemp2,getType(),inDest, destType, true );
        }
        else
           compiler->convert( sJitTemp2.star(jtPointer,offset) ,getType(),inDest, destType );
     }
     else if (REFMODE==locThis)
     {
        JitThisPos target(offset, isBoolInt() ? jtByte : getJitType(getType()) );
        compiler->convert( target,getType(),inDest, destType, isBoolInt() );
     }
     else
     {
        JitFramePos target(offset, isBoolInt() ? jtByte : getJitType(getType()));
        compiler->convert( target,getType(),inDest, destType, isBoolInt() );
     }
   }
   #endif


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


#ifdef CPPIA_JIT

static void SLJIT_CALL stringCat(String *ioValue, String *inValue)
{
   *ioValue += *inValue;
}

static hx::Object * SLJIT_CALL dynamicCatString(hx::Object *inLeft, String *inRight)
{
   TRY_NATIVE
   return Dynamic( Dynamic(inLeft) + *inRight ).mPtr;
   CATCH_NATIVE
   return 0;
}


static hx::Object * SLJIT_CALL dynamicCatDynamic(hx::Object *inLeft, hx::Object *inRight)
{
   return ( Dynamic(inLeft) + Dynamic(inRight) ).mPtr;
}

static int SLJIT_CALL modIntFloat(int inI, double *inMod)
{
   return hx::Mod(inI, *inMod);
}


static void SLJIT_CALL modFloatFloat(double *ioVal, double *inMod)
{
   *ioVal =  hx::Mod(*ioVal, *inMod);
}


static hx::Object * SLJIT_CALL modObjectFloat(hx::Object *inVal, double *inMod)
{
   return Dynamic(hx::Mod(Dynamic(inVal), *inMod)).mPtr;
}


void genBitOpSet(CppiaCompiler *compiler, BitOp bitOp, const JitVal &ioValue, ExprType exprType, CppiaExpr *inExpr)
{
   if (exprType==etInt)
   {
      inExpr->genCode(compiler, sJitTemp2, etInt);
      compiler->bitOp(bitOp, ioValue, ioValue, sJitTemp2);
   }
   else
   {
      JitTemp ival(compiler, etInt);
      compiler->convert(ioValue, exprType, ival, etInt);
      inExpr->genCode(compiler, sJitTemp2, etInt);

      compiler->bitOp(bitOp, sJitTemp0.as(jtInt), ival, sJitTemp2);
      compiler->convert(sJitTemp0, etInt, ioValue, exprType);
   }
}

void genSetter(CppiaCompiler *compiler, const JitVal &ioValue, ExprType exprType, AssignOp inOp, CppiaExpr *inExpr)
{
   switch(inOp)
   {
      case aoMult:
         if (exprType==etInt)
         {
            inExpr->genCode(compiler, sJitTemp0, etInt);
            compiler->mult(ioValue, sJitTemp0, ioValue,false);
         }
         else
         {
            inExpr->genCode(compiler, sJitTempF0, etFloat);
            if (exprType==etFloat)
               compiler->mult(ioValue, sJitTempF0, ioValue,true);
            else
            {
               compiler->mult(sJitTempF0, sJitTempF0, ioValue,true);
               compiler->convert(sJitTempF0, etFloat, ioValue, exprType );
            }
         }
         break;


      case aoMod:
         {
            JitTemp fval(compiler, jtFloat);
            inExpr->genCode(compiler, fval, etFloat);
            if (exprType==etInt)
            {
               compiler->callNative( (void *)modIntFloat,  ioValue, fval );
               compiler->move(ioValue, sJitReturnReg.as(jtInt) );
            }
            else if (exprType==etFloat)
            {
               compiler->callNative( (void *)modFloatFloat,  ioValue, fval );
            }
            else if (exprType==etObject)
            {
               compiler->callNative( (void *)modObjectFloat,  ioValue, fval );
               compiler->move(ioValue, sJitReturnReg);
            }
         }
         break;

      case aoAdd:
         if (exprType==etInt)
         {
            inExpr->genCode(compiler, sJitTemp0, etInt);
            compiler->add(ioValue, sJitTemp0, ioValue);
         }
         else if (exprType==etFloat)
         {
            inExpr->genCode(compiler, sJitTempF0, etFloat);
            compiler->add(ioValue, sJitTempF0, ioValue);
         }
         else if (exprType==etString)
         {
            JitTemp sval(compiler,jtString);
            inExpr->genCode(compiler, sval, etString);
            compiler->callNative((void *)stringCat, ioValue, sval );
         }
         else if (exprType==etObject)
         {
            if (inExpr->getType()==etString)
            {
               JitTemp sval(compiler,jtString);
               inExpr->genCode(compiler, sval, etString);
               compiler->callNative((void *)dynamicCatString, ioValue.as(jtPointer), sval );
               compiler->checkException();
            }
            else
            {
               inExpr->genCode(compiler, sJitArg1, etObject);
               compiler->callNative((void *)dynamicCatDynamic, ioValue, sJitArg1.as(jtPointer) );
            }
            compiler->move(ioValue, sJitReturnReg.as(jtPointer) );
         }
         break;

      case aoSub:
         if (exprType==etInt)
         {
            inExpr->genCode(compiler, sJitTemp0, etInt);
            compiler->sub(ioValue, ioValue, sJitTemp0, false);
         }
         else if (exprType==etFloat)
         {
            inExpr->genCode(compiler, sJitTempF0, etFloat);
            compiler->sub(ioValue, ioValue, sJitTempF0, true);
         }
         else
         {
            JitTemp fval(compiler, jtFloat);
            compiler->convert(ioValue, exprType, fval, etFloat);
            inExpr->genCode(compiler, sJitTempF0, etFloat);
            compiler->sub(sJitTempF0, fval, sJitTempF0, true);
            compiler->convert(sJitTempF0, etFloat, ioValue, exprType);
         }
         break;


      case aoDiv:
         if (exprType==etFloat)
         {
            compiler->fdiv(ioValue, ioValue, sJitTempF0);
            inExpr->genCode(compiler, sJitTempF0, etFloat);
         }
         else
         {
            JitTemp fval(compiler, jtFloat);
            compiler->convert(ioValue, exprType, fval, etFloat);

            inExpr->genCode(compiler, sJitTempF0, etFloat);

            compiler->fdiv(sJitTempF0, fval, sJitTempF0);
            compiler->convert(sJitTempF0, etFloat, ioValue, exprType );
         }
         break;

      case aoAnd: genBitOpSet(compiler, bitOpAnd, ioValue, exprType, inExpr); break;
      case aoOr: genBitOpSet(compiler, bitOpOr, ioValue, exprType, inExpr); break;
      case aoXOr: genBitOpSet(compiler, bitOpXOr, ioValue, exprType, inExpr); break;
      case aoShl: genBitOpSet(compiler, bitOpShiftL, ioValue, exprType, inExpr); break;
      case aoShr: genBitOpSet(compiler, bitOpShiftR, ioValue, exprType, inExpr); break;
      case aoUShr: genBitOpSet(compiler, bitOpUSR, ioValue, exprType, inExpr); break;

      default:
         inExpr->genCode(compiler, sJitTemp2, etInt);
         CPPIALOG("Gen setter %d\n", inOp);
   }
}
#endif

template<typename T>
inline static bool isPointerObject(T *) { return false; }
inline static bool isPointerObject(hx::Object **) { return true; }
inline static bool isPointerObject(String *) { return true; }
template<typename T>
inline static void * getPointerFrom(T *) { return 0; }
inline static void * getPointerFrom(hx::Object **o) { return *o; }
inline static void * getPointerFrom(String *s) { return (void *)s->raw_ptr(); }

#ifdef HXCPP_GC_GENERATIONAL
  #define MEM_WB_CHECK \
     if (isPointerObject(t)) {\
        if (REFMODE==locThis) { \
           hx::Object *thiz = ctx->getThis(); \
           HX_OBJ_WB_CTX(thiz,getPointerFrom(t),ctx); \
        }  \
        else if (REFMODE==locObj) { \
           hx::Object *obj = (hx::Object *)( (char *)(t) - offset ); \
           HX_OBJ_WB_CTX(obj,getPointerFrom(t),ctx); \
        } \
     }

#else
  #define MEM_WB_CHECK
#endif

#if defined(HXCPP_GC_GENERATIONAL) && defined(CPPIA_JIT)
static void SLJIT_CALL pushWriteBarrier(hx::StackContext *inCtx, hx::Object *inObj)
{
   unsigned char &mark =  ((unsigned char *)(inObj))[ HX_ENDIAN_MARK_ID_BYTE];
   mark|=HX_GC_REMEMBERED;
   inCtx->pushReferrer(inObj);
}


// objVal - should not be sJitTemp1
void genWriteBarrier(CppiaCompiler *compiler, JitReg objVal, JitVal valuePtr)
{

   // unsigned char mark =  ((unsigned char *)(obj))[ HX_ENDIAN_MARK_ID_BYTE] != ctx->byteMarkId
   compiler->move(sJitTemp1, objVal.star(jtByte, HX_ENDIAN_MARK_ID_BYTE) );
   JumpId newMark = compiler->compare(cmpI_NOT_EQUAL,sJitTemp1,sJitCtx.star(jtInt, offsetof(hx::StackContext,byteMarkId)));

   compiler->move(sJitTemp1, valuePtr );

   // Value != 0
   JumpId nullValue = compiler->compare(cmpP_ZERO, sJitTemp1.as(jtPointer),0);
   compiler->move(sJitTemp1, sJitTemp1.star(jtByte, HX_ENDIAN_MARK_ID_BYTE) );
   JumpId notNursery = compiler->compare(cmpI_NOT_ZERO, sJitTemp1, 0);

   // TODO - not use temp?
   JitTemp tmpObj(compiler, jtPointer);
   compiler->move(tmpObj, objVal);
   compiler->callNative( (void *)pushWriteBarrier, sJitCtx, tmpObj );

   compiler->comeFrom(notNursery);
   compiler->comeFrom(nullValue);
   compiler->comeFrom(newMark);
}


#endif


template<typename T, int REFMODE, typename Assign> 
struct MemReferenceSetter : public CppiaExpr
{
   int offset;
   T         *pointer;
   CppiaExpr *object;
   CppiaExpr *value;
   AssignOp  op;


   MemReferenceSetter(MemReference<T,REFMODE> *inSrc, CppiaExpr *inValue, AssignOp inOp)
       : CppiaExpr(inSrc), op(inOp)
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

   const char *getName() { return "MemReferenceSetter"; }

   void runVoid(CppiaCtx *ctx)
   {
      T *t = MEMGETPTR;
      BCR_VCHECK;
      Assign::run( *t, ctx, value);
      MEM_WB_CHECK;
   }
   int runInt(CppiaCtx *ctx)
   {
      T *t = MEMGETPTR;
      BCR_CHECK;
      int val = ValToInt( Assign::run(*t,ctx, value ) );
      MEM_WB_CHECK;
      return val;
   }
   Float runFloat(CppiaCtx *ctx)
   {
      T *t = MEMGETPTR;
      BCR_CHECK;
      Float val = ValToFloat( Assign::run(*t,ctx, value) );
      MEM_WB_CHECK;
      return val;
   }
   ::String runString(CppiaCtx *ctx)
   {
      T *t = MEMGETPTR;
      BCR_CHECK;
      String val = ValToString( Assign::run(*t,ctx, value) );
      MEM_WB_CHECK;
      return val;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      T *t = MEMGETPTR;
      BCR_CHECK;
      Dynamic result( Assign::run(*t,ctx,value) );
      MEM_WB_CHECK;
      return result.mPtr;
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitType targetType = sizeof(T)==1 ? jtByte : sizeof(T)==2 ? jtShort : getJitType(getType());
      bool useTemp =  targetType == jtByte || targetType==jtShort;

      switch(REFMODE)
      {
         case locObj:
            {
            JitTemp tmpObject(compiler,jtPointer);
            object->genCode(compiler, tmpObject, etObject);
            genNullReferenceExceptionCheck(compiler, tmpObject);
            JitTemp tmpVal(compiler,getType());

            if (op==aoSet)
            {
               value->genCode(compiler, tmpVal, getType());
            }
            else
            {
               compiler->move( sJitTemp2,  tmpObject );
               compiler->move( tmpVal, sJitTemp2.star(targetType) + offset );
               genSetter(compiler, tmpVal, getType(), op, value);
            }
            compiler->move( sJitTemp2,  tmpObject );

            compiler->move( sJitTemp2.star(targetType) + offset, tmpVal );

            #ifdef HXCPP_GC_GENERATIONAL
            if (isPointerObject((T*)0))
               genWriteBarrier(compiler, sJitTemp2, (tmpVal + ((targetType==jtString ? StringOffset::Ptr : 0))).as(jtPointer) );
            #endif

            compiler->convert( tmpVal, getType(), inDest, destType );
            }
            break;

         case locThis:
         case locStack:
            {
            JitVal target = REFMODE==locThis ?  (JitVal)JitThisPos(offset, targetType ) :
                                                (JitVal)JitFramePos(offset,targetType );

            if (op==aoSet)
            {
               if (useTemp)
               {
                  JitTemp tmp(compiler, getType());
                  value->genCode(compiler, tmp, getType());
                  compiler->move(target,tmp);
                  compiler->convert( tmp,getType(),inDest, destType );
               }
               else
               {
                  value->genCode(compiler, target, getType());

                  #ifdef HXCPP_GC_GENERATIONAL
                  if (REFMODE==locThis && isPointerObject((T*)0))
                     genWriteBarrier(compiler, sJitThis, targetType==jtString ? (target+StringOffset::Ptr).as(jtPointer) : target );
                  #endif

                  compiler->convert( target,getType(),inDest, destType );
               }
            }
            else
            {
               JitTemp tmp(compiler,getType());
               compiler->move(tmp,target);
               genSetter(compiler, tmp, getType(), op, value);
               compiler->move(target,tmp);
               compiler->convert( tmp, getType(),inDest, destType );
            }
            break;
            }

         case locAbsolute:
            {
            JitTemp tmpVal(compiler,getType());

            if (op==aoSet)
            {
               value->genCode(compiler, tmpVal, getType());
            }
            else
            {
               compiler->move( sJitTemp2,  JitVal( (void *)pointer ) );
               compiler->move( tmpVal, sJitTemp2.star( targetType, offset ) );
               genSetter(compiler, tmpVal, getType(), op, value);
            }

            compiler->move( sJitTemp2,  JitVal( (void *)pointer ) );
            compiler->move( sJitTemp2.star(targetType), tmpVal );

            compiler->convert( tmpVal, getType(), inDest, destType );

            }
            break;

         default: CPPIALOG("unknown REFMODE\n");
      }
   }

   #endif

};


template<typename T, int REFMODE> 
CppiaExpr *MemReference<T,REFMODE>::makeSetter(AssignOp op,CppiaExpr *value)
{
   switch(op)
   {
      case aoSet:
         return new MemReferenceSetter<T,REFMODE,AssignSet>(this,value,op);
      case aoAdd:
         return new MemReferenceSetter<T,REFMODE,AssignAdd>(this,value,op);
      case aoMult:
         return new MemReferenceSetter<T,REFMODE,AssignMult>(this,value,op);
      case aoDiv:
         return new MemReferenceSetter<T,REFMODE,AssignDiv>(this,value,op);
      case aoSub:
         return new MemReferenceSetter<T,REFMODE,AssignSub>(this,value,op);
      case aoAnd:
         return new MemReferenceSetter<T,REFMODE,AssignAnd>(this,value,op);
      case aoOr:
         return new MemReferenceSetter<T,REFMODE,AssignOr>(this,value,op);
      case aoXOr:
         return new MemReferenceSetter<T,REFMODE,AssignXOr>(this,value,op);
      case aoShl:
         return new MemReferenceSetter<T,REFMODE,AssignShl>(this,value,op);
      case aoShr:
         return new MemReferenceSetter<T,REFMODE,AssignShr>(this,value,op);
      case aoUShr:
         return new MemReferenceSetter<T,REFMODE,AssignUShr>(this,value,op);
      case aoMod:
         return new MemReferenceSetter<T,REFMODE,AssignMod>(this,value,op);
      default: ;
   }
   throw "Bad assign op";
   return 0;
}

#ifdef CPPIA_JIT
static double sOne = 1.0;
static double sMinusOne = -1.0;

static void SLJIT_CALL objDec(hx::Object **ioVal)
{
   *ioVal = (Dynamic(*ioVal)-1).mPtr;
}
static void SLJIT_CALL objInc(hx::Object **ioVal)
{
   *ioVal = (Dynamic(*ioVal)+1).mPtr;
}

static hx::Object * SLJIT_CALL objPostInc(hx::Object **ioVal)
{
   hx::Object *result = *ioVal;
   *ioVal = (Dynamic(*ioVal)+1).mPtr;
   return result;
}
static hx::Object * SLJIT_CALL objPostDec(hx::Object **ioVal)
{
   hx::Object *result = *ioVal;
   *ioVal = (Dynamic(*ioVal)-1).mPtr;
   return result;
}

static hx::Object * SLJIT_CALL objPreInc(hx::Object **ioVal)
{
   *ioVal = (Dynamic(*ioVal)+1).mPtr;
   return *ioVal;
}
static hx::Object * SLJIT_CALL objPreDec(hx::Object **ioVal)
{
   *ioVal = (Dynamic(*ioVal)-1).mPtr;
   return *ioVal;
}

#ifdef HXCPP_GC_GENERATIONAL
static void SLJIT_CALL objDecWb(hx::Object *inObj,int inOffset)
{
   hx::Object ** ioVal = (hx::Object **)( (char *)inObj + inOffset );
   *ioVal = (Dynamic(*ioVal)-1).mPtr;
   HX_OBJ_WB_GET(inObj, *ioVal);
}
static void SLJIT_CALL objIncWb(hx::Object *inObj,int inOffset)
{
   hx::Object ** ioVal = (hx::Object **)( (char *)inObj + inOffset );
   *ioVal = (Dynamic(*ioVal)+1).mPtr;
   HX_OBJ_WB_GET(inObj, *ioVal);
}

static hx::Object * SLJIT_CALL objPostIncWb(hx::Object *inObj,int inOffset)
{
   hx::Object ** ioVal = (hx::Object **)( (char *)inObj + inOffset );
   hx::Object *result = *ioVal;
   *ioVal = (Dynamic(result)+1).mPtr;
   HX_OBJ_WB_GET(inObj, *ioVal);
   return result;
}
static hx::Object * SLJIT_CALL objPostDecWb(hx::Object *inObj,int inOffset)
{
   hx::Object ** ioVal = (hx::Object **)( (char *)inObj + inOffset );
   hx::Object *result = *ioVal;
   *ioVal = (Dynamic(result)-1).mPtr;
   HX_OBJ_WB_GET(inObj, *ioVal);
   return result;
}

static hx::Object * SLJIT_CALL objPreIncWb(hx::Object *inObj,int inOffset)
{
   hx::Object ** ioVal = (hx::Object **)( (char *)inObj + inOffset );
   *ioVal = (Dynamic(*ioVal)+1).mPtr;
   HX_OBJ_WB_GET(inObj, *ioVal);
   return *ioVal;
}
static hx::Object * SLJIT_CALL objPreDecWb(hx::Object *inObj,int inOffset)
{
   hx::Object ** ioVal = (hx::Object **)( (char *)inObj + inOffset );
   *ioVal = (Dynamic(*ioVal)-1).mPtr;
   HX_OBJ_WB_GET(inObj, *ioVal);
   return *ioVal;
}

#endif

#endif

template<typename T, int REFMODE,typename CREMENT> 
struct MemReferenceCrement : public CppiaExpr
{
   int offset;
   T   *pointer;
   CppiaExpr *object;
   const char *getName() { return "MemReferenceCrement"; }

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
      T *t = MEMGETPTR;
      CREMENT::run( *t );
      MEM_WB_CHECK;
   }
   int runInt(CppiaCtx *ctx) {
      T *t = MEMGETPTR;
      BCR_CHECK;
      int result = ValToInt( CREMENT::run(*t) );
      MEM_WB_CHECK;
      return result;
   }
   Float       runFloat(CppiaCtx *ctx) {
      T *t = MEMGETPTR;
      BCR_CHECK;
      Float result = ValToFloat( CREMENT::run(*t));
      MEM_WB_CHECK;
      return result;
   }
   ::String    runString(CppiaCtx *ctx) {
      T *t = MEMGETPTR;
      BCR_CHECK;
      String result = ValToString( CREMENT::run(*t) );
      MEM_WB_CHECK;
      return result;
   }

   hx::Object *runObject(CppiaCtx *ctx) {
      T *t = MEMGETPTR;
      BCR_CHECK;
      Dynamic result( CREMENT::run(*t) );
      MEM_WB_CHECK;
      return result.mPtr;
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


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      CrementOp op = (CrementOp)CREMENT::OP;
      int diff =  op==coPostDec || op==coPreDec ? -1 : 1;

      JitVal ioPtr;

      switch(REFMODE)
      {
         case locObj:
            object->genCode(compiler, sJitTemp0.as(jtPointer), etObject);
            genNullReferenceExceptionCheck(compiler, sJitTemp0);
            ioPtr = sJitTemp0.star() + offset;
            break;

         case locThis:
            ioPtr = sJitThis.star() + offset;
            break;

         case locAbsolute:
            compiler->move( sJitTemp0,  JitVal( (void *)pointer ) );
            ioPtr =  sJitTemp0.star();
            break;

         case locStack:
            ioPtr = JitFramePos(offset,jtInt);
            break;
      }

      switch(getType())
      {
         case etInt:
            ioPtr.type = jtInt;
            if ( inDest.type==jtVoid)
            {
               compiler->move( sJitTemp1.as(jtInt), ioPtr );
               compiler->add( ioPtr,  sJitTemp1.as(jtInt), diff );
            }
            else if (op==coPostInc || op==coPostDec)
            {
               if (destType==etInt)
               {
                  compiler->move( sJitTemp1.as(jtInt), ioPtr );
                  compiler->add( ioPtr, sJitTemp1.as(jtInt), diff );
                  compiler->move( inDest, sJitTemp1.as(jtInt) );
               }
               else
               {
                  JitTemp result(compiler, jtInt);
                  compiler->move( result, ioPtr );
                  compiler->add( ioPtr, ioPtr, diff );
                  compiler->convert( result, etInt, inDest, destType );
               }
            }
            else
            {
               if (destType==etInt)
               {
                  compiler->add( sJitTemp1.as(jtInt), ioPtr, diff );
                  compiler->move( ioPtr, sJitTemp1.as(jtInt));
                  compiler->move( inDest, sJitTemp1.as(jtInt));
               }
               else
               {
                  JitTemp result(compiler, jtInt);
                  compiler->add( result, ioPtr, diff );
                  compiler->move( ioPtr, result );
                  compiler->convert(result, etInt, inDest, destType);
               }
            }
            break;

         case etFloat:
            {
            ioPtr.type = jtFloat;
            compiler->move( sJitTemp1, (void *)(diff<0 ? &sMinusOne : &sOne) );
            if ( inDest.type==jtVoid)
            {
               compiler->add( ioPtr,  ioPtr, sJitTemp1.star(etFloat) );
            }
            else if (op==coPostInc || op==coPostDec)
            {
               compiler->move( sJitTempF0, ioPtr );
               compiler->add( ioPtr, sJitTempF0, sJitTemp1.star(etFloat) );
               compiler->convert( sJitTempF0, etFloat, inDest, destType );
            }
            else
            {
               compiler->add( sJitTempF0, ioPtr, sJitTemp1.star(etFloat) );
               compiler->move( ioPtr, sJitTempF0);
               compiler->convert( sJitTempF0, etFloat, inDest, destType );
            }
            }
            break;

         case etObject:
            #ifdef HXCPP_GC_GENERATIONAL
            if (REFMODE==locObj || REFMODE==locThis)
            {
               JitVal obj = REFMODE==locThis ? sJitThis.as(jtPointer) : sJitTemp0.as(jtPointer);
               if ( inDest.type==jtVoid)
               {
                  compiler->callNative( diff<0 ? (void *)objDecWb : (void *)objIncWb, obj, offset );
               }
               else
               {
                  void *func = 0;
                  switch(op)
                  {
                     case coPostDec: func = (void *)objPostDecWb; break;
                     case coPreDec: func = (void *)objPreDecWb; break;
                     case coPostInc: func = (void *)objPostIncWb; break;
                     case coPreInc: func = (void *)objPreIncWb; break;
                     default: ;
                  }
                  compiler->callNative(func, obj, offset);
                  compiler->convertReturnReg( etObject, inDest, destType );
               }

            }
            else
            #endif
            {
               // Convert to address...
               ioPtr.type = jtString;
               if ( inDest.type==jtVoid)
               {
                  compiler->callNative( diff<0 ? (void *)objDec : (void *)objInc, ioPtr );
               }
               else
               {
                  void *func = 0;
                  switch(op)
                  {
                     case coPostDec: func = (void *)objPostDec; break;
                     case coPreDec: func = (void *)objPreDec; break;
                     case coPostInc: func = (void *)objPostInc; break;
                     case coPreInc: func = (void *)objPreInc; break;
                     default: ;
                  }
                  compiler->callNative(func, ioPtr);
                  compiler->convertReturnReg( etObject, inDest, destType );
               }
            }
            break;

         default:
            compiler->trace("todo - MemReferenceCrement , non-int");
         // TODO
      }
   }
   #endif

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
   const char *getName() { return "MemStackFloatSetter"; }

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
   const char *getName() { return "MemStackFloatCrement"; }

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

   #ifdef CPPIA_JIT
   static int SLJIT_CALL getLen(cpp::VirtualArray_obj *inVArray)
   {
      return inVArray->get_length();
   }
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      thisExpr->genCode(compiler,sJitTemp0,etObject);
      compiler->callNative((void *)getLen, sJitTemp0.as(jtPointer));
      compiler->convertReturnReg(etInt, inDest, destType);
   }
   #endif
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
      bool forceNamedAccess = false;

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
            DBGLOG(" found haxe var %s::%s = %d (%d)\n", type->haxeClass->mName.out_str(), field.out_str(), offset, storeType);
         }
      }

      if (!offset && type->cppiaClass)
      {
         CppiaVar *var = type->cppiaClass->findVar(false, fieldId);
         if (var)
         {
            offset = var->offset;
            storeType = var->storeType;
            DBGLOG(" found script var %s = %d (%d)\n", field.out_str(), offset, storeType);
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
                forceNamedAccess = true;
                break;
                ;
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
         // This is the offset into the StringData class...
         // TODO - convert into non-wrapped string length
         int offset = (int) offsetof( String, length ) + sizeof(void *);
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
      if (!type->isInterface && !type->isDynamic && !forceNamedAccess)
      {
         CPPIALOG("   GetFieldByLinkage %s (%p %p %p) '%s' fallback\n", type->name.out_str(), object, type->haxeClass.mPtr, type->cppiaClass, field.out_str());
         if (type->cppiaClass)
            type->cppiaClass->dump();
         else
           CPPIALOG(" - is Native class\n");
      }

      CppiaExpr *result = new GetFieldByName(this, fieldId, object, false);
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
      //CPPIALOG("Linked %d -> %s\n", stringId, strVal.out_str());
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
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(value);
   }
#endif


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      switch(destType)
      {
         case etObject:
           //TODO - GC! 
           if (!value.mPtr)
              value = strVal;
           compiler->move(inDest, (void *) value.mPtr );
           break;


         case etString:
            if (!isMemoryVal(inDest))
               compiler->setError("Bad String target");
            else
            {
               compiler->move(inDest.as(jtInt),JitVal(strVal.length));
               compiler->move(inDest.as(jtPointer)+StringOffset::Ptr,JitVal((void *)strVal.raw_ptr()));
            }
            break;

         default:
            compiler->setError("Bad String conversion type");
      }
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
   const char *getName() { return "DataVal"; }

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

   #ifdef CPPIA_JIT
   String stringConversion;
   double doubleConversion;

   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      switch(destType)
      {
         case etInt:
            compiler->move(inDest, runInt(0));
            break;
         case etFloat:
            doubleConversion = runFloat(0);
            compiler->move(sJitTemp0, (void *)&doubleConversion);
            compiler->move(inDest, sJitTemp0.star(jtFloat));
            break;
         case etString:
            {
               if (!stringConversion.raw_ptr())
                  stringConversion = runString(0).makePermanent();

               compiler->move(inDest, stringConversion.length);
               compiler->move(inDest+StringOffset::Ptr, (void *)stringConversion.raw_ptr());
               break;
            }
         case etObject:
            // should be by address for gc? or pinned?
            compiler->move(inDest, (void *)runObject(0));
            break;
         default: ;
      }
   }
   #endif
};



struct NullVal : public CppiaExpr
{
   NullVal() { }
   ExprType getType() { return etObject; }
   const char *getName() { return "NullVal"; }

   void        runVoid(CppiaCtx *ctx) {  }
   int runInt(CppiaCtx *ctx) { return 0; }
   Float       runFloat(CppiaCtx *ctx) { return 0.0; }
   ::String    runString(CppiaCtx *ctx) { return null(); }
   hx::Object  *runObject(CppiaCtx *ctx) { return 0; }

   
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      compiler->returnNull(inDest, destType);
   }
   #endif

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


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      // TODO GC
      compiler->move(inDest, (void *)value.mPtr );
   }
   #endif

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

   #ifdef CPPIA_JIT
   static hx::Object * SLJIT_CALL createAnon()
   {
      return hx::Anon_obj::Create().mPtr;
   }

   static void SLJIT_CALL anonAdd(hx::Anon_obj *inAnon, String *inName, hx::Object *inValue)
   {
      inAnon->Add(*inName, Dynamic(inValue), false );
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JitTemp obj(compiler, jtPointer);
      compiler->callNative( (void *)createAnon );
      compiler->move(obj, sJitReturnReg.as(jtPointer) );
      for(int i=0;i<fieldCount;i++)
      {
         values[i]->genCode(compiler, sJitArg2.as(jtPointer), etObject);
         compiler->callNative( (void *)anonAdd, obj, (void *)&data->strings[stringIds[i]],sJitArg2); 
      }
      if (destType!=etVoid && destType!=etNull)
         compiler->convert(obj,etObject,inDest,destType);
   }
   #endif
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
         CPPIALOG("ArrayDef of non array-type %s\n", type->name.out_str());
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
               result->__unsafe_set(i,items[i]->runInt(ctx)!=0);
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrUnsignedChar:
            { 
            Array<unsigned char> result = Array_obj<unsigned char>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result->__unsafe_set(i,items[i]->runInt(ctx));
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrInt:
            { 
            Array<int> result = Array_obj<int>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result->__unsafe_set(i,items[i]->runInt(ctx));
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrFloat:
            { 
            Array<Float> result = Array_obj<Float>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result->__unsafe_set(i,items[i]->runFloat(ctx));
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrFloat32:
            { 
            Array<float> result = Array_obj<float>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result->__unsafe_set(i,items[i]->runFloat(ctx));
               BCR_CHECK;
            }
            return result.mPtr;
            }
         case arrString:
            { 
            Array<String> result = Array_obj<String>::__new(n,n);
            for(int i=0;i<n;i++)
            {
               result->__unsafe_set(i,items[i]->runString(ctx));

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
               result->__unsafe_set(i,items[i]->runObject(ctx));
               BCR_CHECK;
            }
            return result.mPtr;
            }
         default:
            return 0;
      }
      return 0;
   }


#ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      int n = items.size();
      switch(arrayType)
      {
         case arrBool:
            compiler->callNative( (void *)createArrayBool, n);
            break;
         case arrUnsignedChar:
            compiler->callNative( (void *)createArrayUChar, n);
            break;
         case arrInt:
            compiler->callNative( (void *)createArrayInt, n);
            break;
         case arrFloat:
            compiler->callNative( (void *)createArrayFloat, n);
            break;
         case arrFloat32:
            compiler->callNative( (void *)createArrayFloat32, n);
            break;
         case arrString:
            compiler->callNative( (void *)createArrayString, n);
            break;
         case arrAny:
            compiler->callNative( (void *)createArrayAny, n);
            break;
         case arrObject:
            compiler->callNative( (void *)createArrayObject, n);
            break;

         default:
            CPPIALOG("unknown array creation\n");
      }
      JitTemp array(compiler, jtPointer);
      compiler->move( array, sJitReturnReg );

      JitTemp arrayPtr(compiler, jtPointer);
      compiler->move( arrayPtr, sJitReturnReg.star() + hx::ArrayBase::baseOffset() );
      for(int i=0;i<n;i++)
      {
         switch(arrayType)
         {
            case arrInt:
               items[i]->genCode(compiler, sJitTemp0, etInt );
               compiler->move(sJitTemp1, arrayPtr);
               compiler->move(sJitTemp1.star(jtInt)+i*sizeof(int), sJitTemp0.as(jtInt));
               break;
            case arrObject:
               items[i]->genCode(compiler, sJitTemp0, etObject );
               compiler->move(sJitTemp1, arrayPtr);
               compiler->move(sJitTemp1.star(jtPointer)+i*sizeof(void *), sJitTemp0.as(jtPointer));
               #ifdef HXCPP_GC_GENERATIONAL
               compiler->move(sJitTemp2, array);
               genWriteBarrier(compiler, sJitTemp2, sJitTemp0 );
               #endif
               break;
            case arrBool:
            case arrUnsignedChar:
               items[i]->genCode(compiler, sJitTemp0, etInt );
               compiler->move(sJitTemp1, arrayPtr);
               compiler->move(sJitTemp1.star(jtByte)+i, sJitTemp0);
               break;
            case arrFloat:
               items[i]->genCode(compiler, sJitTempF0, etFloat );
               compiler->move(sJitTemp1, arrayPtr);
               compiler->move(sJitTemp1.star(jtFloat)+i*sizeof(double), sJitTempF0);
               break;
            case arrFloat32:
               items[i]->genCode(compiler, sJitTempF0, etFloat );
               compiler->move(sJitTemp1, arrayPtr);
               compiler->move(sJitTemp1.star(jtFloat32)+i*sizeof(float), sJitTempF0);
               break;

            case arrString:
               {
               JitTemp val(compiler,etString);
               items[i]->genCode(compiler, val, etString );
               compiler->move(sJitTemp1, arrayPtr);
               compiler->move(sJitTemp1.star(jtInt)+i*sizeof(String), val.as(jtInt));
               compiler->move(sJitTemp1.star(jtPointer)+i*sizeof(String)+StringOffset::Ptr, val.as(jtPointer) + StringOffset::Ptr );
               #ifdef HXCPP_GC_GENERATIONAL
               compiler->move(sJitTemp0, array);
               genWriteBarrier(compiler, sJitTemp0, val.as(jtPointer) + StringOffset::Ptr);
               #endif
               }
               break;
            case arrAny:
               {
                  switch(items[i]->getType())
                  {
                     case etInt:
                        items[i]->genCode(compiler, sJitArg2, etInt );
                        if (items[i]->isBoolInt())
                           compiler->callNative((void *)varraySetBool,array,i,sJitArg2.as(jtInt));
                        else
                           compiler->callNative((void *)varraySetInt,array,i,sJitArg2.as(jtInt));
                        break;
                     case etFloat:
                        {
                        JitTemp fval(compiler,etFloat);
                        items[i]->genCode(compiler, fval, etFloat );
                        compiler->callNative((void *)varraySetFloat,array,i,fval);
                        }
                        break;
                     case etString:
                        {
                        JitTemp sval(compiler,etString);
                        items[i]->genCode(compiler, sval, etString );
                        compiler->callNative((void *)varraySetString,array,i,sval);
                        }
                        break;

                     default:
                        items[i]->genCode(compiler, sJitArg2, etObject );
                        compiler->callNative((void *)varraySetObject,array,i,sJitArg2.as(jtPointer));
                        break;
                  }
               }
               break;

            default:
               CPPIALOG("todo other array -init\n");
         }
      }

      compiler->convert(array,etObject, inDest, destType);
   }
   #endif
};


#ifdef CPPIA_JIT
   hx::Object * SLJIT_CALL getItem(hx::Object *inObj, int inIndex)
   {
      return (inObj->__GetItem(inIndex)).mPtr;
   }

   void SLJIT_CALL incItem(hx::Object *inObj, int inIndex)
   {
      inObj->__SetItem( inIndex, inObj->__GetItem(inIndex) + 1 );
   }

   void SLJIT_CALL decItem(hx::Object *inObj, int inIndex)
   {
      inObj->__SetItem( inIndex, inObj->__GetItem(inIndex) - 1 );
   }

   hx::Object * SLJIT_CALL preIncItem(hx::Object *inObj, int inIndex)
   {
      Dynamic val = inObj->__GetItem(inIndex) +1;
      inObj->__SetItem( inIndex, val );
      return val.mPtr;
   }

   hx::Object * SLJIT_CALL postIncItem(hx::Object *inObj, int inIndex)
   {
      Dynamic val = inObj->__GetItem(inIndex);
      inObj->__SetItem( inIndex, val + 1 );
      return val.mPtr;
   }

   hx::Object * SLJIT_CALL setDynamicI(hx::Object *inObj, int inIndex, hx::Object *inValue)
   {
      inObj->__SetItem(  inIndex, inValue );
      return inValue;
   }



   hx::Object * SLJIT_CALL preDecItem(hx::Object *inObj, int inIndex)
   {
      Dynamic val = inObj->__GetItem(inIndex) - 1;
      inObj->__SetItem( inIndex, val );
      return val.mPtr;
   }

   hx::Object * SLJIT_CALL postDecItem(hx::Object *inObj, int inIndex)
   {
      Dynamic val = inObj->__GetItem(inIndex);
      inObj->__SetItem( inIndex, val -  1 );
      return val.mPtr;
   }

   struct DynIndexArgs
   {
      JitMultiArg index;
      JitMultiArg lvalue;
      JitMultiArg rvalue;
   };



   #define DynIndexFunc(name, OP) \
      hx::Object * SLJIT_CALL name(hx::Object *inObj, DynIndexArgs *args) \
      { \
         Dynamic result = Dynamic(args->lvalue.obj) OP ; \
         inObj->__SetItem( args->index.ival, result); \
         return result.mPtr; \
      }

   #define DynIndexIntFunc(name, OP) \
      hx::Object * SLJIT_CALL name(hx::Object *inObj, DynIndexArgs *args) \
      { \
         Dynamic result = ((int)Dynamic(args->lvalue.obj)) OP ; \
         inObj->__SetItem( args->index.ival, result); \
         return result.mPtr; \
      }


   DynIndexFunc(dynAdd, +Dynamic(args->rvalue.obj) );
   DynIndexFunc(dynMult, * args->rvalue.dval );
   DynIndexFunc(dynDiv, / args->rvalue.dval );
   DynIndexFunc(dynSub, - args->rvalue.dval );

   hx::Object * SLJIT_CALL dynMod(hx::Object *inObj, DynIndexArgs *args)
   {
      Dynamic result = hx::Mod( Dynamic(args->lvalue.obj), args->rvalue.dval );
      inObj->__SetItem( args->index.ival, result);
      return result.mPtr;
   }

   DynIndexIntFunc(dynAnd, & args->rvalue.ival );
   DynIndexIntFunc(dynOr, | args->rvalue.ival );
   DynIndexIntFunc(dynXOr, ^ args->rvalue.ival );
   DynIndexIntFunc(dynShl, << args->rvalue.ival );
   DynIndexIntFunc(dynShr, >> args->rvalue.ival );

   hx::Object * SLJIT_CALL dynUShr(hx::Object *inObj, DynIndexArgs *args)
   {
      Dynamic result = hx::UShr( Dynamic(args->lvalue.obj), args->rvalue.ival );
      inObj->__SetItem( args->index.ival, result);
      return result.mPtr;
   }


#endif



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
   const char *getName() { return "DynamicArrayI"; }
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
   #ifdef CPPIA_JIT

   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JitTemp obj(compiler,jtPointer);
      object->genCode(compiler, obj, etObject);

      if (crement==coNone && assign==aoNone)
      {
         index->genCode(compiler, sJitTemp1, etInt);
         compiler->callNative( (void *)getItem, obj, sJitTemp1.as(jtInt) );
         compiler->convertReturnReg(etObject, inDest, destType);
         return;
      }
      if (crement!=coNone)
      {
         index->genCode(compiler, sJitTemp1, etInt);
         if (destType==etVoid || destType==etNull)
         {
            compiler->callNative( crement==coPostInc || crement==coPostInc ? (void *)incItem : (void *)decItem,
               obj, sJitTemp1.as(jtInt) );
         }
         else
         {
            void *func = 0;
            switch(crement)
            {
               case coPostInc: func=(void *)postIncItem; break;
               case coPreInc: func=(void *)preIncItem; break;
               case coPostDec: func=(void *)postDecItem; break;
               case coPreDec: func=(void *)preDecItem; break;
               default: ;
            }
            if (!func)
               throw "Bad crement in DynamicArrayI";

            compiler->callNative( func, obj, sJitTemp1.as(jtInt) );
            compiler->convertReturnReg(etObject, inDest, destType );
         }
         return;
      }

      if (assign==aoSet)
      {
         JitTemp idx(compiler,jtInt);
         index->genCode(compiler, idx, etInt);

         value->genCode(compiler, sJitTemp2, etObject);
         compiler->callNative( (void *)setDynamicI, obj, idx, sJitTemp2);
         compiler->convertReturnReg(etObject, inDest, destType );
         return;
      }


      JitMultiArgs argsBuf(compiler, 3);

      index->genCode(compiler, sJitTemp1, etInt);

      compiler->move(argsBuf.arg(0,etInt), sJitTemp1);

      compiler->callNative( (void *)getItem, obj, sJitTemp1.as(jtInt) );

      compiler->move( argsBuf.arg(1,jtPointer), sJitReturnReg.as(jtPointer));

      ExprType type = etInt;
      void *func = 0;

      switch(assign)
      {
         case aoAdd: func = (void *)dynAdd; type = etObject; break;

         case aoMult: func = (void *)dynMult; type = etFloat; break;
         case aoDiv: func = (void *)dynDiv; type = etFloat; break;
         case aoSub: func = (void *)dynSub; type = etFloat; break;
         case aoMod: func = (void *)dynMod; type = etFloat; break;

         case aoAnd: func = (void *)dynAnd; break;
         case aoOr: func = (void *)dynOr; break;
         case aoXOr: func = (void *)dynXOr; break;
         case aoShl: func = (void *)dynShl; break;
         case aoShr: func = (void *)dynShr; break;
         case aoUShr: func = (void *)dynUShr; break;
         default: ;
      }

      if (type==etFloat)
         value->genCode(compiler, argsBuf.arg(2,jtFloat), etFloat);
      else if (type==etInt)
         value->genCode(compiler, argsBuf.arg(2,jtInt), etInt);
      else
         value->genCode(compiler, argsBuf.arg(2,jtPointer), etObject);

      if (!func)
         throw "Bad set in DynamicArrayI";
      compiler->callNative( func, obj, argsBuf );

      compiler->convertReturnReg(etObject, inDest, destType);
   }
   #endif
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

   const char *getName() { return "ArrayAccessI"; }
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
         CPPIALOG("Class %s missing __get\n", type->name.out_str());
         throw "Bad array access - __get";
      }
      __set = type->haxeBase->findFunction("__set");
      if (!__set.execute)
      {
         CPPIALOG("Class %s missing __set\n", type->name.out_str());
         throw "Bad array access - __set";
      }

      if (__get.signature[1]!='i' || __set.signature[1]!='i')
         throw "__get/__set should take an int";

      if (__get.signature[0]!=__set.signature[2])
         throw "Mismatch __get/__set array access";

      switch(__get.signature[0])
      {
         case sigBool:
         case sigInt:
              accessType = etInt; break;
         case sigFloat: accessType = etFloat; break;
         case sigString: accessType = etString; break;
         case sigObject: accessType = etObject; break;
         default: throw "Bad signature on __get";
      }

      if (value)
         value = value->link(inModule);

      DBGLOG("Created ARRAYI wrapper %s %d %s / %s\n", type->name.out_str(), accessType, __get.signature, __set.signature);

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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      object->genCode(compiler, sJitTemp0, etObject);
      int offset = sizeof( EnumBase_obj ) + index*sizeof(cpp::Variant);
      compiler->genVariantValueTemp0(offset, inDest, destType);
   }
   #endif
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



   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JitFramePos pos(var.stackPos,var.expressionType);
      if (init)
      {
         switch(var.expressionType)
         {
            case etInt: case etFloat: case etString: case etObject:
               init->genCode(compiler, pos, var.expressionType );
               break;
            default: ;
         }
      }
      else
      {
         switch(var.expressionType)
         {
            case etInt:
               compiler->move(pos.as(jtInt), (int)0);
               break;
            case etFloat:
               if (sizeof(void *)==8)
               {
                  compiler->move(pos.as(jtPointer), (void *)0);
               }
               else
               {
                  compiler->move(pos.as(jtInt), (int)0);
                  compiler->move(pos.as(jtInt)+StringOffset::Ptr, (int)0);
               }
               break;
            case etString:
               compiler->move(pos.as(jtInt), (int)0);
               compiler->move(pos.as(jtPointer)+StringOffset::Ptr, (void *)0);
               break;
            case etObject:
               compiler->move(pos.as(jtPointer), (void *)0);
               break;

            default: ;
         }
      }
   }
   #endif


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
   const char *getName() { return "TVars"; }
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      for(int v=0;v<vars.size();v++)
         vars[v]->genCode(compiler, JitVal(), etVoid );
   }
   #endif
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
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      LabelId oldCont = compiler->setContinuePos( compiler->addLabel() );
      QuickVec<JumpId> oldBreaks;
      compiler->swapBreakList(oldBreaks);

      JumpId skipCondition;
      if (!isWhileDo)
         skipCondition = compiler->jump();

      JumpId start = condition->genCompare(compiler,true);

      if (!isWhileDo)
         compiler->comeFrom(skipCondition);

      LabelId body = compiler->addLabel();

      loop->genCode(compiler, JitVal(), etVoid);

      condition->genCompare(compiler,false,body);

      compiler->comeFrom(start);
      compiler->setBreakTarget();

      compiler->setContinuePos(oldCont);
      compiler->swapBreakList(oldBreaks);
   }
   #endif
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

   #ifdef CPPIA_JIT

   static int SLJIT_CALL sameObject(hx::Object *o0, hx::Object *o1)
   {
      return Dynamic(o0) == Dynamic(o1);
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      ExprType switchType = condition->getType();
      JitTemp test( compiler, switchType );

      condition->genCode(compiler, test, switchType);

      std::vector<JumpId> doneJumps;

      for(int i=0;i<caseCount;i++)
      {
         JumpId nextCase = 0;
         Case &c = cases[i];
         std::vector<JumpId> onMatch;
         for(int j=0;j<c.conditions.size();j++)
         {
            bool last = j==c.conditions.size()-1;
            if (switchType==etInt)
            {
               c.conditions[j]->genCode(compiler, sJitTemp1, switchType);
               if (last)
                  nextCase = compiler->compare(cmpI_NOT_EQUAL,test, sJitTemp1.as(jtInt) );
               else
                  onMatch.push_back(compiler->compare(cmpI_EQUAL,test, sJitTemp1.as(jtInt)));
            }
            else if (switchType==etFloat)
            {
               c.conditions[j]->genCode(compiler, sJitTemp1, switchType);
               if (last)
                  nextCase = compiler->fcompare(cmpD_NOT_EQUAL,test, sJitTemp1.as(jtFloat),0,false );
               else
                  onMatch.push_back(compiler->fcompare(cmpD_EQUAL,test, sJitTemp1.as(jtFloat),0,false));
            }
            else if (switchType==etString)
            {
               JitTemp cond(compiler, etString);
               c.conditions[j]->genCode(compiler, cond, etString);
               if (last)
                  nextCase = compiler->scompare(cmpP_NOT_EQUAL,test, cond);
               else
                  onMatch.push_back(compiler->scompare(cmpP_EQUAL,test, cond));
            }
            else if (switchType==etObject)
            {
               c.conditions[j]->genCode(compiler, sJitTemp1, switchType);
               compiler->callNative( (void *)sameObject, test, sJitArg1.as(jtPointer) );

               if (last)
                  nextCase = compiler->compare(cmpI_EQUAL, sJitReturnReg.as(jtInt), (int) 0);
               else
                  onMatch.push_back(compiler->compare(cmpI_NOT_EQUAL, sJitReturnReg.as(jtInt), (int) 0) );
            }
            else
            {
               CPPIALOG("Missing switch type\n");
               CppiaTrap();
            }
         }

         for(int m=0;m<onMatch.size();m++)
            compiler->comeFrom(onMatch[m]);

         c.body->genCode(compiler, inDest, destType);

         if (i+1<caseCount || defaultCase)
            doneJumps.push_back( compiler->jump() );

         if (nextCase)
            compiler->comeFrom(nextCase);
      }

      if (defaultCase)
         defaultCase->genCode(compiler, inDest, destType);

      for(int j=0;j<doneJumps.size();j++)
         compiler->comeFrom(doneJumps[j]);
   }
   #endif

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

   #ifdef CPPIA_JIT
   static int SLJIT_CALL isExceptionCatch(hx::Object *exception, TypeData *inType)
   {
      // TODO - HX_STACK_BEGIN_CATCH
      return inType->isClassOf(exception);
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      ThrowList thisThrown;
      ThrowList *oldList = compiler->pushCatching(&thisThrown);
      body->genCode(compiler, inDest, destType);

      compiler->popCatching(oldList);

      if (thisThrown.size()>0)
      {
         std::vector<JumpId> handledExceptions;

         JumpId noThrow = compiler->jump();

         for(int i=0;i<thisThrown.size();i++)
            compiler->comeFrom(thisThrown[i]);

         for(int i=0;i<catches.size();i++)
         {
            Catch &c = catches[i];
            compiler->callNative( (void *)isExceptionCatch, sJitCtx.star(jtPointer,offsetof(hx::StackContext,exception)), c.type );
            JumpId notThisOne = compiler->compare(cmpI_EQUAL, sJitReturnReg, (int)0);

            // Exception is this type
            ExprType type = c.var.expressionType;
            compiler->convert( sJitCtx.star(jtPointer,offsetof(hx::StackContext,exception)), etObject,
                              JitFramePos(c.var.stackPos,getJitType(type)), type );
            compiler->move(sJitCtx.star(jtPointer,offsetof(hx::StackContext,exception)),(void *)0);

            c.body->genCode(compiler, inDest, destType);

            handledExceptions.push_back( compiler->jump() );
            compiler->comeFrom(notThisOne);
         }

         // Unhandled - go to next handler or return
         //if (compiler->hasCatching())
            compiler->addThrow();
         // Optimization
         //else compiler->addReturn();

         // handled/not thrown
         for(int i=0;i<handledExceptions.size();i++)
            compiler->comeFrom( handledExceptions[i] );
         compiler->comeFrom(noThrow);
      }
   }
   #endif
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

         CPPIALOG("Could not link var %d %s.\n", varId, inModule.strings[ getStackVarNameId(varId) ].out_str() );
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

      CPPIALOG("Unknown var ref!\n");
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (flag==bcrBreak)
         compiler->addBreak();
      else
         compiler->addContinue();
   }
   #endif
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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (value)
      {
         if (returnType==etVoid)
            value->genCode(compiler, JitVal(), etVoid);
         else
            value->genCode(compiler, JitFramePos(0,returnType), returnType);
      }
      compiler->addReturn();
   }
   #endif
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

   const char *getName() =0;
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


struct OpMult : public BinOp
{
   OpMult(CppiaStream &stream) : BinOp(stream) { }

   const char *getName() { return "OpMult"; }
   int runInt(CppiaCtx *ctx)
   {
      int lval = left->runInt(ctx);
      BCR_CHECK;
      return lval * right->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return lval * right->runFloat(ctx);
   }
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType==etVoid)
      {
         left->genCode(compiler,JitVal(),etVoid);
         right->genCode(compiler,JitVal(),etVoid);
      }
      else if (destType==etInt || (left->getType()==etInt && right->getType()==etInt) )
      {
         JitTemp lval(compiler,jtInt);
         left->genCode(compiler,lval,etInt);
         right->genCode(compiler,sJitTemp0.as(jtInt),etInt);
         if (destType==etInt)
            compiler->mult(inDest.as(jtInt),lval,sJitTemp0.as(jtInt),false);
         else
         {
            compiler->mult(sJitTemp1.as(jtInt),lval,sJitTemp0.as(jtInt),false);
            compiler->convert(sJitTemp1.as(jtInt),etInt, inDest, destType);
         }
      }
      else
      {
         JitTemp lval(compiler,jtFloat);
         left->genCode(compiler,lval,etFloat);
         right->genCode(compiler,sJitTempF0,etFloat);
         if (destType==etFloat && !isMemoryVal(inDest) )
         {
            compiler->mult(inDest.as(jtFloat),lval,sJitTempF0,true);
         }
         else
         {
            compiler->mult(sJitTempF1,lval,sJitTempF0,true);
            compiler->convert(sJitTempF1,etFloat, inDest, destType);
         }
      }
   }
   #endif
};


struct OpSub : public BinOp
{
   OpSub(CppiaStream &stream) : BinOp(stream) { }
   const char *getName() { return "OpSub"; }

   int runInt(CppiaCtx *ctx)
   {
      int lval = left->runInt(ctx);
      BCR_CHECK;
      return lval - right->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return lval - right->runFloat(ctx);
   }
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType==etVoid)
      {
         left->genCode(compiler,JitVal(),etVoid);
         right->genCode(compiler,JitVal(),etVoid);
      }
      else if (destType==etInt || (left->getType()==etInt && right->getType()==etInt) )
      {
         JitTemp lval(compiler,jtInt);
         left->genCode(compiler,lval,etInt);
         right->genCode(compiler,sJitTemp0.as(jtInt),etInt);
         if (destType==etInt)
         {
            compiler->sub(inDest.as(jtInt),lval,sJitTemp0,false);
         }
         else
         {
            compiler->sub(sJitTemp1.as(jtInt),lval,sJitTemp0,false);
            compiler->convert(sJitTemp1,etInt, inDest, destType);
         }
      }
      else
      {
         JitTemp lval(compiler,jtFloat);
         left->genCode(compiler,lval,etFloat);
         right->genCode(compiler,sJitTempF0,etFloat);
         if (destType==etFloat)
            compiler->sub(inDest.as(jtFloat),lval,sJitTempF0,true);
         else
         {
            compiler->sub(sJitTempF1,lval,sJitTempF0,true);
            compiler->convert(sJitTempF1,etFloat, inDest, destType);
         }
      }
   }
   #endif

};

struct OpDiv : public BinOp
{
   OpDiv(CppiaStream &stream) : BinOp(stream) { }

   const char *getName() { return "OpDiv"; }
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
   CppiaExpr *link(CppiaModule &inModule)
   {
      BinOp::link(inModule);
      type = etFloat;
      return this;
   }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType==etVoid)
      {
         left->genCode(compiler,JitVal(),etVoid);
         right->genCode(compiler,JitVal(),etVoid);
      }
      else
      {
         JitTemp lval(compiler,jtFloat);
         left->genCode(compiler,lval,etFloat);
         right->genCode(compiler,sJitTempF0,etFloat);
         if (destType==etFloat)
         {
            compiler->fdiv(inDest,lval,sJitTempF0);
         }
         else
         {
            compiler->fdiv(sJitTempF0,lval,sJitTempF0);
            compiler->convert(sJitTempF0, etFloat, inDest, destType);
         }
      }
   }
   #endif

};



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
      // HX_STACK_DO_THROW ?
      throw Dynamic( value->runObject(ctx) );
   }
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      value->genCode(compiler, sJitCtx.star(jtPointer, offsetof(hx::StackContext,exception)), etObject);
      compiler->addThrow();
   }
   #endif
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

   #ifdef CPPIA_JIT
   JumpId genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
   {
      return value->genCompare(compiler, !inReverse, inLabel);
   }
   #endif

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


   #ifdef CPPIA_JIT
   JumpId genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
   {
      if (inReverse)
      {
         // !left || !right
         if (inLabel)
         {
            left->genCompare(compiler, true, inLabel);
            right->genCompare(compiler, true, inLabel);
         }
         else
         {
            // !left || !right
            JumpId someBad = left->genCompare(compiler, true, 0);
            // Left is false, goto someBad for a jump

            // If right is also not good, skip the unconditional jump
            JumpId noneBad = right->genCompare(compiler, false, 0);

            compiler->comeFrom(someBad);
            JumpId result = compiler->jump(inLabel);

            compiler->comeFrom(noneBad);
            return result;
         }
      }
      else
      {
         JumpId leftFalse = left->genCompare(compiler, true, 0);

         JumpId result = right->genCompare(compiler, false, inLabel);

         compiler->comeFrom(leftFalse);

         return result;
      }
      return 0;
   }
   #endif

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


   #ifdef CPPIA_JIT
   JumpId genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
   {
      if (inReverse)
      {
         // !left && !right
         //  don't jump if left true..
         JumpId leftTrue = left->genCompare(compiler, false, 0);

         JumpId result = right->genCompare(compiler, true, inLabel);

         compiler->comeFrom(leftTrue);

         return result;

      }
      else if (inLabel)
      {
         left->genCompare(compiler, false, inLabel);
         right->genCompare(compiler, false, inLabel);
      }
      else
      {
         // left || right
         JumpId someGood = left->genCompare(compiler, false, 0);
         // Left is true, goto someGood for a jump

         // If right is also not good, skip the unconditional jump
         JumpId noneGood = right->genCompare(compiler, true, 0);

         compiler->comeFrom(someGood);
         JumpId result = compiler->jump(inLabel);

         compiler->comeFrom(noneGood);
         return result;
      }
      return 0;

   }
   #endif



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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      left->genCode(compiler, sJitTemp0, etInt);
      if (destType==etInt)
         compiler->bitNot(inDest.as(jtInt), sJitTemp0 );
      else
      {
         compiler->bitNot( sJitTemp0.as(jtInt), sJitTemp0.as(jtInt) );
         compiler->convert(sJitTemp0, etInt, inDest, destType);
      }
   }
   #endif
};

struct BitOpBase : public CppiaIntExpr
{
   CppiaExpr *left;
   CppiaExpr *right;
   BitOpBase(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
      right = createCppiaExpr(stream);
   }
   const char *getName() = 0;
   CppiaExpr *link(CppiaModule &inModule)
   {
      left = left->link(inModule);
      right = right->link(inModule);
      return this;
   }


   #ifdef CPPIA_JIT
   virtual BitOp getBitOp() = 0;

   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JitTemp tLeft(compiler, jtInt);
      left->genCode(compiler, tLeft, etInt);
      right->genCode(compiler, sJitTemp2, etInt);
      if (destType==etInt)
         compiler->bitOp( getBitOp(), inDest, tLeft, sJitTemp2 );
      else
      {
         compiler->bitOp( getBitOp(), sJitTemp2.as(jtInt), tLeft, sJitTemp2 );
         compiler->convert(sJitTemp2, etInt, inDest, destType);
      }
   }
   #endif
};

struct BitAnd : public BitOpBase
{
   BitAnd(CppiaStream &stream) : BitOpBase(stream) { }
   const char *getName() { return "BitAnd"; }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return l & right->runInt(ctx);
   }

   #ifdef CPPIA_JIT
   BitOp getBitOp() { return bitOpAnd; }
   #endif
};

struct BitOr : public BitOpBase
{
   BitOr(CppiaStream &stream) : BitOpBase(stream) { }
   const char *getName() { return "BitOr"; }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return l | right->runInt(ctx);
   }
   #ifdef CPPIA_JIT
   BitOp getBitOp() { return bitOpOr; }
   #endif
};


struct BitXOr : public BitOpBase
{
   BitXOr(CppiaStream &stream) : BitOpBase(stream) { }
   const char *getName() { return "BitXOr"; }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return l ^ right->runInt(ctx);
   }
   #ifdef CPPIA_JIT
   BitOp getBitOp() { return bitOpXOr; }
   #endif
};


struct BitUSR : public BitOpBase
{
   BitUSR(CppiaStream &stream) : BitOpBase(stream) { }
   const char *getName() { return "BitUSR"; }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return  hx::UShr(l , right->runInt(ctx));
   }
   #ifdef CPPIA_JIT
   BitOp getBitOp() { return bitOpUSR; }
   #endif
};


struct BitShiftR : public BitOpBase
{
   BitShiftR(CppiaStream &stream) : BitOpBase(stream) { }
   const char *getName() { return "BitShiftR"; }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return  l >> right->runInt(ctx);
   }
   #ifdef CPPIA_JIT
   BitOp getBitOp() { return bitOpShiftR; }
   #endif
};


struct BitShiftL : public BitOpBase
{
   BitShiftL(CppiaStream &stream) : BitOpBase(stream) { }
   const char *getName() { return "BitShiftL"; }
   int runInt(CppiaCtx *ctx)
   {
      int l = left->runInt(ctx);
      BCR_CHECK;
      return  l << right->runInt(ctx);
   }
   #ifdef CPPIA_JIT
   BitOp getBitOp() { return bitOpShiftL; }
   #endif
};




#ifdef CPPIA_JIT

void SLJIT_CALL dynamicAddStr(hx::Object *inObj1, hx::Object *inObj2, String *outString) {
   TRY_NATIVE
   *outString = Dynamic(inObj1) + Dynamic(inObj2);
   CATCH_NATIVE
}
void *SLJIT_CALL dynamicAddObj(hx::Object *inObj1, hx::Object *inObj2) {
   TRY_NATIVE
   return (Dynamic(inObj1) + Dynamic(inObj2)).mPtr;
   CATCH_NATIVE
   return 0;
}
int SLJIT_CALL dynamicAddInt(hx::Object *inObj1, hx::Object *inObj2) {
   TRY_NATIVE
   return Dynamic(inObj1) + Dynamic(inObj2);
   CATCH_NATIVE
   return 0;
}
void SLJIT_CALL dynamicAddFloat(hx::Object *inObj1, hx::Object *inObj2, double *outResult) {
   TRY_NATIVE
   *outResult = (Dynamic(inObj1) + Dynamic(inObj2));
   CATCH_NATIVE
}

void SLJIT_CALL strAddStrToStrOver(String *ioStr, String *inStr1) {
   *ioStr = *ioStr + *inStr1;
}
void SLJIT_CALL strAddStrToStr(String *inStr0, String *inStr1, String *outStr) {
   *outStr = *inStr0 + *inStr1;
}
hx::Object *SLJIT_CALL strAddStrToObj(String *inStr0, String *inStr1) {
   TRY_NATIVE
   hx::Object *result = Dynamic(*inStr0 + *inStr1).mPtr;
   return result;
   CATCH_NATIVE
   return 0;
}

#endif


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
   virtual const char *getName() { return "SpecialAdd"; }
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
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (!AS_DYNAMIC)
      {
         // String version
         if (destType==etObject || destType==etString)
         {
            JitTemp s0(compiler,jtString);
            JitTemp s1(compiler,jtString);
            left->genCode(compiler, s0, etString);
            right->genCode(compiler, s1, etString);
            compiler->add( sJitTemp0, s0.getReg().as(jtPointer), s0.offset );
            compiler->add( sJitTemp1, s1.getReg().as(jtPointer), s1.offset );
            if (destType==etString)
            {
               compiler->callNative((void *)strAddStrToStrOver, sJitTemp0.as(jtPointer), sJitTemp1.as(jtPointer));
               compiler->move(inDest.as(jtInt), s0.as(jtInt));
               compiler->move(inDest.as(jtPointer)+StringOffset::Ptr, s0.as(jtPointer)+StringOffset::Ptr);
            }
            else // Object
            {
               compiler->callNative((void *)strAddStrToObj, sJitTemp0.as(jtPointer), sJitTemp1.as(jtPointer));
               compiler->checkException();
               compiler->move( inDest, sJitReturnReg.as(jtPointer) );
            }
         }
         else
         {
            left->genCode(compiler, JitVal(), etObject);
            right->genCode(compiler, JitVal(), etObject);
         }
      }
      else
      {
         JitTemp tLeft(compiler,jtPointer);
         left->genCode(compiler, tLeft, etObject);
         right->genCode(compiler, sJitTemp1.as(jtPointer), etObject);

         switch(destType)
         {
            case etString:
               if (inDest.offset==0)
               {
                  compiler->callNative((void *)dynamicAddStr, tLeft, sJitTemp1.as(jtPointer), inDest.getReg());
                  compiler->checkException();
               }
               else
               {
                  compiler->add(sJitTemp2, inDest.getReg(), inDest.offset);
                  compiler->callNative((void *)dynamicAddStr, tLeft, sJitTemp1.as(jtPointer), sJitTemp2);
                  compiler->checkException();
               }
               break;

            case etObject:
               compiler->callNative((void *)dynamicAddObj, tLeft, sJitTemp1.as(jtPointer));
               compiler->checkException();
               compiler->move(inDest, sJitReturnReg.as(jtPointer));
               break;

            case etInt:
               compiler->callNative((void *)dynamicAddInt, tLeft, sJitTemp1.as(jtPointer));
               compiler->checkException();
               compiler->move(inDest, sJitReturnReg.as(jtInt));
               break;

            case etFloat:
               if (isMemoryVal(inDest))
               {
                  compiler->callNative((void *)dynamicAddFloat, tLeft, sJitTemp1.as(jtPointer),inDest.as(jtFloat));
                  compiler->checkException();
               }
               else
               {
                  JitTemp result(compiler,jtFloat);
                  compiler->callNative((void *)dynamicAddFloat, tLeft, sJitTemp1.as(jtPointer), result);
                  compiler->checkException();
                  compiler->move(inDest,result);
               }
               break;


            default:
               // Nothing to do
               ;
         }
      }
   }
   #endif
};

struct OpNeg : public CppiaExpr
{
   CppiaExpr *value;
   ExprType type;
   OpNeg(CppiaStream &stream)
   {
      value = createCppiaExpr(stream);
   }
   virtual const char *getName() { return "OpNeg"; }

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

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType==etInt)
      {
         value->genCode(compiler, sJitTemp0.as(jtInt), etInt);
         compiler->negate(inDest, sJitTemp0.as(jtInt) );
      }
      else
      {
         value->genCode(compiler, sJitTempF0, etFloat);
         if (destType==etFloat)
         {
            compiler->negate(inDest.as(jtFloat), sJitTempF0 );
         }
         else
         {
            compiler->negate(sJitTempF0, sJitTempF0);
            compiler->convert(sJitTempF0, etFloat, inDest, destType);
         }
      }
   }
   #endif
};

struct OpAdd : public BinOp
{
   OpAdd(CppiaStream &stream) : BinOp(stream)
   {
   }

   const char *getName() { return "Add"; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      BinOp::link(inModule);

      if (left->getType()==etString || right->getType()==etString )
      {
         CppiaExpr *result = new SpecialAdd<false>(this,left,right);
         delete this;
         return result;
      }
      else if ( !isNumeric(left->getType()) || left->isBoolInt() ||
                !isNumeric(right->getType()) || right->isBoolInt() )
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
   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType==etVoid)
      {
         left->genCode(compiler,JitVal(),etVoid);
         right->genCode(compiler,JitVal(),etVoid);
      }
      else if (destType==etInt || (left->getType()==etInt && right->getType()==etInt && destType==etObject) )
      {
         JitTemp temp(compiler,jtInt);
         left->genCode(compiler,temp,etInt);
         right->genCode(compiler,sJitTemp0,etInt);
         if (destType==etObject)
         {
            compiler->add( sJitTemp0.as(jtInt), temp, sJitTemp0.as(jtInt) );
            compiler->convert(sJitTemp0, etInt, inDest, destType);
         }
         else
         {
            compiler->add( inDest, temp, sJitTemp0.as(jtInt) );
         }
      }
      else
      {
         JitTemp temp(compiler,jtFloat);
         left->genCode(compiler,temp,etFloat);
         right->genCode(compiler,sJitTempF0,etFloat);
         if (destType==etObject || destType==etString)
         {
            compiler->add( sJitTempF0, temp, sJitTempF0 );
            compiler->convert(sJitTempF0, etFloat, inDest, destType);
         }
         else
         {
            compiler->add( inDest, temp, sJitTempF0 );
         }
      }
   }
   #endif
};

#ifdef CPPIA_JIT
static void SLJIT_CALL double_mod(double *params)
{
   params[0] = hx::DoubleMod(params[0], params[1]);
}
#endif

struct OpMod : public BinOp
{
   OpMod(CppiaStream &stream) : BinOp(stream)
   {
   }
   const char *getName() { return "OpMod"; }

   // Need this for /0 behaviour
   ExprType getType() { return etFloat; }

   CppiaExpr *link(CppiaModule &inModule)
   {
      BinOp::link(inModule);
      type = etFloat;
      return this;
   }

   int runInt(CppiaCtx *ctx)
   {
      double lval = left->runFloat(ctx);
      BCR_CHECK;
      return hx::DoubleMod(lval,right->runFloat(ctx));
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      BCR_CHECK;
      return hx::DoubleMod(lval,right->runFloat(ctx));
   }

   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      if (destType==etVoid)
      {
         left->genCode(compiler,JitVal(),etVoid);
         right->genCode(compiler,JitVal(),etVoid);
      }
      else
      {
         JitTemp leftRightVal(compiler,etFloat, sizeof(Float)*2 );
         left->genCode(compiler, leftRightVal, etFloat);
         right->genCode(compiler, leftRightVal + sizeof(Float), etFloat);
         compiler->add(sJitArg0, leftRightVal.getReg(), leftRightVal.offset );
         compiler->callNative((void *)double_mod, sJitArg0 );
         compiler->convert(leftRightVal,etFloat,inDest,destType);
      }
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
         CPPIALOG("Could not create increment operator\n");
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
   #ifdef CPPIA_JIT

   static int SLJIT_CALL stringCompare(String *s0, String *s1)
   {
      return s0->compare(*s1);
   }

   static int SLJIT_CALL dynamicCompare(hx::Object *o0, hx::Object *o1)
   {
      COMPARE compare;

      return compare.test( Dynamic(o0), Dynamic(o1) );
   }


   JumpId genCompare(CppiaCompiler *compiler,bool inReverse,LabelId inLabel)
   {
      switch(compareType)
      {
         case compInt:
         {
            JitTemp lhs(compiler,jtInt);
            left->genCode(compiler, lhs, etInt);
            right->genCode(compiler, sJitTemp0, etInt);
            return compiler->compare( (JitCompare)(inReverse ? COMPARE::reverse :COMPARE::compare),
                                       lhs, sJitTemp0.as(jtInt), inLabel );
         }

         case compFloat:
         {
            JitTemp lhs(compiler,jtFloat);
            left->genCode(compiler, lhs, etFloat);
            right->genCode(compiler, sJitTempF0, etFloat);
            return compiler->fcompare( (JitCompare)COMPARE::fcompare, lhs, sJitTempF0, inLabel, inReverse );
         }

         case compString:
         {
            JitTemp lhs(compiler,jtString);
            JitTemp rhs(compiler,jtString);
            left->genCode(compiler, lhs, etString);
            right->genCode(compiler, rhs, etString);
            compiler->callNative( (void *)stringCompare, lhs, rhs );
            return compiler->compare( (JitCompare)(inReverse ? COMPARE::reverse :COMPARE::compare),
                                       sJitReturnReg, (int) 0, inLabel );
         }
         case compDynamic:
         {
            JitTemp lhs(compiler,jtPointer);
            left->genCode(compiler, lhs, etObject);
            right->genCode(compiler, sJitArg1, etObject);
            compiler->callNative( (void *)dynamicCompare, lhs, sJitArg1.as(jtPointer) );
            return compiler->compare( inReverse ? cmpI_EQUAL : cmpI_NOT_EQUAL, sJitReturnReg.as(jtInt), (int) 0, inLabel );
         }


         default:
            left->genCode(compiler, sJitTemp0, etObject);
            right->genCode(compiler, sJitTemp0, etObject);
      }
      return 0;
   }

   #endif
};


#define DEFINE_COMPARE_OP(name,OP,COMP,REVERSE,FCOMP,FREVERSE) \
struct name \
{ \
   enum { compare = COMP, reverse=REVERSE }; \
   enum { fcompare = FCOMP, freverse=FREVERSE }; \
   template<typename T> \
   inline bool test(const T &left, const T&right) \
   { \
      return OP(left,right); \
   } \
};

#ifdef CPPIA_JIT
DEFINE_COMPARE_OP(CompareLess, hx::IsLess,      cmpI_SIG_LESS,         cmpI_SIG_GREATER_EQUAL, cmpD_LESS,cmpD_GREATER_EQUAL);
DEFINE_COMPARE_OP(CompareLessEq, hx::IsLessEq,   cmpI_SIG_LESS_EQUAL,   cmpI_SIG_GREATER,       cmpD_LESS_EQUAL,cmpD_GREATER);
DEFINE_COMPARE_OP(CompareGreater, hx::IsGreater,   cmpI_SIG_GREATER,      cmpI_SIG_LESS_EQUAL,    cmpD_GREATER, cmpD_LESS_EQUAL);
DEFINE_COMPARE_OP(CompareGreaterEq, hx::IsGreaterEq ,cmpI_SIG_GREATER_EQUAL,cmpI_SIG_LESS,          cmpD_GREATER_EQUAL, cmpD_LESS);
DEFINE_COMPARE_OP(CompareEqual, hx::IsEq,    cmpI_EQUAL,            cmpI_NOT_EQUAL,         cmpD_EQUAL, cmpD_NOT_EQUAL);
DEFINE_COMPARE_OP(CompareNotEqual, hx::IsNotEq, cmpI_NOT_EQUAL,        cmpI_EQUAL,             cmpD_NOT_EQUAL, cmpD_EQUAL);
#else
DEFINE_COMPARE_OP(CompareLess, hx::IsLess ,0,0,0,0);
DEFINE_COMPARE_OP(CompareLessEq, hx::IsLessEq,0,0,0,0);
DEFINE_COMPARE_OP(CompareGreater, hx::IsGreater ,0,0,0,0);
DEFINE_COMPARE_OP(CompareGreaterEq, hx::IsGreaterEq,0,0,0,0);
DEFINE_COMPARE_OP(CompareEqual,hx::IsEq,0,0,0,0);
DEFINE_COMPARE_OP(CompareNotEqual,hx::IsNotEq,0,0,0,0);
#endif



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
      result = new DataVal<Float>(atof( stream.module->strings[stream.getInt()].out_str() ));
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
      result = createEnumField(stream,false);
   else if (tok=="CREATEENUM")
      result = createEnumField(stream,true);
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
   else if (tok=="TCAST")
      result = new CastExpr(stream,castInstance);
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
   isDynamic = name == HX_CSTRING("Dynamic");
   isFloat = name == HX_CSTRING("Float");
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
         DBGLOG("Reference to old script %s - ignoring\n", haxeClass.mPtr->mName.out_str());
         haxeClass.mPtr = 0;
      }
      DBGLOG("Link %s, haxe=%s\n", name.out_str(), haxeClass.mPtr ? haxeClass.mPtr->mName.out_str() : "?" );
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

      DBGLOG(" link type '%s' %s ", name.out_str(), haxeClass.mPtr ? "native" : "script" );
      interfaceBase = HaxeNativeInterface::findInterface(name.utf8_str());
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
            else if (t==HX_CSTRING("float"))
               arrayType = arrFloat32;
            else if (t==HX_CSTRING("String"))
               arrayType = arrString;
            else if (t==HX_CSTRING("unsigned char"))
               arrayType = arrUnsignedChar;
            else if (t==HX_CSTRING("Any"))
               arrayType = arrAny;
            else if (t==HX_CSTRING("Object") || t==HX_CSTRING(".cpp.Int64"))
               arrayType = arrObject;
            else
            {
               throw (HX_CSTRING("Unknown array type:") + t).utf8_str();
            }
         }

         DBGLOG("array type %d\n",arrayType);
      }
      else if (!haxeClass.mPtr && cppiaSuperType)
      {
         haxeBase = cppiaSuperType->haxeBase;
         haxeClass.mPtr = cppiaSuperType->haxeClass.mPtr;
         DBGLOG("extends %s\n", cppiaSuperType->name.out_str());
      }
      else if (!haxeClass.mPtr)
      {
         haxeBase = isInterface ? 0 : HaxeNativeClass::hxObject();

         /*
         if (!isInterface && (*sScriptRegistered)[name.utf8_str()])
         {
            CPPIALOG("Base class %s\n", name.out_str());
            (*sScriptRegistered)[name.utf8_str()]->dump();
            throw "New class, but with existing def";
         }
         */
         DBGLOG(isInterface ? "interface base\n" : "base\n");
      }
      else
      {
         haxeBase = HaxeNativeClass::findClass(name.utf8_str());
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



} // end namespace hx



