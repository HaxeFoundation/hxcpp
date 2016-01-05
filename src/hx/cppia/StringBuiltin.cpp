#include <hxcpp.h>
#include "Cppia.h"

namespace hx
{

struct StringExpr : public CppiaExpr
{
   CppiaExpr *strVal;
   StringExpr(CppiaExpr *inSrc, CppiaExpr *inThis )
      : CppiaExpr(inSrc)
   {
      strVal = inThis;
   }
   ExprType getType() { return etString; }
   CppiaExpr *link(CppiaModule &inData)
   {
      strVal = strVal->link(inData);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      return Dynamic(runString(ctx)).mPtr;
   }
};

template<bool SUBSTR>
struct SubStrExpr : public StringExpr
{
   CppiaExpr *a0;
   CppiaExpr *a1;
   SubStrExpr(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inA0, CppiaExpr *inA1)
      : StringExpr(inSrc,inThis)
   {
      a0 = inA0;
      a1 = inA1;
   }
   CppiaExpr *link(CppiaModule &inData)
   {
      a0 = a0->link(inData);
      a1 = a1->link(inData);
      return StringExpr::link(inData);
   }
   String runString(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      int start = a0->runInt(ctx);
      BCR_CHECK;
      Dynamic end = a1->runObject(ctx);
      BCR_CHECK;
      if (SUBSTR)
         return val.substr(start,end);
      else
         return val.substring(start,end);
   }
};


template<bool UPPER>
struct ToCaseExpr : public StringExpr
{
   ToCaseExpr(CppiaExpr *inSrc, CppiaExpr *inThis ) : StringExpr(inSrc,inThis) { }
   String runString(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      if (UPPER)
         return val.toUpperCase();
      else
         return val.toLowerCase();
   }
};

template<bool CODE,bool AS_INT>
struct CharAtExpr : public StringExpr
{
   CppiaExpr *a0;

   CharAtExpr(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inIndex ) : StringExpr(inSrc,inThis)
   {
      a0 = inIndex;
   }
   CppiaExpr *link(CppiaModule &inData)
   {
      a0 = a0->link(inData);
      return StringExpr::link(inData);
   }
   ExprType getType() { return CODE ? (AS_INT ? etInt : etObject) : etString; }

   String runString(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      int idx = a0->runInt(ctx);
      BCR_CHECK;
      return val.charAt(idx);
   }
   int runInt(CppiaCtx *ctx)
   {
      //printf("Char code at %d INT\n", CODE);
      String val = strVal->runString(ctx);
      BCR_CHECK;
      int idx = a0->runInt(ctx);
      BCR_CHECK;

      if (AS_INT)
         return (int)( ((unsigned char *)val.__s) [idx]);
      else
         return val.charCodeAt(idx);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      int idx = a0->runInt(ctx);
      BCR_CHECK;

      if (CODE)
      {
         if (AS_INT)
            return Dynamic( (int)( ((unsigned char *)val.__s) [idx]) ).mPtr;
         else
            return val.charCodeAt(idx).mPtr;
      }
      else
         return Dynamic(val.charAt(idx)).mPtr;
   }
};



struct SplitExpr : public CppiaExpr
{
   CppiaExpr *strVal;
   CppiaExpr *a0;

   SplitExpr(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inDelim ) :
      CppiaExpr(inSrc)
   {
      strVal = inThis;
      a0 = inDelim;
   }
   CppiaExpr *link(CppiaModule &inData)
   {
      strVal = strVal->link(inData);
      a0 = a0->link(inData);
      return this;
   }
   ExprType getType() { return etObject; }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      String separator = a0->runString(ctx);
      BCR_CHECK;
      return val.split(separator).mPtr;
   }
};



template<bool LAST>
struct IndexOfExpr : public CppiaExpr
{
   CppiaExpr *strVal;
   CppiaExpr *sought;
   CppiaExpr *start;

   IndexOfExpr(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inSought, CppiaExpr *inStart ) :
      CppiaExpr(inSrc)
   {
      strVal = inThis;
      sought = inSought;
      start = inStart;
   }
   ExprType getType() { return etInt; }
   CppiaExpr *link(CppiaModule &inData)
   {
      strVal = strVal->link(inData);
      sought = sought->link(inData);
      start = start->link(inData);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      String s = sought->runString(ctx);
      BCR_CHECK;
      hx::Object *first = start->runObject(ctx);
      BCR_CHECK;
      if (LAST)
         return val.lastIndexOf(s,first);
      else
         return val.indexOf(s,first);
   }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runInt(ctx)).mPtr; }
};



// TODO
// static function fromCharCode( code : Int ) : String;


CppiaExpr *createStringBuiltin(CppiaExpr *inSrc, CppiaExpr *inThisExpr, String field, Expressions &ioExpressions )
{
   if (field==HX_CSTRING("toString"))
   {
      if (ioExpressions.size()!=0) throw "Bad arg count";
      return inThisExpr;
   }
   else if (field==HX_CSTRING("toUpperCase"))
   {
      if (ioExpressions.size()!=0) throw "Bad arg count";
      return new ToCaseExpr<true>(inSrc,inThisExpr);
   }
   else if (field==HX_CSTRING("toLowerCase"))
   {
      if (ioExpressions.size()!=0) throw "Bad arg count";
      return new ToCaseExpr<false>(inSrc,inThisExpr);
   }
   else if (field==HX_CSTRING("charAt"))
   {
      if (ioExpressions.size()!=1) throw "Bad arg count";
      return new CharAtExpr<false,false>(inSrc,inThisExpr,ioExpressions[0]);
   }
   else if (field==HX_CSTRING("cca"))
   {
      if (ioExpressions.size()!=1) throw "Bad arg count";
      return new CharAtExpr<true,true>(inSrc,inThisExpr,ioExpressions[0]);
   }
   else if (field==HX_CSTRING("charCodeAt"))
   {
      if (ioExpressions.size()!=1) throw "Bad arg count";
      return new CharAtExpr<true,false>(inSrc,inThisExpr,ioExpressions[0]);
   }
   else if (field==HX_CSTRING("split"))
   {
      if (ioExpressions.size()!=1) throw "Bad arg count";
      return new SplitExpr(inSrc,inThisExpr,ioExpressions[0]);
   }
   else if (field==HX_CSTRING("indexOf"))
   {
      if (ioExpressions.size()!=2) throw "Bad arg count";
      return new IndexOfExpr<false>(inSrc,inThisExpr,ioExpressions[0], ioExpressions[1]);
   }
   else if (field==HX_CSTRING("lastIndexOf"))
   {
      if (ioExpressions.size()!=2) throw "Bad arg count";
      return new IndexOfExpr<true>(inSrc,inThisExpr,ioExpressions[0], ioExpressions[1]);
   }

   else if (field==HX_CSTRING("substr"))
   {
      if (ioExpressions.size()!=2) throw "Bad arg count";
      return new SubStrExpr<true>(inSrc,inThisExpr, ioExpressions[0], ioExpressions[1]);
   }
   else if (field==HX_CSTRING("substring"))
   {
      if (ioExpressions.size()!=2) throw "Bad arg count";
      return new SubStrExpr<false>(inSrc,inThisExpr, ioExpressions[0], ioExpressions[1]);
   }

   return 0;
}

} // end namespace hx
