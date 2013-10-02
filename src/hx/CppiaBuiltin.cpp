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
   CppiaExpr *link(CppiaData &inData)
   {
      thisExpr = thisExpr->link(inData);
      for(int a=0;a<args.size();a++)
         args[a] = args[a]->link(inData);
      return this;
   }
};

template<typename ELEM, int FUNC>
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

};


template<int BUILTIN>
CppiaExpr *TCreateArrayBuiltin(CppiaExpr *inSrc, ArrayType inType, CppiaExpr *thisExpr, Expressions &args )
{
   if (sArgCount[BUILTIN]!=args.size())
      throw "Bad arg count for array builtin";

   switch(inType)
   {
      case arrBool:
         return new ArrayBuiltin<bool,BUILTIN>(inSrc, thisExpr, args);
      case arrUnsignedChar:
         return new ArrayBuiltin<unsigned char,BUILTIN>(inSrc, thisExpr, args);
      case arrInt:
         return new ArrayBuiltin<int,BUILTIN>(inSrc, thisExpr, args);
      case arrFloat:
         return new ArrayBuiltin<Float,BUILTIN>(inSrc, thisExpr, args);
      case arrString:
         return new ArrayBuiltin<String,BUILTIN>(inSrc, thisExpr, args);
      case arrDynamic:
         return new ArrayBuiltin<Dynamic,BUILTIN>(inSrc, thisExpr, args);
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
      return TCreateArrayBuiltin<afConcat>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("copy"))
      return TCreateArrayBuiltin<afCopy>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("insert"))
      return TCreateArrayBuiltin<afInsert>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("iterator"))
      return TCreateArrayBuiltin<afIterator>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("join"))
      return TCreateArrayBuiltin<afJoin>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("pop"))
      return TCreateArrayBuiltin<afPop>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("push"))
      return TCreateArrayBuiltin<afPush>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("remove"))
      return TCreateArrayBuiltin<afRemove>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("reverse"))
      return TCreateArrayBuiltin<afReverse>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("shift"))
      return TCreateArrayBuiltin<afShift>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("sort"))
      return TCreateArrayBuiltin<afSort>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("toString"))
      return TCreateArrayBuiltin<afToString>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("unshift"))
      return TCreateArrayBuiltin<afUnshift>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("map"))
      return TCreateArrayBuiltin<afMap>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("filter"))
      return TCreateArrayBuiltin<afFilter>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__get"))
      return TCreateArrayBuiltin<af__get>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__set"))
      return TCreateArrayBuiltin<af__set>(src, inType, inThisExpr, ioExpressions);

   throw "Unknown array field";
   return 0;
}



} // end namespace hx
