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
   afIndexOf,
   afLastIndexOf,
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
   2, //afIndexOf,
   2, //afLastIndexOf,
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


   CppiaExpr *link(CppiaModule &inData)
   {
      thisExpr = thisExpr->link(inData);
      for(int a=0;a<args.size();a++)
         args[a] = args[a]->link(inData);
      return this;
   }
};

struct ArrayEq
{
   template<typename ELEM>
   inline static void set(ELEM &elem, CppiaCtx *ctx, CppiaExpr *expr)
   {
      elem = runValue(elem,ctx,expr);
   }
};


struct ArrayAddEq
{
   template<typename ELEM>
   inline static void set(ELEM &elem, CppiaCtx *ctx, CppiaExpr *expr)
   {
      elem = runValue(elem,ctx,expr);
   }
};



template<typename ELEM, typename FUNC>
struct ArraySetter : public ArrayBuiltinBase
{
   ArraySetter(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }
   virtual ExprType getType()
   {
      return (ExprType)ExprTypeOf<ELEM>::value;
   }
   int runInt(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      return ValToInt(elem);
   }
   Float  runFloat(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      return ValToFloat(elem);
   }
   String  runString(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      return ValToString(elem);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      return Dynamic(elem).mPtr;
   }
};



template<typename ELEM, int FUNC, typename CREMENT>
struct ArrayBuiltin : public ArrayBuiltinBase
{
   ArrayBuiltin(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }

   ExprType getType()
   {
      switch(FUNC)
      {
         case af__get:
         case af__set:
         case af__crement:
            return (ExprType)ExprTypeOf<ELEM>::value;

         case afPop:
         //case afUnshift:
            if (ExprTypeOf<ELEM>::value==(int)etString)
               return etString;
            return etObject;

         case afJoin:
         case afToString:
            return etString;

         case afSort:
         case afInsert:
         case afUnshift:
            return etVoid;

         case afPush:
         case afRemove:
         case afIndexOf:
         case afLastIndexOf:
            return etInt;

         default:
            return etObject;
      }

      return etObject;
   }


   int runInt(CppiaCtx *ctx)
   {
      if (FUNC==afPush)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[0]);
         BCR_CHECK;
         int result =  thisVal->push(elem);
         return result;
      }
      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return ValToInt(thisVal->pop());
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return ValToInt(thisVal->shift());
      }
      if (FUNC==afRemove)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[0]);
         BCR_CHECK;
         return thisVal->remove(elem);
      }
      if (FUNC==afIndexOf)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[0]);
         BCR_CHECK;
         hx::Object *start = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->indexOf(elem,start);
      }
      if (FUNC==afLastIndexOf)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[0]);
         BCR_CHECK;
         hx::Object *start = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->lastIndexOf(elem, start);
      }
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         return ValToInt(thisVal->__get(idx));
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int i = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[1]);
         BCR_CHECK;
         thisVal->Item(i) = elem;
         return ValToInt(elem);
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM &elem = thisVal->Item(idx);
         return ValToInt(CREMENT::run(elem));
      }


      return 0;
   }
   Float       runFloat(CppiaCtx *ctx)
   {
      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return ValToFloat(thisVal->pop());
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return ValToFloat(thisVal->shift());
      }
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return ValToFloat(thisVal->__get(args[0]->runInt(ctx)));
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int i = args[0]->runInt(ctx);
         ELEM elem;
         runValue(elem,ctx,args[1]);
         BCR_CHECK;
         thisVal->Item(i) = elem;
         return ValToFloat(elem);
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         ELEM &elem = thisVal->Item(args[0]->runInt(ctx));
         return ValToFloat(CREMENT::run(elem));
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
         BCR_CHECK;
         return ValToString(thisVal->pop());
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return ValToString(thisVal->shift());
      }
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         return ValToString(thisVal->__get(idx));
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int i = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[1]);
         BCR_CHECK;
         return ValToString( thisVal->Item(i) = elem);
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM &elem = thisVal->Item(idx);
         return ValToString(CREMENT::run(elem));
      }


      if (FUNC==afJoin)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         String space = args[0]->runString(ctx);
         BCR_CHECK;
         return thisVal->join(space);
      }

      if (FUNC==afToString)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return thisVal->toString();
      }

      if (FUNC==afPush || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
         return Dynamic(runInt(ctx))->toString();

      return runObject(ctx)->toString();
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (FUNC==af__get)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         return thisVal->__GetItem(idx).mPtr;
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int i = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
         return Dynamic(elem).mPtr;
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM &elem = thisVal->Item(idx);
         return Dynamic(CREMENT::run(elem)).mPtr;
      }

      if (FUNC==afPop)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return Dynamic(thisVal->pop()).mPtr;
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return Dynamic(thisVal->shift()).mPtr;
      }


      if (FUNC==afConcat)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         Array_obj<ELEM> *inVal = (Array_obj<ELEM>*)args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->concat(inVal).mPtr;
      }
      if (FUNC==afCopy)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return thisVal->copy().mPtr;
      }
      if (FUNC==afSplice)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int pos = args[0]->runInt(ctx);
         BCR_CHECK;
         int end = args[1]->runInt(ctx);
         BCR_CHECK;
         return thisVal->splice(pos,end).mPtr;
      }
      if (FUNC==afSlice)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int pos = args[0]->runInt(ctx);
         BCR_CHECK;
         hx::Object *end = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->slice(pos,end).mPtr;
      }
      if (FUNC==afMap)
      {
         // TODO - maybe make this more efficient
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object *func = args[0]->runObject(ctx);
         BCR_CHECK;
         Array<ELEM> result = thisVal->map(func);
         return result.mPtr;
      }
      if (FUNC==afFilter)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object *func = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->filter(func).mPtr;
      }

      if (FUNC==afIterator)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return thisVal->iterator().mPtr;
      }

      if (FUNC==afPush || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
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
         BCR_VCHECK;
         thisVal->pop();
      }
      if (FUNC==afShift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         thisVal->shift();
      }
      if (FUNC==af__set)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int i = args[0]->runInt(ctx);
         BCR_VCHECK;
         ELEM elem;
         thisVal->Item(i) = runValue(elem,ctx,args[1]);
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int idx = args[0]->runInt(ctx);
         BCR_VCHECK;
         ELEM &elem = thisVal->Item(idx);
         CREMENT::run(elem);
      }


      if (FUNC==afPush || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
         runInt(ctx);

      if (FUNC==afConcat || FUNC==afCopy || FUNC==afReverse || FUNC==afSplice || FUNC==afSlice ||
           FUNC==afMap || FUNC==afFilter)
         runObject(ctx);

      if (FUNC==afReverse)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         thisVal->reverse();
      }
      if (FUNC==afSort)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         hx::Object * func = args[0]->runObject(ctx);
         BCR_VCHECK;
         thisVal->sort(func);
      }
      if (FUNC==afInsert)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int pos = args[0]->runInt(ctx);
         BCR_VCHECK;
         ELEM elem;
         runValue(elem,ctx,args[1]);
         BCR_VCHECK;
         thisVal->insert(pos,elem);
      }
      if (FUNC==afUnshift)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         ELEM elem;
         runValue(elem,ctx,args[0]);
         BCR_VCHECK;
         thisVal->unshift(elem);
      }


   }

   CppiaExpr   *makeSetter(AssignOp op,CppiaExpr *inValue)
   {
      if (FUNC==af__get)
      {
         args.push_back(inValue);
         CppiaExpr *replace = 0;

         switch(op)
         {
            case aoSet:
               replace = new ArraySetter<ELEM,AssignSet>(this,thisExpr,args);
               break;
            case aoAdd:
               replace = new ArraySetter<ELEM,AssignAdd>(this,thisExpr,args);
               break;
            case aoMult:
               replace = new ArraySetter<ELEM,AssignMult>(this,thisExpr,args);
               break;
            case aoDiv:
               replace = new ArraySetter<ELEM,AssignDiv>(this,thisExpr,args);
               break;
            case aoSub:
               replace = new ArraySetter<ELEM,AssignSub>(this,thisExpr,args);
               break;
            case aoAnd:
               replace = new ArraySetter<ELEM,AssignAnd>(this,thisExpr,args);
               break;
            case aoOr:
               replace = new ArraySetter<ELEM,AssignOr>(this,thisExpr,args);
               break;
            case aoXOr:
               replace = new ArraySetter<ELEM,AssignXOr>(this,thisExpr,args);
               break;
            case aoShl:
               replace = new ArraySetter<ELEM,AssignShl>(this,thisExpr,args);
               break;
            case aoShr:
               replace = new ArraySetter<ELEM,AssignShr>(this,thisExpr,args);
               break;
            case aoUShr:
               replace = new ArraySetter<ELEM,AssignUShr>(this,thisExpr,args);
               break;
            case aoMod:
               replace = new ArraySetter<ELEM,AssignMod>(this,thisExpr,args);
               break;
      
            default: ;
               printf("make setter %d\n", op);
               throw "setter not implemented";
         }

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




template<int FUNC,int OP>
struct ArrayBuiltinAny : public ArrayBuiltinBase
{
   ArrayBuiltinAny(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }

   int  runInt(CppiaCtx *ctx)
   {
      if (FUNC==afPush)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object * val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__push(val);
      }
      if (FUNC==afRemove)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object * val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__remove(val);
      }
      if (FUNC==afIndexOf)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_CHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         hx::Object *a1 = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__indexOf( a0, a1 );
      }
      if (FUNC==afLastIndexOf)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_CHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         hx::Object *a1 = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__lastIndexOf( a0, a1 );
      }

      return Dynamic( runObject(ctx) );
   }

   Float runFloat(CppiaCtx *ctx)
   {
      return runInt(ctx);
   }


   ::String    runString(CppiaCtx *ctx)
   {
      if (FUNC==afJoin)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_CHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__join( a0 );
      }

      if (FUNC==afToString)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_CHECK;
         return thisVal->toString();
      }

      return runObject(ctx)->toString();
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (FUNC==afPush || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
         return Dynamic(runInt(ctx)).mPtr;

      if (FUNC==afRemove)
         return Dynamic((bool)runInt(ctx)).mPtr;

      if (FUNC==afJoin || FUNC==afToString)
         return Dynamic(runString(ctx)).mPtr;

      if (FUNC==afSort || FUNC==afInsert || FUNC==afUnshift)
      {
         runVoid(ctx);
         return 0;
      }


      ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
      BCR_CHECK;
      CPPIA_CHECK(thisVal);

      if (FUNC==af__get)
      {
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         return thisVal->__GetItem(idx).mPtr;
      }
      if (FUNC==af__set)
      {
         int i = args[0]->runInt(ctx);
         BCR_CHECK;
         Dynamic val = args[1]->runObject(ctx);
         BCR_CHECK;
         if (OP==aoSet)
            return thisVal->__SetItem(i, val).mPtr;

         Dynamic orig = thisVal->__GetItem(i);
         CPPIA_CHECK(orig.mPtr);
         switch(OP)
         {
            case aoSet: break;

            case aoAdd:
               val = orig + val;
               break;
            case aoMult:
               val = orig * val;
               break;
            case aoDiv:
               val = orig / val;
               break;
            case aoSub:
               val = orig - val;
               break;
            case aoAnd:
               val = orig->__ToInt() & val->__ToInt();
               break;
            case aoOr:
               val = orig->__ToInt() | val->__ToInt();
               break;
            case aoXOr:
               val = orig->__ToInt() ^ val->__ToInt();
               break;
            case aoShl:
               val = orig->__ToInt() << val->__ToInt();
               break;
            case aoShr:
               val = orig->__ToInt() >> val->__ToInt();
               break;
            case aoUShr:
               val = hx::UShr(orig,val);
               break;
            case aoMod:
               val = orig % val;
               break;
            default: ;
         }
         return thisVal->__SetItem(i,val).mPtr;
      }
      if (FUNC==af__crement)
      {
         int i = args[0]->runInt(ctx);
         BCR_CHECK;
         if (OP==coPreInc)
            return thisVal->__SetItem(i, thisVal->__GetItem(i) + 1).mPtr;
         if (OP==coPreDec)
            return thisVal->__SetItem(i, thisVal->__GetItem(i) - 1).mPtr;

         Dynamic result = thisVal->__GetItem(i);
         if (OP==coPostDec)
            thisVal->__SetItem(i, thisVal->__GetItem(i) - 1);
         if (OP==coPostInc)
            thisVal->__SetItem(i, thisVal->__GetItem(i) + 1);
         return result.mPtr;
      }

      if (FUNC==afPop)
      {
         return thisVal->__pop().mPtr;
      }
      if (FUNC==afShift)
      {
         return thisVal->__shift().mPtr;
      }

      if (FUNC==afConcat)
      {
         Dynamic val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__concat(val).mPtr;
      }
      if (FUNC==afCopy)
      {
         return thisVal->__copy().mPtr;
      }
      if (FUNC==afSplice)
      {
         Dynamic pos = args[0]->runObject(ctx);
         BCR_CHECK;
         Dynamic end = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__splice(pos,end).mPtr;
      }
      if (FUNC==afSlice)
      {
         Dynamic pos = args[0]->runObject(ctx);
         BCR_CHECK;
         Dynamic end = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__slice(pos,end).mPtr;
      }
      if (FUNC==afMap)
      {
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__map(a0).mPtr;
      }
      if (FUNC==afFilter)
      {
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->__filter(a0).mPtr;
      }

      if (FUNC==afIterator)
      {
         return thisVal->__iterator().mPtr;
      }

      return 0;
   }
   void runVoid(CppiaCtx *ctx)
   {
      if (FUNC==afSort)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_VCHECK;
         thisVal->__sort(a0);
         return;
      }
      if (FUNC==afInsert)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         Dynamic pos = args[0]->runObject(ctx);
         BCR_VCHECK;
         Dynamic val = args[1]->runObject(ctx);
         BCR_VCHECK;
         thisVal->__insert(pos, val);
         return;
      }
      if (FUNC==afUnshift)
      {
         ArrayBase *thisVal = (ArrayBase *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         Dynamic val = args[0]->runObject(ctx);
         BCR_VCHECK;
         thisVal->__unshift(val);
         return;
      }

      switch(inlineGetType())
      {
         case etString:
            runString(ctx);
            return;
         case etInt:
            runInt(ctx);
            return;
         default:
            runObject(ctx);
      }
   }

   CppiaExpr   *makeSetter(AssignOp op,CppiaExpr *inValue)
   {
      if (FUNC==af__get)
      {
         args.push_back(inValue);
         CppiaExpr *replace = 0;

         switch(op)
         {
            case aoSet:
               replace = new ArrayBuiltinAny<af__set,aoSet>(this,thisExpr,args);
               break;
            case aoAdd:
               replace = new ArrayBuiltinAny<af__set,aoAdd>(this,thisExpr,args);
               break;
            case aoMult:
               replace = new ArrayBuiltinAny<af__set,aoMult>(this,thisExpr,args);
               break;
            case aoDiv:
               replace = new ArrayBuiltinAny<af__set,aoDiv>(this,thisExpr,args);
               break;
            case aoSub:
               replace = new ArrayBuiltinAny<af__set,aoSub>(this,thisExpr,args);
               break;
            case aoAnd:
               replace = new ArrayBuiltinAny<af__set,aoAnd>(this,thisExpr,args);
               break;
            case aoOr:
               replace = new ArrayBuiltinAny<af__set,aoOr>(this,thisExpr,args);
               break;
            case aoXOr:
               replace = new ArrayBuiltinAny<af__set,aoXOr>(this,thisExpr,args);
               break;
            case aoShl:
               replace = new ArrayBuiltinAny<af__set,aoShl>(this,thisExpr,args);
               break;
            case aoShr:
               replace = new ArrayBuiltinAny<af__set,aoShr>(this,thisExpr,args);
               break;
            case aoUShr:
               replace = new ArrayBuiltinAny<af__set,aoUShr>(this,thisExpr,args);
               break;
            case aoMod:
               replace = new ArrayBuiltinAny<af__set,aoMod>(this,thisExpr,args);
               break;
      
            default: ;
               printf("make setter %d\n", op);
               throw "setter not implemented";
         }

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
               replace = new ArrayBuiltinAny<af__crement,coPreInc>(this,thisExpr,args);
               break;
            case coPostInc :
               replace = new ArrayBuiltinAny<af__crement,coPostInc>(this,thisExpr,args);
               break;
            case coPreDec :
               replace = new ArrayBuiltinAny<af__crement,coPreDec>(this,thisExpr,args);
               break;
            case coPostDec :
               replace = new ArrayBuiltinAny<af__crement,coPostDec>(this,thisExpr,args);
               break;
            default:
               return 0;
         }
         return replace;
      }
      return 0;
   }


   inline ExprType inlineGetType()
   {
      switch(FUNC)
      {
         case afJoin:
         case afToString:
            return etString;

         case afSort:
         case afInsert:
         case afUnshift:
            return etVoid;

         case afPush:
         case afRemove:
         case afIndexOf:
         case afLastIndexOf:
            return etInt;

         default:
            return etObject;
      }

      return etObject;
   }
   ExprType getType() { return inlineGetType(); }




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
      case arrObject:
         return new ArrayBuiltin<Dynamic,BUILTIN,CREMENT>(inSrc, thisExpr, args);
      case arrAny:
         return new ArrayBuiltinAny<BUILTIN,CREMENT::OP>(inSrc, thisExpr, args);
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
   if (field==HX_CSTRING("indexOf"))
      return TCreateArrayBuiltin<afIndexOf,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("lastIndexOf"))
      return TCreateArrayBuiltin<afLastIndexOf,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__get"))
      return TCreateArrayBuiltin<af__get,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__set"))
      return TCreateArrayBuiltin<af__set,NoCrement>(src, inType, inThisExpr, ioExpressions);

   return 0;
}






























} // end namespace hx
