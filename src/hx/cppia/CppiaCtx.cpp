#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <hx/Thread.h>
#include <stdio.h>

#include "Cppia.h"


namespace hx
{

static HxMutex *sCppiaCtxLock = 0;

DECLARE_TLS_DATA(CppiaCtx,tlsCppiaCtx)

CppiaCtx sCppiaCtx;

static std::vector<CppiaCtx *> sAllContexts;

CppiaCtx::CppiaCtx()
{
   stack = new unsigned char[128*1024];
   pointer = &stack[0];
   push((hx::Object *)0);
   frame = pointer;
   exception = 0;
   returnJumpBuf = 0;
   loopJumpBuf = 0;
   breakContReturn = 0;
}

CppiaCtx::~CppiaCtx()
{
   delete [] stack;
}


CppiaCtx *CppiaCtx::getCurrent()
{
   CppiaCtx *result = tlsCppiaCtx;
   if (!result)
   {
      tlsCppiaCtx = result = new CppiaCtx();

      if (!sCppiaCtxLock)
         sCppiaCtxLock = new HxMutex();
      sCppiaCtxLock->Lock();
      sAllContexts.push_back(result);
      sCppiaCtxLock->Unlock();
   }
   return result;
}

void CppiaCtx::mark(hx::MarkContext *__inCtx)
{
   hx::MarkConservative((int *)(stack), (int *)(pointer),__inCtx);
}

void scriptMarkStack(hx::MarkContext *__inCtx)
{
   HxMutex *m = sCppiaCtxLock;
   if (m)
       m->Lock();

   for(int i=0;i<sAllContexts.size();i++)
      sAllContexts[i]->mark(__inCtx);

   if (m)
       m->Unlock();
}


#ifdef DEBUG_RETURN_TYPE
   #define DEBUG_RETURN_TYPE_CHECK \
       if (gLastRet!=CHECK && CHECK!=etVoid) { printf("BAD RETURN TYPE, got %d, expected %d!\n",gLastRet,CHECK); CPPIA_CHECK(0); }
#else
   #define DEBUG_RETURN_TYPE_CHECK
#endif


#ifdef SJLJ_RETURN

struct AutoJmpBuf
{
   CppiaCtx *ctx;
   jmp_buf here;
   jmp_buf *old;
  
   AutoJmpBuf(CppiaCtx *inCtx) : ctx(inCtx)
   {
      old = ctx->returnJumpBuf;
      ctx->returnJumpBuf = &here;
   }
   ~AutoJmpBuf() { ctx->returnJumpBuf = old; }
};

#define GET_RETURN_VAL(RET,CHECK) \
   CPPIA_STACK_FRAME(((CppiaExpr *)vtable)); \
   AutoJmpBuf autoJmpBuf(this); \
   if (setjmp(autoJmpBuf.here)) \
   { \
       DEBUG_RETURN_TYPE_CHECK \
       RET; \
   } \
   ((CppiaExpr *)vtable)->runFunction(this);

#else

#define GET_RETURN_VAL(RET,CHECK) \
   CPPIA_STACK_FRAME(((CppiaExpr *)vtable)); \
   ((CppiaExpr *)vtable)->runFunction(this); \
   breakContReturn = 0; \
   DEBUG_RETURN_TYPE_CHECK \
   RET;

#endif
 

void CppiaCtx::runVoid(void *vtable)
{
   if (breakContReturn) return;
   GET_RETURN_VAL(return,etVoid);
   /* Reached end of routine without return */
}

int CppiaCtx::runInt(void *vtable)
{
   if (breakContReturn) return 0;
   GET_RETURN_VAL(return getInt(), etInt );
   //printf("No Int return?\n");
   // Should not really get here...
   return 0;
}
Float CppiaCtx::runFloat(void *vtable)
{
   if (breakContReturn) return 0;
   GET_RETURN_VAL(return getFloat(),etFloat );
   // Should not really get here...
   //printf("No Float return?\n");
   return 0;
}
String CppiaCtx::runString(void *vtable)
{
   if (breakContReturn) return String();
   GET_RETURN_VAL(return getString(),etString );
   // Should not really get here...
   //printf("No String return?\n");
   return null();
}
Dynamic CppiaCtx::runObject(void *vtable)
{
   if (breakContReturn) return null();
   GET_RETURN_VAL(return getObject(),etObject );
   //printf("No Object return?\n");
   return null();
}
hx::Object *CppiaCtx::runObjectPtr(void *vtable)
{
   if (breakContReturn) return 0;
   GET_RETURN_VAL(return getObjectPtr(),etObject );
   //printf("No Object return?\n");
   return 0;
}


int runContextConvertInt(CppiaCtx *ctx, ExprType inType, void *inFunc)
{
   switch(inType)
   {
      case etInt: return ctx->runInt(inFunc);
      case etFloat: return ctx->runFloat(inFunc);
      case etObject: return ctx->runObject(inFunc);
      default:
          ctx->runVoid(inFunc);
   }
   return 0;
}


Float runContextConvertFloat(CppiaCtx *ctx, ExprType inType, void *inFunc)
{
   switch(inType)
   {
      case etInt: return ctx->runInt(inFunc);
      case etFloat: return ctx->runFloat(inFunc);
      case etObject: return ctx->runObject(inFunc);
      default:
          ctx->runVoid(inFunc);
   }
   return 0;
}

String runContextConvertString(CppiaCtx *ctx, ExprType inType, void *inFunc)
{
   switch(inType)
   {
      case etInt: return String(ctx->runInt(inFunc));
      case etFloat: return String(ctx->runFloat(inFunc));
      case etObject: return ctx->runObject(inFunc);
      case etString: return ctx->runString(inFunc);
      default:
          ctx->runVoid(inFunc);
   }
   return 0;
}

hx::Object *runContextConvertObject(CppiaCtx *ctx, ExprType inType, void *inFunc)
{
   switch(inType)
   {
      case etInt: return Dynamic(ctx->runInt(inFunc)).mPtr;
      case etFloat: return Dynamic(ctx->runFloat(inFunc)).mPtr;
      case etObject: return ctx->runObject(inFunc).mPtr;
      case etString: return Dynamic(ctx->runString(inFunc)).mPtr;
      default:
          ctx->runVoid(inFunc);
   }
   return 0;
}






} // end namespace hx


