#include <hxcpp.h>
#include "Hash.h"


using namespace hx;


// --- HashRoot ---------------------------------------------------

int __root_hash_size(Dynamic &rtHash)
{
   HashRoot *hash = static_cast<HashRoot *>(rtHash.GetPtr());
   if(!hash)
      return 0;
   return hash->getSize();
}


// --- IntHash ----------------------------------------------------

namespace
{
typedef hx::HashBase<int>                   IntHashBase;
typedef hx::Hash< TIntElement<Dynamic> >    IntHashObject;
typedef hx::Hash< TIntElement<int> >        IntHashInt;
typedef hx::Hash< TIntElement<Float> >      IntHashFloat;
typedef hx::Hash< TIntElement<String> >     IntHashString;
typedef hx::Hash< TIntElement<cpp::Int64> > IntHashInt64;
}

void __int_hash_set(HX_MAP_THIS_ARG,int inKey,const Dynamic &value)
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
         hxObjectType type = (hxObjectType)value->__GetType();
         switch (type)
         {
            case vtInt   : hash = new IntHashInt(); break;
            case vtFloat : hash = new IntHashFloat(); break;
            case vtString: hash = new IntHashString(); break;
            case vtInt64 : hash = new IntHashInt64(); break;
            default      : hash = new IntHashObject(); break;
         }
      }
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         if (type==vtInt)
         {
            if (hash->store==hashFloat)
               want = hashFloat;
            else if (hash->store==hashInt)
               want = hashInt;
            else if (hash->store==hashInt64)
               want = hashInt64;
         }
         else if (type==vtFloat)
         {
            if (hash->store==hashInt || hash->store==hashFloat)
               want = hashFloat;
         }
         else if (type==vtString)
         {
            if (hash->store==hashString)
               want = hashString;
         }
         else if (type==vtInt64)
         {
            if (hash->store==hashInt || hash->store==hashInt64)
               want = hashInt64;
         }
      }
      if (hash->store!=want)
      {
         hash = hash->convertStore(want);
         ioHash = hash;
         HX_OBJ_WB_GET(owner,hash);
      }
   }

   hash->set(inKey,value);
}

void __int_hash_set_int(HX_MAP_THIS_ARG,int inKey,int inValue)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashInt();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}


void __int_hash_set_float(HX_MAP_THIS_ARG,int inKey,Float inValue)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashFloat();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashFloat);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}



void __int_hash_set_string(HX_MAP_THIS_ARG,int inKey, ::String inValue)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashString();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt || hash->store==hashFloat || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}

void __int_hash_set_int64(HX_MAP_THIS_ARG, int inKey, cpp::Int64 inValue)
{
   IntHashBase *hash = static_cast<IntHashBase*>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new IntHashInt64();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashInt64);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey, inValue);
}

Dynamic  __int_hash_get(Dynamic inHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey,result);
   return result;
}

int  __int_hash_get_int(Dynamic inHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(inHash.GetPtr());
   if (!hash)
      return 0;

   int result = 0;
   hash->query(inKey,result);
   return result;
}


Float  __int_hash_get_float(Dynamic inHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(inHash.GetPtr());
   if (!hash)
      return 0;

   Float result = 0;
   hash->query(inKey,result);
   return result;
}

String  __int_hash_get_string(Dynamic inHash,int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase *>(inHash.GetPtr());
   if (!hash)
      return String();

   String result;
   hash->query(inKey,result);
   return result;
}

cpp::Int64 __int_hash_get_int64(Dynamic inHash, int inKey)
{
   IntHashBase *hash = static_cast<IntHashBase*>(inHash.GetPtr());
   if (!hash)
      return 0;

   cpp::Int64 result = 0;
   hash->query(inKey, result);
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

Array<int> __int_hash_keys(Dynamic &ioHash)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<int>::__new();
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
   #if (HXCPP_API_LEVEL >= 430)
   return HX_CSTRING("[]");
   #else
   return HX_CSTRING("{}");
   #endif
}

void __int_hash_clear(Dynamic &ioHash)
{
   IntHashBase *hash = static_cast<IntHashBase *>(ioHash.GetPtr());
   if (hash)
      hash->clear();
}

// --- Int64Hash -----------------------------------------------------


namespace
{
typedef hx::HashBase<cpp::Int64>              Int64HashBase;
typedef hx::Hash< TInt64Element<Dynamic> >    Int64HashObject;
typedef hx::Hash< TInt64Element<int> >        Int64HashInt;
typedef hx::Hash< TInt64Element<Float> >      Int64HashFloat;
typedef hx::Hash< TInt64Element<String> >     Int64HashString;
typedef hx::Hash< TInt64Element<cpp::Int64> > Int64HashInt64;
}

void __int64_hash_set(HX_MAP_THIS_ARG, cpp::Int64 inKey, const Dynamic &value)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      if (value==null())
      {
         hash = new Int64HashObject();
      }
      else
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         switch (type)
         {
            case vtInt: hash = new Int64HashInt(); break;
            case vtFloat: hash = new Int64HashFloat(); break;
            case vtString: hash = new Int64HashString(); break;
            case vtInt64: hash = new Int64HashInt64(); break;
            default: hash = new Int64HashObject(); break;
         }
      }
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store != hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         if (type==vtInt)
         {
            if (hash->store==hashFloat)
               want = hashFloat;
            else if (hash->store==hashInt)
               want = hashInt;
            else if (hash->store==hashInt64)
               want = hashInt64;
         }
         else if (type==vtFloat)
         {
            if (hash->store==hashInt || hash->store==hashFloat)
               want = hashFloat;
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
         HX_OBJ_WB_GET(owner, hash);
      }
   }

   hash->set(inKey, value);
}

void __int64_hash_set_int(HX_MAP_THIS_ARG, cpp::Int64 inKey, int inValue)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new Int64HashInt();
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }

   hash->set(inKey, inValue);
}

void __int64_hash_set_float(HX_MAP_THIS_ARG, cpp::Int64 inKey, Float inValue)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new Int64HashFloat();
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store==hashString || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashFloat);
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }

   hash->set(inKey, inValue);
}

void __int64_hash_set_string(HX_MAP_THIS_ARG, cpp::Int64 inKey, ::String inValue)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new Int64HashString();
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store==hashInt || hash->store==hashFloat || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }

   hash->set(inKey, inValue);
}

void __int64_hash_set_int64(HX_MAP_THIS_ARG, cpp::Int64 inKey, cpp::Int64 inValue)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new Int64HashInt64();
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashInt64);
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }
   else if (hash->store==hashString || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner, hash);
   }

   hash->set(inKey, inValue);
}

Dynamic __int64_hash_get(Dynamic inHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey, result);
   return result;
}

int __int64_hash_get_int(Dynamic inHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(inHash.GetPtr());
   if (!hash)
      return 0;

   int result = 0;
   hash->query(inKey, result);
   return result;
}

Float __int64_hash_get_float(Dynamic inHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(inHash.GetPtr());
   if (!hash)
      return 0;

   Float result = 0;
   hash->query(inKey, result);
   return result;
}

String __int64_hash_get_string(Dynamic inHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(inHash.GetPtr());
   if (!hash)
      return String();

   String result;
   hash->query(inKey, result);
   return result;
}

cpp::Int64 __int64_hash_get_int64(Dynamic inHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(inHash.GetPtr());
   if (!hash)
      return 0;

   cpp::Int64 result = 0;
   hash->query(inKey, result);
   return result;
}

bool __int64_hash_exists(Dynamic &inHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(inHash.GetPtr());
   if (!hash)
      return false;

   return hash->exists(inKey);
}

bool __int64_hash_remove(Dynamic &ioHash, cpp::Int64 inKey)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
      return false;

   return hash->remove(inKey);
}

Array<cpp::Int64> __int64_hash_keys(Dynamic &ioHash)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<cpp::Int64>::__new();

   return hash->keys();
}

Dynamic __int64_hash_values(Dynamic &ioHash)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
      return Array_obj<Dynamic>::__new();

   return hash->values();
}

String __int64_hash_to_string(Dynamic &ioHash)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (!hash)
      return HX_CSTRING("{}");

   return hash->toString();
}

void __int64_hash_clear(Dynamic &ioHash)
{
   Int64HashBase *hash = static_cast<Int64HashBase *>(ioHash.GetPtr());
   if (hash)
      hash->clear();
}

// --- StringHash ----------------------------------------------------


namespace
{
typedef hx::HashBase<String>                    StringHashBase;
typedef hx::Hash< TStringElement<Dynamic> >     StringHashObject;
typedef hx::Hash< TStringElement<int> >         StringHashInt;
typedef hx::Hash< TStringElement<Float> >       StringHashFloat;
typedef hx::Hash< TStringElement<String> >      StringHashString;
typedef hx::Hash< TStringElement<cpp::Int64> >  StringHashInt64;
}


void __string_hash_set(HX_MAP_THIS_ARG,String inKey,const Dynamic &value, bool inForceDynamic)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      if (inForceDynamic || value==null() )
      {
         hash = new StringHashObject();
      }
      else
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         switch (type)
         {
            case vtInt   : hash = new StringHashInt(); break;
            case vtFloat : hash = new StringHashFloat(); break;
            case vtString: hash = new StringHashString(); break;
            case vtInt64 : hash = new StringHashInt64(); break;
            default      : hash = new StringHashObject(); break;
         }
      }
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         if (type==vtInt)
         {
            if (hash->store==hashFloat)
               want = hashFloat;
            else if (hash->store==hashInt)
               want = hashInt;
            else if (hash->store==hashInt64)
               want = hashInt64;
         }
         else if (type==vtFloat)
         {
            if (hash->store==hashInt || hash->store==hashFloat)
               want = hashFloat;
         }
         else if (type==vtString)
         {
            if (hash->store==hashString)
               want = hashString;
         }
         else if (type==vtInt64)
         {
            if (hash->store==hashInt || hash->store==hashInt64)
               want = hashInt64;
         }
      }
      if (hash->store!=want)
      {
         hash = hash->convertStore(want);
         ioHash = hash;
         HX_OBJ_WB_GET(owner,hash);
      }
   }

   hash->set(inKey,value);
}

void __string_hash_set_int(HX_MAP_THIS_ARG,String inKey,int inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashInt();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}


void __string_hash_set_float(HX_MAP_THIS_ARG,String inKey,Float inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashFloat();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashFloat);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}



void __string_hash_set_string(HX_MAP_THIS_ARG,String inKey, ::String inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashString();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt || hash->store==hashFloat || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}

void __string_hash_set_int64(HX_MAP_THIS_ARG, String inKey, cpp::Int64 inValue)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = new StringHashInt64();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashInt64);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey, inValue);
}

::String __string_hash_map_substr(HX_MAP_THIS_ARG,String inKey, int inStart, int inLength)
{
   StringHashBase *sash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!sash)
   {
      sash = new StringHashInt();
      ioHash = sash;
      HX_OBJ_WB_GET(owner,sash);
   }
   else if (sash->store!=hashInt)
   {
      sash = sash->convertStore(hashInt);
      ioHash = sash;
      HX_OBJ_WB_GET(owner,sash);
   }

   StringHashInt *shi = static_cast<StringHashInt *>(sash);


   struct Finder
   {
      ::String bigString;
      int len;
      int offset;

      Finder(const String &inBigStr, int inStart, int inLength) : bigString(inBigStr), offset(inStart), len(inLength) { }

      bool operator==(const String &inKey) const
      {
         if (inKey.length!=len)
            return false;
         #ifdef HX_SMART_STRINGS
         if (inKey.isUTF16Encoded())
         {
            // Has no wide chars, so can't match
            if (!bigString.isUTF16Encoded())
               return false;
            return !memcmp(inKey.__w, bigString.__w+offset, sizeof(char16_t)*len);
         }
         else if (bigString.isUTF16Encoded())
         {
            const char *k = inKey.__s;
            const char16_t *v = bigString.__w + offset;
            for(int i=0;i<len;i++)
               if (k[i] != v[i])
                  return false;
            return true;
         }
         // fallthough...
         #endif
         return !memcmp(inKey.__s, bigString.__s+offset, len);
      }
   };
   Finder finder(inKey, inStart, inLength);

   unsigned int code = inKey.calcSubHash(inStart,inLength);
   String found;
   if (shi->findEquivalentKey(found,code,finder))
      return found;
   String k = inKey.substr(inStart, inLength);
   shi->set(k,1);
   return k;
}


Dynamic  __string_hash_get(Dynamic inHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey,result);
   return result;
}

int  __string_hash_get_int(Dynamic inHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   int result = 0;
   hash->query(inKey,result);
   return result;
}


Float  __string_hash_get_float(Dynamic inHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   Float result = 0;
   hash->query(inKey,result);
   return result;
}

String  __string_hash_get_string(Dynamic inHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   String result;
   hash->query(inKey,result);
   return result;
}

cpp::Int64 __string_hash_get_int64(Dynamic inHash,String inKey)
{
   StringHashBase *hash = static_cast<StringHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   cpp::Int64 result = 0;
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
   #if (HXCPP_API_LEVEL >= 430)
   return HX_CSTRING("[]");
   #else
   return HX_CSTRING("{}");
   #endif
}

String __string_hash_to_string_raw(Dynamic &ioHash)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (hash)
      return hash->toStringRaw();
   return null();
}

void __string_hash_clear(Dynamic &ioHash)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (hash)
      hash->clear();
}



// --- ObjectHash ----------------------------------------------------


namespace
{
typedef hx::HashBase<Dynamic>                DynamicHashBase;

typedef hx::Hash< TDynamicElement<Dynamic,false> > DynamicHashObject;
typedef hx::Hash< TDynamicElement<int,false> >    DynamicHashInt;
typedef hx::Hash< TDynamicElement<Float,false> >   DynamicHashFloat;
typedef hx::Hash< TDynamicElement<String,false> >  DynamicHashString;
typedef hx::Hash< TDynamicElement<cpp::Int64,false> >  DynamicHashInt64;

typedef hx::Hash< TDynamicElement<Dynamic,true> > WeakDynamicHashObject;
typedef hx::Hash< TDynamicElement<int,true> >    WeakDynamicHashInt;
typedef hx::Hash< TDynamicElement<Float,true> >   WeakDynamicHashFloat;
typedef hx::Hash< TDynamicElement<String,true> >  WeakDynamicHashString;
typedef hx::Hash< TDynamicElement<cpp::Int64,true> >  WeakDynamicHashInt64;

#define toRealObject(x)
}


void __object_hash_set(HX_MAP_THIS_ARG,Dynamic inKey,const Dynamic &value,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      if (value==null())
      {
         hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashObject() :
                             (DynamicHashBase *)new DynamicHashObject();
      }
      else
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         switch (type)
         {
            case vtInt:
               hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashInt() :
                                   (DynamicHashBase *)new DynamicHashInt();
               break;
            case vtFloat:
               hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashFloat() :
                                   (DynamicHashBase *)new DynamicHashFloat();
               break;
            case vtString:
               hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashString() :
                                   (DynamicHashBase *)new DynamicHashString();
               break;
            case vtInt64:
               hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashInt64() :
                                   (DynamicHashBase *)new DynamicHashInt64();
               break;
            default:
               hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashObject() :
                                   (DynamicHashBase *)new DynamicHashObject();
               break;
         }
      }
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store!=hashObject)
   {
      HashStore want = hashObject;
      if (value!=null())
      {
         hxObjectType type = (hxObjectType)value->__GetType();
         if (type==vtInt)
         {
            if (hash->store==hashFloat)
               want = hashFloat;
            else if (hash->store==hashInt)
               want = hashInt;
            else if (hash->store==hashInt64)
               want = hashInt64;
         }
         else if (type==vtFloat)
         {
            if (hash->store==hashInt || hash->store==hashFloat)
               want = hashFloat;
         }
         else if (type==vtString)
         {
            if (hash->store==hashString)
               want = hashString;
         }
         else if (type==vtInt64)
         {
            if (hash->store==hashInt || hash->store==hashInt64)
               want = hashInt64;
         }
      }
      if (hash->store!=want)
      {
         hash = hash->convertStore(want);
         ioHash = hash;
         HX_OBJ_WB_GET(owner,hash);
      }
   }

   hash->set(inKey,value);
}

void __object_hash_set_int(HX_MAP_THIS_ARG,Dynamic inKey,int inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashInt() :
                          (DynamicHashBase *)new DynamicHashInt();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}


void __object_hash_set_float(HX_MAP_THIS_ARG,Dynamic inKey,Float inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ?  (DynamicHashBase *)new WeakDynamicHashFloat() :
                           (DynamicHashBase *)new DynamicHashFloat();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashFloat);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}



void __object_hash_set_string(HX_MAP_THIS_ARG,Dynamic inKey, ::String inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ? (DynamicHashBase *)new WeakDynamicHashString() :
                          (DynamicHashBase *)new DynamicHashString();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt || hash->store==hashFloat || hash->store==hashInt64)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}

void __object_hash_set_int64(HX_MAP_THIS_ARG,Dynamic inKey, cpp::Int64 inValue,bool inWeakKeys)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      hash = inWeakKeys ?  (DynamicHashBase *)new WeakDynamicHashInt64() :
                           (DynamicHashBase *)new DynamicHashInt64();
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashString || hash->store==hashFloat)
   {
      hash = hash->convertStore(hashObject);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }
   else if (hash->store==hashInt)
   {
      hash = hash->convertStore(hashInt64);
      ioHash = hash;
      HX_OBJ_WB_GET(owner,hash);
   }

   hash->set(inKey,inValue);
}

Dynamic  __object_hash_get(Dynamic inHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   Dynamic result = null();
   hash->query(inKey,result);
   return result;
}


int  __object_hash_get_int(Dynamic inHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   int result = 0;
   hash->query(inKey,result);
   return result;
}


Float  __object_hash_get_float(Dynamic inHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   Float result = 0;
   hash->query(inKey,result);
   return result;
}


String  __object_hash_get_string(Dynamic inHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   String result;
   hash->query(inKey,result);
   return result;
}


cpp::Int64 __object_hash_get_int64(Dynamic inHash,Dynamic inKey)
{
   toRealObject(inKey);
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(inHash.GetPtr());
   if (!hash)
      return null();

   cpp::Int64 result = 0;
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
   #if (HXCPP_API_LEVEL >= 430)
   return HX_CSTRING("[]");
   #else
   return HX_CSTRING("{}");
   #endif
}

void __object_hash_clear(Dynamic &ioHash)
{
   DynamicHashBase *hash = static_cast<DynamicHashBase *>(ioHash.GetPtr());
   if (hash)
      hash->clear();
}

