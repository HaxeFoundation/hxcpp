#include <hxcpp.h>
#include "Cppia.h"

namespace hx
{

enum ArrayFunc
{
   afConcat,
   afCopy,
   afInsert,
   afIterator,
   afJoin,
   afPop,
   afPush,
   afRemove,
   afReverse,
   afShift,
   afSlice,
   afSplice,
   afSort,
   afToString,
   afUnshift,
   afMap,
   afFilter,
   af__get,
   af__set,
   af__crement,
};

static int sArgCount[] = 
{
   1, //afConcat,
   0, //afCopy,
   2, //afInsert,
   0, //afIterator,
   1, //afJoin,
   0, //afPop,
   1, //afPush,
   1, //afRemove,
   0, //afReverse,
   0, //afShift,
   2, //afSlice,
   2, //afSplice,
   1, //afSort,
   0, //afToString,
   1, //afUnshift,
   1, //afMap,
   1, //afFilter,
   1, //af__get,
   2, //af__set,
   1, //af__crement,
};

struct ArrayBuiltinBase : public CppiaExpr
{
   CppiaExpr *thisExpr;
   Expressions args;

   ArrayBuiltinBase(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : CppiaExpr(inSrc)
   {
      thisExpr = inThisExpr;
      args.swap(ioExpressions);
   }
   const char *getName() { return "ArrayBuiltinBase"; }

   CppiaExpr *link(CppiaData &inData)
   {
      thisExpr = thisExpr->link(inData);
      for(int a=0;a<args.size();a++)
         args[a] = args[a]->link(inData);
      return this;
   }
};




template<typename ELEM, int FUNC, typename CREMENT>
struct ArrayBuiltin : public ArrayBuiltinBase
{
   ArrayBuiltin(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }
   virtual ExprType getType()
   {
      switch(FUNC)
      {
         case afJoin:
         case afToString:
            return etString;

         case afSort:
         case afInsert:
         case afShift:
            return etVoid;

         case afPush:
         case afRemove:
            return etInt;

         case af__get:
         case af__set:
         case af__crement:
            return (ExprType)ExprTypeOf<ELEM>::value;

         case afPop:
         case afUnshift:
            if (ExprTypeOf<ELEM>::value==etString)
               return etString;
            return etObject;

         default:
            return etObject;
      }

      return etObject;
   }


   int         runInt(CppiaCtx *ctx)
   {
      if (FUNC==afPush)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         ELEM elem;
         return thisVal->push( runValue(elem,ctx,args[0]) );
      }
      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToInt(thisVal->pop());
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToInt(thisVal->shift());
      }
      if (FUNC==afRemove)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         ELEM elem;
         runValue(elem,ctx,args[0]);
         return thisVal->remove(runValue(elem,ctx,args[0]));
      }
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToInt(thisVal->__get(args[0]->runInt(ctx)));
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int i = args[0]->runInt(ctx);
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
         return ValToInt(elem);
      }



      return 0;
   }
   Float       runFloat(CppiaCtx *ctx)
   {
      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToFloat(thisVal->pop());
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToFloat(thisVal->shift());
      }
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToFloat(thisVal->__get(args[0]->runInt(ctx)));
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int i = args[0]->runInt(ctx);
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
         return ValToFloat(elem);
      }



      if (FUNC==afPush)
         return runInt(ctx);
      return 0.0;
   }


   ::String    runString(CppiaCtx *ctx)
   {
      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToString(thisVal->pop());
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToString(thisVal->shift());
      }
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return ValToString(thisVal->__get(args[0]->runInt(ctx)));
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int i = args[0]->runInt(ctx);
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
         return ValToString(elem);
      }




      if (FUNC==afJoin)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->join( args[0]->runString(ctx) );
      }

      if (FUNC==afToString)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->toString();
      }

      if (FUNC==afPush)
         return Dynamic(runInt(ctx))->toString();

      return runObject(ctx)->toString();
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->__GetItem(args[0]->runInt(ctx)).mPtr;
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int i = args[0]->runInt(ctx);
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
         return Dynamic(elem).mPtr;
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         ELEM &elem = thisVal->Item(args[0]->runInt(ctx));
         return Dynamic(CREMENT::run(elem)).mPtr;
      }


      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return Dynamic(thisVal->pop()).mPtr;
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return Dynamic(thisVal->shift()).mPtr;
      }


      if (FUNC==afConcat)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         Array_obj<ELEM> *inVal = (Array_obj<ELEM>*)args[0]->runObject(ctx);
         return thisVal->concat(inVal).mPtr;
      }
      if (FUNC==afCopy)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->copy().mPtr;
      }
      if (FUNC==afSplice)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int pos = args[0]->runInt(ctx);
         int end = args[1]->runInt(ctx);
         return thisVal->splice(pos,end).mPtr;
      }
      if (FUNC==afSlice)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int pos = args[0]->runInt(ctx);
         hx::Object *end = args[1]->runObject(ctx);
         return thisVal->slice(pos,end).mPtr;
      }
      if (FUNC==afMap)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->map(args[0]->runObject(ctx)).mPtr;
      }
      if (FUNC==afFilter)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->filter(args[0]->runObject(ctx)).mPtr;
      }

      if (FUNC==afIterator)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         return thisVal->iterator().mPtr;
      }

      if (FUNC==afPush || FUNC==afRemove)
         return Dynamic(runInt(ctx)).mPtr;

      if (FUNC==afJoin)
         return Dynamic(runString(ctx)).mPtr;


      return 0;
   }
   void        runVoid(CppiaCtx *ctx)
   {
      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         thisVal->pop();
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         thisVal->shift();
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int i = args[0]->runInt(ctx);
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
      }

      if (FUNC==afPush || FUNC==afRemove)
         runInt(ctx);
      if (FUNC==afConcat || FUNC==afCopy || FUNC==afReverse || FUNC==afSplice || FUNC==afSlice ||
           FUNC==afMap || FUNC==afFilter)
         runObject(ctx);
      if (FUNC==afReverse)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         thisVal->reverse();
      }
      if (FUNC==afSort)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         thisVal->sort(args[0]->runObject(ctx));
      }
      if (FUNC==afInsert)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         int pos = args[0]->runInt(ctx);
         ELEM elem;
         thisVal->insert(pos,runValue(elem,ctx,args[1]));
      }
      if (FUNC==afUnshift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         ELEM elem;
         thisVal->unshift(runValue(elem,ctx,args[0]));
      }


   }

   CppiaExpr   *makeSetter(AssignOp op,CppiaExpr *inValue)
   {
      if (FUNC==af__get)
      {
         args.push_back(inValue);
         // TODO - other setters
         CppiaExpr *replace = new ArrayBuiltin<ELEM,af__set,NoCrement>(this,thisExpr,args);
         delete this;
         return replace;
      }
      return 0;
   }

   CppiaExpr   *makeCrement(CrementOp inOp)
   {
      if (FUNC==af__get)
      {
         CppiaExpr *replace = 0;

         switch(inOp)
         {
            case coPreInc :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPreInc>(this,thisExpr,args);
               break;
            case coPostInc :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPostInc>(this,thisExpr,args);
               break;
            case coPreDec :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPreDec>(this,thisExpr,args);
               break;
            case coPostDec :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPostDec>(this,thisExpr,args);
               break;
            default:
               return 0;
         }
         return replace;
      }
      return 0;
   }

};


template<int BUILTIN,typename CREMENT>
CppiaExpr *TCreateArrayBuiltin(CppiaExpr *inSrc, ArrayType inType, CppiaExpr *thisExpr, Expressions &args)
{
   if (sArgCount[BUILTIN]!=args.size())
      throw "Bad arg count for array builtin";

   switch(inType)
   {
      case arrBool:
         return new ArrayBuiltin<bool,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrUnsignedChar:
         return new ArrayBuiltin<unsigned char,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrInt:
         return new ArrayBuiltin<int,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrFloat:
         return new ArrayBuiltin<Float,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrString:
         return new ArrayBuiltin<String,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrDynamic:
         return new ArrayBuiltin<Dynamic,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrNotArray:
         break;
   }
   throw "Unknown array type";
   return 0;
}



CppiaExpr *createArrayBuiltin(CppiaExpr *src, ArrayType inType, CppiaExpr *inThisExpr, String field,
                              Expressions &ioExpressions )
{
   if (field==HX_CSTRING("concat"))
      return TCreateArrayBuiltin<afConcat,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("copy"))
      return TCreateArrayBuiltin<afCopy,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("insert"))
      return TCreateArrayBuiltin<afInsert,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("iterator"))
      return TCreateArrayBuiltin<afIterator,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("join"))
      return TCreateArrayBuiltin<afJoin,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("pop"))
      return TCreateArrayBuiltin<afPop,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("push"))
      return TCreateArrayBuiltin<afPush,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("remove"))
      return TCreateArrayBuiltin<afRemove,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("reverse"))
      return TCreateArrayBuiltin<afReverse,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("shift"))
      return TCreateArrayBuiltin<afShift,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("splice"))
      return TCreateArrayBuiltin<afSplice,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("slice"))
      return TCreateArrayBuiltin<afSlice,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("sort"))
      return TCreateArrayBuiltin<afSort,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("toString"))
      return TCreateArrayBuiltin<afToString,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("unshift"))
      return TCreateArrayBuiltin<afUnshift,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("map"))
      return TCreateArrayBuiltin<afMap,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("filter"))
      return TCreateArrayBuiltin<afFilter,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__get"))
      return TCreateArrayBuiltin<af__get,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__set"))
      return TCreateArrayBuiltin<af__set,NoCrement>(src, inType, inThisExpr, ioExpressions);

   printf("Bad array field '%s'\n", field.__s);
   throw "Unknown array field";
   return 0;
}



// --- String -------------------------

struct StringExpr : public CppiaExpr
{
   CppiaExpr *strVal;
   StringExpr(CppiaExpr *inSrc, CppiaExpr *inThis )
      : CppiaExpr(inSrc)
   {
      strVal = inThis;
   }
   ExprType getType() { return etString; }
   CppiaExpr *link(CppiaData &inData)
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
   CppiaExpr *link(CppiaData &inData)
   {
      a0 = a0->link(inData);
      a1 = a1->link(inData);
      return StringExpr::link(inData);
   }
   String runString(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      int start = a0->runInt(ctx);
      if (SUBSTR)
         return val.substr(start, a1->runObject(ctx) );
      else
         return val.substring(start, a1->runObject(ctx) );
   }
};


template<bool UPPER>
struct ToCaseExpr : public StringExpr
{
   ToCaseExpr(CppiaExpr *inSrc, CppiaExpr *inThis ) : StringExpr(inSrc,inThis) { }
   String runString(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      if (UPPER)
         return val.toUpperCase();
      else
         return val.toLowerCase();
   }
};

template<bool CODE>
struct CharAtExpr : public StringExpr
{
   CppiaExpr *a0;

   CharAtExpr(CppiaExpr *inSrc, CppiaExpr *inThis, CppiaExpr *inIndex ) : StringExpr(inSrc,inThis)
   {
      a0 = inIndex;
   }
   CppiaExpr *link(CppiaData &inData)
   {
      a0 = a0->link(inData);
      return StringExpr::link(inData);
   }
   ExprType getType() { return CODE ? etObject : etString; }

   String runString(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      return val.charAt(a0->runInt(ctx));
   }
   int runInt(CppiaCtx *ctx)
   {
      //printf("Char code at %d INT\n", CODE);
      String val = strVal->runString(ctx);
      return val.charCodeAt(a0->runInt(ctx));
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);

      if (CODE)
         return val.charCodeAt(a0->runInt(ctx)).mPtr;
      else
         return Dynamic(val.charAt(a0->runInt(ctx))).mPtr;
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
   CppiaExpr *link(CppiaData &inData)
   {
      strVal = strVal->link(inData);
      a0 = a0->link(inData);
      return this;
   }
   ExprType getType() { return etObject; }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      return val.split( a0->runString(ctx) ).mPtr;
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
   CppiaExpr *link(CppiaData &inData)
   {
      strVal = strVal->link(inData);
      sought = sought->link(inData);
      start = start->link(inData);
      return this;
   }
   int runInt(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      String s = sought->runString(ctx);
      if (LAST)
         return val.lastIndexOf(s,start->runObject(ctx));
      else
         return val.indexOf(s,start->runObject(ctx));
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
      return new CharAtExpr<false>(inSrc,inThisExpr,ioExpressions[0]);
   }
   else if (field==HX_CSTRING("charCodeAt") || field==HX_CSTRING("cca"))
   {
      if (ioExpressions.size()!=1) throw "Bad arg count";
      return new CharAtExpr<true>(inSrc,inThisExpr,ioExpressions[0]);
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
