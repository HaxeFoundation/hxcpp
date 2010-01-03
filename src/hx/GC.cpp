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
#else
typedef std::set<hx::Object **> RootSet;
static RootSet sgRootSet;
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
static hxObject ***sgExtraRoots = 0;
static int sgExtraAlloced = 0;
static int sgExtraSize = 0;

#endif

namespace hx
{

void GCAddRoot(hx::Object **inRoot)
{
#ifdef HX_INTERNAL_GC
   sgRootSet.insert(inRoot);
#else
	if (sgExtraSize+1>=sgExtraAlloced)
	{
		sgExtraAlloced = 10 + sgExtraSize*3/2;
		if (!sgExtraRoots)
		   sgExtraRoots = (hx::Object ***)GC_MALLOC( sgExtraAlloced * sizeof(hx::Object **) );
		else
		   sgExtraRoots = (hx::Object ***)GC_REALLOC( sgExtraRoots, sgExtraAlloced * sizeof(hx::Object **) );
	}
	sgExtraRoots[ sgExtraSize++ ] = inRoot;
#endif
}

void GCRemoveRoot(hx::Object **inRoot)
{
#ifdef HX_INTERNAL_GC
   sgRootSet.erase(inRoot);
#else
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
#endif
}


#ifdef HX_INTERNAL_GC
void GCMarkNow()
{
   MarkClassStatics();

   for(RootSet::iterator i = sgRootSet.begin(); i!=sgRootSet.end(); ++i)
   {
		Object *&obj = **i;
      HX_MARK_OBJECT( obj );
   }
}
#endif



} // end namespace hx


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
   finalizer f = (finalizer)client_data;
   if (f)
      f( (hxObject *)obj );
}
#endif


namespace hx
{

void GCAddFinalizer(hx::Object *v, finalizer f)
{
   if (v)
   {
#ifdef HX_INTERNAL_GC
      throw Dynamic(HX_STR(L"Add finalizer error"));
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
void MarkAlloc(void *inPtr) {  }
void MarkObjectAlloc(hx::Object *inPtr) { }
void EnterGCFreeZone() { }
void ExitGCFreeZone() { }
#endif





wchar_t *NewString(int inLen)
{
#ifdef HX_INTERNAL_GC
   wchar_t *result =  (wchar_t *)hx::InternalNew( (inLen+1)*sizeof(wchar_t), false );
#else
   wchar_t *result =  (wchar_t *)GC_MALLOC_ATOMIC((inLen+1)*sizeof(wchar_t));
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


const wchar_t *ConvertToWChar(const char *inStr, int *ioLen)
{
   int len = ioLen ? *ioLen : strlen(inStr);

   wchar_t *result = hx::NewString(len);
   int l = 0;

   unsigned char *b = (unsigned char *)inStr;
   for(int i=0;i<len;)
   {
      int c = b[i++];
      if (c==0) break;
      else if( c < 0x80 )
      {
        result[l++] = c;
      }
      else if( c < 0xE0 )
        result[l++] = ( ((c & 0x3F) << 6) | (b[i++] & 0x7F) );
      else if( c < 0xF0 )
      {
        int c2 = b[i++];
        result[l++] += ( ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( b[i++] & 0x7F) );
      }
      else
      {
        int c2 = b[i++];
        int c3 = b[i++];
        result[l++] += ( ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 << 6) & 0x7F) | (b[i++] & 0x7F) );
      }
   }
   result[l] = '\0';
   if (ioLen)
      *ioLen = l;
   return result;
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


