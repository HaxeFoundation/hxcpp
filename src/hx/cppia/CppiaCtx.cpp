#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <hx/Thread.h>
#include <stdio.h>

#include "Cppia.h"


namespace hx
{


#ifdef DEBUG_RETURN_TYPE
   #define DEBUG_RETURN_TYPE_CHECK \
       if (gLastRet!=CHECK && CHECK!=etVoid) { printf("BAD RETURN TYPE, got %d, expected %d!\n",gLastRet,CHECK); CPPIA_CHECK(0); }
#else
   #define DEBUG_RETURN_TYPE_CHECK
#endif


#define GET_RETURN_VAL(RET,CHECK) \
   ((ScriptCallable *)vtable)->runFunction(this); \
   breakContReturn = 0; \
   DEBUG_RETURN_TYPE_CHECK \
   RET;
 

void StackContext::runVoid(void *vtable)
{
   if (breakContReturn) return;
   GET_RETURN_VAL(return,etVoid);
   /* Reached end of routine without return */
}

int StackContext::runInt(void *vtable)
{
   if (breakContReturn) return 0;
   GET_RETURN_VAL(return getInt(), etInt );
   //printf("No Int return?\n");
   // Should not really get here...
   return 0;
}
Float StackContext::runFloat(void *vtable)
{
   if (breakContReturn) return 0;
   GET_RETURN_VAL(return getFloat(),etFloat );
   // Should not really get here...
   //printf("No Float return?\n");
   return 0;
}
String StackContext::runString(void *vtable)
{
   if (breakContReturn) return String();
   GET_RETURN_VAL(return getString(),etString );
   // Should not really get here...
   //printf("No String return?\n");
   return null();
}
Dynamic StackContext::runObject(void *vtable)
{
   if (breakContReturn) return null();
   GET_RETURN_VAL(return getObject(),etObject );
   //printf("No Object return?\n");
   return null();
}
hx::Object *StackContext::runObjectPtr(void *vtable)
{
   if (breakContReturn) return 0;
   GET_RETURN_VAL(return getObjectPtr(),etObject );
   //printf("No Object return?\n");
   return 0;
}


int runContextConvertInt(StackContext *ctx, ExprType inType, void *inFunc)
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


Float runContextConvertFloat(StackContext *ctx, ExprType inType, void *inFunc)
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

String runContextConvertString(StackContext *ctx, ExprType inType, void *inFunc)
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

hx::Object *runContextConvertObject(StackContext *ctx, ExprType inType, void *inFunc)
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

