#include <hxcpp.h>

#include <map>
#include <vector>
#include <set>
#include <stdlib.h>

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




namespace hx
{
#if defined(HX_MACOS) || defined(HX_WINDOWS) || defined(HX_LINUX) || defined(__ORBIS__)
int sgMinimumWorkingMemory       = 20*1024*1024;
int sgMinimumFreeSpace           = 10*1024*1024;
#else
int sgMinimumWorkingMemory       = 8*1024*1024;
int sgMinimumFreeSpace           = 4*1024*1024;
#endif
// Once you use more than the minimum, this kicks in...
int sgTargetFreeSpacePercentage  = 100;






// Called internally before and GC operations
void CommonInitAlloc()
{
   #if !defined(HX_WINRT) && !defined(__SNC__) && !defined(__ORBIS__)
   const char *minimumWorking = getenv("HXCPP_MINIMUM_WORKING_MEMORY");
   if (minimumWorking)
   {
      int mem =  atoi(minimumWorking);
      if (mem>0)
         sgMinimumWorkingMemory = mem;
   }

   const char *minimumFreeSpace = getenv("HXCPP_MINIMUM_FREE_SPACE");
   if (minimumFreeSpace)
   {
      int mem =  atoi(minimumFreeSpace);
      if (mem>0)
         sgMinimumFreeSpace = mem;
   }


   const char *targetFree = getenv("HXCPP_TARGET_FREE_SPACE");
   if (targetFree)
   {
      int percent =  atoi(targetFree);
      if (percent>0)
         sgTargetFreeSpacePercentage = percent;
   }
   #endif
}

} // end namespace hx



void *String::operator new( size_t inSize )
{
   return hx::InternalNew(inSize,false);
}


void __hxcpp_collect(bool inMajor)
{
   hx::InternalCollect(inMajor,inMajor);
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
   char *result =  (char *)hx::InternalNew( (inLen+1)*sizeof(char), false );
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

void  __hxcpp_set_minimum_working_memory(int inBytes)
{
   hx::sgMinimumWorkingMemory = inBytes;
}

void  __hxcpp_set_minimum_free_space(int inBytes)
{
   hx::sgMinimumFreeSpace = inBytes;
}

void  __hxcpp_set_target_free_space_percentage(int inPercentage)
{
   hx::sgTargetFreeSpacePercentage = inPercentage;
}

bool __hxcpp_is_const_string(const ::String &inString)
{
   #ifdef HXCPP_ALIGN_ALLOC
   // Unaligned must be native const
   if ( ((size_t)inString.__s) & 0x3 )
      return true;
   #endif
   return ((unsigned int *)inString.raw_ptr())[-1] & HX_GC_CONST_ALLOC_BIT;
}

