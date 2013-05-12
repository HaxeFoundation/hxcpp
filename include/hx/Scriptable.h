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


typedef hx::Object * (*ScriptableClassFactory)(void **inVTable,ScriptHandler *,unsigned char *);
typedef hx::Object * (*ScriptableInterfaceFactory)(::hx::Object *);

void ScriptableRegisterClass( String inName, String *inFunctions, ScriptableClassFactory inFactory);
void ScriptableRegisterInterface( String inName, const hx::type_info *inType, ScriptableInterfaceFactory inFactory);

void ScriptableMark(ScriptHandler *, unsigned char *, HX_MARK_PARAMS);
void ScriptableVisit(ScriptHandler *, unsigned char **, HX_VISIT_PARAMS);
bool ScriptableField(hx::Object *, const ::String &,bool inCallProp,Dynamic &outResult);
bool ScriptableField(hx::Object *, int inName,bool inCallProp,Float &outResult);
bool ScriptableField(hx::Object *, int inName,bool inCallProp,Dynamic &outResult);
void ScriptableGetFields(hx::Object *inObject, Array< ::String> &outFields);
bool ScriptableSetField(hx::Object *, const ::String &, Dynamic inValue,bool inCallProp, Dynamic &outValue);


}

void __scriptable_load_neko(String inName);
void __scriptable_load_neko_bytes(Array<unsigned char> inBytes);
void __scriptable_load_abc(Array<unsigned char> inBytes);


#define HX_SCRIPTABLE_REGISTER_INTERFACE(name,class) \
    hx::ScriptableRegisterInterface( HX_CSTRING(#name), &typeid(class), class##__scriptable::__script_create )

#define HX_SCRIPTABLE_REGISTER_CLASS(name,class) \
    hx::ScriptableRegisterClass( HX_CSTRING(name), __scriptableFunctionNames, class##__scriptable::__script_create )


#define HX_DEFINE_SCRIPTABLE(ARG_LIST) \
   void **__scriptVTable; \
   hx::ScriptHandler *__instanceInfo; \
   unsigned char *__instanceData; \
   public: \
   static hx::Object *__script_create(void **inVTable, hx::ScriptHandler *inInfo, unsigned char *inData) { \
    __ME *result = new __ME(); \
    result->__scriptVTable = inVTable; \
    result->__instanceInfo = inInfo; \
    result->__instanceData = inData; \
   return result; } \
   void __Construct(Array<Dynamic> inArgs) { __construct(ARG_LIST); } \
   hx::ScriptHandler *__GetScriptHandler() { return __instanceInfo; } \
   unsigned char *__GetScriptData() { return __instanceData; }


#define HX_DEFINE_SCRIPTABLE_INTERFACE \
   Dynamic mDelegate; \
   hx::Object *__GetRealObject() { return mDelegate.mPtr; } \
   void __Visit(HX_VISIT_PARAMS) { HX_VISIT_OBJECT(mDelegate.mPtr); } \
   public: \
   static hx::Object *__script_create(hx::Object *inDelegate) { \
    __ME *result = new __ME(); \
    result->mDelegate = inDelegate; \
    return result; } \



#define HX_DEFINE_SCRIPTABLE_DYNAMIC \
	void __Mark(HX_MARK_PARAMS) { super::__Mark(HX_MARK_ARG); hx::ScriptableMark(__instanceInfo,__instanceData,HX_MARK_ARG); } \
   void __Visit(HX_VISIT_PARAMS) { super::__Visit(HX_VISIT_ARG); hx::ScriptableVisit(__instanceInfo,&__instanceData,HX_VISIT_ARG); } \
 \
	Dynamic __Field(const ::String &inName,bool inCallProp) \
      { Dynamic result; if (hx::ScriptableField(this,inName,inCallProp,result)) return result; return super::__Field(inName,inCallProp); } \
	Float __INumField(int inFieldID) \
		{ Float result; if (hx::ScriptableField(this,inFieldID,true,result)) return result; return super::__INumField(inFieldID); } \
	Dynamic __IField(int inFieldID) \
		{ Dynamic result; if (hx::ScriptableField(this,inFieldID,true,result)) return result; return super::__IField(inFieldID); } \
	Dynamic __SetField(const ::String &inName,const Dynamic &inValue,bool inCallProp) \
   { \
      Dynamic value; \
      if (hx::ScriptableSetField(this, inName, inValue,inCallProp,value)) \
         return value; \
		return super::__SetField(inName,inValue,inCallProp); \
   } \
	void __GetFields(Array< ::String> &outFields) \
		{ super::__GetFields(outFields); hx::ScriptableGetFields(this,outFields); }




#endif
