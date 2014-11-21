#include <hxcpp.h>
//#include <hx/CFFI.h>
#include "Hash.h"


using namespace hx;


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




// --- StringHash ----------------------------------------------------


namespace
{
typedef hx::HashBase<String>                StringHashBase;
typedef hx::Hash< TStringElement<Dynamic> > StringHashObject;
typedef hx::Hash< TStringElement<int> >     StringHashInt;
typedef hx::Hash< TStringElement<Float> >   StringHashFloat;
typedef hx::Hash< TStringElement<String> >  StringHashString;
}


void __string_hash_set(Dynamic &ioHash,String inKey,const Dynamic &value)
{
   StringHashBase *hash = static_cast<StringHashBase *>(ioHash.GetPtr());
   if (!hash)
   {
      if (value==null())
      {
         hash = new StringHashObject();
      }
      else
      {
         ObjectType type = (ObjectType)value->__GetType();
         if (type==vtBool || type==vtInt)
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

   hash->set(inKey,inValue);
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

   hash->set(inKey,inValue);
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

   hash->set(inKey,inValue);
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
   return String();

}

