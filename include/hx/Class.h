   #ifndef HX_CLASS_H
#define HX_CLASS_H


namespace hx
{
// --- hxClassOf --------------------------------------------------------------
//
// Gets the class definition that relates to a specific type.
// Most classes have their own class data, but the standard types (non-classes)
//  use the template traits to get the class


template<typename T> 
inline hx::Class &ClassOf() { typedef typename T::Obj Obj; return Obj::__SGetClass(); }

template<> 
inline hx::Class &ClassOf<int>() { return GetIntClass(); }

template<> 
inline hx::Class &ClassOf<double>() { return GetFloatClass(); }

template<> 
inline hx::Class &ClassOf<float>() { return GetFloatClass(); }

template<> 
inline hx::Class &ClassOf<bool>() { return GetBoolClass(); }

template<> 
inline hx::Class &ClassOf<null>() { return GetVoidClass(); }

template<> 
inline hx::Class &ClassOf<String>() { return GetStringClass(); }


template<typename T>
struct hxBaseType { typedef T type; };
template<typename T>
struct hxBaseType< hx::ObjectPtr<T> > { typedef T type; };

template<typename T> inline int ClassSizeOf() { return sizeof( typename hx::hxBaseType<T>::type ); }

} // end namespace hx


// --- Class_obj --------------------------------------------------------------------
//
// The Class_obj provides the type information required by the Reflect and type APIs.

namespace hx
{
typedef Dynamic (*ConstructEmptyFunc)();
typedef Dynamic (*ConstructArgsFunc)(DynamicArray inArgs);
typedef Dynamic (*ConstructEnumFunc)(String inName,DynamicArray inArgs);
typedef void (*MarkFunc)(hx::MarkContext *__inCtx);
typedef bool (*CanCastFunc)(hx::Object *inPtr);
#ifdef HXCPP_VISIT_ALLOCS
typedef void (*VisitFunc)(hx::VisitContext *__inCtx);
#endif
typedef bool (*GetStaticFieldFunc)(const String &inString, Dynamic &outValue, hx::PropertyAccess inCallProp);
typedef bool (*SetStaticFieldFunc)(const String &inString, Dynamic &ioValue, hx::PropertyAccess inCallProp);
}

inline bool operator!=(hx::ConstructEnumFunc inFunc,const null &inNull) { return inFunc!=0; }

#ifdef HXCPP_SCRIPTABLE
namespace hx
{
enum FieldStorage
{
   fsUnknown = 0,
   fsBool,
   fsInt,
   fsFloat,
   fsString,
   fsByte,
   fsObject,
};
struct StorageInfo
{
   FieldStorage type;
   int          offset;
   String       name;
};
struct StaticInfo
{
   FieldStorage type;
   void         *address;
   String       name;
};

}
#endif

namespace hx
{

class HXCPP_EXTERN_CLASS_ATTRIBUTES Class_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdClass };


   inline void *operator new( size_t inSize )
   {
      return hx::InternalCreateConstBuffer(0,(int)inSize);
   }
   void operator delete( void *) { }


   Class_obj() : mSuper(0) { };
   Class_obj(const String &inClassName, String inStatics[], String inMembers[],
             hx::ConstructEmptyFunc inConstructEmpty, hx::ConstructArgsFunc inConstructArgs,
             hx::Class *inSuperClass, hx::ConstructEnumFunc inConstructEnum,
             hx::CanCastFunc inCanCast, hx::MarkFunc inMarkFunc
             #ifdef HXCPP_VISIT_ALLOCS
             , hx::VisitFunc inVisitFunc
             #endif
             #ifdef HXCPP_SCRIPTABLE
             ,const hx::StorageInfo *inStorageInfo
             ,const hx::StaticInfo *inStaticInfo
             #endif
             );

   String __ToString() const;

   void MarkStatics(hx::MarkContext *__inCtx);

   #ifdef HXCPP_VISIT_ALLOCS
   void VisitStatics(hx::VisitContext *__inCtx);
   #endif

   static ::Array< ::String > dupFunctions(String inStatics[]);

   // the "Class class"
   hx::Class              __GetClass() const;
   static hx::Class      & __SGetClass();
	static void       __boot();

   hx::Val __Field(const String &inString ,hx::PropertyAccess inCallProp);

   hx::Val __SetField(const String &inString,const hx::Val &inValue ,hx::PropertyAccess inCallProp);

   bool __HasField(const String &inString);

   virtual Dynamic ConstructEmpty();
   virtual Dynamic ConstructArgs(hx::DynamicArray inArgs);
   virtual Dynamic ConstructEnum(String inName,hx::DynamicArray inArgs);
   virtual bool VCanCast(hx::Object *inPtr) { return false; }

   int __GetType() const { return vtObject; }

   virtual bool __IsEnum();

	inline bool CanCast(hx::Object *inPtr) { return mCanCast ? mCanCast(inPtr) : VCanCast(inPtr); }
   static bool GetNoStaticField(const String &inString, Dynamic &outValue, hx::PropertyAccess inCallProp);
   static bool SetNoStaticField(const String &inString, Dynamic &ioValue, hx::PropertyAccess inCallProp);

   void registerScriptable(bool inOverwrite);
  
	hx::CanCastFunc     mCanCast;



   virtual Array<String>  GetInstanceFields();
   virtual Array<String>  GetClassFields();
   hx::Class              GetSuper();
   #ifdef HXCPP_SCRIPTABLE
   const hx::StorageInfo*  GetMemberStorage(String inName);
   const hx::StaticInfo*  GetStaticStorage(String inName);
   #endif

   static hx::Class       Resolve(String inName);


   hx::Class              *mSuper;
   String             mName;
   Dynamic            __meta__;
   String             __rtti__;

	hx::ConstructArgsFunc  mConstructArgs;
	hx::ConstructEmptyFunc mConstructEmpty;
	hx::ConstructEnumFunc  mConstructEnum;
   hx::GetStaticFieldFunc mGetStaticField;
   hx::SetStaticFieldFunc mSetStaticField;

	hx::MarkFunc           mMarkFunc;
   #ifdef HXCPP_VISIT_ALLOCS
	hx::VisitFunc           mVisitFunc;
   #endif
   Array<String>      mStatics;
   Array<String>      mMembers;

   #ifdef HXCPP_SCRIPTABLE
   const hx::StorageInfo*    mMemberStorageInfo;
   const hx::StaticInfo*    mStaticStorageInfo;
   #endif
};

} // end namespace hx

void __hxcpp_boot_std_classes();


// --- All classes should be registered with this function via the "__boot" method
#ifdef RegisterClass
#undef RegisterClass
#endif

namespace hx
{


HXCPP_EXTERN_CLASS_ATTRIBUTES
hx::Class _hx_RegisterClass(const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    hx::Class *inSuperClass, ConstructEnumFunc inConst=0, MarkFunc inMarkFunc=0
                    #ifdef HXCPP_VISIT_ALLOCS
                    , VisitFunc inVisitFunc=0
                    #endif
                    #ifdef HXCPP_SCRIPTABLE
                    ,const hx::StorageInfo *inStorageInfo=0
                    ,const hx::StaticInfo *inStaticInfo=0
                    #endif
                    );

// For older versions
inline hx::Class RegisterClass(
                    const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    hx::Class *inSuperClass, ConstructEnumFunc inConst=0, MarkFunc inMarkFunc=0
                    #ifdef HXCPP_VISIT_ALLOCS
                    , VisitFunc inVisitFunc=0
                    #endif
                    #ifdef HXCPP_SCRIPTABLE
                    ,const hx::StorageInfo *inStorageInfo=0
                    ,const hx::StaticInfo *inStaticInfo=0
                    #endif
                    )
{
   return _hx_RegisterClass(inClassName, inCanCast, inStatics, inMembers,
                    inConstructEmpty, inConstructArgs, inSuperClass, inConst, inMarkFunc
                    #ifdef HXCPP_VISIT_ALLOCS
                    , inVisitFunc
                    #endif
                    #ifdef HXCPP_SCRIPTABLE
                    ,inStorageInfo ,inStaticInfo
                    #endif
                    );
}


HXCPP_EXTERN_CLASS_ATTRIBUTES
void _hx_RegisterClass(const String &inClassName, hx::Class inClass);

inline void RegisterClass(const String &inClassName, hx::Class inClass)
{
   _hx_RegisterClass(inClassName, inClass);
}

template<typename T>
inline bool TCanCast(hx::Object *inPtr)
{
	return inPtr && (
                  #if (HXCPP_API_LEVEL >= 332)
                     inPtr->_hx_isInstanceOf(T::_hx_ClassId)
                  #elif (HXCPP_API_LEVEL==331)
                     dynamic_cast<T *>(inPtr)
                  #else
                     dynamic_cast<T *>(inPtr->__GetRealObject())
                     #if (HXCPP_API_LEVEL < 330)
                     || inPtr->__ToInterface(typeid(T))
                     #endif
                  #endif
                  );
}


#if (HXCPP_API_LEVEL >= 330)
template<int HASH>
inline bool TIsInterface(hx::Object *inPtr)
{
	return inPtr && inPtr->_hx_getInterface(HASH);
}
#endif


HXCPP_EXTERN_CLASS_ATTRIBUTES void RegisterVTableOffset(int inOffset);

#define HX_REGISTER_VTABLE_OFFSET( CLASS, INTERFACE ) \
{ \
   CLASS *dummy = (CLASS *)0; \
   INTERFACE *intf = dummy; \
   hx::RegisterVTableOffset( (int)( (size_t)((char *)intf - (char *)dummy)) ); \
}



}


#endif
