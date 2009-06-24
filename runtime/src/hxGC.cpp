#include <hxObject.h>
#include <map>
#include <vector>

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

#define IS_CONST   0x40000000
#define IS_BYTES   0x20000000
#define ID_MASK    0x1fffffff

typedef unsigned int AllocData;

struct AllocInfo
{
   unsigned int     mSize;
   AllocData        *mPtr;
   finalizer        mFinalizer;
};



template<typename T>
struct QuickVec
{
	QuickVec() : mPtr(0), mAlloc(0), mSize(0) { } 
	inline void push(T inT)
	{
		if (mSize+1>=mAlloc)
		{
			mAlloc = 10 + (mSize*3/2);
			mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
		}
		mPtr[mSize++]=inT;
	}
	inline T pop()
	{
		return mPtr[--mSize];
	}
	inline bool empty() const { return !mSize; }
	inline int next()
	{
		if (mSize+1>=mAlloc)
		{
			mAlloc = 10 + (mSize*3/2);
			mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
		}
		return mSize++;
	}
	inline int size() const { return mSize; }
	inline T &operator[](int inIndex) { return mPtr[inIndex]; }

	int mAlloc;
	int mSize;
	T *mPtr;
};


QuickVec<AllocInfo> *sgActiveAllocs = 0;
QuickVec<int> *sgFreeIDs = 0;

static int sgTotalAllocBytes = 0;
static int sgTotalAllocObjs = 0;
static bool sgUsePool = false;

typedef QuickVec<AllocData *> SparePointers;


#define POOL_SIZE 65
static SparePointers sgSmallPool[POOL_SIZE];



void *hxInternalNew( size_t inSize, bool inIsObj = false )
{
   AllocData *data = 0;

   //printf("hxInternalNew %d\n", inSize);
   if (!sgActiveAllocs)
   {
      sgActiveAllocs = new QuickVec<AllocInfo>;
      sgFreeIDs = new QuickVec<int>;
   }
   // First run, we can't be sure the pool has initialised - but now we can.
   else if (inSize < POOL_SIZE )
   {
      SparePointers &spares = sgSmallPool[inSize];
      int n = spares.size();
      if (!spares.empty())
			data = spares.pop();
   }

   int id;
   if (!sgFreeIDs->empty())
		id = sgFreeIDs->pop();
   else
      id = sgActiveAllocs->next();

   if (!data)
      data = (AllocData *)malloc(sizeof(AllocData) + inSize );
   memset(data+1,0,inSize);
   *data = id | ( inIsObj ? 0 : IS_BYTES) ;
	AllocInfo &info = (*sgActiveAllocs)[id];

   info.mPtr = data;
   info.mSize = inSize;
   info.mFinalizer = 0;

   sgTotalAllocBytes += inSize;
   sgTotalAllocObjs++;

   return data + 1;
}


void *hxObject::operator new( size_t inSize, bool inContainer )
{
#ifdef INTERNAL_GC
   return hxInternalNew(inSize,true);
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
   // printf("Collecting...\n");
   if (!sgActiveAllocs) return;

   hxMarkClassStatics();
   hxLibMark();

   // And sweep ...
   int n = sgActiveAllocs->size();
   if (n)
   {
      AllocInfo *info_array = &(*sgActiveAllocs)[0];
      for(int i=0;i<n;i++)
      {
         AllocInfo &info = info_array[i];
         if (info.mPtr)
         {
            AllocData &val = *(info.mPtr);
            if ( !(val & HX_GC_MARKED) )
            {
                if ( !(val & IS_BYTES) )
                {
                   hxObject *obj = (hxObject *)(info.mPtr + 1);
                   if (info.mFinalizer)
                   {
                      info.mFinalizer(Dynamic(obj));
                   }
                   //printf("Free obj %d : %p\n", i, obj );
                }
                //else printf("Free bytes %d: %S\n",i, info.mPtr+1);
                sgFreeIDs->push(i);
                sgTotalAllocBytes -= info.mSize;
                sgTotalAllocBytes -= info.mSize;
                sgTotalAllocObjs--;
                if ( info.mSize<POOL_SIZE )
                   sgSmallPool[info.mSize].push(info.mPtr);
                else
                   free( info.mPtr );
                info.mPtr = 0;
                info.mFinalizer = 0;
            }
				else
					val ^= HX_GC_MARKED;
         }
      }
   }

   //printf("Total bytes = %d in %d objs (max objs %d)\n",
    //  sgTotalAllocBytes, sgTotalAllocObjs, sgActiveAllocs->size() );

   #else
   GC_gcollect();
   #endif
}


#ifndef INTERNAL_GC
static void hxcpp_finalizer(void * obj, void * client_data)
{
   finalizer f = (finalizer)client_data;
   if (f)
      f( (hxObject *)obj );
}
#endif




void hxGCAddFinalizer(hxObject *v, finalizer f)
{
   if (v)
   {
#ifdef INTERNAL_GC
      AllocData *data = ((AllocData *)v) - 1;
      int id = (*data) & ID_MASK;
      (*sgActiveAllocs)[id].mFinalizer = f;
#else
      GC_register_finalizer(v,hxcpp_finalizer,(void *)f,0,0);
#endif
   }
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
   if (inData==0)
      return hxInternalNew(inSize);

   AllocData *data = (AllocData*)(inData) - 1;
   int id = *data & ID_MASK;

   AllocData *new_data = (AllocData *)malloc(sizeof(AllocData) + inSize );
   *new_data =  id | ( *data & IS_BYTES );
   int old_size = (*sgActiveAllocs)[id].mSize;

   sgTotalAllocBytes -= old_size;

   (*sgActiveAllocs)[id].mPtr = new_data;
   (*sgActiveAllocs)[id].mSize = inSize;

   sgTotalAllocBytes += inSize;

   memcpy(new_data+1,inData, old_size);
   memset((char *)(new_data+1) + old_size, 0, inSize-old_size);

   free(data);

   return new_data+1;
#else
   return GC_REALLOC(inData, inSize );
#endif
}



void hxGCMark(hxObject *inPtr)
{
   AllocData &info = ((AllocData *)( dynamic_cast<void *>(inPtr)))[-1];
   if (  !(info & HX_GC_MARKED) )
   {
      info |= HX_GC_MARKED;
      inPtr->__Mark();
   }
}

void hxGCMarkString(const void *inPtr)
{
   // printf("Mark %S\n", inPtr);
   AllocData &info = ((AllocData *)inPtr)[-1];
   if (  !(info & HX_GC_MARKED) && !(info & IS_CONST) )
      info |= HX_GC_MARKED;
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

class hxFieldMap : public FieldMap
{
#ifndef INTERNAL_GC
public:
   void *operator new( size_t inSize ) { return GC_MALLOC(inSize); }
   void operator delete( void * ) { }
#endif

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

void hxFieldMapMark(hxFieldMap *inMap)
{
   for(hxFieldMap::const_iterator i = inMap->begin(); i!= inMap->end(); ++i)
   {
      hxGCMarkString(i->first.__s);
      hxObject *ptr = i->second.GetPtr();
      AllocData &info = ((AllocData *)( dynamic_cast<void *>(ptr)))[-1];
      if (  !(info & HX_GC_MARKED) )
      {
         info |= HX_GC_MARKED;
         ptr->__Mark();
      }
   }
}

// --- Anon -----
//

void hxAnon_obj::Destroy(Dynamic inObj)
{
   hxAnon_obj *obj = dynamic_cast<hxAnon_obj *>(inObj.GetPtr());
	if (obj)
		delete obj->mFields;
}

hxAnon_obj::hxAnon_obj()
{
   mFields = new hxFieldMap;
	#ifdef INTERNAL_GC
	hxGCAddFinalizer(this,Destroy);
	#endif
}

void hxAnon_obj::__Mark()
{
   hxFieldMapMark(mFields);
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



