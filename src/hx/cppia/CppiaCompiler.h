#ifndef HX_CPPIA_COMPILER_H_INCLUDED
#define HX_CPPIA_COMPILER_H_INCLUDED

#ifdef HX_ARM // TODO v7, 64
  #define SLJIT_CONFIG_ARM_V5 1
#else
   #ifdef HXCPP_M64
      #define SLJIT_CONFIG_X86_64 1
   #else
      #define SLJIT_CONFIG_X86_32 1
   #endif
#endif

#define SLJIT_UTIL_STACK 0

#include "sljit_src/sljitLir.h"
#include <vector>


extern "C" struct sljit_jump;
extern "C" struct sljit_label;

namespace hx
{

enum JitPosition
{
   jposDontCare,
   jposCtx,
   jposFrame,
   jposLocal,
   jposThis,
   jposArray,
   jposRegister,
   jposStar,
   jposPointerVal,
   jposIntVal,
   jposFloatVal,
};

enum JitType
{
  jtAny,
  jtPointer,
  jtString,
  jtFloat,
  jtInt,
  jtVoid,
  jtUnknown,
};


JitType getJitType(ExprType inType);
int getJitTypeSize(JitType inType);

struct JitVal
{
   JitPosition    position;
   JitType        type;
   union
   {
     struct
     {
        int offset;
        unsigned short reg0;
        unsigned short reg1;
     };
     void   *pVal;
     double dVal;
     int    iVal;
   };

   JitVal(JitType inType=jtVoid, int inOffset=0, JitPosition inPosition=jposDontCare,int inReg0=0, int inReg1=0)
   { 
      position = inPosition;
      type = inType;
      offset = inOffset;
      reg0 = inReg0;
      reg1 = inReg1;
   }
   JitVal(int inValue)
   {
      position = jposIntVal;
      type = jtInt;
      iVal = inValue;
   }
   JitVal(double inValue)
   {
      position = jposFloatVal;
      type = jtFloat;
      dVal = inValue;
   }
   JitVal(void *inValue)
   {
      position = jposPointerVal;
      type = jtPointer;
      pVal = inValue;
   }

   JitVal operator +(int inDiff) const { return JitVal(type, offset+inDiff, position, reg0, reg1); }
   JitVal as(JitType type) const { return JitVal(type, offset, position, reg0, reg1); }
   bool valid() const { return type!=jtVoid; }

   bool operator==(const JitVal &inOther) const
   {
      return position==inOther.position && type==inOther.type && offset==inOther.offset && reg0==inOther.reg0 && reg1==inOther.reg1; 
   }
   bool operator!=(const JitVal &inOther) const { return !(*this == inOther); }

   JitVal getReg() const { return JitVal(jtPointer, 0, jposRegister, reg0, reg1); }
};

struct JitReg : public JitVal
{
   JitReg(int inReg, JitType inType=jtAny) : JitVal(inType, 0, jposRegister, inReg, 0) { }

   JitVal star(JitType inType=jtPointer, int inOffset=0) { return JitVal(inType, inOffset, jposStar, reg0, reg1); }
};


extern int sFrameReg;
struct JitFramePos : public JitVal
{
   JitFramePos(int inOffset, JitType inType=jtPointer) : JitVal(inType, inOffset, jposFrame, sFrameReg) { }
};

extern int sLocalReg;
struct JitLocalPos : public JitVal
{
   JitLocalPos(int inOffset, JitType inType=jtPointer) : JitVal(inType, inOffset, jposLocal, sLocalReg) { }
};

extern int sThisReg;
struct JitThisPos : public JitVal
{
   JitThisPos(int inOffset, JitType inType=jtPointer) : JitVal(inType, inOffset, jposThis, sThisReg) { }
};


enum JitCompare
{
   // Pointer compare
   cmpP_EQUAL =             0,
   cmpP_ZERO =              0,
   cmpP_NOT_EQUAL =         1,
   cmpP_NOT_ZERO =          1,

   cmpP_LESS =              2,
   cmpP_GREATER_EQUAL =     3,
   cmpP_GREATER =           4,
   cmpP_LESS_EQUAL =        5,
   cmpP_SIG_LESS =          6,
   cmpP_SIG_GREATER_EQUAL = 7,
   cmpP_SIG_GREATER =       8,
   cmpP_SIG_LESS_EQUAL =    9,

   cmpP_OVERFLOW =          10,
   cmpP_NOT_OVERFLOW =      11,

   cmpP_MUL_OVERFLOW =      12,
   cmpP_MUL_NOT_OVERFLOW =  13,


   // Integer compares
   cmpI_EQUAL =             0 | 0x100,
   cmpI_ZERO =              0 | 0x100,
   cmpI_NOT_EQUAL =         1 | 0x100,
   cmpI_NOT_ZERO =          1 | 0x100,

   cmpI_LESS =              2 | 0x100,
   cmpI_GREATER_EQUAL =     3 | 0x100,
   cmpI_GREATER =           4 | 0x100,
   cmpI_LESS_EQUAL =        5 | 0x100,
   cmpI_SIG_LESS =          6 | 0x100,
   cmpI_SIG_GREATER_EQUAL = 7 | 0x100,
   cmpI_SIG_GREATER =       8 | 0x100,
   cmpI_SIG_LESS_EQUAL =    9 | 0x100,

   cmpI_OVERFLOW =          10 | 0x100,
   cmpI_NOT_OVERFLOW =      11 | 0x100,

   cmpI_MUL_OVERFLOW =      12 | 0x100,
   cmpI_MUL_NOT_OVERFLOW =  13 | 0x100,

   /* Floating point comparison types - double. */
   cmpD_EQUAL =             14,
   cmpD_NOT_EQUAL =         15,
   cmpD_LESS =              16,
   cmpD_GREATER_EQUAL =     17,
   cmpD_GREATER =           18,
   cmpD_LESS_EQUAL =        19,
   cmpD_UNORDERED =         20,
   cmpD_ORDERED =           21,

   /* Floating point comparison types - single. */
   cmpS_EQUAL =             14 | 0x100,
   cmpS_NOT_EQUAL =         15 | 0x100,
   cmpS_LESS =              16 | 0x100,
   cmpS_GREATER_EQUAL =     17 | 0x100,
   cmpS_GREATER =           18 | 0x100,
   cmpS_LESS_EQUAL =        19 | 0x100,
   cmpS_UNORDERED =         20 | 0x100,
   cmpS_ORDERED =           21 | 0x100,

};

bool isMemoryVal(const JitVal &inVal);

extern JitReg sJitFrame;

extern JitReg sJitTemp0;
extern JitReg sJitTemp1;
extern JitReg sJitTemp2;

extern JitReg sJitReturnReg;
extern JitReg sJitArg0;
extern JitReg sJitArg1;
extern JitReg sJitArg2;
extern JitReg sJitCtx;
extern JitVal sJitCtxPointer;
extern JitVal sJitCtxFrame;

typedef sljit_label *LabelId;
typedef sljit_jump  *JumpId;

typedef void (*CppiaFunc)(CppiaCtx *inCtx);

class CppiaCompiler
{
public:
   static CppiaCompiler *create(int inFrameSize);
   virtual ~CppiaCompiler() { }

   static void freeCompiled(CppiaFunc inFunc);

   virtual void setError(const char *inError) = 0;

   virtual void allocArgs(int inCount)=0;
   virtual int getCurrentFrameSize() = 0;
   virtual void restoreFrameSize(int inSize) = 0;
   virtual void addFrame(ExprType inType) = 0;
   virtual int  allocTemp(JitType inType) = 0;
   virtual void freeTemp(JitType inType) = 0;
   virtual JitVal  addLocal(const char *inName, JitType inType) = 0;
   virtual JitVal functionArg(int inIndex) = 0;

   
   virtual void convert(const JitVal &inSrc, ExprType inSrcType, const JitVal &inTarget, ExprType inToType) = 0;
   virtual void convertResult(ExprType inSrcType, const JitVal &inTarget, ExprType inToType) = 0;

   virtual void beginGeneration(int inArgs=1) = 0;
   virtual CppiaFunc finishGeneration() = 0;

   virtual void setFunctionDebug() = 0;
   virtual void setLineDebug() = 0;

   // Unconditional
   virtual JumpId jump(LabelId inTo=0) = 0;
   virtual void   jump(const JitVal &inWhere) = 0;
   // Conditional
   virtual JumpId compare(JitCompare condition, const JitVal &v0, LabelId andJump=0) = 0;
   virtual JumpId compare(JitCompare condition, const JitVal &v0, const JitVal &v1, LabelId andJump=0) = 0;
   // Link
   virtual void  comeFrom(JumpId inWhere) = 0;
   virtual LabelId  addLabel() = 0;

   inline  JumpId notNull(const JitVal &v0) { return compare(cmpP_NOT_ZERO, v0); }

   virtual void setFramePointer(int inArgStart) = 0;

   // Scriptable?
   virtual void addReturn() = 0;
   virtual void pushScope() = 0;
   virtual void popScope() = 0;
   virtual void trace(const char *inValue) = 0;
   virtual void traceObject(const char *inLabel, const JitVal &inValue) = 0;
   virtual void tracePointer(const char *inLabel, const JitVal &inValue) = 0;
   virtual void traceInt(const char *inLabel, const JitVal &inValue) = 0;

   virtual void set(const JitVal &inDest, const JitVal &inSrc) = 0;
   virtual void add(const JitVal &inDest, const JitVal &v0, const JitVal &v1 ) = 0;
   virtual void move(const JitVal &inDest, const JitVal &src) = 0;
   //virtual void compare(Condition condition,const JitVal &v0, const JitVal &v1) = 0;


   virtual JitVal call(CppiaFunc func, JitType inReturnType=jtVoid)=0;
   virtual JitVal call(const JitVal &inFunc, JitType inReturnType=jtVoid)=0;
   virtual JitVal call(const JitVal &inFunc, const JitVal &inArg0, JitType inReturnType=jtVoid)=0;

   virtual JitVal callNative(void *func, JitType inReturnType=jtVoid)=0;
   virtual JitVal callNative(void *func, const JitVal &inArg0, JitType inReturnType=jtVoid)=0;
   virtual JitVal callNative(void *func, const JitVal &inArg0, const JitVal &inArg1, JitType inReturnType=jtVoid)=0;
   virtual JitVal callNative(void *func, const JitVal &inArg0, const JitVal &inArg1, const JitVal &inArg2, JitType inReturnType=jtVoid)=0;


};


struct JitTemp : public JitVal
{
   CppiaCompiler *compiler;

   JitTemp(CppiaCompiler *inCompiler, JitType inType)
      : JitVal(inType, inCompiler->allocTemp(inType), jposLocal, sLocalReg)
   {
      compiler = inCompiler;
   }

   ~JitTemp()
   {
      compiler->freeTemp(type);
   }
};


struct AutoFramePos
{
   CppiaCompiler *compiler;
   int framePos;

   AutoFramePos(CppiaCompiler *inCompiler)
   {
      compiler = inCompiler;
      framePos=compiler->getCurrentFrameSize();
   }

   ~AutoFramePos()
   {
      compiler->restoreFrameSize(framePos);
   }
};

/*


struct Addr
{
   inline Addr(sljit_si inBase, sljit_sw inMod=0) : base(inBase), mod(inMod) { }
   Addr offset(int inDelta) const { return Addr(base,mod+inDelta); }
   bool operator==(const Addr &a) const { return a.base==base && a.mod==mod; }
   bool operator!=(const Addr &a) const { return a.base!=base || a.mod!=mod; }
   sljit_si base;
   sljit_sw mod;
};

struct AddrVoid : public Addr
{
   AddrVoid() : Addr(SLJIT_UNUSED,0) { }
};

struct Reg : public Addr
{
   inline Reg(int inReg) : Addr(SLJIT_R0+inReg) { }
   int getReg() const { return base; }
};


struct FReg : public Reg
{
   inline FReg(int inReg) : Reg(SLJIT_FR0+inReg) { }
};



struct SReg : public Reg
{
   inline SReg(int inReg) : Reg(SLJIT_S(inReg)-SLJIT_R0) { }
};

struct SRegMem : public Addr
{
   inline SRegMem(int inReg,int inOffset=0) : Addr(SLJIT_FS(inReg),inOffset) { }
};




struct CtxReg : public SReg
{
   inline CtxReg() : SReg(0) { }
};


struct ThisReg : public SReg
{
   inline ThisReg() : SReg(1) { }
};


struct TempReg : public SReg
{
   inline TempReg() : SReg(2) { }
};




struct ArrayAddr : public Addr
{
   inline ArrayAddr(const Reg &inRegBase, const Reg &inRegOffset, int inShift=0) :
       Addr(SLJIT_MEM2(inRegBase.getReg(),inRegOffset.getReg()), inShift) { }
};

struct StarAddr : public Addr
{
   inline StarAddr(const Reg &inRegBase, int inOffset=0) :
       Addr(SLJIT_MEM1(inRegBase.getReg()), inOffset) { }
};

struct CtxMemberVal : public StarAddr
{
   inline CtxMemberVal(int inOffset) : StarAddr(CtxReg(), inOffset) { }
};


struct AddrStack : public Addr
{
   inline AddrStack(int inOffset) : Addr(SLJIT_MEM1(SLJIT_SP), inOffset) { }
};



struct ConstValue : public Addr
{
   inline ConstValue(int inValue) : Addr(SLJIT_IMM, inValue) { }
   inline ConstValue(const void *inValue) : Addr(SLJIT_IMM, (sljit_sw)inValue) { }
};


struct ConstRef : public Addr
{
   inline ConstRef(const void *inAddr) : Addr(SLJIT_MEM0(), (sljit_sw)inAddr) { }
};




typedef void (*CppiaCompiled)(struct CppiaCtx *inCtx);


typedef std::vector< sljit_jump * > ComeFrom;

class CppiaCompiler
{
   sljit_compiler* compiler;
   int             maxTempSize;
   int             tempSize;
   bool            useThis;
   bool            useFTemp;

public:
   ComeFrom        *exceptionHandler;

    inline CppiaCompiler()
    {
       compiler = sljit_create_compiler();
       tempSize = maxTempSize = 0;
       useThis = false;
       useFTemp = false;
       exceptionHandler = 0;
    }
    ~CppiaCompiler()
    {
       sljit_free_compiler(compiler);
    }
 
    void registerThis() { useThis = true; }
    void registerFTemp() { useFTemp = true; }

    int addTemp(int inSize)
    {
       int result = tempSize;
       tempSize += inSize;
       if (tempSize>maxTempSize)
          maxTempSize = tempSize;
       return result;
    }

    void releaseTemp(int inSize)
    {
       tempSize -= inSize;
    }
       
    //Addr getCtx() { return AddrStack(ctxOffset); }
 

    static void SLJIT_CALL doTrace(const char *inVal)
    {
       printf("trace %s\n",inVal);
    }

    void trace(const char *inText)
    {
	    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0,  0,  SLJIT_IMM, (sljit_sw)inText );
       sljit_emit_ijump(compiler, SLJIT_CALL1, SLJIT_IMM, SLJIT_FUNC_OFFSET(doTrace));
    }

    void enter(int options,int inStackSize)
    {
       int args = 1;
       int saved = 3;
       int localSize = inStackSize + maxTempSize;
       int fscratches = useFTemp ? 1 : 0;
       int fsaved = 0;
       int scratches = 3;
       sljit_emit_enter(compiler, options, args, scratches, saved, fscratches, fsaved, localSize);
       // S0 = Arg0 = Ctx 
       // S1 = this
       // S2 = temp
       if (useThis)
       {
          move( Reg(0), CtxMemberVal( offsetof(CppiaCtx, frame) ) );
          move( SReg(1), StarAddr(Reg(0)) );
       }
       if (inStackSize>0 && inStackSize<=20)
       {
          for(int i=0;i<inStackSize;i+=4)
             move32( AddrStack(i), ConstValue(0) );
       }
       else if (inStackSize>0)
       {
          trace("memset");
          sljit_get_local_base(compiler, SLJIT_R0, 0, 0);
          move32( Reg(1), ConstValue(0) );
          move32( Reg(2), ConstValue(inStackSize) );
          call(memset,3);
       }
    }

    void ret()
    {
	    sljit_emit_return(compiler, SLJIT_UNUSED, 0, 0);
    }
 
    template<typename T>
    void call(T inFunc, int inArgs)
    {
       sljit_emit_ijump(compiler, SLJIT_CALL0+inArgs, SLJIT_IMM, SLJIT_FUNC_OFFSET(inFunc));
    }

    sljit_jump *jump(sljit_si type, sljit_uw target = 0)
    {
       sljit_jump *result = sljit_emit_jump(compiler, type);
       if (target)
          sljit_set_target(result,target);
       return result;
    }

    sljit_jump *ifZero(const Addr &value)
    {
       return sljit_emit_cmp(compiler, SLJIT_EQUAL, value.base, value.mod, SLJIT_IMM, 0 );
    }

    sljit_jump *ifNotZero(const Addr &value)
    {
       return sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, value.base, value.mod, SLJIT_IMM, 0 );
    }




    void jumpHere(sljit_jump *inJump) 
    {
       sljit_label *label =  sljit_emit_label(compiler);
       sljit_set_label(inJump, label);
    }



    inline void emit(sljit_si op, const Addr &out, const Addr &inA, const Addr &inB)
    {
       sljit_emit_op2(compiler, op,
            out.base, out.mod,
            inA.base, inA.mod,
            inB.base, inB.mod );
    }


    inline void emitf(sljit_si op, const Addr &out, const Addr &inA)
    {
       sljit_emit_fop1(compiler, op,
            out.base, out.mod,
            inA.base, inA.mod );
    }



    inline void emit(sljit_si op, const Addr &out, const Addr &inA)
    {
       sljit_emit_op1(compiler, op,
            out.base, out.mod,
            inA.base, inA.mod );
    }

    inline void move32(const Addr &out, const Addr &inA)
    {
       emit(SLJIT_IMOV, out, inA);
    }


    inline void move(const Addr &out, const Addr &inA,int inMode = SLJIT_MOV)
    {
       emit(inMode, out, inA);
    }


    inline void add(const Addr &out, const Addr &inA, const Addr &inB)
    {
       emit(SLJIT_ADD, out, inA, inB);
    }

    CppiaCompiled generate()
    {
       return (CppiaCompiled)sljit_generate_code(compiler);

    }

    inline static void freeCompiled(CppiaCompiled inCompiled)
    {
       sljit_free_code( (void*) inCompiled );
    }

};

struct AllocTemp : public Addr
{
   int size;
   CppiaCompiler &compiler;

   AllocTemp(CppiaCompiler &inCompiler, int inSize=sizeof(void *)) :
        Addr( SLJIT_MEM1(SLJIT_SP) ), compiler(inCompiler), size(inSize)
   {
      mod = compiler.addTemp(size);
   }
   ~AllocTemp() { compiler.releaseTemp(size); }
   
};

*/


}

#endif
