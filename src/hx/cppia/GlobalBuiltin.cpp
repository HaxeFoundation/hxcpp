#include <hxcpp.h>
#include "Cppia.h"

namespace hx
{

template<> inline Array<unsigned char> &runValue(Array<unsigned char> &outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   outValue.mPtr = (Array_obj<unsigned char>*) expr->runObject(ctx);
   return outValue;
}


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


template<typename ARG0, typename ARG1, int (*FUNC)(ARG0 arg0, ARG1 arg1)>
class IntBuiltin2 : public CppiaExpr
{
public:
   Expressions args;

   IntBuiltin2(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "IntBuiltin2"; }
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
      ARG1 val1;
      runValue(val1, ctx, args[1]);
      BCR_CHECK;
      return FUNC(val0,val1);
   }
};


template<typename ARG0, typename ARG1, void (*FUNC)(ARG0,ARG1)>
class VoidBuiltin2 : public CppiaExpr
{
public:
   Expressions args;

   VoidBuiltin2(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "VoidBuiltin2"; }
   ExprType getType() { return etVoid; }

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


template<typename ARG0, typename ARG1, typename ARG2, void (*FUNC)(ARG0,ARG1,ARG2)>
class VoidBuiltin3 : public CppiaExpr
{
public:
   Expressions args;

   VoidBuiltin3(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "VoidBuiltin3"; }
   ExprType getType() { return etVoid; }

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
      ARG2 val2;
      runValue(val2, ctx, args[2]);
      BCR_VCHECK;
      FUNC(val0,val1,val2);
   }
};

template<typename ARG0, typename RET, RET (*FUNC)(ARG0)>
class FloatBuiltin1 : public CppiaExpr
{
public:
   Expressions args;

   FloatBuiltin1(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "FloatBuiltin1"; }
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
      return  FUNC(val0);
   }
};



template<typename ARG0, typename ARG1, typename RET, RET (*FUNC)(ARG0,ARG1)>
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


template<typename ARG0, typename ARG1, typename ARG2, typename RET, RET (*FUNC)(ARG0,ARG1,ARG2)>
class FloatBuiltin3 : public CppiaExpr
{
public:
   Expressions args;

   FloatBuiltin3(CppiaExpr *inSrc, Expressions &inArgs) : CppiaExpr(inSrc), args(inArgs) { }

   const char *getName() { return "FloatBuiltin3"; }
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
      ARG2 val2;
      runValue(val2, ctx, args[2]);
      BCR_CHECK;
      return  FUNC(val0,val1,val2);
   }
};

#define MEMORY_INT(GETTER,SETTER) \
   if (function==HX_CSTRING( #GETTER ) ) \
   { \
      if (ioExpressions.size()==1) \
         return new IntBuiltin1<int, GETTER>(src,ioExpressions); \
      return new IntBuiltin2<Array<unsigned char>, int, GETTER>(src,ioExpressions); \
   } \
   if (function==HX_CSTRING( #SETTER ) ) \
   { \
      if (ioExpressions.size()==2) \
         return new VoidBuiltin2<int,int, SETTER>(src,ioExpressions); \
      return new VoidBuiltin3<Array<unsigned char>,int,int,SETTER>(src,ioExpressions); \
   }




CppiaExpr *createGlobalBuiltin(CppiaExpr *src, String function, Expressions &ioExpressions )
{
   MEMORY_INT(__hxcpp_memory_get_byte, __hxcpp_memory_set_byte);
   MEMORY_INT(__hxcpp_memory_get_i32, __hxcpp_memory_set_i32);
   MEMORY_INT(__hxcpp_memory_get_ui32, __hxcpp_memory_set_ui32);
   MEMORY_INT(__hxcpp_memory_get_i16, __hxcpp_memory_set_i16);
   MEMORY_INT(__hxcpp_memory_get_ui16, __hxcpp_memory_set_ui16);

   if (function==HX_CSTRING("__hxcpp_memory_get_float") )
   {
      if (ioExpressions.size()==1)
         return new FloatBuiltin1<int,float,__hxcpp_memory_get_float>(src,ioExpressions);
      return new FloatBuiltin2<Array<unsigned char>,int,float,__hxcpp_memory_get_float>(src,ioExpressions);
   }
   if (function==HX_CSTRING("__hxcpp_memory_set_float") )
   {
      if (ioExpressions.size()==2)
         return new VoidBuiltin2<int,float,__hxcpp_memory_set_float>(src,ioExpressions);
      return new VoidBuiltin3<Array<unsigned char>,int,float,__hxcpp_memory_set_float>(src,ioExpressions);
   }

   if (function==HX_CSTRING("__hxcpp_memory_get_double") )
   {
      if (ioExpressions.size()==1)
         return new FloatBuiltin1<int,double,__hxcpp_memory_get_double>(src,ioExpressions);
      return new FloatBuiltin2<Array<unsigned char>,int,double,__hxcpp_memory_get_double>(src,ioExpressions);
   }
   if (function==HX_CSTRING("__hxcpp_memory_set_double") )
   {
      if (ioExpressions.size()==2)
         return new VoidBuiltin2<int,double,__hxcpp_memory_set_double>(src,ioExpressions);
      return new VoidBuiltin3<Array<unsigned char>,int,double,__hxcpp_memory_set_double>(src,ioExpressions);
   }

   printf("Unknown function : %s\n", function.__s );
   throw "Unknown global";
   return 0;
}



} // end namespace hx
