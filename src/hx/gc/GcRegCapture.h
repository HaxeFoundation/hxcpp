#ifndef HX_GC_HELPERS_INCLUDED
#define HX_GC_HELPERS_INCLUDED

#if defined(HX_WINDOWS) && defined(HXCPP_ARM64)
#define HXCPP_CAPTURE_SETJMP
#endif

#ifdef HXCPP_CAPTURE_SETJMP
   #include <setjmp.h>
#else

   #if (defined(HX_WINDOWS) || defined(HX_MACOS)) && !defined(HXCPP_M64)
      #define HXCPP_CAPTURE_x86
   #endif

   #if (defined(HX_MACOS) || (defined(HX_WINDOWS) && !defined(HX_WINRT)) || defined(_XBOX_ONE)) && defined(HXCPP_M64)
      #define HXCPP_CAPTURE_x64
   #endif

#endif


namespace hx
{

// Capture Registers
//
#ifdef HXCPP_CAPTURE_SETJMP // {

typedef jmp_buf RegisterCaptureBuffer;

#define CAPTURE_REGS \
   setjmp(mRegisterBuf);

#define CAPTURE_REG_START (int *)(&mRegisterBuf)
#define CAPTURE_REG_END (int *)(&mRegisterBuf+1)

#elif defined(HXCPP_CAPTURE_x86) // } {

struct RegisterCaptureBuffer
{
   void *ebx;
   void *edi;
   void *esi;
};

void CaptureX86(RegisterCaptureBuffer &outBuffer);

#define CAPTURE_REGS \
   hx::CaptureX86(mRegisterBuf);

#define CAPTURE_REG_START (int *)(&mRegisterBuf)
#define CAPTURE_REG_END (int *)(&mRegisterBuf+1)

#elif defined(HXCPP_CAPTURE_x64) // }  {


struct RegisterCaptureBuffer
{
   void *rbx;
   void *rbp;
   void *rdi;
   void *r12;
   void *r13;
   void *r14;
   void *r15;

   void *xmm[16*2];
};

void CaptureX64(RegisterCaptureBuffer &outBuffer);

#define CAPTURE_REGS \
   hx::CaptureX64(mRegisterBuf);

#define CAPTURE_REG_START (int *)(&mRegisterBuf)
#define CAPTURE_REG_END (int *)(&mRegisterBuf+1)

#else 


class RegisterCapture
{
public:
	virtual int Capture(int *inTopOfStack,int **inBuf,int &outSize,int inMaxSize,int *inDummy);
   static RegisterCapture *Instance();
};

typedef int *RegisterCaptureBuffer[20];

#define CAPTURE_REGS \
   hx::RegisterCapture::Instance()->Capture(mTopOfStack, \
                mRegisterBuf,mRegisterBufSize,20,mBottomOfStack); \

#define CAPTURE_REG_START (int *)mRegisterBuf
#define CAPTURE_REG_END (int *)(mRegisterBuf+mRegisterBufSize)

#endif // }



}


#endif
