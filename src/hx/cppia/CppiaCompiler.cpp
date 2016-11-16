#include <hxcpp.h>
#include <hx/Scriptable.h>

#include "Cppia.h"

#include "sljit_src/sljitLir.c"

namespace hx
{

static void test_func(CppiaCtx *inCtx)
{
   printf("Test!\n");
}

static void SLJIT_CALL my_trace_func(const char *inText)
{
   printf("trace: %s\n", inText); 
}
static void SLJIT_CALL my_trace_strings(const char *inText, const char *inValue)
{
   printf("%s%s\n", inText, inValue); 
}

static void SLJIT_CALL my_trace_ptr_func(const char *inText, void *inPtr)
{
   printf("%s = %p\n",inText, inPtr);
}
static void SLJIT_CALL my_trace_int_func(const char *inText, int inValue)
{
   printf("%s = %d\n",inText, inValue);
}
static void SLJIT_CALL my_trace_float_func(const char *inText, double *inValue)
{
   printf("%s = %f\n",inText, *inValue);
}

static void SLJIT_CALL my_trace_obj_func(const char *inText, hx::Object *inPtr)
{
   printf("%s = %s\n",inText, inPtr ? inPtr->__ToString().__s : "NULL" );
}

static void *SLJIT_CALL intToObj(int inVal)
{
   return Dynamic(inVal).mPtr;
}
static void SLJIT_CALL intToStr(int inVal, String *outString)
{
   *outString = String(inVal);
}
static void SLJIT_CALL objToStr(hx::Object *inVal, String *outString)
{
   *outString = inVal ? inVal->toString() : HX_CSTRING("null");
}
static int SLJIT_CALL objToInt(hx::Object *inVal)
{
   // TODO - check pointer
   return inVal->__ToInt();
}
static void SLJIT_CALL objToFloat(hx::Object *inVal,double *outFloat)
{
   // TODO - check pointer
   *outFloat =  inVal->__ToDouble();
}

static void *SLJIT_CALL strToObj(String *inStr)
{
   return Dynamic(*inStr).mPtr;
}
static void *SLJIT_CALL floatToObj(double *inDouble)
{
   return Dynamic(*inDouble).mPtr;
}

static void SLJIT_CALL floatToStr(double *inDouble, String *outStr)
{
   *outStr = String(*inDouble);

}




int getJitTypeSize(JitType inType)
{
   switch(inType)
   {
      case jtAny : return sizeof(void *);
      case jtPointer : return sizeof(void *);
      case jtString : return sizeof(String);
      case jtFloat : return sizeof(double);
      case jtInt : return sizeof(int);
      default:
        return 0;
   }
}


JitType getJitType(ExprType inType)
{
   switch(inType)
   {
      case etVoid:
         return jtAny;
      case etNull:
      case etObject:
         return jtPointer;
      case etString:
         return jtString;
      case etFloat:
         return jtFloat;
      case etInt:
         return jtInt;
   }
   return jtPointer;
}


bool isMemoryVal(const JitVal &inVal)
{
   switch(inVal.position)
   {
      case jposFrame:
      case jposLocal:
      case jposThis:
      case jposStar:
         return true;
      default:
         return false;
   }
}

int sLocalReg = SLJIT_SP;

JitReg sJitReturnReg(SLJIT_R0);

JitReg sJitArg0(SLJIT_R0);
JitReg sJitArg1(SLJIT_R1);
JitReg sJitArg2(SLJIT_R2);

JitReg sJitTemp0(SLJIT_R0);
JitReg sJitTemp1(SLJIT_R1);
JitReg sJitTemp2(SLJIT_R2);

JitReg sJitTempF0(SLJIT_FR0,jtFloat);
JitReg sJitTempF1(SLJIT_FR1,jtFloat);
JitReg sJitTempF2(SLJIT_FR2,jtFloat);
JitReg sJitTempF3(SLJIT_FR3,jtFloat);
JitReg sJitTempF4(SLJIT_FR4,jtFloat);
JitReg sJitTempF5(SLJIT_FR5,jtFloat);

int sCtxReg = SLJIT_S0;
int sFrameReg = SLJIT_S1;
int sThisReg = SLJIT_S2;

JitReg sJitCtx(SLJIT_S0,jtPointer);
JitReg sJitFrame(SLJIT_S1,jtPointer);
JitReg sJitThis(SLJIT_S2,jtPointer);


JitVal sJitCtxFrame = sJitCtx.star(jtPointer, offsetof(CppiaCtx,frame));
JitVal sJitCtxPointer = sJitCtx.star(jtPointer, offsetof(CppiaCtx,pointer));

static double sZero = 0.0;

class CppiaJitCompiler : public CppiaCompiler
{
public:
   struct sljit_compiler *compiler;

   QuickVec<JumpId> allBreaks;
   LabelId continuePos;

   bool usesCtx;
   bool usesThis;
   bool usesFrame;

   int localSize;
   int frameSize;
   int baseFrameSize;

   int maxTempCount;
   int maxFTempCount;
   int maxLocalSize;


   CppiaJitCompiler(int inFrameSize)
   {
      maxTempCount = 0;
      maxFTempCount = 0;
      maxLocalSize = 0;
      localSize = 0;
      compiler = 0;
      usesFrame = false;
      usesThis = false;
      usesCtx = false;
      continuePos = 0;
      frameSize = baseFrameSize = sizeof(void *) + inFrameSize;
   }


   ~CppiaJitCompiler()
   {
      if (compiler)
      {
         sljit_free_compiler(compiler);
         compiler = 0;
      }
   }

   int getCurrentFrameSize()
   {
      return frameSize;
   }
   void restoreFrameSize(int inSize)
   {
      frameSize = inSize;
   }

   void addFrame(ExprType inType)
   {
      frameSize += getJitTypeSize( getJitType(inType) );
   }


   void beginGeneration(int inArgs)
   {
      compiler = sljit_create_compiler();

      int options = 0;
      // S0 is stack
      int saveds = inArgs;
      if (usesCtx && saveds<1)
         saveds = 1;
      if (usesFrame && saveds<2)
         saveds = 2;
      if (usesThis && saveds<3)
      {
         usesFrame = true;
         saveds = 3;
      }
      saveds = 3;
      int fsaveds = 0;
      int scratches = std::max(maxTempCount,inArgs);

      sljit_emit_enter(compiler, options, inArgs, scratches, saveds, maxFTempCount, fsaveds, maxLocalSize);
      usesCtx = true;

      if (usesFrame)
         move( sJitFrame, sJitCtxFrame );

      if (usesThis)
         move( sJitThis, JitFramePos(0) );

      frameSize = baseFrameSize;

   }

   CppiaFunc finishGeneration()
   {
      sljit_emit_return(compiler, SLJIT_UNUSED, SLJIT_UNUSED, 0);
      CppiaFunc func = (CppiaFunc)sljit_generate_code(compiler);
      sljit_free_compiler(compiler);
      compiler = 0;
      return func;
   }


   int  allocTempSize(int size)
   {
      int result = localSize;
      localSize += size;
      if (localSize>maxLocalSize)
         maxLocalSize = localSize;
      return result;
   }

   void freeTempSize(int inSize)
   {
      localSize -= inSize;
   }


   int  allocTemp(JitType inType)
   {
      return allocTempSize(getJitTypeSize(inType));
   }

   void freeTemp(JitType inType)
   {
      freeTempSize( getJitTypeSize(inType) );
   }

   
   LabelId setContinuePos(LabelId inNewPos)
   {
      LabelId oldPos = continuePos;
      continuePos = inNewPos;
      return oldPos;
   }

   void  addContinue()
   {
      jump(continuePos);
   }

   void addBreak()
   {
      allBreaks.push( jump(0) );
   }
   void setBreakTarget()
   {
      for(int i=0;i<allBreaks.size();i++)
         comeFrom(allBreaks[i]);
      allBreaks.setSize(0);
   }


   void setFunctionDebug()
   {
   }

   void setLineDebug()
   {
   }

   // Scriptable?
   void addReturn()
   {
      if (compiler)
         sljit_emit_return(compiler, SLJIT_UNUSED, SLJIT_UNUSED, 0);
   }


   void emit_ijump(const JitVal &inVal,int inArgs=1)
   {
      sljit_sw t = getTarget(inVal);
      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_CALL0+inArgs, t, getData(inVal));
   }

   JumpId jump(LabelId inTo)
   {
      if (compiler)
      {
         JumpId result = sljit_emit_jump(compiler, SLJIT_JUMP);
         if (inTo)
            sljit_set_label(result, inTo);

         return result;
      }
      return 0;
   }

   void jump(const JitVal &inWhere)
   {
      sljit_sw t = getTarget(inWhere);
      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_JUMP, t, getData(inWhere) );
   }

   JumpId compare(JitCompare condition, const JitVal &v0, const JitVal &v1, LabelId andJump)
   {
      sljit_sw t0 = getTarget(v0);
      sljit_sw t1 = getTarget(v1);
      if (compiler)
      {
         JumpId result = sljit_emit_cmp(compiler, condition, t0, getData(v0), t1, getData(v1) );
         if (andJump)
            sljit_set_label(result, andJump);
         return result;
      }
      return 0;
   }


   JumpId fcompare(JitCompare condition, const JitVal &v0, const JitVal &v1, LabelId andJump)
   {
      sljit_sw t0 = getTarget(v0);
      sljit_sw t1 = getTarget(v1);
      if (compiler)
      {
         JumpId result = sljit_emit_fcmp(compiler, condition, t0, getData(v0), t1, getData(v1) );
         if (andJump)
            sljit_set_label(result, andJump);
         return result;
      }
      return 0;
   }


   // Link
   void  comeFrom(JumpId inJump)
   {
      if (compiler)
      {
         sljit_label *label =  sljit_emit_label(compiler);
         sljit_set_label(inJump, label);
      }
   }
   LabelId addLabel()
   {
      if (compiler)
         return sljit_emit_label(compiler);
      return  0;
   }


   JitVal  addLocal(const char *inName, JitType inType)
   {
      return JitVal();
   }

   JitVal  functionArg(int inIndex)
   {
      return JitVal();
   }



   void emit_op1(sljit_si op, const JitVal &inArg0, const JitVal &inArg1)
   {
      sljit_sw t0 = getTarget(inArg0);
      sljit_sw t1 = getTarget(inArg1);
      if (compiler)
         sljit_emit_op1(compiler, op, t0, getData(inArg0), t1, getData(inArg1) );
   }


   void emit_op2(sljit_si op, const JitVal &inArg0, const JitVal &inArg1, const JitVal inArg2)
   {
      sljit_sw t0 = getTarget(inArg0);
      sljit_sw t1 = getTarget(inArg1);
      sljit_sw t2 = getTarget(inArg2);
      if (compiler)
         sljit_emit_op2(compiler, op, t0, getData(inArg0), t1, getData(inArg1), t2, getData(inArg2) );
   }

   void emit_fop1(sljit_si op, const JitVal &inArg0, const JitVal &inArg1)
   {
      sljit_sw t0 = getTarget(inArg0);
      sljit_sw t1 = getTarget(inArg1);
      if (compiler)
         sljit_emit_fop1(compiler, op, t0, getData(inArg0), t1, getData(inArg1) );
   }

   void emit_fop2(sljit_si op, const JitVal &inArg0, const JitVal &inArg1, const JitVal inArg2)
   {
      sljit_sw t0 = getTarget(inArg0);
      sljit_sw t1 = getTarget(inArg1);
      sljit_sw t2 = getTarget(inArg2);
      if (compiler)
         sljit_emit_fop2(compiler, op, t0, getData(inArg0), t1, getData(inArg1), t2, getData(inArg2) );
   }


   void setError(const char *inError)
   {
      printf("Error: %s\n", inError);
   }

   void crash()
   {
      if (compiler)
         sljit_emit_op0(compiler, SLJIT_BREAKPOINT);
      //move(sJitTemp0, (void *)0);
      //move(sJitTemp0.star(), (void *)0);
   }


   sljit_si getTarget(const JitVal &inVal)
   {
      switch(inVal.position)
      {
         case jposRegister:
            if (inVal.type==jtFloat)
            {
               if (inVal.reg0>=maxFTempCount)
                  maxFTempCount = inVal.reg0+1;
            }
            else if (inVal.reg0<=3 && inVal.reg0>=maxTempCount)
               maxTempCount = inVal.reg0;
            if (inVal.reg0==sFrameReg)
               usesFrame = true;
            else if (inVal.reg0==sThisReg)
               usesThis = true;

            return inVal.reg0;

         case jposLocal:
            return SLJIT_MEM1(SLJIT_SP);

         case jposArray:
            setError("TODO");
            break;

         case jposStar:
            if (inVal.reg0<=3 && inVal.reg0>=maxTempCount)
               maxTempCount = inVal.reg0;
            return SLJIT_MEM1(inVal.reg0);

         case jposStarReg:
            if (inVal.reg0<=3 && inVal.reg0>=maxTempCount)
               maxTempCount = inVal.reg0;
            if (inVal.reg1<=3 && inVal.reg1>=maxTempCount)
               maxTempCount = inVal.reg1;
            return SLJIT_MEM2(inVal.reg0,inVal.reg1);


         case jposFrame:
            usesFrame = true;
            return SLJIT_MEM1(SLJIT_S1);

         case jposThis:
            usesThis = true;
            return SLJIT_MEM1(SLJIT_S2);

         case jposPointerVal:
         case jposIntVal:
         case jposFloatVal:
            return SLJIT_IMM;


         case jposDontCare:
            setError("No position specification");
            *(int *)0=0;
            break;

         default:
            setError("Invalid position specification");
      }

      // ???
      return SLJIT_IMM;
   }

   sljit_sw getData(const JitVal &inVal)
   {
      switch(inVal.position)
      {
         case jposPointerVal:
            return (sljit_sw)inVal.pVal;

         case jposIntVal:
            return (sljit_sw)inVal.iVal;

         case jposFloatVal:
            // ? dval pointer?
            return (sljit_sw)inVal.dVal;

         case jposStarReg:
            // ? dval pointer?
            return (sljit_sw)inVal.offset;

         default:
            return (sljit_sw)inVal.offset;
      }
   }

   JitType getCommonType(const JitVal &inV1, const JitVal &inV2)
   {
      if (inV1.type==jtAny)
         return inV2.type;
      if (inV2.type==jtAny)
         return inV1.type;
      if (inV1.type==jtByte || inV2.type==jtByte)
         return jtByte;

      // Copying parts into string ...
      if (inV1.type==jtString && inV2.type==jtInt)
         return jtInt;
      if (inV1.type==jtString && inV2.type==jtPointer)
         return jtPointer;
      if (inV1.type==jtInt && inV2.type==jtString)
         return jtInt;
      if (inV1.type==jtPointer && inV2.type==jtString)
         return jtPointer;

      if (inV1.type!=inV1.type)
         setError("Type mismatch");
      return inV1.type;
   }


   // May required indirect offsets
   void move(const JitVal &inDest, const JitVal &inSrc)
   {
      if (inDest==inSrc || !inDest.valid())
         return;

      switch(getCommonType(inDest,inSrc))
      {
         case jtInt:
            emit_op1(SLJIT_MOV_SI, inDest, inSrc);
            break;
         case jtPointer:
            if (inSrc.reg0==sLocalReg && inSrc.position==jposRegister)
            {
               sljit_si tDest = getTarget(inDest);
               if (compiler)
                  sljit_get_local_base(compiler, tDest, getData(inDest), inSrc.offset );
            }
            else
               emit_op1(SLJIT_MOV_P, inDest, inSrc);
            break;
         case jtFloat:
            emit_fop1(SLJIT_DMOV, inDest, inSrc);
            break;

         case jtByte:
            emit_op1(SLJIT_IMOV_UB, inDest, inSrc);
            break;

         case jtString:
            if (!isMemoryVal(inDest) || !isMemoryVal(inSrc))
            {
               setError("Bad string move");
               *(int *)0=0;
            }
            else
            {
               emit_op1(SLJIT_MOV_SI, inDest, inSrc);
               emit_op1(SLJIT_MOV_P, inDest + 4, inSrc + 4);
            }
            break;

         case jtVoid:
         case jtUnknown:
            setError("Bad move target");
            *(int *)0=0;
      }
   }


   void setFramePointer(int inArgStart)
   {
      add( sJitCtxPointer, sJitFrame, inArgStart );
   }

   void makeAddress(const JitVal &outAddress, const JitVal &inSrc)
   {
      if (inSrc.offset==0)
      {
         move(outAddress, inSrc.getReg() );
      }
      else
      {
         add(outAddress, inSrc.getReg(), inSrc.offset );
      }
   }


   void convert(const JitVal &inSrc, ExprType inSrcType, const JitVal &inTarget, ExprType inToType)
   {
      if (!inTarget.valid())
         return;

      if (inSrcType==inToType)
      {
         getTarget(inSrc);
         getTarget(inTarget);
         move( inTarget, inSrc );
      }
      else if (inToType==etObject)
      {
         switch(inSrcType)
         {
            case etInt:
               callNative( (void *)intToObj, inSrc.as(jtInt));
               if (inTarget!=sJitReturnReg)
                  emit_op1(SLJIT_MOV_P, inTarget, sJitReturnReg.as(jtPointer));
               break;

            case etFloat:
               callNative( (void *)floatToObj, inSrc.as(jtFloat));
               if (inTarget!=sJitReturnReg)
                  emit_op1(SLJIT_MOV_P, inTarget, sJitReturnReg);
               break;

            case etObject:
               move(inTarget.as(jtPointer), inSrc.as(jtPointer));
               break;

            case etString:
               add( sJitArg0, inSrc.getReg().as(jtPointer), inSrc.offset );
               callNative( (void *)strToObj, sJitArg0);
               if (inTarget!=sJitReturnReg)
                  emit_op1(SLJIT_MOV_P, inTarget, sJitReturnReg);
               break;

            default:
               move(inTarget, (void *)0);
         }
      }
      else if (inToType==etString)
      {
         switch(inSrcType)
         {
            case etInt:
               if (inSrc==sJitTemp1)
               {
                  move(sJitArg0, inSrc);
                  add( sJitTemp1, inTarget.getReg(), inTarget.offset );
                  callNative( (void *)intToStr, sJitArg0.as(jtInt), sJitTemp1.as(jtPointer));
               }
               else
               {
                  makeAddress(sJitTemp1,inTarget);
                  callNative( (void *)intToStr, inSrc.as(jtInt), sJitTemp1.as(jtPointer) );
               }
               break;
            case etFloat:
               makeAddress(sJitTemp1,inTarget);
               callNative( (void *)floatToStr, inSrc.as(jtFloat), sJitTemp1 );
               break;
            case etObject:
               if (inSrc==sJitTemp1)
               {
                  move(sJitArg0, inSrc);
                  add( sJitTemp1, inTarget.getReg(), inTarget.offset );
                  callNative( (void *)objToStr, sJitArg0.as(jtPointer), sJitTemp1.as(jtPointer) );
               }
               else
               {
                  makeAddress(sJitTemp1,inTarget);
                  callNative( (void *)objToStr, inSrc.as(jtPointer), sJitTemp1.as(jtPointer) );
               }
               break;


            default:
               printf("TODO - other to string\n");
         }
      }
      else if (inToType==etFloat)
      {
         switch(inSrcType)
         {
            case etInt:
               emit_fop1( SLJIT_CONVD_FROMI, inTarget.as(jtInt), inSrc.as(jtFloat) );
               break;

            case etObject:
               if (inSrc==sJitTemp1)
               {
                  move(sJitArg0, inSrc);
                  makeAddress(sJitTemp1,inTarget);
                  callNative( (void *)objToFloat, sJitArg0.as(jtPointer), sJitTemp1.as(jtPointer));
               }
               else
               {
                  if (isMemoryVal(inTarget))
                  {
                     makeAddress(sJitTemp1,inTarget);
                     callNative( (void *)objToFloat, inSrc.as(jtPointer), sJitTemp1.as(jtPointer) );
                  }
                  else
                  {
                     JitTemp temp(this,jtFloat);
                     makeAddress(sJitTemp1,temp);
                     callNative( (void *)objToFloat, inSrc.as(jtPointer), sJitTemp1.as(jtPointer) );
                     move(inTarget,temp);
                  }
               }
               break;

            default:
               printf("TODO - other to float\n");
         }
      }
      else if (inToType==etInt)
      {
         switch(inSrcType)
         {
            case etFloat:
               emit_fop1( SLJIT_CONVD_FROMI, inTarget.as(jtFloat), inSrc.as(jtInt) );
               break;

            case etObject:
               callNative( (void *)objToInt, inSrc.as(jtPointer));
               move(inTarget.as(jtInt), sJitReturnReg );
               break;

            default:
               printf("TODO - other to int\n");
         }
      }
      else
      {
         printf("unknown convert %d\n", inToType);
         *(int *)0=0;
      }
   }

   void convertResult(ExprType inSrcType, const JitVal &inTarget, ExprType inToType)
   {
      if (inSrcType!=etVoid && inSrcType!=etNull && inToType!=etVoid && inToType!=etNull)
      {
         convert( JitFramePos(frameSize, getJitType(inSrcType)), inSrcType, inTarget, inToType);
      }
   }

   void returnNull(const JitVal &inTarget, ExprType inToType)
   {
      switch(inToType)
      {
         case etObject:
            move(inTarget, (void *)0);
            break;
         case etString:
            move(inTarget.as(jtPointer), (int)0);
            move(inTarget.as(jtInt) + 4, (void *)0);
            break;
         case etInt:
            move(inTarget, (int)0);
            break;
         case etFloat:
            move(inTarget, JitVal((void *)&sZero).as(jtFloat) );
            break;
      }
   }


   void traceObject(const char *inLabel, const JitVal &inObj)
   {
      callNative( (void *)my_trace_obj_func, JitVal((void *)inLabel), inObj);
   }
   void tracePointer(const char *inLabel, const JitVal &inPtr)
   {
      callNative( (void *)my_trace_ptr_func, JitVal((void *)inLabel), inPtr);
   }
   void traceInt(const char *inLabel, const JitVal &inValue)
   {
      callNative( (void *)my_trace_int_func, JitVal((void *)inLabel), inValue);
   }
   void traceFloat(const char *inLabel, const JitVal &inValue)
   {
      if (isMemoryVal(inValue))
      {
         makeAddress(sJitArg1,inValue);
         callNative( (void *)my_trace_float_func, (void *)inLabel, sJitArg1);
      }
      else
      {
         JitTemp tmp(this,etFloat);
         move(tmp,inValue);
         makeAddress(sJitArg1,tmp);
         callNative( (void *)my_trace_float_func, (void *)inLabel,sJitArg1);
      }
   }

   void trace(const char *inText)
   {
      callNative( (void *)my_trace_func, JitVal( (void *)inText ));
   }
   void traceStrings(const char *inS0,const char *inS1)
   {
      callNative( (void *)my_trace_strings, JitVal( (void *)inS0 ), JitVal( (void *)inS1 ));
   }



   void set(const JitVal &inDest, const JitVal &inSrc)
   {
   }

   void add(const JitVal &inDest, const JitVal &v0, const JitVal &v1 )
   {
      if (v0.type==jtFloat)
      {
         emit_fop2(SLJIT_DADD, inDest, v0, v1);
      }
      else if (v0.reg0==sLocalReg && v0.position==jposRegister)
      {
         sljit_si tDest = getTarget(inDest);
         sljit_si t0 = getTarget(v0);
         sljit_si t1 = getTarget(v1);
         if (compiler)
            sljit_get_local_base(compiler, tDest, getData(inDest), v1.offset );
      }
      else
      {
         // SLJIT_ADD ?
         emit_op2(SLJIT_IADD, inDest, v0, v1);
      }
   }


   void bitOr(const JitVal &inDest, const JitVal &v0, const JitVal &v1 )
   {
     emit_op2(SLJIT_IOR, inDest, v0, v1);
   }

   void mult(const JitVal &inDest, const JitVal &v0, const JitVal &v1, bool asFloat )
   {
      sljit_si tDest = getTarget(inDest);
      sljit_si t0 = getTarget(v0);
      sljit_si t1 = getTarget(v1);
      bool isFloat = v0.type==jtFloat;

      if (asFloat != isFloat)
      {
         if (isFloat)
         {
            emit_fop2(SLJIT_DMUL, sJitTempF0, v0, v1 );
            convert( sJitTempF0, etFloat, inDest, etInt );
         }
         else
         {
            emit_op2(SLJIT_MUL, sJitTemp0, v0.as(jtInt), v1.as(jtInt) );
            convert( sJitTemp0, etInt, inDest, etFloat );
         }
      }
      else
      {
         if (isFloat)
         {
            emit_fop2(SLJIT_DMUL, inDest, v0, v1 );
         }
         else
            emit_op2(SLJIT_MUL, inDest, v0, v1 );
      }
   }

   void sub(const JitVal &inDest, const JitVal &v0, const JitVal &v1, bool asFloat )
   {
      sljit_si tDest = getTarget(inDest);
      sljit_si t0 = getTarget(v0);
      sljit_si t1 = getTarget(v1);
      bool isFloat = v0.type==jtFloat;

      if (asFloat != isFloat)
      {
         if (isFloat)
         {
            emit_fop2(SLJIT_DSUB, sJitTempF0, v0, v1 );
            convert( sJitTempF0, etFloat, inDest, etInt );
         }
         else
         {
            emit_op2(SLJIT_SUB, sJitTemp0, v0.as(jtInt), v1.as(jtInt) );
            convert( sJitTemp0, etInt, inDest, etFloat );
         }
      }
      else
      {
         if (isFloat)
            emit_fop2(SLJIT_DSUB, inDest, v0, v1 );
         else
            emit_op2(SLJIT_SUB, inDest, v0, v1 );
      }

   }

   void fdiv(const JitVal &inDest, const JitVal &v0, const JitVal &v1)
   {
      emit_fop2(SLJIT_DDIV, inDest, v0, v1 );
   }

   void divmod()
   {
      if (sJitTemp1.reg0>=maxTempCount)
         maxTempCount = sJitTemp1.reg0;
      if (compiler)
         sljit_emit_op0(compiler,SLJIT_ILSDIV);
   }




   void allocArgs(int inCount)
   {
   }
   /*
   void call(CppiaFunc func,JitType inReturnType)
   {
   }

   void call(const JitVal &func,JitType inReturnType)
   {
   }
   */

   void call(const JitVal &func,const JitVal &inArg0)
   {
      move(sJitArg0,inArg0);
      emit_ijump(func,1);
   }

   void callNative(void *func)
   {
      if (maxTempCount<1)
         maxTempCount =1;
      if (compiler)
      {
         sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));
      }
   }
   void callNative(void *func, const JitVal &inArg0)
   {
      if (maxTempCount<1)
         maxTempCount =1;
      int restoreLocal = -1;

      if (inArg0.type==jtFloat || inArg0.type==jtString)
      {
         if (isMemoryVal(inArg0))
            add( sJitArg0, inArg0.getReg().as(jtPointer), inArg0.offset);
         else
         {
            restoreLocal = localSize;
            JitLocalPos temp(allocTemp(jtFloat),jtFloat);
            move( temp, inArg0 );
            add(sJitArg0, temp.getReg().as(jtPointer), temp.offset);
         }
      }
      else
         move( sJitArg0, inArg0);

      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));

      if (restoreLocal>=0)
         localSize = restoreLocal;
   }
   void callNative(void *func, const JitVal &inArg0, const JitVal &inArg1)
   {
      if (maxTempCount<2)
         maxTempCount =2;

      int restoreLocal = -1;

      if (inArg0.type==jtFloat || inArg0.type==jtString)
      {
         if (isMemoryVal(inArg0))
            add( sJitArg0, inArg0.getReg().as(jtPointer), inArg0.offset);
         else
         {
            restoreLocal = localSize;
            JitLocalPos temp(allocTemp(jtFloat),jtFloat);
            move( temp, inArg0 );
            add(sJitArg0, temp.getReg().as(jtPointer), temp.offset);
         }
      }
      else
         move( sJitArg0, inArg0);



      if (inArg1.type==jtFloat || inArg0.type==jtString)
      {
         if (isMemoryVal(inArg1))
            add( sJitArg1, inArg1.getReg().as(jtPointer), inArg1.offset);
         else
         {
            restoreLocal = localSize;
            JitLocalPos temp(allocTemp(jtFloat),jtFloat);
            move( temp, inArg1 );
            add(sJitArg1, temp.getReg().as(jtPointer), temp.offset);
         }
      }
      else
         move( sJitArg1, inArg1);



      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_CALL2, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));

      if (restoreLocal>=0)
         localSize = restoreLocal;

   }

   void callNative(void *func, const JitVal &inArg0, const JitVal &inArg1, const JitVal &inArg2)
   {
      if (maxTempCount<3)
         maxTempCount =3;

      move( sJitArg0, inArg0);
      move( sJitArg1, inArg1);
      move( sJitArg2, inArg2);
      if (compiler)
      {
         sljit_emit_ijump(compiler, SLJIT_CALL3, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));
      }
   }

};

void CppiaCompiler::freeCompiled(CppiaFunc inFunc)
{
}

CppiaCompiler *CppiaCompiler::create(int inFrameSize)
{
   return new CppiaJitCompiler(inFrameSize);
}


}

