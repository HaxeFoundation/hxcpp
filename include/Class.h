#ifndef HX_CLASS_H
#define HX_CLASS_H


namespace hx
{
// --- hxClassOf --------------------------------------------------------------
//
// Gets the class definition that relates to a specific type.
// Most classes have their own class data, by the standard types (non-classes)
//  use the template traits to get the class


template<typename T> 
inline Class &ClassOf() { typedef typename T::Obj Obj; return Obj::__SGetClass(); }

template<> 
inline Class &ClassOf<int>() { return GetIntClass(); }

template<> 
inline Class &ClassOf<double>() { return GetFloatClass(); }

template<> 
inline Class &ClassOf<float>() { return GetFloatClass(); }

template<> 
inline Class &ClassOf<bool>() { return GetBoolClass(); }

template<> 
inline Class &ClassOf<null>() { return GetVoidClass(); }

template<> 
inline Class &ClassOf<String>() { return GetStringClass(); }

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

}
#endif

class HXCPP_EXTERN_CLASS_ATTRIBUTES Class_obj : public hx::Object
{
public:
   Class_obj() : mSuper(0) { };
   Class_obj(const String &inClassName, String inStatics[], String inMembers[],
             hx::ConstructEmptyFunc inConstructEmpty, hx::ConstructArgsFunc inConstructArgs,
             Class *inSuperClass, hx::ConstructEnumFunc inConstructEnum,
             hx::CanCastFunc inCanCast, hx::MarkFunc inMarkFunc
             #ifdef HXCPP_VISIT_ALLOCS
             , hx::VisitFunc inVisitFunc
             #endif
             #ifdef HXCPP_SCRIPTABLE
             ,const hx::StorageInfo *inStorageInfo
             #endif
             );

   String __ToString() const;

   void __Mark(hx::MarkContext *__inCtx);
   void MarkStatics(hx::MarkContext *__inCtx);

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx);
   void VisitStatics(hx::VisitContext *__inCtx);
   #endif


   // the "Class class"
   Class              __GetClass() const;
   static Class      & __SGetClass();
	static void       __boot();

   Dynamic __Field(const String &inString ,bool inCallProp);

   Dynamic __SetField(const String &inString,const Dynamic &inValue ,bool inCallProp);

   bool __HasField(const String &inString);

   virtual Dynamic ConstructEmpty();
   virtual Dynamic ConstructArgs(hx::DynamicArray inArgs);
   virtual Dynamic ConstructEnum(String inName,hx::DynamicArray inArgs);
   virtual bool VCanCast(hx::Object *inPtr) { return false; }

   int __GetType() const { return vtObject; }

   virtual bool __IsEnum();

	inline bool CanCast(hx::Object *inPtr) { return mCanCast ? mCanCast(inPtr) : VCanCast(inPtr); }

  
	hx::CanCastFunc     mCanCast;



   Array<String>      GetInstanceFields();
   Array<String>      GetClassFields();
   Class              GetSuper();
   #ifdef HXCPP_SCRIPTABLE
   const hx::StorageInfo*  GetMemberStorage(String inName);
   #endif

   static Class       Resolve(String inName);

   Class              *mSuper;
   String             mName;
   Dynamic            __meta__;

	hx::ConstructArgsFunc  mConstructArgs;
	hx::ConstructEmptyFunc mConstructEmpty;
	hx::ConstructEnumFunc  mConstructEnum;

	hx::MarkFunc           mMarkFunc;
   #ifdef HXCPP_VISIT_ALLOCS
	hx::VisitFunc           mVisitFunc;
   #endif
   Array<String>      mStatics;
   Array<String>      mMembers;

   #ifdef HXCPP_SCRIPTABLE
   const hx::StorageInfo*    mMemberStorageInfo;
   #endif
};

typedef hx::ObjectPtr<Class_obj> Class;

void __hxcpp_boot_std_classes();


// --- All classes should be registered with this function via the "__boot" method

namespace hx
{

HXCPP_EXTERN_CLASS_ATTRIBUTES
Class RegisterClass(const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    Class *inSuperClass, ConstructEnumFunc inConst=0, MarkFunc inMarkFunc=0
                    #ifdef HXCPP_VISIT_ALLOCS
                    , VisitFunc inVisitFunc=0
                    #endif
                    #ifdef HXCPP_SCRIPTABLE
                    ,const hx::StorageInfo *inStorageInfo=0
                    #endif
                    );

HXCPP_EXTERN_CLASS_ATTRIBUTES
void RegisterClass(const String &inClassName, Class inClass);

template<typename T>
inline bool TCanCast(hx::Object *inPtr)
{
	return inPtr && ( dynamic_cast<T *>(inPtr->__GetRealObject()) || inPtr->__ToInterface(typeid(T)) );
}

}


#endif
