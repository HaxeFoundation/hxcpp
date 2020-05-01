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
   const char *getName() { return "SubStrExpr"; }
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
   #ifdef CPPIA_JIT
   static void SLJIT_CALL runSubstr(String *ioValue, int start, hx::Object *end)
   {
      *ioValue = ioValue->substr(start, Dynamic(end));
   }
   static void SLJIT_CALL runSubstring(String *ioValue, int start, hx::Object *end)
   {
      *ioValue = ioValue->substring(start, Dynamic(end));
   }
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitTemp ioValue(compiler,jtString);
      JitTemp startVal(compiler,jtInt);

      strVal->genCode(compiler, ioValue, etString);
      a0->genCode(compiler, startVal, etInt);
      a1->genCode(compiler, sJitArg2, etObject);
      compiler->callNative( SUBSTR ? (void *)runSubstr : (void *)runSubstring, ioValue, startVal, sJitArg2.as(jtPointer) );
      compiler->convert(ioValue, etString, inDest, destType);
   }
   #endif
};



template<bool KEYS>
struct StringIteratorExpr : public StringExpr
{
   StringIteratorExpr(CppiaExpr *inSrc, CppiaExpr *inThis) : StringExpr(inSrc,inThis)
   {
   }
   const char *getName() { return "StringIteratorExpr"; }

   ExprType getType() { return etObject; }

   String runString(CppiaCtx *ctx) { return Dynamic(runObject(ctx)); }
   int runInt(CppiaCtx *ctx) { return 0; }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      String val = strVal->runString(ctx);
      BCR_CHECK;
      return ( KEYS ? val.keyValueIterator() : val.iterator() ).mPtr;
   }

   #ifdef CPPIA_JIT
   static hx::Object *SLJIT_CALL run(String *inValue)
   {
      return ( KEYS ? inValue->keyValueIterator() : inValue->iterator() ).mPtr;
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitTemp value(compiler,jtString);
      strVal->genCode(compiler, value, etString);
      compiler->callNative( (void *)run, value);
      compiler->convertReturnReg(etObject, inDest, destType);
   }
   #endif
};




template<bool UPPER>
struct ToCaseExpr : public StringExpr
{
   const char *getName() { return "ToCaseExpr"; }
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
   #ifdef CPPIA_JIT
   static void SLJIT_CALL strToCase(String *ioVal)
   {
      if (UPPER)
         *ioVal = ioVal->toUpperCase();
      else
         *ioVal = ioVal->toLowerCase();
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      if (destType==etString)
      {
         strVal->genCode(compiler, inDest, destType);
         compiler->callNative( (void *)strToCase,  inDest.as(jtString) );
      }
      else
      {
         JitTemp tmpVal(compiler,jtString);
         strVal->genCode(compiler, tmpVal, etString);
         compiler->callNative( (void *)strToCase,  tmpVal);
         compiler->convert( tmpVal, etString, inDest, destType );
      }
   }
   #endif
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
   const char *getName() { return "CharAtExpr"; }
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
         return (int)val.cca(idx);
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
            return Dynamic( val.cca(idx) ).mPtr;
         else
            return val.charCodeAt(idx).mPtr;
      }
      else
         return Dynamic(val.charAt(idx)).mPtr;
   }

   #ifdef CPPIA_JIT
   static hx::Object *SLJIT_CALL runCharCodeAt(String *inValue, int inIndex)
   {
      return (inValue->charCodeAt(inIndex)).mPtr;
   }
   static int SLJIT_CALL runCca(String *inValue, int inIndex)
   {
      return (inValue->cca(inIndex));
   }
   static void SLJIT_CALL runCharAt(String *ioValue, int inIndex)
   {
      *ioValue = ioValue->charAt(inIndex);
   }

   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitTemp value(compiler,jtString);
      strVal->genCode(compiler, value, etString);
      a0->genCode(compiler, sJitTemp1, etInt);

      if (CODE)
      {
         if (AS_INT)
         {
            #ifdef HX_SMART_STRINGS
            compiler->callNative( (void *)runCca, value, sJitTemp1.as(jtInt));
            compiler->convertReturnReg( etInt, inDest, destType);
            #else
            // sJitTemp1 = __s
            compiler->move( sJitTemp0.as(jtPointer), value.star(jtPointer,offsetof(String,__s)) );
            if (destType==etInt)
            {
               compiler->move(inDest.as(jtInt), sJitTemp0.atReg(sJitTemp1,0,jtByte) );
            }
            else
            {
               compiler->move(sJitTemp0.as(jtInt), sJitTemp0.atReg(sJitTemp1,0,jtByte) );
               compiler->convertReturnReg(etInt, inDest, destType);
            }
            #endif
         }
         else
         {
            compiler->callNative( (void *)runCharCodeAt, value, sJitTemp1.as(jtInt));
            compiler->convertReturnReg( etObject, inDest, destType);
         }
      }
      else
      {
         compiler->callNative( (void *)runCharAt, value, sJitTemp1.as(jtInt));
         compiler->convert(value, etString, inDest, destType);
      }
   }
   #endif
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
   const char *getName() { return "SplitExpr"; }
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


   #ifdef CPPIA_JIT
   static hx::Object *SLJIT_CALL runSplit(String *inValue, String *sep)
   {
      return (inValue->split(*sep)).mPtr;
   }
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitTemp value(compiler,jtString);
      JitTemp sep(compiler,jtString);

      strVal->genCode(compiler, value, etString);
      a0->genCode(compiler, sep, etString);
      compiler->callNative( (void *)runSplit, value, sep );
      compiler->convertReturnReg(etObject, inDest, destType);
   }
   #endif

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
   const char *getName() { return "IndexOfExpr"; }
   ExprType getType() { return etInt; }
   CppiaExpr *link(CppiaModule &inData)
   {
      strVal = strVal->link(inData);
      sought = sought->link(inData);
      start = start->link(inData);
      return this;
   }
   Float runFloat(CppiaCtx *ctx)
   {
      return runInt(ctx);
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


   #ifdef CPPIA_JIT
   static int SLJIT_CALL runIndexOf(String *ioValue, String *sought, hx::Object *first)
   {
      return ioValue->indexOf(*sought, Dynamic(first));
   }
   static int SLJIT_CALL runLastIndexOf(String *ioValue, String *sought, hx::Object *first)
   {
      return ioValue->lastIndexOf(*sought, Dynamic(first));
   }
   void genCode(CppiaCompiler *compiler, const JitVal &inDest,ExprType destType)
   {
      JitTemp value(compiler,jtString);
      JitTemp soughtTemp(compiler,jtString);

      strVal->genCode(compiler, value, etString);
      sought->genCode(compiler, soughtTemp, etString);
      start->genCode(compiler, sJitArg2, etObject);
      compiler->callNative( LAST ? (void *)runLastIndexOf : (void *)runIndexOf, value, soughtTemp, sJitArg2.as(jtPointer) );
      compiler->convertReturnReg(etInt, inDest, destType);
   }
   #endif

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
   else if (field==HX_CSTRING("iterator"))
   {
      if (ioExpressions.size()!=0) throw "Bad arg count";
      return new StringIteratorExpr<false>(inSrc,inThisExpr);
   }
   else if (field==HX_CSTRING("keyValueIterator"))
   {
      if (ioExpressions.size()!=0) throw "Bad arg count";
      return new StringIteratorExpr<true>(inSrc,inThisExpr);
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
