#include "GcRegCapture.h"

#ifdef HXCPP_CAPTURE_SETJMP // {

// Nothing

#elif defined(HXCPP_CAPTURE_x86) // }  {

#pragma optimize( "", off )


namespace hx {

void CaptureX86(RegisterCaptureBuffer &outBuffer)
{
   void *regEsi;
   void *regEdi;
   void *regEbx;
   #ifdef __GNUC__
   asm ("mov %%esi, %0\n\t" : "=r" (regEsi) );
   asm ("mov %%edi, %0\n\t" : "=r" (regEdi) );
   asm ("mov %%ebx, %0\n\t" : "=r" (regEbx) );
   #else
   __asm {
      mov regEsi, esi
      mov regEdi, edi
      mov regEbx, ebx
   }
   #endif
   outBuffer.esi = regEsi;
   outBuffer.edi = regEdi;
   outBuffer.ebx = regEbx;
}

} // end namespace hx

#elif defined(HXCPP_CAPTURE_x64) // } {

#if !defined(__GNUC__)
#include <windows.h>
#endif

namespace hx {

void CaptureX64(RegisterCaptureBuffer &outBuffer)
{
   #if !defined(__GNUC__)
      CONTEXT context;

      context.ContextFlags = CONTEXT_INTEGER;
      RtlCaptureContext(&context);

      outBuffer.rbx = (void *)context.Rbx;
      outBuffer.rbp = (void *)context.Rbp;
      outBuffer.rdi = (void *)context.Rdi;
      outBuffer.r12 = (void *)context.R12;
      outBuffer.r13 = (void *)context.R13;
      outBuffer.r14 = (void *)context.R14;
      outBuffer.r15 = (void *)context.R15;
      memcpy(outBuffer.xmm, &context.Xmm0, sizeof(outBuffer.xmm));
   #else
      void *regBx;
      void *regBp;
      void *regDi;
      void *reg12;
      void *reg13;
      void *reg14;
      void *reg15;
      asm ("movq %%rbx, %0\n\t" : "=r" (regBx) );
      asm ("movq %%rbp, %0\n\t" : "=r" (regBp) );
      asm ("movq %%rdi, %0\n\t" : "=r" (regDi) );
      asm ("movq %%r12, %0\n\t" : "=r" (reg12) );
      asm ("movq %%r13, %0\n\t" : "=r" (reg13) );
      asm ("movq %%r14, %0\n\t" : "=r" (reg14) );
      asm ("movq %%r15, %0\n\t" : "=r" (reg15) );
      outBuffer.rbx = regBx;
      outBuffer.rbp = regBp;
      outBuffer.r12 = reg12;
      outBuffer.r13 = reg13;
      outBuffer.r14 = reg14;
      outBuffer.r15 = reg15;
   #endif
}

} // end namespace hx


#else // }  {

#include <string.h>

namespace hx {

// Put this function here so we can be reasonablly sure that "this" register and
// the 4 registers that may be used to pass args are on the stack.
int RegisterCapture::Capture(int *inTopOfStack,int **inBuf,int &outSize,int inMaxSize, int *inBottom)
{
	int size = ( (char *)inTopOfStack - (char *)inBottom )/sizeof(void *);
	if (size>inMaxSize)
		size = inMaxSize;
	outSize = size;
	if (size>0)
	   memcpy(inBuf,inBottom,size*sizeof(void*));
	return 1;
}


RegisterCapture *gRegisterCaptureInstance = 0;
RegisterCapture *RegisterCapture::Instance()
{
	if (!gRegisterCaptureInstance)
		gRegisterCaptureInstance = new RegisterCapture();
	return gRegisterCaptureInstance;
}

} // end namespace hx

#endif // }

