#include <hxcpp.h>

#include <map>
#include <vector>
#include <set>


#ifdef _WIN32

#define GC_WIN32_THREADS
#include <time.h>

#else

#ifndef HX_INTERNAL_GC
extern "C" {
#include <gc_config_macros.h>
#include <gc_pthread_redirects.h>
//#include "private/gc_priv.h"
}
#endif


#endif




#ifndef HX_INTERNAL_GC
#include <gc.h>
#include <gc_allocator.h>
#endif

#include "RedBlack.h"

namespace hx
{

// On Mac, we need to call GC_INIT before first alloc
static int sNeedGCInit = true;

static bool sgAllocInit = 0;



void *Object::operator new( size_t inSize, bool inContainer )
{
#ifdef HX_INTERNAL_GC
   return InternalNew(inSize,true);
#else
#ifdef __APPLE__
   if (sNeedGCInit)
   {
      sNeedGCInit = false;
      GC_no_dls = 1;
      GC_INIT();
   }
#endif
   void *result = inContainer ?  GC_MALLOC(inSize) : GC_MALLOC_ATOMIC(inSize);

   return result;
#endif
}

}


void *String::operator new( size_t inSize )
{
#ifdef HX_INTERNAL_GC
   return hx::InternalNew(inSize,false);
#else
   return GC_MALLOC_ATOMIC(inSize);
#endif
}


#ifndef HX_INTERNAL_GC

static hx::Object ***sgExtraRoots = 0;
static int sgExtraAlloced = 0;
static int sgExtraSize = 0;

namespace hx
{

void GCAddRoot(hx::Object **inRoot)
{
	if (sgExtraSize+1>=sgExtraAlloced)
	{
		sgExtraAlloced = 10 + sgExtraSize*3/2;
		if (!sgExtraRoots)
		   sgExtraRoots = (hx::Object ***)GC_MALLOC( sgExtraAlloced * sizeof(hx::Object **) );
		else
		   sgExtraRoots = (hx::Object ***)GC_REALLOC( sgExtraRoots, sgExtraAlloced * sizeof(hx::Object **) );
	}
	sgExtraRoots[ sgExtraSize++ ] = inRoot;
}

void GCRemoveRoot(hx::Object **inRoot)
{
	int i;
	for(i=0;i<sgExtraSize;i++)
	{
		if (sgExtraRoots[i]==inRoot)
		{
			sgExtraRoots[i] = sgExtraRoots[sgExtraSize-1];
			sgExtraRoots[--sgExtraSize] = 0;
			break;
		}
	}
}

} // end namespace hx
#endif // End not internal


void __hxcpp_collect()
{
   #ifdef HX_INTERNAL_GC
	hx::InternalCollect();
   #else
   GC_gcollect();
   #endif
}



#ifndef HX_INTERNAL_GC
static void hxcpp_finalizer(void * obj, void * client_data)
{
   hx::finalizer f = (hx::finalizer)client_data;
   if (f)
      f( (hx::Object *)obj );
}
#endif


namespace hx
{

void GCAddFinalizer(hx::Object *v, finalizer f)
{
   if (v)
   {
#ifdef HX_INTERNAL_GC
      throw Dynamic(HX_CSTRING("Add finalizer error"));
#else
      GC_register_finalizer(v,hxcpp_finalizer,(void *)f,0,0);
#endif
   }
}


void RegisterObject(Object **inPtr)
{
#ifndef HX_INTERNAL_GC
   GC_add_roots((char *)inPtr, (char *)inPtr + sizeof(void *) );

#endif
}

void RegisterString(const wchar_t **inPtr)
{
#ifndef HX_INTERNAL_GC
   GC_add_roots((char *)inPtr, (char *)inPtr + sizeof(void *) );
#endif
}


void RegisterString(const char **inPtr)
{
#ifndef HX_INTERNAL_GC
   GC_add_roots((char *)inPtr, (char *)inPtr + sizeof(void *) );
#endif
}



void GCInit()
{
#ifndef HX_INTERNAL_GC
   if (sNeedGCInit)
   {
      sNeedGCInit = false;
      // We explicitly register all the statics, and there is quite a performance
      //  boost by doing this...

      GC_no_dls = 1;
      GC_INIT();
   }
#endif
}

#ifndef HX_INTERNAL_GC
// Stubs...
void MarkAlloc(void *inPtr HX_MARK_ADD_PARAMS) {  }
void MarkObjectAlloc(hx::Object *inPtr HX_MARK_ADD_PARAMS) { }
void EnterGCFreeZone() { }
void ExitGCFreeZone() { }
#endif





HX_CHAR *NewString(int inLen)
{
#ifdef HX_INTERNAL_GC
   HX_CHAR *result =  (HX_CHAR *)hx::InternalNew( (inLen+1)*sizeof(HX_CHAR), false );
#else
   HX_CHAR *result =  (HX_CHAR *)GC_MALLOC_ATOMIC((inLen+1)*sizeof(HX_CHAR));
#endif
   result[inLen] = '\0';
   return result;

}

void *NewGCBytes(void *inData,int inSize)
{
#ifdef HX_INTERNAL_GC
   void *result =  hx::InternalNew(inSize,false);
#else
   void *result =  GC_MALLOC(inSize);
#endif
   if (inData)
   {
      memcpy(result,inData,inSize);
   }
   return result;
}


void *NewGCPrivate(void *inData,int inSize)
{
#ifdef HX_INTERNAL_GC
   void *result =  InternalNew(inSize,false);
#else
   void *result =  GC_MALLOC_ATOMIC(inSize);
#endif
   if (inData)
   {
      memcpy(result,inData,inSize);
   }
   return result;
}


void *GCRealloc(void *inData,int inSize)
{
#ifdef HX_INTERNAL_GC
   return InternalRealloc(inData,inSize);
#else
   return GC_REALLOC(inData, inSize );
#endif
}


// Put this function here so we can be reasonablly sure that "this" register and
// the 4 registers that may be used to pass args are on the stack.
int RegisterCapture::Capture(int *inTopOfStack,int **inBuf,int &outSize,int inMaxSize)
{
	int here = 0;
	int size = ( (char *)inTopOfStack - (char *)&here )/sizeof(void *);
	if (size>inMaxSize)
		size = inMaxSize;
	outSize = size;
	if (size>0)
	   memcpy(inBuf,&here,size*sizeof(void*));
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




void __hxcpp_enable(bool inEnable)
{
#ifndef HX_INTERNAL_GC
   if (inEnable)
      GC_enable();
   else
      GC_disable();
#else
	hx::InternalEnableGC(inEnable);
#endif
}


