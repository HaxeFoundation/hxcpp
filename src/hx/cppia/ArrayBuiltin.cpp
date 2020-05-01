#include <hxcpp.h>

#include "Cppia.h"

namespace hx
{

const char *gArrayFuncNames[] =
{
   "afConcat",
   "afCopy",
   "afInsert",
   "afIterator",
   "afKeyValueIterator",
   "afJoin",
   "afPop",
   "afPush",
   "afContains",
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
   "afBlit",
   "afResize",
};

int gArrayArgCount[] = 
{
   1, //afConcat,
   0, //afCopy,
   2, //afInsert,
   0, //afIterator,
   0, //afKeyValueIterator,
   1, //afJoin,
   0, //afPop,
   1, //afPush,
   1, //afContains,
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
   4, //afBlit,
   1, //afResize,
};


// ArrayBuiltinBase
ArrayBuiltinBase::ArrayBuiltinBase(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
   : CppiaExpr(inSrc)
{
   thisExpr = inThisExpr;
   args.swap(ioExpressions);
}
const char *ArrayBuiltinBase::getName() { return "ArrayBuiltinBase"; }


CppiaExpr *ArrayBuiltinBase::link(CppiaModule &inData)
{
   thisExpr = thisExpr->link(inData);
   for(int a=0;a<args.size();a++)
      args[a] = args[a]->link(inData);
   return this;
}


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
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM>*>(thisExpr->runObject(ctx));
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
      #endif
      return ValToInt(elem);
   }
   Float  runFloat(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM>*>(thisExpr->runObject(ctx));
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
      #endif
      return ValToFloat(elem);
   }
   String  runString(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM>*>(thisExpr->runObject(ctx));
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
      #endif
      return ValToString(elem);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM>*>(thisExpr->runObject(ctx));
      BCR_CHECK;
      int i = args[0]->runInt(ctx);
      BCR_CHECK;
      ELEM &elem = thisVal->Item(i);
      FUNC::run(elem, ctx, args[1]);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
      #endif
      return Dynamic(elem).mPtr;
   }

   #ifdef CPPIA_JIT

   static void * SLJIT_CALL nativeRunInt(JitMultiArg *args)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM> *>(args[0].obj);
      ELEM &elem = thisVal->Item( args[1].ival );
      FUNC::apply(elem, args[2].ival);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(thisVal,hx::PointerOf(elem));
      #endif
      return &elem;
   }

   static void * SLJIT_CALL nativeRunFloat(JitMultiArg *args)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM> *>(args[0].obj);
      ELEM &elem = thisVal->Item( args[1].ival );
      FUNC::apply(elem, args[2].dval);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(thisVal,hx::PointerOf(elem));
      #endif
      return &elem;
   }

   static void * SLJIT_CALL nativeRunObject(JitMultiArg *args)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM> *>(args[0].obj);
      ELEM &elem = thisVal->Item( args[1].ival );
      Dynamic obj(args[2].obj);
      FUNC::apply(elem, obj);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(thisVal,hx::PointerOf(elem));
      #endif
      return &elem;
   }

   static void * SLJIT_CALL nativeRunString(JitMultiArg *args)
   {
      Array_obj<ELEM> *thisVal = reinterpret_cast<Array_obj<ELEM> *>(args[0].obj);
      ELEM &elem = thisVal->Item( args[1].ival );
      FUNC::apply(elem, (* ((String *)(&args[2].sval) )) );
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(thisVal,hx::PointerOf(elem));
      #endif
      return &elem;
   }


   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
      ExprType rightHandType;
      switch((int)FUNC::op)
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

      if ((int)FUNC::op==aoSet)
      {
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


         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
            compiler->move(sJitTemp2,sJitTemp0.as(jtPointer));
         #endif
         // sJitTemp0 = this->base
         compiler->move(sJitTemp0, sJitTemp0.star(jtPointer, ArrayBase::baseOffset()).as(jtPointer) );

         if (sizeof(ELEM)==1) // uchar, bool
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1).as(jtByte), value );
         }
         else if (elemType==etString)
         {
            compiler->mult( sJitTemp1, sJitTemp1.as(jtInt), (int)sizeof(String), false );
            compiler->add( sJitTemp0, sJitTemp0.as(jtPointer), sJitTemp1);
            compiler->move( sJitTemp0.star(jtInt), value.as(jtInt) );
            compiler->move( sJitTemp0.star(jtPointer,StringOffset::Ptr), value.as(jtPointer) + StringOffset::Ptr );
            #ifdef HXCPP_GC_GENERATIONAL
            genWriteBarrier(compiler, sJitTemp2, value.as(jtPointer) + StringOffset::Ptr );
            #endif
         }
         else if (sizeof(ELEM)==2)
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1,1), value );
         }
         else if (sizeof(ELEM)==4)
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1,2), value );
            #ifdef HXCPP_GC_GENERATIONAL
            if (hx::ContainsPointers<ELEM>())
               genWriteBarrier(compiler, sJitTemp2, value );
            #endif
         }
         else if (sizeof(ELEM)==8)
         {
            compiler->move( sJitTemp0.atReg(sJitTemp1,3), value );
            #ifdef HXCPP_GC_GENERATIONAL
            if (hx::ContainsPointers<ELEM>())
               genWriteBarrier(compiler, sJitTemp2, value );
            #endif
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
         // this, inDestElement, index, value(right hand side)
         JitMultiArgs margs(compiler, 3);
         thisExpr->genCode(compiler, margs.arg(0,jtPointer), etObject);
         args[0]->genCode(compiler, margs.arg(1,jtInt), etInt);
         args[1]->genCode(compiler, margs.arg(2,getJitType(rightHandType)), rightHandType);

         switch(rightHandType)
         {
            case etInt:
               compiler->callNative( (void *)nativeRunInt, margs );
               break;
            case etFloat:
               compiler->callNative( (void *)nativeRunFloat, margs );
               break;
            case etString:
               compiler->callNative( (void *)nativeRunString, margs );
               break;
            case etObject:
               compiler->callNative( (void *)nativeRunObject, margs );
               break;
            default:
               throw "Bad array set type";
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
template<> struct ExprBaseTypeOf<float> { typedef double &Base ; };
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
   const char *getName() { return gArrayFuncNames[FUNC]; }

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
         case afResize:
            return etVoid;

         case afPush:
         case afContains:
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
      if (FUNC==afContains)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         ELEM elem;
         runValue(elem,ctx,args[0]);
         BCR_CHECK;
         return thisVal->contains(elem);
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
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
            HX_OBJ_WB_GET(thisVal, hx::PointerOf(elem));
         #endif
         return ValToInt(elem);
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM &elem = thisVal->Item(idx);
         int result = ValToInt(CREMENT::run(elem));
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
           HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
         #endif
         return result;
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
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
            HX_OBJ_WB_GET(thisVal, hx::PointerOf(elem));
         #endif
         return ValToFloat(elem);
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         ELEM &elem = thisVal->Item(args[0]->runInt(ctx));

         Float result = ValToFloat(CREMENT::run(elem));
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
           HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
         #endif
         return result;
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
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
            HX_OBJ_WB_GET(thisVal, hx::PointerOf(elem));
         #endif
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
         String result = ValToString(CREMENT::run(elem));
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
           HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
         #endif
         return result;
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

      if (FUNC==afPush || FUNC==afContains || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
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
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
            HX_OBJ_WB_GET(thisVal, hx::PointerOf(elem));
         #endif
         return Dynamic(elem).mPtr;
      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         int idx = args[0]->runInt(ctx);
         BCR_CHECK;
         ELEM &elem = thisVal->Item(idx);
         Dynamic result(CREMENT::run(elem));
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
           HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
         #endif
         return result.mPtr;

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
         Dynamic result = thisVal->map(func);
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
      if (FUNC==afKeyValueIterator)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_CHECK;
         return thisVal->keyValueIterator().mPtr;
      }

      if (FUNC==afPush || FUNC==afContains || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
         return Dynamic(runInt(ctx)).mPtr;

      if (FUNC==afJoin || FUNC==afToString)
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
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
            HX_OBJ_WB_CTX(thisVal, hx::PointerOf(elem), ctx);
         #endif

      }
      if (FUNC==af__crement)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int idx = args[0]->runInt(ctx);
         BCR_VCHECK;
         ELEM &elem = thisVal->Item(idx);
         CREMENT::run(elem);
         #ifdef HXCPP_GC_GENERATIONAL
         if (hx::ContainsPointers<ELEM>())
           HX_OBJ_WB_CTX(thisVal,hx::PointerOf(elem),ctx);
         #endif
      }

      if (FUNC==afPush || FUNC==afContains || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
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

      if (FUNC==afResize)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM>*)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int size = args[0]->runInt(ctx);
         BCR_VCHECK;
         thisVal->resize(size);
      }


      if (FUNC==afBlit)
      {
         Array_obj<ELEM> *thisVal = (Array_obj<ELEM> *)thisExpr->runObject(ctx);
         BCR_VCHECK;
         int destElem = args[0]->runInt(ctx);
         BCR_VCHECK;
         Dynamic srcArray = args[1]->runObject(ctx);
         BCR_VCHECK;
         int srcElem = args[2]->runInt(ctx);
         BCR_VCHECK;
         int elemCount = args[3]->runInt(ctx);
         BCR_VCHECK;
         thisVal->blit(destElem, srcArray, srcElem, elemCount);
         return;
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
      ELEM &t = Array<ELEM>(inArray)[inIndex];
      CREMENT::run(t);
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(inArray,hx::PointerOf(t));
      #endif
   }

   static int SLJIT_CALL runIntCrement( Array_obj<ELEM> *inArray, int inIndex)
   {
      ELEM &t = Array<ELEM>(inArray)[inIndex];
      int result =  ValToInt( CREMENT::run(t) );
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(inArray,hx::PointerOf(t));
      #endif
      return result;
   }

   static void SLJIT_CALL runFloatCrement( Array_obj<ELEM> *inArray, int inIndex, double *d)
   {
      ELEM &t = Array<ELEM>(inArray)[inIndex];
      *d = ValToFloat( CREMENT::run(t) );
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(inArray,hx::PointerOf(t));
      #endif

   }

   static hx::Object * SLJIT_CALL runObjCrement( Array_obj<ELEM> *inArray, int inIndex)
   {
      ELEM &t = Array<ELEM>(inArray)[inIndex];
      Dynamic result( CREMENT::run(t) );
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM>())
         HX_OBJ_WB_GET(inArray,hx::PointerOf(t));
      #endif
      return result.mPtr;
   }

   static int SLJIT_CALL runContains( Array_obj<ELEM> *inArray, typename ExprBaseTypeOf<ELEM>::Base inBase)
   {
      return inArray->contains(inBase);
   }

   static int SLJIT_CALL runRemove( Array_obj<ELEM> *inArray, typename ExprBaseTypeOf<ELEM>::Base inBase)
   {
      return inArray->remove(inBase);
   }


   static void SLJIT_CALL runUnshift( Array_obj<ELEM> *inArray, typename ExprBaseTypeOf<ELEM>::Base inBase)
   {
      inArray->unshift(inBase);
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

   static hx::Object *SLJIT_CALL runGetKeyValueIteratator( Array_obj<ELEM> *inArray )
   {
      return inArray->keyValueIterator().mPtr;
   }

   static void SLJIT_CALL runSetSizeExact( Array_obj<ELEM> *inArray, int size )
   {
      inArray->__SetSizeExact(size);
   }

   static void SLJIT_CALL runResize( Array_obj<ELEM> *inArray, int size )
   {
      inArray->resize(size);
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

   static void SLJIT_CALL runBlit( JitMultiArg *inArgs)
   {
       // this, inDestElement, inSourceArray, inSourceElement, inElementCount
       Array_obj<ELEM> *dest = (Array_obj<ELEM> *)inArgs[0].obj;
       Array_obj<ELEM> *src = (Array_obj<ELEM> *)inArgs[2].obj;
       dest->blit( inArgs[1].ival, src, inArgs[3].ival, inArgs[4].ival );
   }


   static hx::Object * SLJIT_CALL runProcess( Array_obj<ELEM> *inArray, hx::Object *inFunction)
   {
      TRY_NATIVE
      if (FUNC==afMap)
      {
         Dynamic result = inArray->map(inFunction);
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
         // Array<ELEM>
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

               JitTemp length(compiler, jtInt);
               if (destType!=etVoid)
                  compiler->add(length, sJitTemp0, (int)1 );

               #ifdef HXCPP_GC_GENERATIONAL
               if (hx::ContainsPointers<ELEM>())
                  compiler->move(sJitTemp2, sJitTemp1.as(jtPointer));
               #endif

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
                  compiler->move( sJitTemp0.star(jtPointer,StringOffset::Ptr), tempVal.as(jtPointer) + StringOffset::Ptr );
                  #ifdef HXCPP_GC_GENERATIONAL
                  genWriteBarrier(compiler, sJitTemp2, tempVal.as(jtPointer) + StringOffset::Ptr );
                  #endif
               }
               else if (sizeof(ELEM)==2)
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0,1), tempVal );
               }
               else if (sizeof(ELEM)==4)
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0,2), tempVal );
                  #ifdef HXCPP_GC_GENERATIONAL
                  if (hx::ContainsPointers<ELEM>())
                     genWriteBarrier(compiler, sJitTemp2, tempVal );
                  #endif
               }
               else if (sizeof(ELEM)==8)
               {
                  compiler->move( sJitTemp1.atReg(sJitTemp0,3), tempVal );
                  #ifdef HXCPP_GC_GENERATIONAL
                  if (hx::ContainsPointers<ELEM>())
                     genWriteBarrier(compiler, sJitTemp2, tempVal );
                  #endif
               }
               else
               {
                  printf("Unknown element size\n");
               }

               if (destType!=etNull)
                  compiler->convert(length, etInt, inDest, destType );

               break;
            }

         // Array<ELEM>
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


               #ifdef HXCPP_GC_GENERATIONAL
               if (hx::ContainsPointers<ELEM>())
                   compiler->move(sJitTemp2,sJitTemp0.as(jtPointer));
               #endif

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
                  compiler->move( sJitTemp0.star(jtPointer, StringOffset::Ptr), tempVal.as(jtPointer) + StringOffset::Ptr );

                  #ifdef HXCPP_GC_GENERATIONAL
                  genWriteBarrier(compiler, sJitTemp2, tempVal.as(jtPointer) + StringOffset::Ptr );
                  #endif
               }
               else if (sizeof(ELEM)==2)
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1,1), tempVal );
               }
               else if (sizeof(ELEM)==4)
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1,2), tempVal );
                  #ifdef HXCPP_GC_GENERATIONAL
                  if (hx::ContainsPointers<ELEM>())
                     genWriteBarrier(compiler, sJitTemp2, tempVal );
                  #endif
               }
               else if (sizeof(ELEM)==8)
               {
                  compiler->move( sJitTemp0.atReg(sJitTemp1,3), tempVal );
                  #ifdef HXCPP_GC_GENERATIONAL
                  if (hx::ContainsPointers<ELEM>())
                     genWriteBarrier(compiler, sJitTemp2, tempVal );
                  #endif
               }
               else
               {
                  printf("Unknown element size\n");
               }


               break;
            }


         // Array<ELEM>
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
                     compiler->convert(sJitTemp1.as(jtInt), etInt, inDest, destType);
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
                  // todo - float?
                  if (destType!=elemType || isMemoryVal(inDest))
                  {
                     compiler->move(sJitTemp0.as(elemType==etObject ? jtPointer : jtInt), sJitTemp1.atReg(sJitTemp0,2) );
                     compiler->convert( sJitTemp0, elemType, inDest, destType );
                  }
                  else
                     compiler->move(inDest.as(jtInt), sJitTemp1.atReg(sJitTemp0,2) );
               }
               else if (sizeof(ELEM)==8)
               {
                  if (destType!=etObject || elemType!=etObject || isMemoryVal(inDest))
                  {
                     if (elemType==etFloat)
                     {
                        compiler->move(sJitTempF0, sJitTemp1.atReg(sJitTemp0,3) );
                        compiler->convert( sJitTempF0, elemType, inDest, destType );
                     }
                     else
                     {
                        compiler->move(sJitTemp0.as(jtPointer), sJitTemp1.atReg(sJitTemp0,3) );
                        compiler->convert( sJitTemp0, etObject, inDest, destType );
                     }
                  }
                  else
                     compiler->move(inDest.as(jtPointer), sJitTemp1.atReg(sJitTemp0,3) );
               }
               else
               {
                  printf("Unknown element size\n");
               }
               compiler->comeFrom(writtenNull);

               break;
            }


         // Array<ELEM>
         case afUnshift:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp val(compiler,getJitType(elemType));
               args[0]->genCode(compiler, val, elemType);

               compiler->callNative( (void *)runUnshift, thisVal, val );
               break;
            }

         // Array<ELEM>
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
                     compiler->move( sJitTemp0.star(jtPointer) + StringOffset::Ptr,  0 );
                  }
                  else
                  {
                     JitTemp strVal(compiler,jtString);
                     compiler->convert(  sJitTemp0.star(jtString), etString, strVal, etString );
                     compiler->move( sJitTemp0.star(jtInt),  0 );
                     compiler->move( sJitTemp0.star(jtPointer) + StringOffset::Ptr,  0 );
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

         // Array<ELEM>
         case afSort:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitTemp1, etObject);
               compiler->callNative( (void *)runSort, thisVal, sJitTemp1 );
               compiler->checkException();
               break;
            }

         // Array<ELEM>
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

         // Array<ELEM>
         case afContains:
            {
               JitTemp thisVal(compiler, jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);

               ExprType elemType = (ExprType)ExprTypeOf<ELEM>::value;
               JitTemp val(compiler,getJitType(elemType));
               args[0]->genCode(compiler, val, elemType);

               compiler->callNative( (void *)runContains, thisVal, val );

               compiler->convertReturnReg(etInt, inDest, destType, true);
               break;
            }

         // Array<ELEM>
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


         // Array<ELEM>
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

   
         // Array<ELEM>
         case afIterator:
            {
               thisExpr->genCode(compiler, sJitTemp0, etObject);
               compiler->callNative( (void *)runGetIteratator, sJitTemp0.as(jtPointer) );
               compiler->convertReturnReg(etObject, inDest, destType);
               break;
            }
         case afKeyValueIterator:
            {
               thisExpr->genCode(compiler, sJitTemp0, etObject);
               compiler->callNative( (void *)runGetKeyValueIteratator, sJitTemp0.as(jtPointer) );
               compiler->convertReturnReg(etObject, inDest, destType);
               break;
            }

         // Array<ELEM>
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

         // Array<ELEM>
         case afConcat:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);
               compiler->callNative( (void *)runConcat, thisVal, sJitArg1.as(jtPointer));
               compiler->convertReturnReg(etObject, inDest, destType);
               break;
            }

         // Array<ELEM>
         case af__SetSizeExact:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
               compiler->callNative( (void *)runSetSizeExact, thisVal, sJitArg1.as(jtInt));
               break;
            }

         case afResize:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
               compiler->callNative( (void *)runResize, thisVal, sJitArg1.as(jtInt));
               break;
            }

         // Array<ELEM>
         case afBlit:
            {
               // this, inDestElement, inSourceArray, inSourceElement, inElementCount
               JitMultiArgs fargs(compiler, 5);
               thisExpr->genCode(compiler, fargs.arg(0,jtPointer), etObject);
               args[0]->genCode(compiler, fargs.arg(1,jtInt), etInt);
               args[1]->genCode(compiler, fargs.arg(2,jtPointer), etObject);
               args[2]->genCode(compiler, fargs.arg(3,jtInt), etInt);
               args[3]->genCode(compiler, fargs.arg(4,jtInt), etInt);
               compiler->callNative( (void *)runBlit, fargs );
               break;
            }


         // Array<ELEM>
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

         // Array<ELEM>
         case afReverse:
            thisExpr->genCode(compiler, sJitArg0.as(jtPointer), etObject);
            compiler->callNative( (void *)runReverse, sJitArg0.as(jtPointer) );
            break;

         // Array<ELEM>
         case afShift:
            thisExpr->genCode(compiler, sJitArg0.as(jtPointer), etObject);
            compiler->callNative( (void *)runShift, sJitArg0.as(jtPointer) );
            compiler->convertReturnReg(etObject, inDest, destType, isBoolElem() );
            break;


         // Array<ELEM>
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


         // Array<ELEM>
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


         // Array<ELEM>
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


         // Array<ELEM>
         case afCopy:
            thisExpr->genCode(compiler, sJitArg0, etObject);
            compiler->callNative( (void *)runCopy, sJitArg0);
            compiler->convertReturnReg(etObject, inDest, destType );
            break;

         // Array<ELEM>
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
            compiler->traceStrings("ArrayBuiltin::",gArrayFuncNames[FUNC]);
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
static int SLJIT_CALL arrayContains(ArrayAnyImpl *inObj, hx::Object *inValue)
{
   return inObj->contains(inValue);
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
static hx::Object * SLJIT_CALL arrayShift(ArrayAnyImpl *inObj)
{
   return inObj->shift().mPtr;
}

static void SLJIT_CALL runSort( ArrayAnyImpl *inArray, hx::Object *inFunc)
{
   TRY_NATIVE
   inArray->sort( Dynamic(inFunc) );
   CATCH_NATIVE
}
static void SLJIT_CALL runReverse( ArrayAnyImpl *inArray)
{
   inArray->reverse();
}

static hx::Object * SLJIT_CALL runCopy( ArrayAnyImpl *inArray)
{
   return (inArray->copy()).mPtr;
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


static void SLJIT_CALL runBlit( JitMultiArg *inArgs)
{
    // this, inDestElement, inSourceArray, inSourceElement, inElementCount
    ArrayAnyImpl *dest = (ArrayAnyImpl *)inArgs[0].obj;
    ArrayAnyImpl *src = (ArrayAnyImpl *)inArgs[2].obj;
    dest->blit( inArgs[1].ival, src, inArgs[3].ival, inArgs[4].ival );
}



#endif



template<int BUILTIN,typename CREMENT>
CppiaExpr *TCreateArrayBuiltin(CppiaExpr *inSrc, ArrayType inType, CppiaExpr *thisExpr, Expressions &args, bool inUnsafe = false)
{
   if (gArrayArgCount[BUILTIN]!=args.size())
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
      case arrFloat32:
         return new ArrayBuiltin<float,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrString:
         return new ArrayBuiltin<String,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      case arrObject:
         return new ArrayBuiltin<Dynamic,BUILTIN,CREMENT>(inSrc, thisExpr, args, inUnsafe);
      default:
         break;
   }
   throw "Unknown array type";
   return 0;
}


CppiaExpr *createArrayBuiltin(CppiaExpr *src, ArrayType inType, CppiaExpr *inThisExpr, String field,
                              Expressions &ioExpressions )
{
   if (inType==arrAny)
      return createArrayAnyBuiltin(src, inThisExpr, field, ioExpressions );


   if (field==HX_CSTRING("concat"))
      return TCreateArrayBuiltin<afConcat,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("copy"))
      return TCreateArrayBuiltin<afCopy,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("insert"))
      return TCreateArrayBuiltin<afInsert,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("iterator"))
      return TCreateArrayBuiltin<afIterator,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("keyValueIterator"))
      return TCreateArrayBuiltin<afKeyValueIterator,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("join"))
      return TCreateArrayBuiltin<afJoin,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("pop"))
      return TCreateArrayBuiltin<afPop,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("push"))
      return TCreateArrayBuiltin<afPush,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("contains"))
      return TCreateArrayBuiltin<afContains,NoCrement>(src, inType, inThisExpr, ioExpressions);
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
   if (field==HX_CSTRING("blit"))
      return TCreateArrayBuiltin<afBlit,NoCrement>(src, inType, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("resize"))
      return TCreateArrayBuiltin<afResize,NoCrement>(src, inType, inThisExpr, ioExpressions);

   return 0;
}



} // end namespace hx
