#include <hxObject.h>
#include <map>
#include <vector>
#include <set>

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
#else
typedef std::set<hxObject **> RootSet;
static RootSet sgRootSet;
#endif


// On Mac, we need to call GC_INIT before first alloc
static int sNeedGCInit = true;

static bool sgAllocInit = 0;


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
   void *result = inContainer ?  GC_MALLOC(inSize) : GC_MALLOC_ATOMIC(inSize);

   return result;
#endif
}


void *String::operator new( size_t inSize )
{
#ifdef INTERNAL_GC
   return hxInternalNew(inSize);
#else
   return GC_MALLOC_ATOMIC(inSize);
#endif
}


#ifndef INTERNAL_GC
static hxObject ***sgExtraRoots = 0;
static int sgExtraAlloced = 0;
static int sgExtraSize = 0;

#endif

void hxGCAddRoot(hxObject **inRoot)
{
#ifdef INTERNAL_GC
   sgRootSet.insert(inRoot);
#else
	if (sgExtraSize+1>=sgExtraAlloced)
	{
		sgExtraAlloced = 10 + sgExtraSize*3/2;
		if (!sgExtraRoots)
		   sgExtraRoots = (hxObject ***)GC_MALLOC( sgExtraAlloced * sizeof(hxObject **) );
		else
		   sgExtraRoots = (hxObject ***)GC_REALLOC( sgExtraRoots, sgExtraAlloced * sizeof(hxObject **) );
	}
	sgExtraRoots[ sgExtraSize++ ] = inRoot;
#endif
}

void hxGCRemoveRoot(hxObject **inRoot)
{
#ifdef INTERNAL_GC
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




void __hxcpp_collect()
{
   #ifdef INTERNAL_GC
   hxInternalCollect();
   #else
   GC_gcollect();
   #endif
}

#ifdef INTERNAL_GC
void hxGCMarkNow()
{
   hxMarkClassStatics();
   hxLibMark();

   for(RootSet::iterator i = sgRootSet.begin(); i!=sgRootSet.end(); ++i)
   {
		hxObject *&obj = **i;
      HX_MARK_OBJECT( obj );
   }
}
#endif


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
      throw Dynamic(STR(L"Add finalizer error"));
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
#else
   hxInternalEnableGC(inEnable);
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
   {
      memcpy(result,inData,inSize);
   }
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
   {
      memcpy(result,inData,inSize);
   }
   return result;
}


void *hxGCRealloc(void *inData,int inSize)
{
#ifdef INTERNAL_GC
   return hxInternalRealloc(inData,inSize);
#else
   return GC_REALLOC(inData, inSize );
#endif
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
		HX_MARK_STRING(i->first.__s,wchar_t *);
		HX_MARK_OBJECT(i->second.mPtr);
   }
}

// --- Anon -----
//

void hxAnon_obj::Destroy(hxObject *inObj)
{
   hxAnon_obj *obj = dynamic_cast<hxAnon_obj *>(inObj);
	if (obj)
		delete obj->mFields;
}

hxAnon_obj::hxAnon_obj()
{
   mFields = new hxFieldMap;
	#ifdef INTERNAL_GC
	mFinalizer = new hxInternalFinalizer(this);
	mFinalizer->mFinalizer = Destroy;
	#else
	mFinalizer = 0;
	#endif
}

void hxAnon_obj::__Mark()
{
   hxFieldMapMark(mFields);
	#ifdef INTERNAL_GC
	mFinalizer->Mark();
	#endif
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



