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



Anon_obj::Anon_obj(int inElements)
{
   mFixedFields = inElements;
   //mFields = hx::FieldMapCreate();
}

void Anon_obj::__Mark(hx::MarkContext *__inCtx)
{
   if (mFixedFields)
   {
      VariantKey *fixed = getFixed();
      for(int i=0;i<mFixedFields;i++)
         HX_MARK_MEMBER(fixed[i].value);
   }
   HX_MARK_MEMBER(mFields);
}

#ifdef HXCPP_VISIT_ALLOCS
void Anon_obj::__Visit(hx::VisitContext *__inCtx)
{
   if (mFixedFields)
   {
      VariantKey *fixed = getFixed();
      for(int i=0;i<mFixedFields;i++)
         HX_VISIT_MEMBER(fixed[i].value);
   }
   HX_VISIT_MEMBER(mFields);
}
#endif

int Anon_obj::findFixed(const ::String &inKey)
{
   if (!mFixedFields)
      return -1;
   VariantKey *fixed = getFixed();

   int sought = inKey.hash();
   if (mFixedFields<2)
   {
      for(int i=0;i<mFixedFields;i++)
         if (fixed[i].hash==sought && fixed[i].key==inKey)
            return i;
      return -1;
   }

   // Find node with same hash...
   /* hash example
      [0] = -3  <- min
      [1] = -1
      [2] = -1   (sought -1)
      [3] =  4
      [4] =  4  <- max
   */

   int min = 0;
   if (fixed[0].hash>sought)
      return -1;
   if (fixed[0].hash!=sought)
   {
      int max = mFixedFields;
      if (fixed[max-1].hash<sought)
         return -1;

      while(max>min+1)
      {
         int mid = (max+min)>>1;
         if (fixed[mid].hash <= sought)
            min = mid;
         else
            max = mid;
      }
      while(fixed[min].hash==sought)
      {
         if (fixed[min].key==inKey)
            return min;

         min++;
         if (min>=mFixedFields)
            return -1;
      }
   }
   return -1;
}

Dynamic Anon_obj::__Field(const String &inName, hx::PropertyAccess inCallProp)
{
   int fixed = findFixed(inName);
   if (fixed>=0)
   {
      return getFixed()[fixed].value;
   }
   if (!mFields.mPtr)
      return null();
   return __string_hash_get(mFields,inName);
}

bool Anon_obj::__HasField(const String &inName)
{
   if (findFixed(inName)>=0)
      return true;
   if (!mFields.mPtr)
      return false;
   return __string_hash_exists(mFields,inName);
}

bool Anon_obj::__Remove(String inKey)
{
   int slot = findFixed(inKey);
   if (slot>=0)
   {
      VariantKey *fixed = getFixed();
      while(slot<mFixedFields)
      {
         fixed[slot] = fixed[slot+1];
         slot++;
      }
      mFixedFields--;
      return true;
   }

   if (!mFields.mPtr)
      return false;
   return __string_hash_remove(mFields,inKey);
}


Dynamic Anon_obj::__SetField(const String &inName,const Dynamic &inValue, hx::PropertyAccess inCallProp)
{
   // TODO - fixed
   if (!mFields.mPtr)
      mFields = hx::FieldMapCreate();

   __string_hash_set(mFields,inName,inValue,true);
   return inValue;
}

Anon_obj *Anon_obj::Add(const String &inName,const Dynamic &inValue,bool inSetThisPointer)
{
   // TODO - fixed
   if (!mFields.mPtr)
      mFields = hx::FieldMapCreate();

   __string_hash_set(mFields,inName,inValue,true);
   if (inSetThisPointer && inValue.GetPtr())
      inValue.GetPtr()->__SetThis(this);
   return this;
}

String Anon_obj::toString()
{
   if (!mFields.mPtr && !mFixedFields)
      return HX_CSTRING("{ }");

   int fixedToString = findFixed(HX_CSTRING("toString"));
   if (fixedToString>=0)
   {
      Dynamic func = getFixed()[fixedToString].value;
      if (func!=null())
         return func();
   }

   Dynamic func;
   if (FieldMapGet(&mFields, HX_CSTRING("toString"), func))
       return func();

   if (mFixedFields)
   {
      Array<String> array = Array<String>(0,mFixedFields*4+4);
      array->push(HX_CSTRING("{ "));

      if (mFields.mPtr)
      {
         String val = __string_hash_to_string_raw(mFields);
         if (val.__s)
            array->push(val);
      }

      VariantKey *fixed = getFixed();
      for(int i=0;i<mFixedFields;i++)
      {
         if (array->length>1)
           array->push(HX_CSTRING(", "));

         array->push(fixed[i].key);
         array->push(HX_CSTRING(" => "));
         array->push(fixed[i].value);
      }
      array->push(HX_CSTRING(" }"));
      return array->join(HX_CSTRING(""));
   }

   return __string_hash_to_string(mFields);
}

void Anon_obj::__GetFields(Array<String> &outFields)
{
   if (mFields.mPtr)
      outFields = __string_hash_keys(mFields);
   if (mFixedFields>0)
   {
      VariantKey *fixed = getFixed();
      for(int i=0;i<mFixedFields;i++)
         outFields->push( fixed[i].key );
   }
}


String Anon_obj::__ToString() const { return HX_CSTRING("Anon"); }

Dynamic Anon_obj::__Create(DynamicArray inArgs) { return Anon(new (0) Anon_obj); }

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


