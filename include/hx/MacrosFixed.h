#ifndef MACROS_FIXED_H
#define MACROS_FIXED_H

// ---- Forward Declare ---------------


#define HX_DECLARE_CLASS0(klass) \
	class klass##_obj; \
	typedef ::hx::ObjectPtr<klass##_obj> klass;
#define HX_DECLARE_CLASS1(ns1,klass) namespace ns1 { HX_DECLARE_CLASS0(klass) }
#define HX_DECLARE_CLASS2(ns2,ns1,klass) namespace ns2 { HX_DECLARE_CLASS1(ns1,klass) }
#define HX_DECLARE_CLASS3(ns3,ns2,ns1,klass) namespace ns3 { HX_DECLARE_CLASS2(ns2,ns1,klass) }
#define HX_DECLARE_CLASS4(ns4,ns3,ns2,ns1,klass) namespace ns4 { HX_DECLARE_CLASS3(ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS5(ns5,ns4,ns3,ns2,ns1,klass) namespace ns5 { HX_DECLARE_CLASS4(ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS6(ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns6 { HX_DECLARE_CLASS5(ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns7 { HX_DECLARE_CLASS6(ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns8 { HX_DECLARE_CLASS7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS9(ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns9 { HX_DECLARE_CLASS8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS10(ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns10 { HX_DECLARE_CLASS9(ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS11(ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns11 { HX_DECLARE_CLASS10(ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS12(ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns12 { HX_DECLARE_CLASS11(ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS13(ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns13 { HX_DECLARE_CLASS12(ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS14(ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns14 { HX_DECLARE_CLASS13(ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS15(ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns15 { HX_DECLARE_CLASS14(ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS16(ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns16 { HX_DECLARE_CLASS15(ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS17(ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns17 { HX_DECLARE_CLASS16(ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS18(ns18,ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns18 { HX_DECLARE_CLASS17(ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS19(ns19,ns18,ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns19 { HX_DECLARE_CLASS18(ns18,ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS20(ns20,ns19,ns18,ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns20 { HX_DECLARE_CLASS19(ns19,ns18,ns17,ns16,ns15,ns14,ns13,ns12,ns11,ns10,ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }

// ---- Enum ----------------------

#if (HXCPP_API_LEVEL >= 330)

#define HX_DEFINE_CREATE_ENUM(enum_obj) \
static  ::Dynamic Create##enum_obj(::String inName,::hx::DynamicArray inArgs) \
{ \
   int count =  enum_obj::__FindArgCount(inName); \
   int args = inArgs.GetPtr() ? inArgs.__length() : 0; \
   if (args!=count) __hxcpp_dbg_checkedThrow(HX_INVALID_ENUM_ARG_COUNT(#enum_obj, inName, count, args)); \
   ::Dynamic result; \
   if (!enum_obj::__GetStatic(inName,result,::hx::paccDynamic)) __hxcpp_dbg_checkedThrow(HX_INVALID_ENUM_CONSTRUCTOR(#enum_obj, inName)); \
   if (args==0) return result; \
   return result->__Run(inArgs); \
}


#else

#define HX_DEFINE_CREATE_ENUM(enum_obj) \
static  ::Dynamic Create##enum_obj(::String inName,::hx::DynamicArray inArgs) \
{ \
   int idx =  enum_obj::__FindIndex(inName); \
   if (idx<0) __hxcpp_dbg_checkedThrow(HX_INVALID_ENUM_CONSTRUCTOR(#enum_obj, inName)); \
   int count =  enum_obj::__FindArgCount(inName); \
   int args = inArgs.GetPtr() ? inArgs.__length() : 0; \
   if (args!=count) __hxcpp_dbg_checkedThrow(HX_INVALID_ENUM_ARG_COUNT(#enum_obj, inName, count, args)); \
   ::Dynamic result =(new enum_obj())->__Field(inName,HX_PROP_DYNAMIC); \
   if (args==0 || !result.mPtr) return result; \
   return result->__Run(inArgs); \
}

#endif


// ---- Fields ----------------------

#if (HXCPP_API_LEVEL<331)
   #define HX_DO_RTTI_BASE \
      bool __Is(::hx::Object *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; }
#else
   #define HX_DO_RTTI_BASE
#endif

#if (HXCPP_API_LEVEL>331)
   #define HX_IS_INSTANCE_OF bool _hx_isInstanceOf(int inClassId) { return inClassId==1 || inClassId==(int)_hx_ClassId; }
#else
   #define HX_IS_INSTANCE_OF
#endif


#define HX_DO_RTTI_ALL \
   HX_DO_RTTI_BASE \
   static ::hx::ObjectPtr< ::hx::Class_obj> __mClass; \
   ::hx::ObjectPtr< ::hx::Class_obj > __GetClass() const { return __mClass; } \
   inline static ::hx::ObjectPtr< ::hx::Class_obj> &__SGetClass() { return __mClass; } \
   inline operator super *() { return this; }

#define HX_DO_RTTI \
   HX_DO_RTTI_ALL \
    ::hx::Val __Field(const ::String &inString, ::hx::PropertyAccess inCallProp); \
    ::hx::Val __SetField(const ::String &inString,const  ::hx::Val &inValue, ::hx::PropertyAccess inCallProp); \
   void __GetFields(Array< ::String> &outFields);

#define HX_DO_INTERFACE_RTTI \
   static ::hx::ObjectPtr< ::hx::Class_obj> __mClass; \
   static ::hx::ObjectPtr< ::hx::Class_obj> &__SGetClass() { return __mClass; } \
	static void __register();

#define HX_DO_ENUM_RTTI_INTERNAL \
   HX_DO_RTTI_BASE  \
    ::hx::Val __Field(const ::String &inString, ::hx::PropertyAccess inCallProp); \
   static int __FindIndex(::String inName); \
   static int __FindArgCount(::String inName);

#define HX_DO_ENUM_RTTI \
   HX_DO_ENUM_RTTI_INTERNAL \
   static ::hx::ObjectPtr< ::hx::Class_obj> __mClass; \
   ::hx::ObjectPtr< ::hx::Class_obj > __GetClass() const { return __mClass; } \
   static ::hx::ObjectPtr< ::hx::Class_obj> &__SGetClass() { return __mClass; }


#define HX_DECLARE_IMPLEMENT_DYNAMIC   ::Dynamic __mDynamicFields; \
     ::Dynamic *__GetFieldMap() { return &__mDynamicFields; } \
    bool __HasField(const String &inString) \
      { return ::hx::FieldMapHas(&__mDynamicFields,inString) || super::__HasField(inString); }


#define HX_INIT_IMPLEMENT_DYNAMIC

#define HX_MARK_DYNAMIC HX_MARK_MEMBER(__mDynamicFields)


#ifdef HX_VISIT_ALLOCS

#define HX_VISIT_DYNAMIC HX_VISIT_MEMBER(__mDynamicFields);

#else

#define HX_VISIT_DYNAMIC do { } while (0);

#endif

#define HX_CHECK_DYNAMIC_GET_FIELD(inName) \
   {  ::Dynamic d;  if (::hx::FieldMapGet(&__mDynamicFields,inName,d)) return d; }

#define HX_CHECK_DYNAMIC_GET_INT_FIELD(inID) \
   {  ::Dynamic d;  if (::hx::FieldMapGet(&__mDynamicFields,inID,d)) return d; }

#ifdef HXCPP_GC_GENERATIONAL
#define HX_DYNAMIC_SET_FIELD(inName,inValue) ::hx::FieldMapSet(this,&__mDynamicFields,inName,inValue)
#else
#define HX_DYNAMIC_SET_FIELD(inName,inValue) ::hx::FieldMapSet(&__mDynamicFields,inName,inValue)
#endif

#define HX_APPEND_DYNAMIC_FIELDS(outFields) ::hx::FieldMapAppendFields(&__mDynamicFields,outFields)






// ---- Main ---------------


namespace hx {
HXCPP_EXTERN_CLASS_ATTRIBUTES void SetTopOfStack(int *inTopOfStack,bool);
}
#define HX_TOP_OF_STACK \
		int t0 = 99; \
		::hx::SetTopOfStack(&t0,false);


#ifdef __GNUC__
 #define EXPORT_EXTRA __attribute__ ((visibility("default")))
#else
 #define EXPORT_EXTRA __declspec(dllexport)
#endif

#ifdef HX_DECLARE_MAIN

#ifdef HXCPP_DLL_IMPORT

#define HX_BEGIN_MAIN \
   extern "C" { \
   EXPORT_EXTRA void __main__() { \
	__boot_all();

#define HX_END_MAIN \
} \
}


#elif defined(HX_ANDROID)
  #ifdef HXCPP_EXE_LINK
   #define HX_BEGIN_MAIN \
   \
   int main(int argc,char **argv){ \
      HX_TOP_OF_STACK \
      ::hx::Boot(); \
      try{ \
         __boot_all();

   #define HX_END_MAIN \
      } \
      catch ( ::Dynamic e){ \
         __hx_dump_stack(); \
         printf("Error : %s\n",e->toString().__CStr()); \
         return -1; \
      } \
      return 0; \
   }

  #else
   // Java Main....
   #include <jni.h>
   #include <hx/Thread.h>
   #include <android/log.h>

   #define HX_BEGIN_MAIN \
   extern "C" EXPORT_EXTRA void hxcpp_main() { \
      HX_TOP_OF_STACK \
           try { \
      ::hx::Boot(); \
      __boot_all();


   #define HX_END_MAIN \
           } catch ( ::Dynamic e) { \
        __hx_dump_stack(); \
             __android_log_print(ANDROID_LOG_ERROR, "Exception", "%s", e->toString().__CStr()); \
           }\
      ::hx::SetTopOfStack((int *)0,true); \
   } \
   \
   extern "C" EXPORT_EXTRA JNIEXPORT void JNICALL Java_org_haxe_HXCPP_main(JNIEnv * env) \
   { hxcpp_main(); }
  #endif

#elif defined(HX_WINRT)

#include <Roapi.h>

#define HX_BEGIN_MAIN \
[ Platform::MTAThread ] \
int main(Platform::Array<Platform::String^>^) \
{ \
   HX_TOP_OF_STACK \
   RoInitialize(RO_INIT_MULTITHREADED); \
   ::hx::Boot(); \
   try{ \
      __boot_all();

#define HX_END_MAIN \
   } \
   catch ( ::Dynamic e){ \
      __hx_dump_stack(); \
      return -1; \
   } \
   return 0; \
}

#elif defined(HX_WIN_MAIN)


#ifdef HAVE_WINDOWS_H

#define HX_BEGIN_MAIN \
int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) \
{ \
	HX_TOP_OF_STACK \
	::hx::Boot(); \
	try{ \
		__boot_all();

#else

#define HX_BEGIN_MAIN \
extern "C" int __stdcall MessageBoxA(void *,const char *,const char *,int); \
\
int __stdcall WinMain( void * hInstance, void * hPrevInstance, const char *lpCmdLine, int nCmdShow) \
{ \
	HX_TOP_OF_STACK \
	::hx::Boot(); \
	try{ \
		__boot_all();

#endif

#define HX_END_MAIN \
	} \
	catch ( ::Dynamic e){ \
		__hx_dump_stack(); \
		MessageBoxA(0,  e->toString().__CStr(), "Error", 0); \
      return -1; \
	} \
	return 0; \
}


#elif defined(TIZEN)


#define HX_BEGIN_MAIN \
\
extern "C" EXPORT_EXTRA int OspMain (int argc, char* pArgv[]){ \
        HX_TOP_OF_STACK \
        ::hx::Boot(); \
        try{ \
                __boot_all();

#define HX_END_MAIN \
        } \
        catch ( ::Dynamic e){ \
                __hx_dump_stack(); \
                printf("Error : %s\n",e->toString().__CStr()); \
                return -1; \
        } \
        return 0; \
}


#else
// Console Main ...

#define HX_BEGIN_MAIN \
\
int main(int argc,char **argv){ \
	HX_TOP_OF_STACK \
	::hx::Boot(); \
	try{ \
		__boot_all();

#define HX_END_MAIN \
	} \
	catch ( ::Dynamic e){ \
		__hx_dump_stack(); \
		printf("Error : %s\n",e->toString().__CStr()); \
      return -1; \
	} \
	return 0; \
}

#endif

#endif // HX_DECLARE_MAIN

// Run as library
#define HX_BEGIN_LIB_MAIN \
extern "C" {\
\
void __hxcpp_lib_main() \
{ \
	HX_TOP_OF_STACK \
	::hx::Boot(); \
	__boot_all();

#define HX_END_LIB_MAIN \
} }



#endif


