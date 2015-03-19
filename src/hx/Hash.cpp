#include <hxcpp.h>
//#include <hx/CFFI.h>
#include "Hash.h"


using namespace hx;

//#define HX_DYNAMIC_HASH_VALUES

// --- IntHash ----------------------------------------------------

namespace
{
typedef hx::HashBase<int>                IntHashBase;
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
      #ifdef HX_DYNAMIC_HASH_VALUES
      hash = new IntHashObject();
      #else
      if (value==null())
      {
         hash = new IntHashObject();
      }
      else
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtInt)
            hash = new IntHashInt();
         else if (type==vtFloat)
            hash = new IntHashFloat();
         else if (type==vtString)
            hash = new IntHashString();
         else // Object or bool
            hash = new IntHashObject();
      }
      #endif
      ioHash = hash;
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         ObjectType type = (ObjectType)value->__GetType();
         if ( type==vtInt)
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

   ioHash.mPtr = hash->set(inKey,value);
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

   ioHash.mPtr = hash->set(inKey,inValue);
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

   ioHash.mPtr = hash->set(inKey,inValue);
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

   ioHash.mPtr = hash->set(inKey,inValue);
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

Array<Int> __int_hash_keys(Dynamic &ioHash)
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


String __int_hash_to_string(Dynamic &ioHash)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (hash)
      return hash->toString();
   return HX_CSTRING("{}");

}



// --- StringHash ----------------------------------------------------


namespace
{
typedef hx::HashBase<String>                StringHashBase;
typedef hx::Hash< TStringElement<Dynamic> > StringHashObject;
typedef hx::Hash< TStringElement<int> >     StringHashInt;
typedef hx::Hash< TStringElement<Float> >   StringHashFloat;
typedef hx::Hash< TStringElement<String> >  StringHashString;
}


void __string_hash_set(Dynamic &ioHash,String inKey,const Dynamic &value, bool inForceDynamic)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      #ifdef HX_DYNAMIC_HASH_VALUES
      hash = new StringHashObject();
      #else
      if (inForceDynamic || value==null() )
      {
         hash = new StringHashObject();
      }
      else
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtInt)
         {
            hash = new StringHashInt();
         }
         else if (type==vtFloat)
            hash = new StringHashFloat();
         else if (type==vtString)
            hash = new StringHashString();
         else
            hash = new StringHashObject();
      }
      #endif
      ioHash = hash;
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtInt)
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

   ioHash.mPtr = hash->set(inKey,value);
}

void __string_hash_set_int(Dynamic &ioHash,String inKey,int inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashInt();
      ioHash = hash;
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }

   ioHash.mPtr = hash->set(inKey,inValue);
}


void __string_hash_set_float(Dynamic &ioHash,String inKey,Float inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashFloat();
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

   ioHash.mPtr = hash->set(inKey,inValue);
}



void __string_hash_set_string(Dynamic &ioHash,String inKey, ::String inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashString();
      ioHash = hash;
   }
   else if (hash->store==hashInt || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }

   ioHash.mPtr = hash->set(inKey,inValue);
}

Dynamic  __string_hash_get(Dynamic &ioHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey,result);
   return result;
}


bool  __string_hash_exists(Dynamic &ioHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;
   return hash->exists(inKey);
}

bool  __string_hash_remove(Dynamic &ioHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;
   return hash->remove(inKey);
}

Array<String> __string_hash_keys(Dynamic &ioHash)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<String>::__new();
   return hash->keys();
}

Dynamic __string_hash_values(Dynamic &ioHash)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<Dynamic>::__new();
   return hash->values();
}

String __string_hash_to_string(Dynamic &ioHash)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (hash)
      return hash->toString();
   return HX_CSTRING("{}");

}





// --- ObjectHash ----------------------------------------------------


namespace
{
typedef hx::HashBase<Dynamic>                DynamicHashBase;

typedef hx::Hash< TDynamicElement<Dynamic,false> > DynamicHashObject;
typedef hx::Hash< TDynamicElement<int,false> >    DynamicHashInt;
typedef hx::Hash< TDynamicElement<Float,false> >   DynamicHashFloat;
typedef hx::Hash< TDynamicElement<String,false> >  DynamicHashString;

typedef hx::Hash< TDynamicElement<Dynamic,true> > WeakDynamicHashObject;
typedef hx::Hash< TDynamicElement<int,true> >    WeakDynamicHashInt;
typedef hx::Hash< TDynamicElement<Float,true> >   WeakDynamicHashFloat;
typedef hx::Hash< TDynamicElement<String,true> >  WeakDynamicHashString;

inline void toRealObject(Dynamic &ioObject)
{
   if (ioObject!=null())
      ioObject = ioObject->__GetRealObject();
}

}


void __object_hash_set(Dynamic &ioHash,Dynamic inKey,const Dynamic &value,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      #ifdef HX_DYNAMIC_HASH_VALUES
      hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashObject() :
                          (DynamicHashBase *)new DynamicHashObject();
      #else
      if (value==null())
      {
         hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashObject() :
                             (DynamicHashBase *)new DynamicHashObject();
      }
      else
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtInt)
         {
            hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashInt() :
                                (DynamicHashBase *)new DynamicHashInt();
         }
         else if (type==vtFloat)
            hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashFloat() :
                                (DynamicHashBase *)new DynamicHashFloat();
         else if (type==vtString)
            hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashString() :
                                (DynamicHashBase *)new DynamicHashString();
         else
            hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashObject() :
                                (DynamicHashBase *)new DynamicHashObject();
      }
      #endif
      ioHash = hash;
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtInt)
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

   ioHash.mPtr = hash->set(inKey,value);
}

void __object_hash_set_int(Dynamic &ioHash,Dynamic inKey,int inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashInt() :
                          (DynamicHashBase *)new DynamicHashInt();
      ioHash = hash;
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }

   ioHash.mPtr = hash->set(inKey,inValue);
}


void __object_hash_set_float(Dynamic &ioHash,Dynamic inKey,Float inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ?  (DynamicHashBase *)new WeakDynamicHashFloat() :
                           (DynamicHashBase *)new DynamicHashFloat();
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

   ioHash.mPtr = hash->set(inKey,inValue);
}



void __object_hash_set_string(Dynamic &ioHash,Dynamic inKey, ::String inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashString() :
                          (DynamicHashBase *)new DynamicHashString();
      ioHash = hash;
   }
   else if (hash->store==hashInt || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
   }

   ioHash.mPtr = hash->set(inKey,inValue);
}

Dynamic  __object_hash_get(Dynamic &ioHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey,result);
   return result;
}


bool  __object_hash_exists(Dynamic &ioHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;
   return hash->exists(inKey);
}

bool  __object_hash_remove(Dynamic &ioHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;
   return hash->remove(inKey);
}

Array<Dynamic> __object_hash_keys(Dynamic &ioHash)
{
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<String>::__new();
   return hash->keys();
}

Dynamic __object_hash_values(Dynamic &ioHash)
{
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<Dynamic>::__new();
   return hash->values();
}

String __object_hash_to_string(Dynamic &ioHash)
{
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (hash)
      return hash->toString();
   return HX_CSTRING("{}");

}

