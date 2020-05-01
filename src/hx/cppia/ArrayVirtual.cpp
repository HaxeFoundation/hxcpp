#include <hxcpp.h>
#include "Cppia.h"

namespace hx
{


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
static void SLJIT_CALL arrayResize(ArrayAnyImpl *inObj, int inSize)
{
   inObj->resize(inSize);
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



template<int FUNC,int OP>
struct ArrayBuiltinAny : public ArrayBuiltinBase
{



   ArrayBuiltinAny(CppiaExpr *inSrc, CppiaExpr *inThisExpr, Expressions &ioExpressions)
      : ArrayBuiltinBase(inSrc,inThisExpr,ioExpressions)
   {
   }
   const char *getName() { return gArrayFuncNames[FUNC]; }

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
      if (FUNC==afContains)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
         BCR_CHECK;
         hx::Object * val = args[0]->runObject(ctx);
         BCR_CHECK;
         return thisVal->CALL(contains)(val);
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
      if (FUNC==afPush || FUNC==afContains || FUNC==afRemove || FUNC==afIndexOf || FUNC==afLastIndexOf)
         return Dynamic(runInt(ctx)).mPtr;

      if (FUNC==afRemove)
         return Dynamic((bool)runInt(ctx)).mPtr;

      if (FUNC==afJoin || FUNC==afToString)
         return Dynamic(runString(ctx)).mPtr;

      if (FUNC==afSort || FUNC==afInsert || FUNC==afUnshift || FUNC==af__SetSizeExact || FUNC==afBlit)
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
      if (FUNC==afKeyValueIterator)
      {
         return thisVal->CALL(keyValueIterator)().mPtr;
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
      if (FUNC==afBlit)
      {
         ArrayAnyImpl *thisVal = (ArrayAnyImpl *)thisExpr->runObject(ctx);
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
         case afBlit:
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
   ExprType getType() { return inlineGetType(); }

   bool isBoolInt() { return FUNC==afRemove || FUNC==afContains; }


   #ifdef CPPIA_JIT

   static hx::Object * SLJIT_CALL runProcess( ArrayAnyImpl *inArray, hx::Object *inFunction)
   {
      TRY_NATIVE
      if (FUNC==afMap)
      {
         return inArray->map(inFunction).mPtr;
      }
      else
      {
         return inArray->filter(inFunction).mPtr;
      }
      CATCH_NATIVE
      return 0;
   }

   static int SLJIT_CALL runIndexOf( ArrayAnyImpl *inArray, hx::Object *inItem)
   {
      return inArray->indexOf(inItem);
   }

   static int SLJIT_CALL runLastIndexOf( ArrayAnyImpl *inArray, hx::Object *inItem)
   {
      return inArray->lastIndexOf(inItem);
   }


   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      JitTemp thisVal(compiler,etObject);
      if (FUNC!=afBlit)
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

         case afShift:
            compiler->callNative( (void *)arrayShift, thisVal);
            compiler->convertReturnReg(etObject, inDest, destType);
            break;

         case afContains:
            args[0]->genCode(compiler, sJitArg1, etObject);
            compiler->callNative( (void *)arrayContains, thisVal, sJitArg1.as(jtPointer));
            compiler->convertReturnReg(etInt, inDest, destType,true);
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


         case afReverse:
            compiler->callNative( (void *)runReverse, thisVal);
            break;

         case afCopy:
            compiler->callNative( (void *)runCopy, thisVal);
            compiler->convertReturnReg(etObject, inDest, destType );
            break;

         case afMap:
         case afFilter:
            {
            args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);

            compiler->callNative( (void *)runProcess, thisVal, sJitArg1.as(jtPointer));
            compiler->checkException();
            compiler->convertReturnReg(etObject, inDest, destType );
            }
            break;



         case afIndexOf:
         case afLastIndexOf:
            {
            args[0]->genCode(compiler, sJitArg1.as(jtPointer), etObject);
            compiler->callNative( (void *)(FUNC==afIndexOf ? runIndexOf : runLastIndexOf), thisVal, sJitArg1.as(jtPointer));
            compiler->convertReturnReg(etInt, inDest, destType );
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

         case afResize:
            {
               JitTemp thisVal(compiler,jtPointer);
               thisExpr->genCode(compiler, thisVal, etObject);
               args[0]->genCode(compiler, sJitArg1.as(jtInt), etInt);
               compiler->callNative( (void *)arrayResize, thisVal, sJitArg1.as(jtInt));
               break;
            }

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




template<int FUNC>
CppiaExpr *TCreateArrayAnyBuiltin(CppiaExpr *inSrc, CppiaExpr *thisExpr, Expressions &args, bool inUnsafe = false)
{
   if (gArrayArgCount[FUNC]!=args.size())
      throw "Bad arg count for array builtin";

   return new ArrayBuiltinAny<FUNC,(int)NoCrement::OP>(inSrc, thisExpr, args);
   return 0;
}



CppiaExpr *createArrayAnyBuiltin(CppiaExpr *src, CppiaExpr *inThisExpr, String field, Expressions &ioExpressions )
{
   if (field==HX_CSTRING("concat"))
      return TCreateArrayAnyBuiltin<afConcat>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("copy"))
      return TCreateArrayAnyBuiltin<afCopy>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("insert"))
      return TCreateArrayAnyBuiltin<afInsert>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("iterator"))
      return TCreateArrayAnyBuiltin<afIterator>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("keyValueIterator"))
      return TCreateArrayAnyBuiltin<afKeyValueIterator>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("join"))
      return TCreateArrayAnyBuiltin<afJoin>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("pop"))
      return TCreateArrayAnyBuiltin<afPop>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("push"))
      return TCreateArrayAnyBuiltin<afPush>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("contains"))
      return TCreateArrayAnyBuiltin<afContains>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("remove"))
      return TCreateArrayAnyBuiltin<afRemove>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("reverse"))
      return TCreateArrayAnyBuiltin<afReverse>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("shift"))
      return TCreateArrayAnyBuiltin<afShift>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("splice"))
      return TCreateArrayAnyBuiltin<afSplice>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("slice"))
      return TCreateArrayAnyBuiltin<afSlice>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("sort"))
      return TCreateArrayAnyBuiltin<afSort>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("toString"))
      return TCreateArrayAnyBuiltin<afToString>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("unshift"))
      return TCreateArrayAnyBuiltin<afUnshift>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("map"))
      return TCreateArrayAnyBuiltin<afMap>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("filter"))
      return TCreateArrayAnyBuiltin<afFilter>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("indexOf"))
      return TCreateArrayAnyBuiltin<afIndexOf>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("lastIndexOf"))
      return TCreateArrayAnyBuiltin<afLastIndexOf>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("__get") || field==HX_CSTRING("__unsafe_get"))
      return TCreateArrayAnyBuiltin<af__get>(src, inThisExpr, ioExpressions, field==HX_CSTRING("__unsafe_get"));
   if (field==HX_CSTRING("__set") || field==HX_CSTRING("__unsafe_set"))
      return TCreateArrayAnyBuiltin<af__set>(src, inThisExpr, ioExpressions, field==HX_CSTRING("__unsafe_set"));
   if (field==HX_CSTRING("__SetSizeExact"))
      return TCreateArrayAnyBuiltin<af__SetSizeExact>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("blit"))
      return TCreateArrayAnyBuiltin<afBlit>(src, inThisExpr, ioExpressions);
   if (field==HX_CSTRING("resize"))
      return TCreateArrayAnyBuiltin<afResize>(src, inThisExpr, ioExpressions);

   return 0;
}




} // end namespace hx
