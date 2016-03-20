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
      String       tag;
      #if (HXCPP_API_LEVEL >= 330)
         int     mFixedFields;
         #ifdef HXCPP_SCRIPTABLE
         struct CppiaClassInfo *classInfo; 
         #endif
      #else
         DynamicArray mArgs;
      #endif
   public:
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
      inline cpp::Variant *getFixed() { return (cpp::Variant *)(this + 1); }
      inline const cpp::Variant *getFixed() const { return (cpp::Variant *)(this + 1); }
      inline Dynamic __Param(int inID) { return getFixed()[inID]; }
      template<typename T>
      inline EnumBase_obj *init(int inIndex,const T &inValue)
      {
         getFixed()[inIndex] = inValue;
         return this;
      }
      inline void setIdentity(const String &inTag, int inIndex,int inFixedFields)
      {
         tag = inTag;
         index = inIndex;
         mFixedFields = inFixedFields;
      }
      DynamicArray getParameters();

      inline Dynamic getObject(int inId) { return getFixed()[inId].asDynamic(); }
      inline int getInt(int inId) { return getFixed()[inId]; }
      inline Float getFloat(int inId) { return getFixed()[inId]; }
      inline bool getBool(int inId) { return getFixed()[inId]; }
      inline ::String getString(int inId) { return getFixed()[inId].asString(); }
      inline Dynamic getParamI(int inId) { return getFixed()[inId]; }
      inline int getParamCount() { return mFixedFields; }

      // For legacy
      inline String __Tag() const { return tag; }


      String getTag() const { return tag; }
      int getIndex() const { return index; }
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


// --- CreateEnum -------------------------------------------------------------
//
// Template function to return a strongly-typed version fo the Enum.
// Most of the common stuff is in "Set".

#if (HXCPP_API_LEVEL >= 330)
template<typename ENUM>
ENUM *CreateEnum(const String &inName,int inIndex, int inFields)
{
   ENUM *result = new (inFields*sizeof(cpp::Variant)) ENUM;
   result->setIdentity(inName,inIndex,inFields);
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

inline void __hxcpp_enum_force(hx::EnumBase inEnum,String inForceName, int inIndex)
{
   #if (HXCPP_API_LEVEL >= 330)
   inEnum->setIdentity(inForceName, inIndex,0);
   #else
   hx::DynamicArray empty;
   inEnum->__Set(inForceName, inIndex, empty);
   #endif
}



#endif
