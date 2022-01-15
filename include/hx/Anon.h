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
#ifdef HXCPP_GC_GENERATIONAL
void FieldMapSet(hx::Object *inThis,Dynamic *inMap, const ::String &inName, const ::Dynamic &inValue);
#else
void FieldMapSet(Dynamic *inMap, const ::String &inName, const ::Dynamic &inValue);
#endif
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
      int    hash;
      String key;
      cpp::Variant value;
   };

   Dynamic mFields;
   int     mFixedFields;

public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdDynamic };

   inline void *operator new( size_t inSize )
   {
      return hx::Object::operator new(inSize, true, 0);
   }
   inline void operator delete(void *, size_t inSize ) { }
   inline void operator delete(void *, size_t inSize, int inExtra ) { }

   inline Anon_obj *setFixed(int index, const String &inName, const ::cpp::Variant &inValue)
   {
      VariantKey *fixed = getFixed() + index;
      fixed->hash = inName.hash();
      fixed->key = inName;
      fixed->value = inValue;
      if (inValue.type == ::cpp::Variant::typeObject) {
        HX_OBJ_WB_GET(this, inValue.valObject);
      }
      else if (inValue.type == ::cpp::Variant::typeString) {
        HX_OBJ_WB_GET(this, inValue.valStringPtr);
      }
      return this;
   }
   inline VariantKey *getFixed()
   {
      return (VariantKey *)(this + 1);
   }
   inline int findFixed(const ::String &inKey,bool inSkip5 = false);




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


   hx::Val __Field(const String &inString ,hx::PropertyAccess inCallProp);
   bool __HasField(const String &inString);
   hx::Val __SetField(const String &inString,const hx::Val &inValue ,hx::PropertyAccess inCallProp);
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
   #if (HXCPP_API_LEVEL<331)
   bool __Is(hx::Object *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; }
   #endif
   hx::ObjectPtr<hx::Class_obj > __GetClass() const { return __mClass; }

   bool __Remove(String inKey);
};


typedef hx::ObjectPtr<hx::Anon_obj> Anon;

HXCPP_EXTERN_CLASS_ATTRIBUTES
Anon SourceInfo(String inFile, int inLine, String inClass, String inMethod);


HXCPP_EXTERN_CLASS_ATTRIBUTES String StringFromAnonFields(hx::Object *inPtr);


template<typename _hx_T0>
class AnonStruct1_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdDynamic };

   String name0; _hx_T0 t0;

   inline static hx::Object *Create(const String &inName0, _hx_T0 inT0)
   {
      AnonStruct1_obj *result = new AnonStruct1_obj;
      result->name0 = inName0; result->t0 = inT0;
      if (hx::ContainsPointers<_hx_T0>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT0));
      }
      return result;
   }
   hx::Val __Field(const String &inField, hx::PropertyAccess)
   {
      if (HX_QSTR_EQ(inField,name0)) return t0;
      return null();
   }
   hx::Val __SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp)
   {
      if (inField.__s==name0.__s || HX_QSTR_EQ(inField,name0)) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      hx::Throw(HX_CSTRING("Missing field ") + inField);
      return inValue;
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(t0);
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(t0);
   }
   #endif

    void __GetFields(Array<String> &outFields)
    {
       outFields->push(name0);
    }

   String toString() { return StringFromAnonFields(this); }
};



template<typename _hx_T0, typename _hx_T1>
class AnonStruct2_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdDynamic };

   String name0; _hx_T0 t0;
   String name1; _hx_T1 t1;

   inline static hx::Object *Create(const String &inName0, _hx_T0 inT0,
                             const String &inName1, _hx_T1 inT1)
   {
      AnonStruct2_obj *result = new AnonStruct2_obj;
      result->name0 = inName0; result->t0 = inT0;
      if (hx::ContainsPointers<_hx_T0>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT0));
      }
      result->name1 = inName1; result->t1 = inT1;
      if (hx::ContainsPointers<_hx_T1>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT1));
      }
      return result;
   }
   hx::Val __Field(const String &inField, hx::PropertyAccess)
   {
      if (inField.__s==name0.__s) return t0;
      if (inField.__s==name1.__s) return t1;

      #ifdef HX_SMART_STRINGS
      if (!inField.isAsciiEncodedQ())
         return null();
      #endif

      if (HX_QSTR_EQ_AE(inField,name0)) return t0;
      if (HX_QSTR_EQ_AE(inField,name1)) return t1;
      return null();
   }
   hx::Val __SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp)
   {
      if (inField.__s==name0.__s) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (inField.__s==name1.__s) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }

      if (HX_QSTR_EQ(inField,name0)) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name1)) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }
      hx::Throw(HX_CSTRING("Missing field ") + inField);
      return inValue;
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(t0);
      HX_MARK_MEMBER(t1);
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(t0);
      HX_VISIT_MEMBER(t1);
   }
   #endif

    void __GetFields(Array<String> &outFields)
    {
       outFields->push(name0);
       outFields->push(name1);
    }

   String toString() { return StringFromAnonFields(this); }

};



template<typename _hx_T0, typename _hx_T1, typename _hx_T2>
class AnonStruct3_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdDynamic };

   String name0; _hx_T0 t0;
   String name1; _hx_T1 t1;
   String name2; _hx_T2 t2;

   inline static hx::Object *Create(const String &inName0, _hx_T0 inT0,
                             const String &inName1, _hx_T1 inT1,
                             const String &inName2, _hx_T2 inT2)
   {
      AnonStruct3_obj *result = new AnonStruct3_obj;
      result->name0 = inName0; result->t0 = inT0;
      if (hx::ContainsPointers<_hx_T0>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT0));
      }
      result->name1 = inName1; result->t1 = inT1;
      if (hx::ContainsPointers<_hx_T1>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT1));
      }
      result->name2 = inName2; result->t2 = inT2;
      if (hx::ContainsPointers<_hx_T2>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT2));
      }
      return result;
   }
   hx::Val __Field(const String &inField, hx::PropertyAccess)
   {
      if (inField.__s==name0.__s) return t0;
      if (inField.__s==name1.__s) return t1;
      if (inField.__s==name2.__s) return t2;
      #ifdef HX_SMART_STRINGS
      if (!inField.isAsciiEncodedQ())
         return null();
      #endif
      if (HX_QSTR_EQ_AE(inField,name0)) return t0;
      if (HX_QSTR_EQ_AE(inField,name1)) return t1;
      if (HX_QSTR_EQ_AE(inField,name2)) return t2;
      return null();
   }
   hx::Val __SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp)
   {
      if (inField.__s==name0.__s) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (inField.__s==name1.__s) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }
      if (inField.__s==name2.__s) {
        t2 = inValue.Cast< _hx_T2 >();
        if (hx::ContainsPointers<_hx_T2>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t2));
        }
        return inValue;
      }


      if (HX_QSTR_EQ(inField,name0)) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name1)) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name2)) {
        t2 = inValue.Cast< _hx_T2 >();
        if (hx::ContainsPointers<_hx_T2>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t2));
        }
        return inValue;
      }
      hx::Throw(HX_CSTRING("Missing field ") + inField);
      return inValue;
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(t0);
      HX_MARK_MEMBER(t1);
      HX_MARK_MEMBER(t2);
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(t0);
      HX_VISIT_MEMBER(t1);
      HX_VISIT_MEMBER(t2);
   }
   #endif

    void __GetFields(Array<String> &outFields)
    {
       outFields->push(name0);
       outFields->push(name1);
       outFields->push(name2);
    }

   String toString() { return StringFromAnonFields(this); }
};


template<typename _hx_T0, typename _hx_T1, typename _hx_T2, typename _hx_T3>
class AnonStruct4_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdDynamic };

   String name0; _hx_T0 t0;
   String name1; _hx_T1 t1;
   String name2; _hx_T2 t2;
   String name3; _hx_T3 t3;

   inline static hx::Object *Create(const String &inName0, _hx_T0 inT0,
                             const String &inName1, _hx_T1 inT1,
                             const String &inName2, _hx_T2 inT2,
                             const String &inName3, _hx_T3 inT3
                             )
   {
      AnonStruct4_obj *result = new AnonStruct4_obj;
      result->name0 = inName0; result->t0 = inT0;
      if (hx::ContainsPointers<_hx_T0>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT0));
      }
      result->name1 = inName1; result->t1 = inT1;
      if (hx::ContainsPointers<_hx_T1>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT1));
      }
      result->name2 = inName2; result->t2 = inT2;
      if (hx::ContainsPointers<_hx_T2>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT2));
      }
      result->name3 = inName3; result->t3 = inT3;
      if (hx::ContainsPointers<_hx_T3>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT3));
      }
      return result;
   }
   hx::Val __Field(const String &inField, hx::PropertyAccess)
   {
      if (inField.__s==name0.__s) return t0;
      if (inField.__s==name1.__s) return t1;
      if (inField.__s==name2.__s) return t2;
      if (inField.__s==name3.__s) return t3;
      #ifdef HX_SMART_STRINGS
      if (!inField.isAsciiEncodedQ())
         return null();
      #endif
      if (HX_QSTR_EQ_AE(inField,name0)) return t0;
      if (HX_QSTR_EQ_AE(inField,name1)) return t1;
      if (HX_QSTR_EQ_AE(inField,name2)) return t2;
      if (HX_QSTR_EQ_AE(inField,name3)) return t3;
      return null();
   }
   hx::Val __SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp)
   {
      if (inField.__s==name0.__s) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (inField.__s==name1.__s) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }
      if (inField.__s==name2.__s) {
        t2 = inValue.Cast< _hx_T2 >();
        if (hx::ContainsPointers<_hx_T2>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t2));
        }
        return inValue;
      }
      if (inField.__s==name3.__s) {
        t3 = inValue.Cast< _hx_T3 >();
        if (hx::ContainsPointers<_hx_T3>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t3));
        }
        return inValue;
      }



      if (HX_QSTR_EQ(inField,name0)) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name1)) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }  
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name2)) {
        t2 = inValue.Cast< _hx_T2 >();
        if (hx::ContainsPointers<_hx_T2>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t2));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name3)) {
        t3 = inValue.Cast< _hx_T3 >();
        if (hx::ContainsPointers<_hx_T3>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t3));
        }
        return inValue;
      }
      hx::Throw(HX_CSTRING("Missing field ") + inField);
      return inValue;
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(t0);
      HX_MARK_MEMBER(t1);
      HX_MARK_MEMBER(t2);
      HX_MARK_MEMBER(t3);
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(t0);
      HX_VISIT_MEMBER(t1);
      HX_VISIT_MEMBER(t2);
      HX_VISIT_MEMBER(t3);
   }
   #endif

    void __GetFields(Array<String> &outFields)
    {
       outFields->push(name0);
       outFields->push(name1);
       outFields->push(name2);
       outFields->push(name3);
    }

   String toString() { return StringFromAnonFields(this); }
};




template<typename _hx_T0, typename _hx_T1, typename _hx_T2, typename _hx_T3, typename _hx_T4>
class AnonStruct5_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdDynamic };

   String name0; _hx_T0 t0;
   String name1; _hx_T1 t1;
   String name2; _hx_T2 t2;
   String name3; _hx_T3 t3;
   String name4; _hx_T4 t4;

   inline static hx::Object *Create(const String &inName0, _hx_T0 inT0,
                             const String &inName1, _hx_T1 inT1,
                             const String &inName2, _hx_T2 inT2,
                             const String &inName3, _hx_T3 inT3,
                             const String &inName4, _hx_T4 inT4
                             )
   {
      AnonStruct5_obj *result = new AnonStruct5_obj;
      result->name0 = inName0; result->t0 = inT0;
      if (hx::ContainsPointers<_hx_T0>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT0));
      }
      result->name1 = inName1; result->t1 = inT1;
      if (hx::ContainsPointers<_hx_T1>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT1));
      }
      result->name2 = inName2; result->t2 = inT2;
      if (hx::ContainsPointers<_hx_T2>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT2));
      }
      result->name3 = inName3; result->t3 = inT3;
      if (hx::ContainsPointers<_hx_T3>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT3));
      }
      result->name4 = inName4; result->t4 = inT4;
      if (hx::ContainsPointers<_hx_T4>()) {
        HX_OBJ_WB_GET(result, hx::PointerOf(inT4));
      }
      return result;
   }
   hx::Val __Field(const String &inField, hx::PropertyAccess)
   {
      if (inField.__s==name0.__s) return t0;
      if (inField.__s==name1.__s) return t1;
      if (inField.__s==name2.__s) return t2;
      if (inField.__s==name3.__s) return t3;
      if (inField.__s==name4.__s) return t4;
      #ifdef HX_SMART_STRINGS
      if (!inField.isAsciiEncodedQ())
         return null();
      #endif
      if (HX_QSTR_EQ_AE(inField,name0)) return t0;
      if (HX_QSTR_EQ_AE(inField,name1)) return t1;
      if (HX_QSTR_EQ_AE(inField,name2)) return t2;
      if (HX_QSTR_EQ_AE(inField,name3)) return t3;
      if (HX_QSTR_EQ_AE(inField,name4)) return t4;
      return null();
   }
   hx::Val __SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp)
   {
      if (inField.__s==name0.__s) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (inField.__s==name1.__s) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }
      if (inField.__s==name2.__s) {
        t2 = inValue.Cast< _hx_T2 >();
        if (hx::ContainsPointers<_hx_T2>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t2));
        }
        return inValue;
      }
      if (inField.__s==name3.__s) {
        t3 = inValue.Cast< _hx_T3 >();
        if (hx::ContainsPointers<_hx_T3>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t3));
        }
        return inValue;
      }
      if (inField.__s==name4.__s) {
        t4 = inValue.Cast< _hx_T4 >();
        if (hx::ContainsPointers<_hx_T4>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t4));
        }
        return inValue;
      }




      if (HX_QSTR_EQ(inField,name0)) {
        t0 = inValue.Cast< _hx_T0 >();
        if (hx::ContainsPointers<_hx_T0>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t0));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name1)) {
        t1 = inValue.Cast< _hx_T1 >();
        if (hx::ContainsPointers<_hx_T1>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t1));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name2)) {
        t2 = inValue.Cast< _hx_T2 >();
        if (hx::ContainsPointers<_hx_T2>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t2));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name3)) {
        t3 = inValue.Cast< _hx_T3 >();
        if (hx::ContainsPointers<_hx_T3>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t3));
        }
        return inValue;
      }
      if (HX_QSTR_EQ(inField,name4)) {
        t4 = inValue.Cast< _hx_T4 >();
        if (hx::ContainsPointers<_hx_T4>()) {
          HX_OBJ_WB_GET(this, hx::PointerOf(t4));
        }
        return inValue;
      }
      hx::Throw(HX_CSTRING("Missing field ") + inField);
      return inValue;
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(t0);
      HX_MARK_MEMBER(t1);
      HX_MARK_MEMBER(t2);
      HX_MARK_MEMBER(t3);
      HX_MARK_MEMBER(t4);
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(t0);
      HX_VISIT_MEMBER(t1);
      HX_VISIT_MEMBER(t2);
      HX_VISIT_MEMBER(t3);
      HX_VISIT_MEMBER(t4);
   }
   #endif

    void __GetFields(Array<String> &outFields)
    {
       outFields->push(name0);
       outFields->push(name1);
       outFields->push(name2);
       outFields->push(name3);
       outFields->push(name4);
    }

   String toString() { return StringFromAnonFields(this); }
};






} // end namespace hx

HXCPP_EXTERN_CLASS_ATTRIBUTES
bool __hxcpp_anon_remove(Dynamic inObj,::String inKey);

#endif
