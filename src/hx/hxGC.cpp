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

#include "RedBlack.h"

// On Mac, we need to call GC_INIT before first alloc
static int sNeedGCInit = true;

static bool sgAllocInit = 0;


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
   void *result = inContainer ?  GC_MALLOC(inSize) : GC_MALLOC_ATOMIC(inSize);

   return result;
#endif
}


void *String::operator new( size_t inSize )
{
#ifdef INTERNAL_GC
   return hxInternalNew(inSize,false);
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

#ifndef INTERNAL_GC
// Stubs...
void hxMarkAlloc(void *inPtr) {  }
void hxMarkObjectAlloc(hxObject *inPtr) { }
void hxEnterGCFreeZone() { }
void hxExitGCFreeZone() { }
#endif


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
   wchar_t *result =  (wchar_t *)hxInternalNew( (inLen+1)*sizeof(wchar_t), false );
#else
   wchar_t *result =  (wchar_t *)GC_MALLOC_ATOMIC((inLen+1)*sizeof(wchar_t));
#endif
   result[inLen] = '\0';
   return result;

}

void *hxNewGCBytes(void *inData,int inSize)
{
#ifdef INTERNAL_GC
   void *result =  hxInternalNew(inSize,false);
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
   void *result =  hxInternalNew(inSize,false);
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


int DoCompare(const String &inA, const String &inB)
{
	return inA.compare(inB);
}

class hxFieldMap : public RBTree<String,Dynamic>
{
};

hxFieldMap *hxFieldMapCreate()
{
	return (hxFieldMap *)(RBTree<String,Dynamic>::Create());
}

bool hxFieldMapGet(hxFieldMap *inMap, const String &inName, Dynamic &outValue)
{
	Dynamic *value = inMap->Find(inName);
	if (!value)
		return false;
	outValue = *value;
	return true;
}

bool hxFieldMapGet(hxFieldMap *inMap, int inID, Dynamic &outValue)
{
	Dynamic *value = inMap->Find(__hxcpp_field_from_id(inID));
	if (!value)
		return false;
	outValue = *value;
	return true;
}

void hxFieldMapSet(hxFieldMap *inMap, const String &inName, const Dynamic &inValue)
{
	inMap->Insert(inName,inValue);
}

struct KeyGetter
{
	KeyGetter(Array<String> &inArray) : mArray(inArray)  { }
	void Visit(void *, const String &inStr, const Dynamic &) { mArray->push(inStr); }
	Array<String> &mArray;
};

void hxFieldMapAppendFields(hxFieldMap *inMap,Array<String> &outFields)
{
	KeyGetter getter(outFields);
	inMap->Iterate(getter);
}

struct Marker
{
	void Visit(void *inPtr, const String &inStr, Dynamic &inDyn)
	{
		hxMarkAlloc(inPtr);
		HX_MARK_STRING(inStr.__s);
		if (inDyn.mPtr)
		   HX_MARK_OBJECT(inDyn.mPtr);
	}
};

void hxFieldMapMark(hxFieldMap *inMap)
{
	Marker m;
	inMap->Iterate(m);
}

// --- Anon -----
//

hxAnon_obj::hxAnon_obj()
{
   mFields = hxFieldMapCreate();
}

void hxAnon_obj::__Mark()
{
	// We will get mFields=0 here if we collect in the constructor before mFields is assigned
	if (mFields)
	{
	   hxMarkAlloc(mFields);
      hxFieldMapMark(mFields);
	}
}



Dynamic hxAnon_obj::__Field(const String &inString)
{
   Dynamic *v = mFields->Find(inString);
	if (!v)
      return null();
   return *v;
}

bool hxAnon_obj::__HasField(const String &inString)
{
   return mFields->Find(inString);
}


bool hxAnon_obj::__Remove(String inKey)
{
	return mFields->Erase(inKey);
}


Dynamic hxAnon_obj::__SetField(const String &inString,const Dynamic &inValue)
{
	mFields->Insert(inString,inValue);
   return inValue;
}

hxAnon_obj *hxAnon_obj::Add(const String &inName,const Dynamic &inValue)
{
	mFields->Insert(inName,inValue);
   if (inValue.GetPtr())
		inValue.GetPtr()->__SetThis(this);
   return this;
}

struct Stringer
{
	Stringer(String &inString) : result(inString), first(true) { }
	void Visit(void *, const String &inStr, Dynamic &inDyn)
	{
		if (first)
      {
         result += inStr + String(L"=",1) + (String)(inDyn);
         first = false;
      }
      else
         result += String(L", ") + inStr + String(L"=") + (String)(inDyn);
	}

	bool first;
	String &result;
};

String hxAnon_obj::toString()
{
   String result = String(L"{ ",2);
	Stringer stringer(result);
	mFields->Iterate(stringer);
   return result + String(L" }",2);
}

void hxAnon_obj::__GetFields(Array<String> &outFields)
{
	KeyGetter getter(outFields);
	mFields->Iterate(getter);
}



