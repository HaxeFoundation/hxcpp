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


int sCtxReg = SLJIT_S0;
int sThisReg = SLJIT_S1;


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

JitVal sJitReturnReg(jtAny, 0, jposRegister, SLJIT_R0);
JitVal sJitArg0(jtAny, 0, jposRegister, SLJIT_R0);
JitVal sJitArg1(jtAny, 0, jposRegister, SLJIT_R1);
JitVal sJitArg2(jtAny, 0, jposRegister,SLJIT_R2);

JitVal sJitReg0(jtAny, 0, jposRegister, SLJIT_R0);

JitVal sJitCtx(jtPointer, 0, jposRegister, sCtxReg);
JitVal sJitThis(jtPointer, 0, jposRegister, sThisReg);
JitVal sJitCtxFrame(jtPointer, offsetof(CppiaCtx,frame), jposStar, sCtxReg);
JitVal sJitCtxPointer(jtPointer, offsetof(CppiaCtx,pointer), jposStar, sCtxReg);


class CppiaJitCompiler : public CppiaCompiler
{
public:
   struct sljit_compiler *compiler;


   bool usesCtx;
   bool usesThis;
   int localSize;
   int maxTempCount;
   int maxFtempCount;
   int maxLocalSize;

   CppiaJitCompiler()
   {
      maxTempCount = 0;
      maxFtempCount = 0;
      maxLocalSize = 0;
      localSize = 0;
      compiler = 0;
      usesThis = false;
      usesCtx = false;
   }


   ~CppiaJitCompiler()
   {
      if (compiler)
      {
         sljit_free_compiler(compiler);
         compiler = 0;
      }
   }

   void beginGeneration(int inArgs)
   {
      compiler = sljit_create_compiler();

      int options = 0;
      // S0 is stack
      int saveds = inArgs;
      if (usesCtx && saveds<1)
         saveds = 1;
      if (usesThis && saveds<2)
         saveds = 2;
      int fsaveds = 0;
      int scratches = std::max(maxTempCount,inArgs);

      sljit_emit_enter(compiler, options, inArgs, scratches, saveds, maxFtempCount, fsaveds, maxLocalSize);
      if (usesThis)
         move( sJitThis, sJitCtxFrame );
   }

   CppiaFunc finishGeneration()
   {
      sljit_emit_return(compiler, SLJIT_UNUSED, SLJIT_UNUSED, 0);
      CppiaFunc func = (CppiaFunc)sljit_generate_code(compiler);
      sljit_free_compiler(compiler);
      compiler = 0;
      return func;
   }

   int  allocTemp(JitType inType)
   {
      int result = localSize;
      int size = getJitTypeSize(inType);
      localSize += size;
      if (localSize>maxLocalSize)
         maxLocalSize = localSize;
      return result;
   }

   void freeTemp(JitType inType)
   {
      localSize -= getJitTypeSize(inType);
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
   }
   void pushScope()
   {
   }
   void popScope()
   {
   }

   LabelId  addLabel()
   {
      return 0;
   }

   void     comeFrom(JumpId)
   {
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
      if (compiler)
         sljit_emit_op1(compiler, op, getTarget(inArg0), getData(inArg0), getTarget(inArg1), getData(inArg1) );
   }

   void setError(const std::string &inError)
   {
      printf("Error: %s\n", inError.c_str()); 
   }

   bool isMemoryVal(const JitVal &inVal)
   {
      switch(inVal.position)
      {
         case jposStack:
         case jposLocal:
         case jposThis:
            return true;
         default:
            return false;
      }
   }


   sljit_si getTarget(const JitVal &inVal)
   {
      switch(inVal.position)
      {
         case jposRegister:
            return inVal.reg0;

         case jposStack:
         case jposLocal:
         case jposArray:
            setError("TODO");
            break;

         case jposStar:
            return SLJIT_MEM1(inVal.reg0);

         case jposThis:
            usesThis = true;
            return SLJIT_S1;

         case jposPointerVal:
         case jposIntVal:
         case jposFloatVal:
            return SLJIT_IMM;


         case jposDontCare:
            setError("No position specification");
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
            // ?
            return (sljit_sw)inVal.dVal;

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

      if (inV1.type!=inV1.type)
         setError("Type mismatch");
      return inV1.type;
   }


   // May required indirect offsets
   void move(const JitVal &inDest, const JitVal &inSrc)
   {
      switch(getCommonType(inDest,inSrc))
      {
         case jtInt:
            emit_op1(SLJIT_MOV_SI, inDest, inSrc);
            break;
         case jtPointer:
            emit_op1(SLJIT_MOV_P, inDest, inSrc);
            break;
         case jtFloat:
            emit_op1(SLJIT_DMOV, inDest, inSrc);
            break;

         case jtString:
            if (!isMemoryVal(inDest) || !isMemoryVal(inSrc))
               setError("Bad string move");
            else
            {
               emit_op1(SLJIT_MOV_SI, inDest, inSrc);
               emit_op1(SLJIT_MOV_P, inDest + 4, inSrc + 4);
            }

         case jtVoid:
         case jtUnknown:
            setError("Bad move target");
      }
   }


   void  trace(const char *inText)
   {
      if (compiler)
      {
         sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0,  0,  SLJIT_IMM, (sljit_sw)inText );
         sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(my_trace_func));
      }
   }

   void  trace(const char *inLabel, hx::Object *inObj)
   {
   }

   void set(const JitVal &inDest, const JitVal &inSrc)
   {
   }

   JitVal add(const JitVal &v0, const JitVal &v1, JitVal inDest )
   {
      return JitVal();
   }

   void compare(Condition condition,const JitVal &v0, const JitVal &v1)
   {
   }

   JumpId jump(LabelId inLabel, Condition condition)
   {
      return 0;
   }
   JitVal allocArgs(int inCount)
   {
      return JitVal();
   }

   JitVal call(CppiaFunc func)
   {
      return JitVal();
   }
   JitVal callNative(void *func, int inArgCount, JitType inReturnType)
   {
      return JitVal();
   }

   virtual JitVal callNative(void *func, JitType inReturnType)
   {
      if (compiler)
      {
         sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));
      }
      return JitVal(inReturnType,0,jposRegister);
   }
   virtual JitVal callNative(void *func, const JitVal &inArg0, JitType inReturnType)
   {
      if (maxTempCount<1)
         maxTempCount =1;
      move( sJitArg0, inArg0);
      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));

      return JitVal(inReturnType,0,jposRegister);
   }
   virtual JitVal callNative(void *func, const JitVal &inArg0, const JitVal &inArg1, JitType inReturnType)
   {
      if (maxTempCount<2)
         maxTempCount =2;
      move( sJitArg0, inArg0);
      move( sJitArg1, inArg1);
      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_CALL2, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));

      return JitVal(inReturnType,0,jposRegister);
   }
   virtual JitVal callNative(void *func, const JitVal &inArg0, const JitVal &inArg1, const JitVal &inArg2, JitType inReturnType)
   {
      if (maxTempCount<3)
         maxTempCount =3;
      move( sJitArg0, inArg0);
      move( sJitArg1, inArg1);
      move( sJitArg2, inArg2);
      if (compiler)
         sljit_emit_ijump(compiler, SLJIT_CALL3, SLJIT_IMM, SLJIT_FUNC_OFFSET(func));

      return JitVal(inReturnType,0,jposRegister);
   }


};

void CppiaCompiler::freeCompiled(CppiaFunc inFunc)
{
}

CppiaCompiler *CppiaCompiler::create()
{
   return new CppiaJitCompiler();
}


}

