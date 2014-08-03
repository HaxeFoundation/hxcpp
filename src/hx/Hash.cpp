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


class IntHash : public Object
{
   typedef hx::Hash< TIntElement<Dynamic> > Hash;

   Hash hash;
	hx::InternalFinalizer *mFinalizer;

public:
   IntHash()
   {
      mFinalizer = new hx::InternalFinalizer(this);
      mFinalizer->mFinalizer = Destroy;
   }

   void set(int inKey,const Dynamic &inValue)
   {
      hash.set(inKey,inValue);
   }

   static void Destroy(Object *inObj)
   {
      IntHash *hash = dynamic_cast<IntHash *>(inObj);
      if (hash)
         hash->hash.destroy();
   }

   Dynamic get(int inKey)
   {
      Dynamic result = null();
      hash.query(inKey,result);
      return result;
   }
   bool exists(int inKey)
   {
      return hash.exists(inKey);
   }
   bool remove(int inKey)
   {
      return hash.remove(inKey);
   }



   Dynamic keys()
   {
      KeyBuilder<Int> builder(hash.getSize());
      hash.iterate(builder);
      return builder.array;;
   }
   Dynamic values()
   {
      ValueBuilder<Dynamic> builder(hash.getSize());
      hash.iterate(builder);
      return builder.array;;
   }
   String toString()
   {
      StringBuilder builder(hash.getSize());
      hash.iterate(builder);
      return builder.toString();
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      mFinalizer->Mark();
      if (NeedsMarking<Hash::Key>::Yes || NeedsMarking<Hash::Value>::Yes)
      {
         HashMarker<Hash> marker(__inCtx);
         hash.iterate(marker);
      }
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      if (mFinalizer)
         mFinalizer->Visit(__inCtx);
      if (NeedsMarking<Hash::Key>::Yes || NeedsMarking<Hash::Value>::Yes)
      {
        HashVisitor<Hash> vistor(__inCtx);
        hash.iterate(vistor);
      }
   }
   #endif
};


}


Object * __int_hash_create() { return new IntHash; }

void __int_hash_set(Dynamic inHash,int inKey,const Dynamic &value)
{
   IntHash *h = static_cast<IntHash *>(inHash.GetPtr());
   h->set(inKey,value);
}

Dynamic  __int_hash_get(Dynamic inHash,int inKey)
{
   IntHash *h = static_cast<IntHash *>(inHash.GetPtr());
   return h->get(inKey);
}

bool  __int_hash_exists(Dynamic inHash,int inKey)
{
   IntHash *h = static_cast<IntHash *>(inHash.GetPtr());
   return h->exists(inKey);
}

bool  __int_hash_remove(Dynamic inHash,int inKey)
{
   IntHash *h = static_cast<IntHash *>(inHash.GetPtr());
   return h->remove(inKey);
}

Dynamic __int_hash_keys(Dynamic inHash)
{
   IntHash *h = static_cast<IntHash *>(inHash.GetPtr());
   return h->keys();
}

Dynamic __int_hash_values(Dynamic inHash)
{
   IntHash *h = static_cast<IntHash *>(inHash.GetPtr());
   return h->values();
}

