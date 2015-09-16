#include <hxcpp.h>

#include <map>
#include <vector>
#include <set>

#if defined(HXCPP_CAPTURE_x64) && !defined(__GNUC__)
#include <windows.h>
#endif



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



} // end namespace hx




void __hxcpp_enable(bool inEnable)
{
	hx::InternalEnableGC(inEnable);
}


