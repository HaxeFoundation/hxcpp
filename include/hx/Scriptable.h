#ifndef INCLUDED_HX_SCRIPTABLE
#define INCLUDED_HX_SCRIPTABLE

#include <typeinfo>

namespace hx
{
Dynamic ScriptableCall0(void *user, ::hx::Object *thiz);
Dynamic ScriptableCall1(void *user, ::hx::Object *thiz,Dynamic);
Dynamic ScriptableCall2(void *user, ::hx::Object *thiz,Dynamic,Dynamic);
Dynamic ScriptableCall3(void *user, ::hx::Object *thiz,Dynamic,Dynamic,Dynamic);
Dynamic ScriptableCall4(void *user, ::hx::Object *thiz,Dynamic,Dynamic,Dynamic,Dynamic);
Dynamic ScriptableCall5(void *user, ::hx::Object *thiz,Dynamic,Dynamic,Dynamic,Dynamic,Dynamic);
Dynamic ScriptableCallMult(void *user, ::hx::Object *thiz,Dynamic *inArgs);

typedef hx::Object * (*ScriptableClassFactory)(Array<Dynamic> inArgs, void **inVTable);
typedef hx::Object * (*ScriptableInterfaceFactory)(::hx::Object *);

void ScriptableRegisterClass( String inName, String *inFunctions, ScriptableClassFactory inFactory);
void ScriptableRegisterInterface( String inName, const hx::type_info *inType, ScriptableInterfaceFactory inFactory);

}

void __scriptable_load_neko(String inName);
void __scriptable_load_neko_bytes(Array<unsigned char> inBytes);
void __scriptable_load_abc(Array<unsigned char> inBytes);


#define HX_SCRIPTABLE_REGISTER_INTERFACE(name,class) \
    hx::ScriptableRegisterInterface( HX_CSTRING(#name), &typeid(class), class##__scriptable::__script_create )

#define HX_SCRIPTABLE_REGISTER_CLASS(name,class) \
    hx::ScriptableRegisterClass( HX_CSTRING(#name), __scriptableFunctionNames, class##__scriptable::__script_create )


#define HX_DEFINE_SCRIPTABLE(ARG_LIST) \
   void **__scriptVTable; \
   public: \
   static hx::Object *__script_create(Array<Dynamic> inArgs, void **inVTable) { \
	__ME *result = new __ME(); \
   result->__scriptVTable = inVTable; \
   result->__construct(ARG_LIST); \
   return result; }

#define HX_DEFINE_SCRIPTABLE_INTERFACE \
   Dynamic mDelegate; \
   hx::Object *__GetRealObject() { return mDelegate.mPtr; } \
   void __Visit(HX_VISIT_PARAMS) { HX_VISIT_OBJECT(mDelegate.mPtr); } \
   public: \
   static hx::Object *__script_create(hx::Object *inDelegate) { \
	__ME *result = new __ME(); \
   result->mDelegate = inDelegate; \
   return result; }



#define HX_DEFINE_SCRIPTABLE_DYNAMIC \
   HX_DECLARE_IMPLEMENT_DYNAMIC \
	void __Mark(HX_MARK_PARAMS) { super::__Mark(HX_MARK_ARG); HX_MARK_DYNAMIC; } \
   void __Visit(HX_VISIT_PARAMS) { super::__Visit(HX_VISIT_ARG); HX_VISIT_DYNAMIC; } \
 \
	Dynamic __Field(const ::String &inName,bool inCallProp) \
      { HX_CHECK_DYNAMIC_GET_FIELD(inName); return super::__Field(inName,inCallProp); } \
	Float __INumField(int inFieldID) \
		{ HX_CHECK_DYNAMIC_GET_INT_FIELD(inFieldID); return super::__INumField(inFieldID); } \
	Dynamic __IField(int inFieldID) \
		{ HX_CHECK_DYNAMIC_GET_INT_FIELD(inFieldID); return super::__IField(inFieldID); } \
	Dynamic __SetField(const ::String &inName,const Dynamic &inValue,bool inCallProp) \
   { \
		try { return super::__SetField(inName,inValue,inCallProp); } \
		catch(Dynamic e) { HX_DYNAMIC_SET_FIELD(inName,inValue); } \
		return inValue; \
   } \
	void __GetFields(Array< ::String> &outFields) \
		{ HX_APPEND_DYNAMIC_FIELDS(outFields); super::__GetFields(outFields); }




#endif
