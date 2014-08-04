#include <hxcpp.h>
#include "RedBlack.h"
#include "FieldMap.h"

#include <hx/CFFI.h>
#include "Hash.h"
#include <map>


using namespace hx;

// --- IntHash ------------------------------------------------------------------------------

namespace hx
{

// --- FieldObject ------------------------------

FieldMap *FieldMapCreate()
{
	return (FieldMap *)(RBTree<String,Dynamic>::Create());
}

bool FieldMapGet(FieldMap *inMap, const String &inName, Dynamic &outValue)
{
	Dynamic *value = inMap->Find(inName);
	if (!value)
		return false;
	outValue = *value;
	return true;
}


bool FieldMapHas(FieldMap *inMap, const String &inName)
{
	return inMap->Find(inName);
}


bool FieldMapGet(FieldMap *inMap, int inID, Dynamic &outValue)
{
	Dynamic *value = inMap->Find(__hxcpp_field_from_id(inID));
	if (!value)
		return false;
	outValue = *value;
	return true;
}

void FieldMapSet(FieldMap *inMap, const String &inName, const Dynamic &inValue)
{
	inMap->Insert(inName,inValue);
}



void FieldMapAppendFields(FieldMap *inMap,Array<String> &outFields)
{
	KeyGetter getter(outFields);
	inMap->Iterate(getter);
}

struct Marker
{
	Marker(hx::MarkContext *__inCtx) { this->__inCtx = __inCtx;  }
	hx::MarkContext *__inCtx;

	void VisitNode(void **inPtr)
   {
		hx::MarkAlloc(*inPtr, __inCtx);
   }
	void VisitValue(const String &inStr, Dynamic &inDyn)
	{
		HX_MARK_STRING(inStr.__s);
		if (inDyn.mPtr)
      {
         #ifdef HXCPP_DEBUG
         hx::MarkSetMember(inStr.__s, __inCtx);
         #endif
		   HX_MARK_OBJECT(inDyn.mPtr);
      }
	}
};

void FieldMapMark(FieldMap *inMap,hx::MarkContext *__inCtx)
{
	if (inMap)
	{
		hx::MarkAlloc(inMap, __inCtx);
		Marker m(__inCtx);
		inMap->Iterate(m);
	}
}

#ifdef HXCPP_VISIT_ALLOCS


struct Visitor
{
	Visitor(hx::VisitContext *__inCtx) { this->__inCtx = __inCtx;  }
	hx::VisitContext *__inCtx;

	void VisitNode(void **inPtr)
	{
		__inCtx->visitAlloc(inPtr);
	}

	void VisitValue(const String &inStr, Dynamic &inDyn)
	{
		HX_VISIT_STRING(inStr.__s);
		if (inDyn.mPtr)
		   HX_VISIT_OBJECT(inDyn.mPtr);
	}
};

void FieldMapVisit(FieldMap **inMap,hx::VisitContext *__inCtx)
{
	if (*inMap)
	{
		__inCtx->visitAlloc((void **)inMap);
		Visitor v(__inCtx);
		(*inMap)->Iterate(v);
	}
}
#endif


}



namespace
{
typedef hx::HashBase<Int>                IntHashBase;
typedef hx::Hash< TIntElement<Dynamic> > IntHashObject;
typedef hx::Hash< TIntElement<int> >     IntHashInt;
typedef hx::Hash< TIntElement<Float> >   IntHashFloat;
typedef hx::Hash< TIntElement<String> >  IntHashString;
}

void __int_hash_set(Dynamic &ioHash,int inKey,const Dynamic &value)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      if (value==null())
      {
         hash = new IntHashObject();
      }
      else
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtBool || type==vtInt)
            hash = new IntHashInt();
         else if (type==vtFloat)
            hash = new IntHashFloat();
         else if (type==vtString)
            hash = new IntHashString();
         else
            hash = new IntHashObject();
      }
      ioHash = hash;
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtBool || type==vtInt)
         {
            if (hash->store==hashFloat)
               want = hashFloat;
            else if (hash->store==hashInt)
               want = hashInt;
         }
         else if (type==vtFloat)
         {
            if (hash->store==hashInt || hash->store==hashFloat) 
               want =hashFloat;
         }
         else if (type==vtString)
         {
            if (hash->store==hashString)
               want = hashString;
         }
      }
      if (hash->store!=want)
      {
         hash = hash->convertStore(want);
         ioHash = hash;
      }
   }

   hash->set(inKey,value);
}

void __int_hash_set_int(Dynamic &ioHash,int inKey,int inValue)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashInt();
      ioHash = hash;
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }

   hash->set(inKey,inValue);
}


void __int_hash_set_float(Dynamic &ioHash,int inKey,Float inValue)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashFloat();
      ioHash = hash;
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashFloat);
      ioHash = hash;
   }

   hash->set(inKey,inValue);
}



void __int_hash_set_string(Dynamic &ioHash,int inKey, ::String inValue)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashString();
      ioHash = hash;
   }
   else if (hash->store==hashInt || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }

   hash->set(inKey,inValue);
}

Dynamic  __int_hash_get(Dynamic &ioHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey,result);
   return result;
}


bool  __int_hash_exists(Dynamic &ioHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;
   return hash->exists(inKey);
}

bool  __int_hash_remove(Dynamic &ioHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;
   return hash->remove(inKey);
}

Dynamic __int_hash_keys(Dynamic &ioHash)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<Int>::__new();
   return hash->keys();
}

Dynamic __int_hash_values(Dynamic &ioHash)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<Dynamic>::__new();
   return hash->values();
}

