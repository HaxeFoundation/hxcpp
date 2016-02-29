#ifndef HX_ANON_H
#define HX_ANON_H


namespace hx
{

typedef Dynamic FieldMap;

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic FieldMapCreate();

HXCPP_EXTERN_CLASS_ATTRIBUTES
bool FieldMapGet(Dynamic *inMap, const ::String &inName, ::Dynamic &outValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES
bool FieldMapHas(Dynamic *inMap, const ::String &inName);
HXCPP_EXTERN_CLASS_ATTRIBUTES
bool FieldMapGet(Dynamic *inMap, int inID, ::Dynamic &outValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void FieldMapSet(Dynamic *inMap, const ::String &inName, const ::Dynamic &inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void FieldMapAppendFields(Dynamic *inMap,::Array< ::String> &outFields);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void FieldMapMark(Dynamic *inMap,hx::MarkContext *__inCtx);
#ifdef HXCPP_VISIT_ALLOCS
HXCPP_EXTERN_CLASS_ATTRIBUTES
void FieldMapVisit(Dynamic **inMap,hx::VisitContext *__inCtx);
#endif

} // end namespace hx




namespace hx
{

// --- hx::Anon_obj ----------------------------------------------
//
// The hx::Anon_obj contains an arbitrary string map of fields.

class HXCPP_EXTERN_CLASS_ATTRIBUTES Anon_obj : public hx::Object
{
   typedef hx::Anon_obj OBJ_;
   typedef hx::ObjectPtr<hx::Anon_obj> Anon;
   typedef hx::Object super;

   inline void *operator new( size_t inSize, int inExtra )
   {
      return hx::Object::operator new(inSize+inExtra, true, 0);
   }

   struct VariantKey
   {
      String key;
      cpp::Variant value;
   };

   Dynamic mFields;
   int     mFixedFields;

public:
   inline void *operator new( size_t inSize )
   {
      return hx::Object::operator new(inSize, true, 0);
   }


   Anon_obj(int inFixedFields = 0);

   static Anon Create(int inElements)
   {
      return Anon(new (inElements*sizeof(VariantKey) ) hx::Anon_obj(inElements) );
   }

   static Anon Create() { return Anon(new (0) hx::Anon_obj); }
   static Anon Create(const Dynamic &inSrc) { return Anon(new (0) hx::Anon_obj); }

   static Dynamic __CreateEmpty() { return Anon(new (0) hx::Anon_obj); }
   static Dynamic __Create(DynamicArray inArgs);
   static void __boot();

   void operator delete( void *, int) { }


   Dynamic __Field(const String &inString ,hx::PropertyAccess inCallProp);
   bool __HasField(const String &inString);
   Dynamic __SetField(const String &inString,const Dynamic &inValue ,hx::PropertyAccess inCallProp);
   virtual void __GetFields(Array<String> &outFields);
   Dynamic *__GetFieldMap() { return &mFields; }

   virtual int __GetType() const { return vtObject; }

   hx::Anon_obj *Add(const String &inName,const Dynamic &inValue,bool inSetThisPointer=true);
	void __Mark(hx::MarkContext *__inCtx);
   #ifdef HXCPP_VISIT_ALLOCS
	void __Visit(hx::VisitContext *__inCtx);
   #endif

   String __ToString() const;
   String toString();

   static hx::ObjectPtr<hx::Class_obj> __mClass; \
   static hx::ObjectPtr<hx::Class_obj> &__SGetClass() { return __mClass; }
   bool __Is(hx::Object *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; }
   hx::ObjectPtr<hx::Class_obj > __GetClass() const { return __mClass; }

   bool __Remove(String inKey);
};


typedef hx::ObjectPtr<hx::Anon_obj> Anon;

HXCPP_EXTERN_CLASS_ATTRIBUTES
Anon SourceInfo(String inFile, int inLine, String inClass, String inMethod);


} // end namespace hx

HXCPP_EXTERN_CLASS_ATTRIBUTES
bool __hxcpp_anon_remove(Dynamic inObj,::String inKey);

#endif
