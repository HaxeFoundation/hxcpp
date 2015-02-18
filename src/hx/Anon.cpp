#include <hxcpp.h>

namespace hx
{

Dynamic FieldMapCreate() { return Dynamic(); }

bool FieldMapGet(FieldMap *inMap, const String &inName, Dynamic &outValue)
{
   if (!inMap || !inMap->mPtr)
      return false;

   if (!__string_hash_exists(*inMap,inName))
      return false;
   outValue = __string_hash_get(*inMap, inName);
   return true;
}


bool FieldMapGet(FieldMap *inMap, int inId, Dynamic &outValue)
{
   if (!inMap || !inMap->mPtr)
      return false;

   const String &key = __hxcpp_field_from_id(inId);
   if (!__string_hash_exists(*inMap,key))
      return false;
   outValue = __string_hash_get(*inMap, key);
   return true;
}




bool FieldMapHas(FieldMap *inMap, const String &inName)
{
   return inMap && inMap->mPtr &&  __string_hash_exists(*inMap,inName);
}


void FieldMapSet(FieldMap *inMap, const String &inName, const Dynamic &inValue)
{
   __string_hash_set(*inMap, inName, inValue,true);
}


void FieldMapAppendFields(FieldMap *inMap,Array<String> &outFields)
{
   Array<String> keys = __string_hash_keys(*inMap);
   if (outFields->length==0)
      outFields = keys;
   else
      outFields = outFields->concat(keys);
}


// -- Anon_obj -------------------------------------------------



Anon_obj::Anon_obj()
{
   mFields = hx::FieldMapCreate();
}

void Anon_obj::__Mark(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(mFields);
}

#ifdef HXCPP_VISIT_ALLOCS
void Anon_obj::__Visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(mFields);
}
#endif

Dynamic Anon_obj::__Field(const String &inName, hx::PropertyAccess inCallProp)
{
   return __string_hash_get(mFields,inName);
}

bool Anon_obj::__HasField(const String &inName)
{
   return __string_hash_exists(mFields,inName);
}

bool Anon_obj::__Remove(String inKey)
{
   return __string_hash_remove(mFields,inKey);
}


Dynamic Anon_obj::__SetField(const String &inName,const Dynamic &inValue, hx::PropertyAccess inCallProp)
{
   __string_hash_set(mFields,inName,inValue,true);
   return inValue;
}

Anon_obj *Anon_obj::Add(const String &inName,const Dynamic &inValue,bool inSetThisPointer)
{
   __string_hash_set(mFields,inName,inValue,true);
   if (inSetThisPointer && inValue.GetPtr())
      inValue.GetPtr()->__SetThis(this);
   return this;
}

String Anon_obj::toString()
{
   Dynamic func;
   if (FieldMapGet(&mFields, HX_CSTRING("toString"), func))
       return func();

   return __string_hash_to_string(mFields);
}

void Anon_obj::__GetFields(Array<String> &outFields)
{
   outFields = __string_hash_keys(mFields);
}


String Anon_obj::__ToString() const { return HX_CSTRING("Anon"); }

Dynamic Anon_obj::__Create(DynamicArray inArgs) { return Anon(new Anon_obj); }

hx::Class Anon_obj::__mClass;


void Anon_obj::__boot()
{
   Static(__mClass) = hx::RegisterClass(HX_CSTRING("__Anon"),TCanCast<Anon_obj>,sNone,sNone,0,0,0,0);
}



Anon SourceInfo(String inFile, int inLine, String inClass, String inMethod)
{
   Anon result = Anon_obj::Create();
   result->Add(HX_CSTRING("fileName"),inFile);
   result->Add(HX_CSTRING("lineNumber"),inLine);
   result->Add(HX_CSTRING("className"),inClass);
   result->Add(HX_CSTRING("methodName"),inMethod);
   return result;
}



}

bool __hxcpp_anon_remove(Dynamic inObj,String inKey)
{
   Dynamic *map = inObj->__GetFieldMap();
   if (map)
      return __string_hash_remove(*map,inKey);
   return false;
}


