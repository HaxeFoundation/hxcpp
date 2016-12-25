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
   af__SetSizeExact,
};

static const char *sFuncNames[] =
{
   "afConcat",
   "afCopy",
   "afInsert",
   "afIterator",
   "afJoin",
   "afPop",
   "afPush",
   "afRemove",
   "afReverse",
   "afShift",
   "afSlice",
   "afSplice",
   "afSort",
   "afToString",
   "afUnshift",
   "afMap",
   "afFilter",
   "afIndexOf",
   "afLastIndexOf",
   "af__get",
   "af__set",
   "af__crement",
   "af__SetSizeExact",
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
   1, //af__SetSizeExact,
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


#ifdef CPPIA_JIT
static ArrayBase * SLJIT_CALL array_expand_size(ArrayBase *inArray, int inSize)
{
   inArray->Realloc(inSize+1);
   return inArray;
}

#endif
 



template<typename ELEM, typename FUNC>
struct ArraySetter : public ArrayBuiltinBase
{
   ArraySetter(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }
   const char *getName() { return "ArraySetter"; }
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

   #ifdef CPPIA_JIT

   static void * SLJIT_CALL nativeRunInt(ELEM *base, int index, int inValue)
   {
      ELEM *ptr = base + index;
      FUNC::apply(*ptr, inValue);
      return ptr;
   }

   static void * SLJIT_CALL nativeRunFloat(ELEM *base, int index, double *inValue)
   {
      ELEM *ptr = base + index;
      FUNC::apply(*ptr, *inValue);
      return ptr;
   }

   static void * SLJIT_CALL nativeRunObject(ELEM *base, int index, hx::Object *inValue)
   {
      ELEM *ptr = base + index;
      FUNC::apply(*ptr, Dynamic(inValue) );
      return ptr;
   }

   static void * SLJIT_CALL nativeRunString(ELEM *base, int index, String *inValue)
   {
      ELEM *ptr = base + index;
      FUNC::apply(*ptr, *inValue );
      return ptr;
   }


   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
      ExprType rightHandType;
      switch(FUNC::op)
      {
         case aoMult:
         case aoDiv:
         case aoSub:
         case aoMod:
            rightHandType = etFloat;
            break;

         case aoAnd:
         case aoOr:
         case aoXOr:
         case aoShl:
         case aoShr:
         case aoUShr:
            rightHandType = etInt;

         default:
            rightHandType = elemType;
      }

      JitTemp thisVal(compiler,jtPointer);
      thisExpr->genCode(compiler, thisVal, etObject);

      // sJitTemp1 = index
      JitTemp index(compiler,jtInt);
      args[0]->genCode(compiler, index, etInt);

      // Right-hand-size
      JitTemp value(compiler,getJitType(rightHandType));
      args[1]->genCode(compiler, value, rightHandType);

      compiler->move(sJitTemp1,index);

      // sJitTemp0 = this
      compiler->move(sJitTemp0, thisVal);


      // Check length..
      JumpId lengthOk = compiler->compare( cmpI_LESS, sJitTemp1.as(jtInt),
                                         sJitTemp0.star(jtInt, ArrayBase::lengthOffset()) );


      JumpId enough = compiler->compare( cmpI_LESS, sJitTemp1.as(jtInt),
                                         sJitTemp0.star(jtInt, ArrayBase::allocOffset()) );
      // Make some room
      compiler->callNative( (void *)array_expand_size,  sJitTemp0.as(jtPointer), sJitTemp1.as(jtInt) );
      // sJitTemp0 is still this, restore length...
      compiler->move( sJitTemp1,index);

      compiler->comeFrom(enough);

      // length = index + 1
      compiler->add(sJitTemp0.star(jtInt, ArrayBase::lengthOffset()), sJitTemp1.as(jtInt), (int)1);


      compiler->comeFrom(lengthOk);

      // sJitTemp0 = this,
      // sJitTemp1 = index,
      // value = right hand side

      // sJitTemp0 = this->base
      compiler->move(sJitTemp0, sJitTemp0.star(jtPointer, ArrayBase::baseOffset()).as(jtPointer) );

      if (FUNC::op==aoSet)
      {
         if (sizeof(ELEM)==1) // uchar, bool
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1).as(jtByte), value );
         }
         else if (elemType==etString)
         {
            compiler->mult( sJitTemp1, sJitTemp1.as(jtInt), (int)sizeof(String), false );
            compiler->add( sJitTemp0, sJitTemp0.as(jtPointer), sJitTemp1);
            compiler->move( sJitTemp0.star(jtInt), value.as(jtInt) );
            compiler->move( sJitTemp0.star(jtPointer,sizeof(int)), value.as(jtPointer) + sizeof(int) );
         }
         else if (sizeof(ELEM)==2)
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1,1), value );
         }
         else if (sizeof(ELEM)==4)
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1,2), value );
         }
         else if (sizeof(ELEM)==8)
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1,3), value );
         }
         else
         {
            printf("Unknown element size\n");
         }


         if (destType!=etVoid && destType!=etNull)
         {
            compiler->convert( value, rightHandType, inDest, destType );
         }
      }
      else
      {
         switch(rightHandType)
         {
            case etInt:
               compiler->callNative( (void *)nativeRunInt, sJitTemp0, sJitTemp1, value.as(jtInt) );
               break;
            case etFloat:
               compiler->callNative( (void *)nativeRunFloat, sJitTemp0, sJitTemp1, value.as(jtFloat) );
               break;
            case etString:
               compiler->callNative( (void *)nativeRunString, sJitTemp0, sJitTemp1, value.as(jtString) );
               break;
            case etObject:
               compiler->callNative( (void *)nativeRunObject, sJitTemp0, sJitTemp1, value.as(jtPointer) );
               break;
         }

         if (destType!=etVoid && destType!=etNull)
         {
            JitType ptrType = getJitType(elemType);

            if (sizeof(ELEM)==1)
                ptrType = jtByte;
            else if (sizeof(ELEM)==2)
                ptrType = jtShort;
            compiler->convert( sJitReturnReg.star(ptrType), elemType, inDest, destType );
         }
      }
   }
   #endif
};

#ifdef CPPIA_JIT
static ArrayBase * SLJIT_CALL array_expand(ArrayBase *inArray)
{
  inArray->Realloc( inArray->length + 1 );
  return inArray;
}
static hx::Object *SLJIT_CALL objGetIndex(hx::Object *inArray, int inIndex)
{
   return Dynamic(inArray->__GetItem(inIndex)).mPtr;
}
#endif
 

template<typename T>
struct ExprBaseTypeOf { typedef hx::Object *Base ; };
template<> struct ExprBaseTypeOf<int> { typedef int Base ; };
template<> struct ExprBaseTypeOf<unsigned char> { typedef int Base ; };
template<> struct ExprBaseTypeOf<bool> { typedef int Base ; };
template<> struct ExprBaseTypeOf<Float> { typedef double &Base ; };
template<> struct ExprBaseTypeOf<String> { typedef String &Base ; };



template<typename ELEM, int FUNC, typename CREMENT>
struct ArrayBuiltin : public ArrayBuiltinBase
{
   bool unsafe;

   ArrayBuiltin(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions, bool inUnsafe)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
      unsafe = inUnsafe;
   }
   const char *getName() { return sFuncNames[FUNC]; }

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
         case af__SetSizeExact:
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
      if (FUNC==af__SetSizeExact)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int size = args[0]->runInt(ctx);
         BCR_VCHECK;
         thisVal->__SetSizeExact(size);
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
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPreInc>(this,thisExpr,args,false);
               break;
            case coPostInc :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPostInc>(this,thisExpr,args,false);
               break;
            case coPreDec :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPreDec>(this,thisExpr,args,false);
               break;
            case coPostDec :
               replace = new ArrayBuiltin<ELEM,af__crement,CrementPostDec>(this,thisExpr,args,false);
               break;
            default:
               return 0;
         }
         return replace;
      }
      return 0;
   }

   #ifdef CPPIA_JIT
   static ArrayBase * SLJIT_CALL array_expand(ArrayBase *inArray)
   {
      inArray->Realloc( inArray->length + 1 );
      return inArray;
   }
   static void SLJIT_CALL runSort( Array_obj<ELEM> *inArray, hx::Object *inFunc)
   {
      TRY_NATIVE
      inArray->sort( Dynamic(inFunc) );
      CATCH_NATIVE
   }
   static void SLJIT_CALL runJoin( Array_obj<ELEM> *inArray, String *ioValue)
   {
      TRY_NATIVE
      *ioValue = inArray->join( *ioValue );
      CATCH_NATIVE
   }

   static void SLJIT_CALL runElem( Array_obj<ELEM> *inArray, int inIndex)
   {
      CREMENT::run( Array<ELEM>(inArray)[inIndex] );
   }

   static int SLJIT_CALL runIntCrement( Array_obj<ELEM> *inArray, int inIndex)
   {
      return ValToInt( CREMENT::run( Array<ELEM>(inArray)[inIndex] ) );
   }

   static void SLJIT_CALL runFloatCrement( Array_obj<ELEM> *inArray, int inIndex, double *d)
   {
      *d = ValToFloat( CREMENT::run( Array<ELEM>(inArray)[inIndex] ) );
   }

   static hx::Object * SLJIT_CALL runObjCrement( Array_obj<ELEM> *inArray, int inIndex)
   {
      return Dynamic( CREMENT::run( Array<ELEM>(inArray)[inIndex] ) ).mPtr;
   }

   static int SLJIT_CALL runRemove( Array_obj<ELEM> *inArray, typename ExprBaseTypeOf<ELEM>::Base inBase)
   {
      return inArray->remove(inBase);
   }

   static int SLJIT_CALL runIndex( Array_obj<ELEM> *inArray, typename ExprBaseTypeOf<ELEM>::Base inBase, hx::Object *pos)
   {
      if (FUNC==afIndexOf)
         return inArray->indexOf(inBase,pos);
      else
         return inArray->lastIndexOf(inBase,pos);
   }

   static void SLJIT_CALL runInsert( Array_obj<ELEM> *inArray, int pos, typename ExprBaseTypeOf<ELEM>::Base inBase)
   {
      inArray->insert(pos, inBase);
   }

   // TODO - string
   static hx::Object * SLJIT_CALL runShift( Array_obj<ELEM> *inArray )
   {
      return Dynamic(inArray->shift()).mPtr;
   }

   static hx::Object * SLJIT_CALL runSlice( Array_obj<ELEM> *inArray, int pos, hx::Object *end )
   {
      return inArray->slice(pos, end).mPtr;
   }

   static hx::Object * SLJIT_CALL runSplice( Array_obj<ELEM> *inArray, int pos, int len )
   {
      return inArray->splice(pos, len).mPtr;
   }

   static void SLJIT_CALL runRemoveRange( Array_obj<ELEM> *inArray, int pos, int len )
   {
      inArray->removeRange(pos, len);
   }

   static void SLJIT_CALL runReverse( Array_obj<ELEM> *inArray )
   {
      inArray->reverse();
   }

   static hx::Object *SLJIT_CALL runConcat( Array_obj<ELEM> *inArray, Array_obj<ELEM> *inOther )
   {
      return inArray->concat(inOther).mPtr;
   }


   static hx::Object *SLJIT_CALL runGetIteratator( Array_obj<ELEM> *inArray )
   {
      return inArray->iterator().mPtr;
   }

   static void SLJIT_CALL runSetSizeExact( Array_obj<ELEM> *inArray, int size )
   {
      inArray->__SetSizeExact(size);
   }

   static void SLJIT_CALL runToString( Array_obj<ELEM> *inArray, String *outString )
   {
      TRY_NATIVE
      *outString = inArray->toString();
      CATCH_NATIVE
   }

   static hx::Object * SLJIT_CALL runCopy( Array_obj<ELEM> *inArray)
   {
      return inArray->copy().mPtr;
   }

   static hx::Object * SLJIT_CALL runProcess( Array_obj<ELEM> *inArray, hx::Object *inFunction)
   {
      TRY_NATIVE
      if (FUNC==afMap)
      {
         Array<ELEM> result = inArray->map(inFunction);
         return result.mPtr;
      }
      else
      {
         Array<ELEM> result = inArray->filter(inFunction);
         return result.mPtr;
      }
      CATCH_NATIVE
      return 0;
   }


   static bool isBoolElem() { return ExprTypeIsBool<ELEM>::value; }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      // TODO - null check
      switch(FUNC)
      {
         case afPush:
            {
               thisExpr->genCode(compiler, sJitTemp0, etObject);
               JumpId enough = compiler->compare( cmpI_LESS, sJitTemp0.star(jtInt, ArrayBase::lengthOffset()),
                                                             sJitTemp0.star(jtInt, ArrayBase::allocOffset()) );

               // result comes back in sJitTemp0
               compiler->callNative( (void *)array_expand,  sJitTemp0 );
               compiler->comeFrom(enough);

               JitTemp arrayThis(compiler,jtPointer);
               compiler->move(arrayThis, sJitTemp0);

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp tempVal(compiler,getJitType(elemType));
               args[0]->genCode(compiler, tempVal, elemType);

               // sJitTemp1 = this
               compiler->move(sJitTemp1, arrayThis);
               // sJitTemp0 = length
               compiler->move(sJitTemp0, sJitTemp1.star(jtInt, ArrayBase::lengthOffset()) );
               // this->length = length+1
               compiler->add(sJitTemp1.star(jtInt, ArrayBase::lengthOffset()), sJitTemp0, (int)1 );

               // sJitTemp1 = this->base
               compiler->move(sJitTemp1, sJitTemp1.star(jtPointer, ArrayBase::baseOffset()).as(jtPointer) );

               if (sizeof(ELEM)==1) // uchar, bool
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0).as(jtByte), tempVal );
               }
               else if (elemType==etString)
               {
                  compiler->mult( sJitTemp0, sJitTemp0.as(jtInt), (int)sizeof(String), false );
                  compiler->add( sJitTemp0, sJitTemp1.as(jtPointer), sJitTemp0);

                  compiler->move( sJitTemp0.star(jtInt), tempVal.as(jtInt) );
                  compiler->move( sJitTemp0.star(jtPointer,sizeof(int)), tempVal.as(jtPointer) + sizeof(int) );
               }
               else if (sizeof(ELEM)==2)
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0,1), tempVal );
               }
               else if (sizeof(ELEM)==4)
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0,2), tempVal );
               }
               else if (sizeof(ELEM)==8)
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0,3), tempVal );
               }
               else
               {
                  printf("Unknown element size\n");
               }

               break;
            }

         case af__set:
            {
               JitTemp thisVal(compiler, jtPointer);
               JitTemp index(compiler, jtPointer);
               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp tempVal(compiler, elemType);

               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, index, etInt);
               args[1]->genCode(compiler, tempVal, elemType);

               compiler->move(sJitTemp0.as(jtPointer), thisVal);
               compiler->move(sJitTemp1.as(jtInt), index);
               if (!unsafe)
               {
                  JumpId enoughLength = compiler->compare( cmpI_LESS, sJitTemp1.as(jtInt),
                                                                 sJitTemp0.star(jtInt, ArrayBase::lengthOffset()) );

                  // Not in length, but maybe enough range
                  JumpId enoughAlloc = compiler->compare( cmpI_LESS, sJitTemp1.as(jtInt),
                                                                     sJitTemp0.star(jtInt, ArrayBase::allocOffset()) );

                  // result comes back in sJitTemp0
                  compiler->callNative( (void *)array_expand_size,  sJitTemp0, sJitTemp1 );
                  // but need to reload sJitTemp1
                  compiler->move(sJitTemp1.as(jtInt), index);

                  compiler->comeFrom(enoughAlloc);
                  // Set length = index+1
                  compiler->add( sJitTemp0.star(jtInt, ArrayBase::lengthOffset()), sJitTemp1.as(jtInt), 1 );

                  compiler->comeFrom(enoughLength);
               }

               // sJitTemp0 = array
               // sJitTemp1 = index
               //  length, alloc have been checked


               // sJitTemp0 = this->base
               compiler->move(sJitTemp0, sJitTemp0.star(jtPointer, ArrayBase::baseOffset()).as(jtPointer) );

               if (sizeof(ELEM)==1) // uchar, bool
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1).as(jtByte), tempVal );
               }
               else if (elemType==etString)
               {
                  compiler->mult( sJitTemp1, sJitTemp1.as(jtInt), (int)sizeof(String), false );
                  compiler->add( sJitTemp0, sJitTemp0.as(jtPointer), sJitTemp1);

                  compiler->move( sJitTemp0.star(jtInt), tempVal.as(jtInt) );
                  compiler->move( sJitTemp0.star(jtPointer,sizeof(int)), tempVal.as(jtPointer) + sizeof(int) );
               }
               else if (sizeof(ELEM)==2)
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1,1), tempVal );
               }
               else if (sizeof(ELEM)==4)
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1,2), tempVal );
               }
               else if (sizeof(ELEM)==8)
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1,3), tempVal );
               }
               else
               {
                  printf("Unknown element size\n");
               }

               break;
            }




         case af__get:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               if (destType==etObject)
               {
                  args[0]->genCode(compiler, sJitArg1, etInt);
                  compiler->callNative((void *)objGetIndex, thisVal, sJitArg1.as(jtInt));
                  compiler->convertReturnReg(etObject, inDest, destType);
                  return;
               }

               // sJitTemp0 = index
               args[0]->genCode(compiler, sJitTemp0, etInt);

               if (destType==etNull || destType==etVoid)
                  return;

               // sJitTemp1 = this
               compiler->move(sJitTemp1, thisVal);


               JumpId writtenNull = 0;
               if (!unsafe)
               {
                  // Check length..
                  JumpId lengthOk = compiler->compare( cmpI_LESS, sJitTemp0.as(jtInt),
                                                     sJitTemp1.star(jtInt, ArrayBase::lengthOffset()) );
                  // Out of bounds - return null / zero
                  compiler->returnNull(inDest, destType);
                  writtenNull = compiler->jump();


                  compiler->comeFrom(lengthOk);
               }

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;

               // sJitTemp0 = index
               // sJitTemp1 = this

               compiler->move(sJitTemp1, sJitTemp1.star(jtPointer, ArrayBase::baseOffset()).as(jtPointer) );
               // sJitTemp1 = this->base


               if (sizeof(ELEM)==1) // uchar, bool
               {
                  if (destType!=etInt || isMemoryVal(inDest) )
                  {
                     compiler->move( sJitTemp1.as(jtInt), sJitTemp1.atReg(sJitTemp0).as(jtByte) );
                     compiler->convert(sJitTemp1, etInt, inDest, destType);
                  }
                  else
                     compiler->move( inDest.as(jtInt),  sJitTemp1.atReg(sJitTemp0).as(jtByte) );
               }
               else if (elemType==etString)
               {
                  compiler->mult( sJitTemp0.as(jtInt), sJitTemp0.as(jtInt), (int)sizeof(String), false );
                  compiler->add( sJitTemp0.as(jtPointer), sJitTemp1.as(jtPointer), sJitTemp0);
                  compiler->convert( sJitTemp0.star(jtString), etString, inDest, destType );
               }
               else if (sizeof(ELEM)==2)
               {
                  if (destType!=etInt || isMemoryVal(inDest))
                  {
                     compiler->move(sJitTemp1.as(jtInt),sJitTemp1.atReg(sJitTemp0,1).as(jtShort));
                     compiler->convert(sJitTemp1,etInt, inDest, destType);
                  }
                  else
                     compiler->move( inDest.as(jtInt),  sJitTemp1.atReg(sJitTemp0).as(jtShort) );
               }
               else if (sizeof(ELEM)==4)
               {
                  compiler->convert( sJitTemp1.atReg(sJitTemp0,2), elemType, inDest, destType );
               }
               else if (sizeof(ELEM)==8)
               {
                  compiler->convert( sJitTemp1.atReg(sJitTemp0,3), elemType, inDest, destType );
               }
               else
               {
                  printf("Unknown element size\n");
               }
               compiler->comeFrom(writtenNull);

               break;
            }


         case afPop:
            {
               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitType jt = getJitType(elemType);

               // sJitTemp1 = this
               thisExpr->genCode(compiler, sJitTemp1.as(jtPointer), etObject);
               // sJitTemp0 = length
               compiler->move( sJitTemp0.as(jtInt), sJitTemp1.star(jtInt, ArrayBase::lengthOffset()) );

                  // Check length > 0 ...
               JumpId lengthOk = compiler->compare( cmpI_NOT_ZERO, sJitTemp0.as(jtInt), 0, 0);

               // Out of bounds - return null / zero
               compiler->returnNull(inDest, destType);
               JumpId writtenNull = compiler->jump();


               // sJitTemp1 = this
               //  index has been checked
               compiler->comeFrom(lengthOk);

               // sJitTemp0 = length
               compiler->sub( sJitTemp0.as(jtInt), sJitTemp0.as(jtInt), 1, false );
               // sJitTemp0 = index
               // Store reduced length...
               compiler->move(  sJitTemp1.star(jtInt, ArrayBase::lengthOffset()), sJitTemp0.as(jtInt) );

               // sJitTemp0 = index
               // sJitTemp1 = this

               compiler->move(sJitTemp1, sJitTemp1.star(jtPointer, ArrayBase::baseOffset()).as(jtPointer) );
               // sJitTemp1 = this->base


               if (sizeof(ELEM)==1 || sizeof(ELEM)==2 || elemType==etInt || elemType==etObject) // int-like
               {
                  int shift = sizeof(ELEM)==1 ? 0 : sizeof(ELEM)==2 ? 1 :  sizeof(ELEM)==4 ? 2 : 3;
                  JitType regType = elemType==etObject ? jtPointer : jtInt;
                  JitType jt = sizeof(ELEM)==1 ? jtByte : sizeof(ELEM)==2 ? jtShort : regType;
                  JitVal zero = elemType==etObject ? JitVal((void *)0) : JitVal(0);
                  if (destType==etNull || destType==etVoid)
                  {
                     compiler->move( sJitTemp1.atReg(sJitTemp0,shift).as(jt), zero );
                  }
                  else
                  {
                     compiler->move( sJitTemp2.as(regType), sJitTemp1.atReg(sJitTemp0,shift).as(jt) );
                     // zero out value...
                     compiler->move( sJitTemp1.atReg(sJitTemp0,shift).as(jt), zero );
                     compiler->convert(sJitTemp2.as(regType), elemType, inDest, destType);
                  }
               }
               else if (elemType==etString)
               {
                  compiler->mult( sJitTemp0, sJitTemp0.as(jtInt), (int)sizeof(String), false );
                  compiler->add( sJitTemp0, sJitTemp1.as(jtPointer), sJitTemp0);

                  if (destType==etNull || destType==etVoid)
                  {
                     compiler->move( sJitTemp0.star(jtInt),  0 );
                     compiler->move( sJitTemp0.star(jtPointer) + 4,  0 );
                  }
                  else
                  {
                     JitTemp strVal(compiler,jtString);
                     compiler->convert(  sJitTemp0.star(jtString), etString, strVal, etString );
                     compiler->move( sJitTemp0.star(jtInt),  0 );
                     compiler->move( sJitTemp0.star(jtPointer) + 4,  0 );
                     compiler->convert( strVal, etString, inDest, destType );
                  }
               }
               else if (sizeof(ELEM)==8)
               {
                  if (destType==etVoid || destType==etNull)
                  {
                     if (sizeof(void*)==8)
                        compiler->move( sJitTemp1.atReg(sJitTemp0,3).as(jtPointer), (void *)0 );
                     else
                     {
                        compiler->bitOp(bitOpShiftL, sJitTemp0, sJitTemp0, 3);
                        compiler->move( sJitTemp1.atReg(sJitTemp0).as(jtInt), 0 );
                        compiler->add(sJitTemp0, sJitTemp0, 4);
                        compiler->move( sJitTemp1.atReg(sJitTemp0).as(jtInt), 0 );
                     }
                  }
                  else
                  {
                     // sJitTempF0 ?
                     JitType jt = getJitType(elemType);
                     JitTemp tmp(compiler, elemType);
                     compiler->move( tmp, sJitTemp1.atReg(sJitTemp0,3).as(jt) );

                     if (sizeof(void*)==8)
                        compiler->move( sJitTemp1.atReg(sJitTemp0,3).as(jtPointer), 0 );
                     else
                     {
                        compiler->bitOp(bitOpShiftL, sJitTemp0, sJitTemp0, 3);
                        compiler->move( sJitTemp1.atReg(sJitTemp0).as(jtInt), 0 );
                        compiler->add(sJitTemp0, sJitTemp0, 4);
                        compiler->move( sJitTemp1.atReg(sJitTemp0).as(jtInt), 0 );
                     }

                     compiler->convert(tmp,elemType, inDest, destType);
                  }
               }
               else
               {
                  printf("Unknown element size\n");
               }
               compiler->comeFrom(writtenNull);

               break;
            }

         case afSort:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitTemp1, etObject);
               compiler->callNative( (void *)runSort, thisVal, sJitTemp1 );
               compiler->checkException();
               break;
            }

         case afJoin:
            {
               JitTemp thisVal(compiler, jtPointer);
               JitTemp value(compiler, jtString);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, value, etString);
               compiler->callNative( (void *)runJoin, thisVal, value );
               compiler->checkException();
               compiler->convert(value,etString, inDest, destType);
               break;
            }


         case afRemove:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp val(compiler,getJitType(elemType));
               args[0]->genCode(compiler, val, elemType);

               compiler->callNative( (void *)runRemove, thisVal, val );

               compiler->convertReturnReg(etInt, inDest, destType, true);
               break;
            }


         case afIndexOf:
         case afLastIndexOf:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp val(compiler,getJitType(elemType));
               args[0]->genCode(compiler, val, elemType);

               args[1]->genCode(compiler, sJitArg2.as(jtPointer), etObject);

               compiler->callNative( (void *)runIndex, thisVal, val, sJitArg2.as(jtPointer) );

               compiler->convertReturnReg(etInt, inDest, destType);
               break;
            }

   
            case afIterator:
            {
               thisExpr->genCode(compiler, sJitTemp0, etObject);
               compiler->callNative( (void *)runGetIteratator, sJitTemp0.as(jtPointer) );
               compiler->convertReturnReg(etObject, inDest, destType);
               break;
            }

         case afInsert:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               JitTemp pos(compiler, jtInt);
               args[0]->genCode(compiler, pos, etInt);

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp val(compiler,getJitType(elemType));
               args[1]->genCode(compiler, val, elemType);

               compiler->callNative( (void *)runInsert, thisVal, pos, val );
               break;
            }

         case afConcat:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);
               compiler->callNative( (void *)runConcat, thisVal, sJitArg1.as(jtPointer));
               compiler->convertReturnReg(etObject, inDest, destType);
               break;
            }

         case af__SetSizeExact:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
               compiler->callNative( (void *)runSetSizeExact, thisVal, sJitArg1.as(jtInt));
               break;
            }

         case af__crement:
            {
               CrementOp op = (CrementOp)CREMENT::OP;

               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               // sJitTemp0 = index
               args[0]->genCode(compiler, sJitTemp1, etInt);

               if (destType==etVoid || destType==etNull)
               {
                  compiler->callNative( (void *)runElem, thisVal, sJitTemp1 );
               }
               else if (destType==etInt)
               {
                  compiler->callNative( (void *)runIntCrement, thisVal, sJitTemp1 );
                  compiler->move( inDest.as(jtInt), sJitReturnReg.as(jtInt) );
               }
               else if (destType==etFloat)
               {
                  JitTemp tempFloat(compiler, jtFloat);
                  JitVal dVal = isMemoryVal(inDest) ? inDest : tempFloat;
                  compiler->callNative( (void *)runFloatCrement, thisVal, sJitTemp1 );
                  if (!isMemoryVal(inDest))
                     compiler->move(inDest.as(jtFloat), tempFloat);
               }
               else
               {
                  compiler->callNative( (void *)runObjCrement, thisVal, sJitTemp1 );
                  compiler->convertReturnReg(etObject, inDest, destType);
               }
            }
            break;

         case afReverse:
            thisExpr->genCode(compiler, sJitArg0.as(jtPointer), etObject);
            compiler->callNative( (void *)runReverse, sJitArg0.as(jtPointer) );
            break;

         case afShift:
            thisExpr->genCode(compiler, sJitArg0.as(jtPointer), etObject);
            compiler->callNative( (void *)runShift, sJitArg0.as(jtPointer) );
            compiler->convertReturnReg(etObject, inDest, destType, isBoolElem() );
            break;


         case afSlice:
            {
            JitTemp thisVal(compiler,jtPointer);
            thisExpr->genCode(compiler, thisVal, etObject);

            JitTemp pos(compiler,jtInt);
            args[0]->genCode(compiler, pos, etInt);

            args[1]->genCode(compiler, sJitArg2.as(jtPointer), etObject);

            compiler->callNative( (void *)runSlice, thisVal, pos, sJitArg2.as(jtPointer) );

            compiler->convertReturnReg(etObject, inDest, destType);
            }
            break;


         case afSplice:
            {
            JitTemp thisVal(compiler,jtPointer);
            thisExpr->genCode(compiler, thisVal, etObject);

            JitTemp pos(compiler,jtInt);
            args[0]->genCode(compiler, pos, etInt);

            args[1]->genCode(compiler, sJitArg2.as(jtInt), etInt);

            if (destType==etNull || destType==etVoid)
               compiler->callNative( (void *)runRemoveRange, thisVal, pos, sJitArg2.as(jtInt) );
            else
            {
               compiler->callNative( (void *)runSplice, thisVal, pos, sJitArg2.as(jtInt) );
               compiler->convertReturnReg(etObject, inDest, destType);
            }

            }
            break;


         case afToString:
            thisExpr->genCode(compiler, sJitArg0, etObject);
            if (destType==etString)
            {
               compiler->callNative( (void *)runToString, sJitArg0, inDest);
               compiler->checkException();
            }
            else
            {
               JitTemp result(compiler, jtString);
               compiler->callNative( (void *)runToString, sJitArg0, result);
               compiler->checkException();
               compiler->convert(result, etString, inDest, destType );
            }
            break;


         case afCopy:
            thisExpr->genCode(compiler, sJitArg0, etObject);
            compiler->callNative( (void *)runCopy, sJitArg0);
            compiler->convertReturnReg(etObject, inDest, destType );
            break;

         case afMap:
         case afFilter:
            {
            JitTemp thisVal(compiler,jtPointer);
            thisExpr->genCode(compiler, thisVal, etObject);
            args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);

            compiler->callNative( (void *)runProcess, thisVal, sJitArg1.as(jtPointer));
            compiler->checkException();
            compiler->convertReturnReg(etObject, inDest, destType );
            }
            break;



         default:
            compiler->traceStrings("ArrayBuiltin::",sFuncNames[FUNC]);
      }
   }
   #endif
};


#if (HXCPP_API_LEVEL>=330)
  #define BasePtr(x) x
  typedef cpp::VirtualArray_obj ArrayAnyImpl;
  #define CALL(x) x
#else
  #define BasePtr(x) x.mPtr
  typedef ArrayBase ArrayAnyImpl;
  #define CALL(x) __##x
#endif


#ifdef CPPIA_JIT
static hx::Object * SLJIT_CALL objGetItem(hx::Object *inObj, int inIndex)
{
   return inObj->__GetItem(inIndex).mPtr;
}
static int SLJIT_CALL arrayRemove(ArrayAnyImpl *inObj, hx::Object *inValue)
{
   return inObj->remove(inValue);
}
static hx::Object * SLJIT_CALL arrayConcat(ArrayAnyImpl *inObj, hx::Object *inValue)
{
   return inObj->concat(Dynamic(inValue)).mPtr;
}
static void SLJIT_CALL arraySetSizeExact(ArrayAnyImpl *inObj, int inSize)
{
   inObj->__SetSizeExact(inSize);
}


static hx::Object * SLJIT_CALL arraySplice(ArrayAnyImpl *inObj, hx::Object *a0, hx::Object *a1)
{
   return inObj->splice( Dynamic(a0), Dynamic(a1) ).mPtr;
}
static hx::Object * SLJIT_CALL arraySlice(ArrayAnyImpl *inObj, hx::Object *a0, hx::Object *a1)
{
   return inObj->slice( Dynamic(a0), Dynamic(a1) ).mPtr;
}
static hx::Object * SLJIT_CALL arrayPop(ArrayAnyImpl *inObj)
{
   return inObj->pop().mPtr;
}

static void SLJIT_CALL runSort( ArrayAnyImpl *inArray, hx::Object *inFunc)
{
   TRY_NATIVE
   inArray->sort( Dynamic(inFunc) );
   CATCH_NATIVE
}
static void SLJIT_CALL runJoin( ArrayAnyImpl *inArray, String *ioValue)
{
   TRY_NATIVE
   *ioValue = inArray->join( *ioValue );
   CATCH_NATIVE
}



static int SLJIT_CALL arrayPushInt( ArrayAnyImpl *inArray, int inVal)
{
   return inArray->push(inVal);
}
static int SLJIT_CALL arrayPushObject( ArrayAnyImpl *inArray, hx::Object *inVal)
{
   return inArray->push(Dynamic(inVal));
}
static int SLJIT_CALL arrayPushFloat( ArrayAnyImpl *inArray, double *inVal)
{
   return inArray->push(*inVal);
}
static int SLJIT_CALL arrayPushString( ArrayAnyImpl *inArray, String *inVal)
{
   return inArray->push(*inVal);
}



static int SLJIT_CALL arraySetInt( ArrayAnyImpl *inArray, int inIndex, int inVal)
{
   inArray->set(inIndex,inVal);
   return inVal;
}
static hx::Object * SLJIT_CALL arraySetObject( ArrayAnyImpl *inArray, int inIndex, hx::Object *inVal)
{
   inArray->set(inIndex,Dynamic(inVal));
   return inVal;
}
static void SLJIT_CALL arraySetFloat( ArrayAnyImpl *inArray, int inIndex, double *inVal)
{
   inArray->set(inIndex,*inVal);
}
static void SLJIT_CALL arraySetString( ArrayAnyImpl *inArray, int inIndex, String *inVal)
{
   inArray->set(inIndex,*inVal);
}



static void SLJIT_CALL arrayUnshiftInt( ArrayAnyImpl *inArray, int inVal)
{
   inArray->unshift(inVal);
}
static void SLJIT_CALL arrayUnshiftObject( ArrayAnyImpl *inArray, hx::Object *inVal)
{
   inArray->unshift(Dynamic(inVal));
}
static void SLJIT_CALL arrayUnshiftFloat( ArrayAnyImpl *inArray, double *inVal)
{
   inArray->unshift(*inVal);
}
static void SLJIT_CALL arrayUnshiftString( ArrayAnyImpl *inArray, String *inVal)
{
   inArray->unshift(*inVal);
}




#endif



template<int FUNC,int OP>
struct ArrayBuiltinAny : public ArrayBuiltinBase
{



   ArrayBuiltinAny(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }
   const char *getName() { return sFuncNames[FUNC]; }

   int  runInt(CppiaCtx *ctx)
   {
      if (FUNC==afPush)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object * val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(push)(Dynamic(val));
      }
      if (FUNC==afRemove)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object * val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(remove)(val);
      }
      if (FUNC==afIndexOf)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         hx::Object *a1 = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(indexOf)( a0, a1 );
      }
      if (FUNC==afLastIndexOf)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         hx::Object *a1 = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(lastIndexOf)( a0, a1 );
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
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(join)( a0 );
      }

      if (FUNC==afToString)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         return thisVal->toString();
      }

      hx::Object *obj = runObject(ctx);
      return obj ? obj->toString() : ::String();
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (FUNC==afPush || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
         return Dynamic(runInt(ctx)).mPtr;

      if (FUNC==afRemove)
         return Dynamic((bool)runInt(ctx)).mPtr;

      if (FUNC==afJoin || FUNC==afToString)
         return Dynamic(runString(ctx)).mPtr;

      if (FUNC==afSort || FUNC==afInsert || FUNC==afUnshift || FUNC==af__SetSizeExact)
      {
         runVoid(ctx);
         return 0;
      }


      ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
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
         if (OP==aoSet)
         {
            Dynamic val = args[1]->runObject(ctx);
            BCR_CHECK;
            return thisVal->__SetItem(i, val).mPtr;
         }

         Dynamic orig = thisVal->__GetItem(i);
         CPPIA_CHECK(orig.mPtr);

         Dynamic val = args[1]->runObject(ctx);
         BCR_CHECK;
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
         return thisVal->CALL(pop)().mPtr;
      }
      if (FUNC==afShift)
      {
         return thisVal->CALL(shift)().mPtr;
      }

      if (FUNC==afConcat)
      {
         Dynamic val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(concat)(val).mPtr;
      }
      if (FUNC==afCopy)
      {
         return thisVal->CALL(copy)().mPtr;
      }
      if (FUNC==afSplice)
      {
         Dynamic pos = args[0]->runObject(ctx);
         BCR_CHECK;
         Dynamic end = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(splice)(pos,end).mPtr;
      }
      if (FUNC==afSlice)
      {
         Dynamic pos = args[0]->runObject(ctx);
         BCR_CHECK;
         Dynamic end = args[1]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(slice)(pos,end).mPtr;
      }
      if (FUNC==afMap)
      {
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(map)(a0).mPtr;
      }
      if (FUNC==afFilter)
      {
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(filter)(a0).mPtr;
      }

      if (FUNC==afIterator)
      {
         return thisVal->CALL(iterator)().mPtr;
      }

      return 0;
   }
   void runVoid(CppiaCtx *ctx)
   {
      if (FUNC==afSort)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         Dynamic a0 = args[0]->runObject(ctx);
         BCR_VCHECK;

         thisVal->CALL(sort)(a0);
         return;
      }
      if (FUNC==afInsert)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         Dynamic pos = args[0]->runObject(ctx);
         BCR_VCHECK;
         Dynamic val = args[1]->runObject(ctx);
         BCR_VCHECK;
         thisVal->CALL(insert)(pos, val);
         return;
      }
      if (FUNC==afUnshift)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         Dynamic val = args[0]->runObject(ctx);
         BCR_VCHECK;
         thisVal->CALL(unshift)(val);
         return;
      }
      if (FUNC==af__SetSizeExact)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int size = args[0]->runInt(ctx);
         BCR_VCHECK;
         thisVal->__SetSizeExact(size);
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
         case af__SetSizeExact:
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

   bool isBoolInt() { return FUNC==afRemove; }


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JitTemp thisVal(compiler,etObject);
      thisExpr->genCode(compiler,thisVal,etObject);

      switch(FUNC)
      {
         case af__get:
            args[0]->genCode(compiler, sJitArg1, etInt);
            compiler->callNative( (void *)objGetItem, thisVal, sJitArg1.as(jtInt));
            compiler->convertReturnReg(etObject, inDest, destType);
            break;

         case afPop:
            compiler->callNative( (void *)arrayPop, thisVal);
            compiler->convertReturnReg(etObject, inDest, destType);
            break;


         case afRemove:
            args[0]->genCode(compiler, sJitArg1, etObject);
            compiler->callNative( (void *)arrayRemove, thisVal, sJitArg1.as(jtPointer));
            compiler->convertReturnReg(etInt, inDest, destType,true);
            break;

         case afSplice:
            {
            JitTemp a0(compiler,etObject);
            args[0]->genCode(compiler, a0, etObject);
            args[1]->genCode(compiler, sJitArg2, etObject);
            compiler->callNative( (void *)arraySplice, thisVal, a0.as(jtPointer), sJitArg2.as(jtPointer));
            compiler->convertReturnReg(etObject, inDest, destType);
            }
            break;

         case afSlice:
            {
            JitTemp a0(compiler,etObject);
            args[0]->genCode(compiler, a0, etObject);
            args[1]->genCode(compiler, sJitArg2, etObject);
            compiler->callNative( (void *)arraySlice, thisVal, a0.as(jtPointer), sJitArg2.as(jtPointer));
            compiler->convertReturnReg(etObject, inDest, destType);
            }
            break;

         case afSort:
            {
            args[0]->genCode(compiler, sJitArg1, etObject);
            compiler->callNative( (void *)runSort, thisVal, sJitArg1.as(jtPointer));
            compiler->checkException();
            }
            break;

         case afJoin:
            {
            JitTemp value(compiler,jtString);
            args[0]->genCode(compiler, value, etString);
            compiler->callNative( (void *)runJoin, thisVal, value);
            compiler->checkException();
            compiler->convert(value,etString, inDest, destType);
            }
            break;


         case afConcat:
            {
            args[0]->genCode(compiler, sJitArg1, etObject);
            compiler->callNative( (void *)arrayConcat, thisVal, sJitArg1.as(jtPointer));
            compiler->convertReturnReg(etObject, inDest, destType);
            }
            break;

         case af__SetSizeExact:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
               compiler->callNative( (void *)arraySetSizeExact, thisVal, sJitArg1.as(jtInt));
               break;
            }


         case af__set:
            {
               JitTemp index(compiler,etInt);
               args[0]->genCode(compiler, index, etInt);

               switch(args[1]->getType())
               {
                  case etInt:
                     args[1]->genCode(compiler, sJitArg2.as(jtInt), etInt);
                     compiler->callNative( (void *)arraySetInt, thisVal, index, sJitArg2.as(jtInt));
                     compiler->convertReturnReg(etInt, inDest, destType);
                     break;
                  case etFloat:
                     {
                     JitTemp value(compiler, jtFloat);
                     args[1]->genCode(compiler, value, etFloat);
                     compiler->callNative( (void *)arraySetFloat, thisVal, index, value);
                     compiler->convert(value,etFloat, inDest, destType);
                     }
                     break;
                  case etString:
                     {
                     JitTemp value(compiler, jtString);
                     args[1]->genCode(compiler, value, etString);
                     compiler->callNative( (void *)arraySetString, thisVal, index, value);
                     compiler->convert(value,etString, inDest, destType);
                     }
                     break;
                  case etObject:
                     args[1]->genCode(compiler, sJitArg2.as(jtPointer), etObject);
                     compiler->callNative( (void *)arraySetObject, thisVal, index, sJitArg2.as(jtPointer));
                     compiler->convertReturnReg(etObject, inDest, destType);
                     break;
                  default: ; //?
               }
            }
            break;



         case afPush:
            {
               switch(args[0]->getType())
               {
                  case etInt:
                     args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
                     compiler->callNative( (void *)arrayPushInt, thisVal, sJitArg1.as(jtInt));
                     break;
                  case etFloat:
                     {
                     JitTemp value(compiler, jtFloat);
                     args[0]->genCode(compiler, value, etFloat);
                     compiler->callNative( (void *)arrayPushFloat, thisVal, value);
                     }
                     break;
                  case etString:
                     {
                     JitTemp value(compiler, jtString);
                     args[0]->genCode(compiler, value, etString);
                     compiler->callNative( (void *)arrayPushString, thisVal, value);
                     }
                     break;
                  case etObject:
                     args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);
                     compiler->callNative( (void *)arrayPushObject, thisVal, sJitArg1.as(jtPointer));
                     break;
                  default: ; //?
               }
               compiler->convertReturnReg(etInt, inDest, destType);
            }
            break;


         case afUnshift:
            {
               switch(args[0]->getType())
               {
                  case etInt:
                     args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
                     compiler->callNative( (void *)arrayUnshiftInt, thisVal, sJitArg1.as(jtInt));
                     break;
                  case etFloat:
                     {
                     JitTemp value(compiler, jtFloat);
                     args[0]->genCode(compiler, value, etFloat);
                     compiler->callNative( (void *)arrayUnshiftFloat, thisVal, value);
                     }
                     break;
                  case etString:
                     {
                     JitTemp value(compiler, jtString);
                     args[0]->genCode(compiler, value, etString);
                     compiler->callNative( (void *)arrayUnshiftString, thisVal, value);
                     }
                     break;
                  case etObject:
                     args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);
                     compiler->callNative( (void *)arrayUnshiftObject, thisVal, sJitArg1.as(jtPointer));
                     break;
                  default: ; //?
               }
            }
            break;




         default:
            compiler->traceStrings("Unknown ArrayBuiltinAny:", getName() );
      }
   }
   #endif
};




template<int BUILTIN,typename CREMENT>
CppiaExpr *TCreateArrayBuiltin(CppiaExpr *inSrc, ArrayType inType, CppiaExpr *thisExpr, Expressions &args, bool inUnsafe = false)
{
   if (sArgCount[BUILTIN]!=args.size())
      throw "Bad arg count for array builtin";

   switch(inType)
   {
      case arrBool:
         return new ArrayBuiltin<bool,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrUnsignedChar:
         return new ArrayBuiltin<unsigned char,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrInt:
         return new ArrayBuiltin<int,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrFloat:
         return new ArrayBuiltin<Float,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrString:
         return new ArrayBuiltin<String,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrObject:
         return new ArrayBuiltin<Dynamic,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
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
   if (field==HX_CSTRING("__get") || field==HX_CSTRING("__unsafe_get"))
      return TCreateArrayBuiltin<af__get,NoCrement>(src, inType, inThisExpr, ioExpressions, field==HX_CSTRING("__unsafe_get"));
   if (field==HX_CSTRING("__set") || field==HX_CSTRING("__unsafe_set"))
      return TCreateArrayBuiltin<af__set,NoCrement>(src, inType, inThisExpr, ioExpressions, field==HX_CSTRING("__unsafe_set"));
   if (field==HX_CSTRING("__SetSizeExact"))
      return TCreateArrayBuiltin<af__SetSizeExact,NoCrement>(src, inType, inThisExpr, ioExpressions);

   return 0;
}






























} // end namespace hx
