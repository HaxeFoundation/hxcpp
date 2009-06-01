#include <hxObject.h>
#include <map>
#include <vector>

//#define INTERNAL_GC

#ifdef _WIN32

#define GC_WIN32_THREADS
#include <time.h>

#else

#ifndef INTERNAL_GC
extern "C" {
#include <gc_config_macros.h>
#include <gc_pthread_redirects.h>
//#include "private/gc_priv.h"
}
#endif


#endif

#ifndef INTERNAL_GC
#include <gc.h>
#include <gc_allocator.h>
#endif

// On Mac, we need to call GC_INIT before first alloc
static int sNeedGCInit = true;

#define MARK_BIT   0x80000000
#define IS_CONST   0x40000000
#define SIZE_MASK  0x3fffffff

static unsigned int sCurrentMark = 0;

typedef unsigned int AllocInfo;

std::vector<AllocInfo *> *sgActiveAllocs = 0;
std::vector<int> *sgFreeIDs = 0;
static int sgTotalAllocBytes = 0;
static int sgTotalAllocObjs = 0;

void *hxInternalNew( size_t inSize )
{
   //printf("hxInternalNew %d\n", inSize);
   if (!sgActiveAllocs)
   {
      sgActiveAllocs = new std::vector<AllocInfo *>;
      sgFreeIDs = new std::vector<int>;
   }

   AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo) + inSize );
   *info = sCurrentMark | inSize;
   if (!sgFreeIDs->empty())
   {
      int n  = sgFreeIDs->size() - 1;
      int id = (*sgFreeIDs)[ n - 1];
      sgFreeIDs->resize(n);
      (*sgActiveAllocs)[id] = info;
   }
   else
   {
      sgActiveAllocs->push_back(info);
   }

   sgTotalAllocBytes += inSize;
   sgTotalAllocObjs++;

   return info + 1;

}


void *hxObject::operator new( size_t inSize, bool inContainer )
{
#ifdef INTERNAL_GC
   return hxInternalNew(inSize);
#else
#ifdef __APPLE__
   if (sNeedGCInit)
   {
      sNeedGCInit = false;
      GC_no_dls = 1;
      GC_INIT();
   }
#endif
   if (inContainer)
      return GC_MALLOC(inSize);
   else
      return GC_MALLOC_ATOMIC(inSize);
#endif
}



void __hxcpp_collect()
{
   #ifdef INTERNAL_GC
   printf("Collecting...\n");
   if (!sgActiveAllocs) return;

   sCurrentMark ^= MARK_BIT;
   hxMarkClassStatics();

   // And sweep ...
   int n = sgActiveAllocs->size();
   if (n)
   {
      AllocInfo **info = &(*sgActiveAllocs)[0];
      for(int i=0;i<n;i++)
      {
         if (info[i])
         {
            AllocInfo &val = *info[i];
            if ( (val & MARK_BIT) != sCurrentMark )
            {
                sgFreeIDs->push_back(i);
                printf("Free %d\n", (*info[i]) & SIZE_MASK);
                sgTotalAllocBytes -= (*info[i]) & SIZE_MASK;
                sgTotalAllocObjs--;
                free( info[i] );
                info[i] = 0;
            }
         }
      }
   }

   printf("Total bytes = %d in %d objs (max objs %d)\n",
      sgTotalAllocBytes, sgTotalAllocObjs, sgActiveAllocs->size() );

   #else
   GC_gcollect();
   #endif
}




void __RegisterStatic(void *inPtr,int inSize)
{
#ifndef INTERNAL_GC
   GC_add_roots((char *)inPtr, (char *)inPtr + inSize );
#endif
}


void hxGCInit()
{
#ifndef INTERNAL_GC
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


/*
void __hxcpp_reachable(hxObject *inPtr)
{
   void *ptr = (char *)inPtr;
   bool ok = GC_is_marked((ptr_t)GC_base(ptr));
   wprintf(L"Marked : %d\n",ok);
}
*/

#if defined(_MSC_VER)
struct ThreadData
{
   ThreadFunc func;
	void       *data;
};

unsigned int __stdcall thread_func(void *data)
{
	ThreadData d = *(ThreadData *)data;
	data=0;
	d.func(d.data);
	return 0;
}

#endif

void hxStartThread(ThreadFunc inFunc,void *inUserData)
{
// TODO
#ifndef INTERNAL_GC

#if defined(_MSC_VER)
	ThreadData *data = (ThreadData *)GC_MALLOC( sizeof(ThreadData) );
	data->func = inFunc;
	data->data = inUserData;
   GC_beginthreadex(0,0,thread_func,data,0,0);
#else
   pthread_t result;
   GC_pthread_create(&result,0,inFunc,inUserData);
#endif


#endif
}


void __hxcpp_enable(bool inEnable)
{
#ifndef INTERNAL_GC
   if (inEnable)
      GC_enable();
   else
      GC_disable();
#endif
}

wchar_t *hxNewString(int inLen)
{
#ifdef INTERNAL_GC
   wchar_t *result =  (wchar_t *)hxInternalNew( (inLen+1)*sizeof(wchar_t) );
#else
   wchar_t *result =  (wchar_t *)GC_MALLOC_ATOMIC((inLen+1)*sizeof(wchar_t));
#endif
   result[inLen] = '\0';
   return result;

}

void *hxNewGCBytes(void *inData,int inSize)
{
#ifdef INTERNAL_GC
   void *result =  hxInternalNew(inSize);
#else
   void *result =  GC_MALLOC(inSize);
#endif
   if (inData)
      memcpy(result,inData,inSize);
   return result;
}


void *hxNewGCPrivate(void *inData,int inSize)
{
#ifdef INTERNAL_GC
   void *result =  hxInternalNew(inSize);
#else
   void *result =  GC_MALLOC_ATOMIC(inSize);
#endif
   if (inData)
      memcpy(result,inData,inSize);
   return result;
}


void *hxGCRealloc(void *inData,int inSize)
{
#ifdef INTERNAL_GC
   // maybe should do something better here
   void *bytes = hxInternalNew(inSize);
   if (inData==0)
      return bytes;
   AllocInfo *info = (AllocInfo*)(inData) - 1;

   memcpy(bytes,inData, *info & SIZE_MASK );
   return bytes;
#else
   return GC_REALLOC(inData, inSize );
#endif
}



void hxGCMark(hxObject *inPtr)
{
   AllocInfo &info = ((AllocInfo *)inPtr)[-1];
   if (  (info & MARK_BIT) != sCurrentMark )
   {
      info ^= MARK_BIT;
      inPtr->__Mark();
   }
}

void hxGCMarkString(const void *inPtr)
{
   // printf("Mark %S\n", inPtr);
   AllocInfo &info = ((AllocInfo *)inPtr)[-1];
   if (  ((info & MARK_BIT) != sCurrentMark) && !(info & IS_CONST) )
      info ^= MARK_BIT;
}

// --- FieldObject ------------------------------



#ifdef _WIN32
typedef String StringKey;
#else
typedef const String StringKey;
#endif



#ifdef INTERNAL_GC
typedef std::map<StringKey,Dynamic, std::less<StringKey> > FieldMap;
#else
typedef gc_allocator< std::pair<StringKey, Dynamic> > MapAlloc;
typedef std::map<StringKey,Dynamic, std::less<StringKey>, MapAlloc > FieldMap;
#endif

class hxFieldMap : public  FieldMap
{
public:
   void *operator new( size_t inSize ) { return hxNewGCBytes(0,inSize); }
   void operator delete( void * ) { }

   hxFieldMap() { }
};

hxFieldMap *hxFieldMapCreate()
{
	return new hxFieldMap;
}

bool hxFieldMapGet(hxFieldMap *inMap, const String &inName, Dynamic &outValue)
{
	hxFieldMap::iterator i = inMap->find(inName);
	if (i==inMap->end())
		return false;
	outValue = i->second;
	return true;
}
bool hxFieldMapGet(hxFieldMap *inMap, int inID, Dynamic &outValue)
{
	hxFieldMap::iterator i = inMap->find(__hxcpp_field_from_id(inID));
	if (i==inMap->end())
		return false;
	outValue = i->second;
	return true;
}

void hxFieldMapSet(hxFieldMap *inMap, const String &inName, const Dynamic &inValue)
{
	(*inMap)[inName] = inValue;
}

void hxFieldMapAppendFields(hxFieldMap *inMap,Array<String> &outFields)
{
   for(hxFieldMap::const_iterator i = inMap->begin(); i!= inMap->end(); ++i)
      outFields->push(i->first);
}


// --- Anon -----
hxAnon_obj::hxAnon_obj()
{
   mFields = new hxFieldMap;
}


Dynamic hxAnon_obj::__Field(const String &inString)
{
   hxFieldMap::const_iterator f = mFields->find(inString);
   if (f==mFields->end())
      return null();
   return f->second;
}

bool hxAnon_obj::__HasField(const String &inString)
{
   hxFieldMap::const_iterator f = mFields->find(inString);
   return (f!=mFields->end());
}


bool hxAnon_obj::__Remove(String inKey)
{
   hxFieldMap::iterator f = mFields->find(inKey);
	bool found = f!=mFields->end();
	if (found)
	{
		mFields->erase(f);
	}
   return found;
}


Dynamic hxAnon_obj::__SetField(const String &inString,const Dynamic &inValue)
{
   (*mFields)[inString] = inValue;
   return inValue;
}

hxAnon_obj *hxAnon_obj::Add(const String &inName,const Dynamic &inValue)
{
   (*mFields)[inName] = inValue;
   if (inValue.GetPtr())
		inValue.GetPtr()->__SetThis(this);
   return this;
}


String hxAnon_obj::toString()
{
   String result = String(L"{ ",2);
   bool first = true;
   for(hxFieldMap::const_iterator i = mFields->begin(); i!= mFields->end(); ++i)
   {
      if (first)
      {
         result += i->first + String(L"=",1) + (String)(i->second);
         first = false;
      }
      else
         result += String(L", ") + i->first + String(L"=") + (String)(i->second);
   }
   return result + String(L" }",2);
}

void hxAnon_obj::__GetFields(Array<String> &outFields)
{
   for(hxFieldMap::const_iterator i = mFields->begin(); i!= mFields->end(); ++i)
      outFields->push(i->first);
}



