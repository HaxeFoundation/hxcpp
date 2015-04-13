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


namespace hx
{

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


}

#endif
