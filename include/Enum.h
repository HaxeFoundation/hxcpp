#ifndef HX_ENUM_H
#define HX_ENUM_H



// Enum (ie enum object class def)  is the same as Class.
typedef hx::Class Enum;


namespace hx
{

// --- hx::EnumBase_obj ----------------------------------------------------------
//
// Base class for Enums.
// Specializations of this class don't actually add more data, just extra constructors
//  and type information.

class HXCPP_EXTERN_CLASS_ATTRIBUTES EnumBase_obj : public hx::Object
{
   typedef hx::Object super;
   typedef EnumBase_obj OBJ_;


   protected:
      #if (HXCPP_API_LEVEL >= 330)
         String  _hx_tag;
         int     mFixedFields;
         #ifdef HXCPP_SCRIPTABLE
         struct CppiaClassInfo *classInfo; 
         #endif
      #else
         String       tag;
         DynamicArray mArgs;
      #endif
   public:
      HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdEnum };

      int          index;

   public:
      inline void *operator new( size_t inSize, int inExtra=0)
      {
         return hx::Object::operator new(inSize+inExtra, true, 0);
      }
      inline void operator delete(void *, int inExtra ) { }
      inline void operator delete(void *, size_t inSize ) { }
      inline void operator delete(void *, size_t inSize, int inExtra ) { }


      HX_DO_ENUM_RTTI_INTERNAL;
      static hx::ObjectPtr<hx::Class_obj> &__SGetClass();


      String toString();

      EnumBase_obj() : index(-1) { }
      EnumBase_obj(const null &inNull) : index(-1) { }
      int __GetType() const { return vtEnum; }
      static Dynamic __CreateEmpty();
      static Dynamic __Create(DynamicArray inArgs);
      static void __boot();

      void __Mark(hx::MarkContext *__inCtx);
      #ifdef HXCPP_VISIT_ALLOCS
      void __Visit(hx::VisitContext *__inCtx);
      #endif

      static hx::ObjectPtr<EnumBase_obj> Resolve(String inName);
      inline static bool __GetStatic(const ::String &inName, Dynamic &outValue, hx::PropertyAccess inCallProp) { return false; }

      #if (HXCPP_API_LEVEL >= 330)
      inline cpp::Variant *_hx_getFixed() { return (cpp::Variant *)(this + 1); }
      inline const cpp::Variant *_hx_getFixed() const { return (cpp::Variant *)(this + 1); }
      inline ::Dynamic __Param(int inID) { return _hx_getFixed()[inID]; }
      template<typename T>
      inline EnumBase_obj *_hx_init(int inIndex,const T &inValue)
      {
         #ifdef HXCPP_GC_GENERATIONAL
         cpp::Variant &v = _hx_getFixed()[inIndex];
         v = inValue;
         if (v.type<=cpp::Variant::typeString)
             HX_OBJ_WB_GET(this, v.valObject);
         #else
         _hx_getFixed()[inIndex] = inValue;
         #endif
         return this;
      }
      inline void _hx_setIdentity(const String &inTag, int inIndex,int inFixedFields)
      {
         _hx_tag = inTag;
         HX_OBJ_WB_GET(this, _hx_tag.__s);
         index = inIndex;
         mFixedFields = inFixedFields;
      }
      DynamicArray _hx_getParameters();

      inline ::Dynamic _hx_getObject(int inId) { return _hx_getFixed()[inId].asDynamic(); }
      inline int _hx_getInt(int inId) { return _hx_getFixed()[inId]; }
      inline Float _hx_getFloat(int inId) { return _hx_getFixed()[inId]; }
      inline bool _hx_getBool(int inId) { return _hx_getFixed()[inId]; }
      inline ::String _hx_getString(int inId) { return _hx_getFixed()[inId].asString(); }
      inline ::Dynamic _hx_getParamI(int inId) { return _hx_getFixed()[inId]; }
      inline int _hx_getParamCount() { return mFixedFields; }

      // For legacy
      inline String __Tag() const { return _hx_tag; }


      String _hx_getTag() const { return _hx_tag; }
      int _hx_getIndex() const { return index; }
      #else
      Dynamic __Param(int inID) { return mArgs[inID]; }
      DynamicArray __EnumParams() { return mArgs; }
      String __Tag() const { return tag; }
      int __Index() const { return index; }

      void __Set( const String &inName,int inIndex,DynamicArray inArgs)
      {
         tag = inName;
         index = inIndex;
         mArgs = inArgs;
      }
      #endif


      int __Compare(const hx::Object *inRHS) const;

      virtual String GetEnumName( ) const { return HX_CSTRING("Enum"); }
};


typedef hx::ObjectPtr<EnumBase_obj> EnumBase;


HXCPP_EXTERN_CLASS_ATTRIBUTES bool __hxcpp_enum_eq( ::hx::EnumBase a,  ::hx::EnumBase b);

// --- CreateEnum -------------------------------------------------------------
//
// Template function to return a strongly-typed version fo the Enum.
// Most of the common stuff is in "Set".

#if (HXCPP_API_LEVEL >= 330)
template<typename ENUM>
ENUM *CreateEnum(const String &inName,int inIndex, int inFields)
{
   ENUM *result = new (inFields*sizeof(cpp::Variant)) ENUM;
   result->_hx_setIdentity(inName,inIndex,inFields);
   return result;
}

template<typename ENUM>
ENUM *CreateConstEnum(const String &inName,int inIndex)
{
   ENUM vtable;
   ENUM *result = (ENUM *)hx::InternalCreateConstBuffer(&vtable,sizeof(ENUM));
   result->_hx_setIdentity(inName,inIndex,0);
   return result;
}


#else
template<typename ENUM>
hx::ObjectPtr<ENUM> CreateEnum(const String &inName,int inIndex, DynamicArray inArgs=DynamicArray())
{
   ENUM *result = new ENUM;
   result->__Set(inName,inIndex,inArgs);
   return result;
}
#endif

} // end namespace hx

#if (HXCPP_API_LEVEL >= 330)
inline int _hx_getEnumValueIndex(hx::EnumBase inEnum)
{
   return inEnum->_hx_getIndex();
}
#endif

inline void __hxcpp_enum_force(hx::EnumBase inEnum,String inForceName, int inIndex)
{
   #if (HXCPP_API_LEVEL >= 330)
   inEnum->_hx_setIdentity(inForceName, inIndex,0);
   #else
   hx::DynamicArray empty;
   inEnum->__Set(inForceName, inIndex, empty);
   #endif
}



#endif
