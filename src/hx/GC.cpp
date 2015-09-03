#include <hxcpp.h>

#include <map>
#include <vector>
#include <set>


#ifdef _WIN32

#define GC_WIN32_THREADS
#include <time.h>

#endif

#ifdef HXCPP_TELEMETRY
extern void __hxt_new_string(void* result, int size);
#endif


void *String::operator new( size_t inSize )
{
   return hx::InternalNew(inSize,false);
}


void __hxcpp_collect(bool inMajor)
{
	hx::InternalCollect(inMajor,false);
}


void __hxcpp_gc_compact()
{
   int mem = hx::InternalCollect(true,true);
   while(true)
   {
      int compact = hx::InternalCollect(true,true);
      if (compact>=mem-16384)
         break;
      mem = compact;
   }
}

namespace hx
{

void GCAddFinalizer(hx::Object *v, finalizer f)
{
   if (v)
   {
      throw Dynamic(HX_CSTRING("Add finalizer error"));
   }
}

HX_CHAR *NewString(int inLen)
{
   HX_CHAR *result =  (HX_CHAR *)hx::InternalNew( (inLen+1)*sizeof(HX_CHAR), false );
   result[inLen] = '\0';
#ifdef HXCPP_TELEMETRY
   __hxt_new_string(result, inLen+1);
#endif
   return result;

}

void *NewGCBytes(void *inData,int inSize)
{
   void *result =  hx::InternalNew(inSize,false);
   if (inData)
   {
      memcpy(result,inData,inSize);
   }
   return result;
}


void *NewGCPrivate(void *inData,int inSize)
{
   void *result =  InternalNew(inSize,false);
   if (inData)
   {
      memcpy(result,inData,inSize);
   }
   return result;
}


void *GCRealloc(void *inData,int inSize)
{
   return InternalRealloc(inData,inSize);
}

#ifdef HXCPP_CAPTURE_x86 // {

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

#elif defined(HXCPP_CAPTURE_x64) // {

void CaptureX64(RegisterCaptureBuffer &outBuffer)
{
   void *regBx;
   void *regBp;
   void *reg12;
   void *reg13;
   void *reg14;
   void *reg15;
   #ifdef __GNUC__
   asm ("movq %%rbx, %0\n\t" : "=r" (regBx) );
   asm ("movq %%rbp, %0\n\t" : "=r" (regBp) );
   asm ("movq %%r12, %0\n\t" : "=r" (reg12) );
   asm ("movq %%r13, %0\n\t" : "=r" (reg13) );
   asm ("movq %%r14, %0\n\t" : "=r" (reg14) );
   asm ("movq %%r15, %0\n\t" : "=r" (reg15) );
   #else
   __asm {
      mov regBx, rbx
      mov regBp, rbp
      mov reg12, r12
      mov reg13, r13
      mov reg14, r14
      mov reg15, r15
   }
   #endif
   outBuffer.rbx = regBx;
   outBuffer.rbp = regBp;
   outBuffer.r12 = reg12;
   outBuffer.r13 = reg13;
   outBuffer.r14 = reg14;
   outBuffer.r15 = reg15;
}


#else // }  {

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

#endif // }


} // end namespace hx




void __hxcpp_enable(bool inEnable)
{
	hx::InternalEnableGC(inEnable);
}


