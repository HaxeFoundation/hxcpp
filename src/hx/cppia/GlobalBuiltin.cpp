#include <hxcpp.h>
#include "Cppia.h"

namespace hx
{

template<typename ARG0, int (*FUNC)(ARG0 arg0)>
class IntBuiltin1 : public CppiaExpr
{
public:
   Expressions args;

   IntBuiltin1(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "IntBuiltin1"; }
   ExprType getType() { return etInt; }

   void runVoid(CppiaCtx *ctx) { runInt(ctx); }
   Float runFloat(CppiaCtx *ctx) { return runInt(ctx); }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runInt(ctx)).mPtr; }
   String runString(CppiaCtx *ctx) { return String(runInt(ctx)); }

   int runInt(CppiaCtx *ctx)
   {
      ARG0 val0;
      runValue(val0, ctx, args[0]);
      BCR_CHECK;
      return FUNC(val0);
   }
};


template<typename ARG0, typename ARG1, void (*FUNC)(ARG0,ARG1)>
class VoidBuiltin2 : public CppiaExpr
{
public:
   Expressions args;

   VoidBuiltin2(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "VoidBuiltin2"; }
   ExprType getType() { return etInt; }

   int runInt(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   Float runFloat(CppiaCtx *ctx) { runVoid(ctx); return 0;}
   hx::Object *runObject(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   String runString(CppiaCtx *ctx) { runVoid(ctx); return String(); }
   void runVoid(CppiaCtx *ctx)
   {
      ARG0 val0;
      runValue(val0, ctx, args[0]);
      BCR_VCHECK;
      ARG1 val1;
      runValue(val1, ctx, args[1]);
      BCR_VCHECK;
      FUNC(val0,val1);
   }
};

double memory_get_float( Dynamic inValue, int inIndex) 
{
   return __hxcpp_memory_get_float(inValue, inIndex);
}

template<typename ARG0, typename ARG1, double (*FUNC)(ARG0,ARG1)>
class FloatBuiltin2 : public CppiaExpr
{
public:
   Expressions args;

   FloatBuiltin2(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "FloatBuiltin2"; }
   ExprType getType() { return etFloat; }

   int runInt(CppiaCtx *ctx) { return runFloat(ctx); }
   void runVoid(CppiaCtx *ctx) { runFloat(ctx); }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runFloat(ctx)).mPtr; }
   String runString(CppiaCtx *ctx) { return String(runFloat(ctx)); }
   Float runFloat(CppiaCtx *ctx)
   {
      ARG0 val0;
      runValue(val0, ctx, args[0]);
      BCR_CHECK;
      ARG1 val1;
      runValue(val1, ctx, args[1]);
      BCR_CHECK;
      return  FUNC(val0,val1);
   }
};




CppiaExpr *createGlobalBuiltin(CppiaExpr *src, String function, Expressions &ioExpressions )
{
   if (function==HX_CSTRING("__hxcpp_memory_get_byte") )
   {
      return new IntBuiltin1<int, __hxcpp_memory_get_byte>(src,ioExpressions);
   }
   if (function==HX_CSTRING("__hxcpp_memory_set_byte") )
   {
      return new VoidBuiltin2<int,int,__hxcpp_memory_set_byte>(src,ioExpressions);
   }
   else if (function==HX_CSTRING("__hxcpp_memory_get_float") )
   {
      return new FloatBuiltin2< Dynamic,int, memory_get_float>(src,ioExpressions);
   }
 
   printf("Unknown function : %s\n", function.__s );
   throw "Unknown global";
   return 0;
}



} // end namespace hx
