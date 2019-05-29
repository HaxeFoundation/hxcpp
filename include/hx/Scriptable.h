#ifndef INCLUDED_HX_SCRIPTABLE
#define INCLUDED_HX_SCRIPTABLE

#include <typeinfo>
#ifdef __clang__
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif


namespace hx
{

extern bool gEnableJit;
inline void EnableJit(bool inEnable) { gEnableJit = inEnable; }

#define HXCPP_CPPIA_SUPER_ARG(x) , (x)

struct ScriptNamedFunction : public ScriptFunction
{
   ScriptNamedFunction(const ScriptFunction &s) : ScriptFunction(s), name(0), isStatic(false), superExecute(0) { }

   ScriptNamedFunction(const char *inName=0,StackExecute inExe=0,const char *inSig=0, bool inIsStatic=false, StackExecute superExecute=0)
      : ScriptFunction(inExe, inSig), name(inName), isStatic(inIsStatic), superExecute(superExecute)  { }

   const char *name;
   bool isStatic;
   StackExecute superExecute;
};


inline void SetFloatAligned(void *inPtr, const Float &inValue)
{
   #ifdef HXCPP_ALIGN_FLOAT
   int *dest = (int *)inPtr;
   const int *src = (const int *)&inValue;
   dest[1] = src[1];
   #else
   *(Float *)inPtr = inValue;
   #endif
}


inline Float GetFloatAligned(const void *inPtr)
{
   #ifdef HXCPP_ALIGN_FLOAT
   Float result;
   int *dest = (int *)&result;
   const int *src = (const int *)inPtr;
   dest[0] = src[0];
   dest[1] = src[1];
   return result;
   #else
   return *(Float *)inPtr;
   #endif
}


inline void StackContext::pushFloat(Float f)
{
   SetFloatAligned(pointer, f);
   pointer += sizeof(Float);
}
inline void StackContext::pushString(const String &s)
{
   *(String *)pointer = s;
   pointer += sizeof(String);
}

inline void StackContext::pushObject(Dynamic d)
{
   *(hx::Object **)pointer = d.mPtr;
   pointer += sizeof(hx::Object *);
}

inline void StackContext::returnFloat(Float f)
{
   SetFloatAligned(frame, f);
}
inline void StackContext::returnString(const String &s)
{
   *(String *)frame = s;
}
inline void StackContext::returnObject(Dynamic d)
{
   *(hx::Object **)frame = d.mPtr;
}

inline hx::Object *StackContext::getThis(bool inCheckPtr)
{
   #ifdef HXCPP_CHECK_POINTER
      if (inCheckPtr)
      {
         if (!*(hx::Object **)frame) NullReference("This", false);
         #ifdef HXCPP_GC_CHECK_POINTER
         GCCheckPointer(*(hx::Object **)frame);
         #endif
      }
   #endif
   return *(hx::Object **)frame;
}


inline Float StackContext::getFloat(int inPos)
{
   return GetFloatAligned(frame+inPos);
}
inline String StackContext::getString(int inPos)
{
   return *(String *)(frame+inPos);
}
inline Dynamic StackContext::getObject(int inPos)
{
   return *(hx::Object **)(frame+inPos);
}


enum SignatureChar
{
   sigVoid = 'v',
   sigBool = 'b',
   sigInt = 'i',
   sigFloat = 'f',
   sigString = 's',
   sigObject = 'o',
};



struct AutoStack
{
   CppiaCtx *ctx;
   unsigned char *pointer;
   unsigned char *frame;

   AutoStack(CppiaCtx *inCtx) : ctx(inCtx)
   {
      frame = ctx->frame;
      pointer = ctx->pointer;
      ctx->frame = pointer;
   }
   AutoStack(CppiaCtx *inCtx,unsigned char *inPointer) : ctx(inCtx)
   {
      frame = ctx->frame;
      pointer = inPointer;
      ctx->frame = pointer;
   }

   ~AutoStack()
   {
      ctx->pointer = pointer;
      ctx->frame = frame;
   }
};





typedef hx::Object * (*ScriptableClassFactory)(void **inVTable,int inDataSize);
typedef hx::Object * (*ScriptableInterfaceFactory)(void **inVTable,::hx::Object *);

void ScriptableRegisterClass( String inName, int inBaseSize, ScriptNamedFunction *inFunctions, ScriptableClassFactory inFactory, ScriptFunction inConstruct);


#if (HXCPP_API_LEVEL >= 330)
void ScriptableRegisterInterface( String inName, ScriptNamedFunction *inFunctions, void *inInterfacePointers);
void ScriptableRegisterNameSlots(const char *inNames[], int inLength);

#else
void ScriptableRegisterInterface( String inName, ScriptNamedFunction *inFunctions,const hx::type_info *inType, ScriptableInterfaceFactory inFactory);
#endif

::String ScriptableToString(void *);
hx::Class ScriptableGetClass(void *);
int ScriptableGetType(void *);
void ScriptableMark(void *, hx::Object *, HX_MARK_PARAMS);
void ScriptableVisit(void *, hx::Object *, HX_VISIT_PARAMS);
bool ScriptableField(hx::Object *, const ::String &,hx::PropertyAccess inCallProp,Dynamic &outResult);
bool ScriptableField(hx::Object *, int inName,hx::PropertyAccess inCallProp,Float &outResult);
bool ScriptableField(hx::Object *, int inName,hx::PropertyAccess inCallProp,Dynamic &outResult);
void ScriptableGetFields(hx::Object *inObject, Array< ::String> &outFields);
bool ScriptableSetField(hx::Object *, const ::String &, Dynamic inValue,hx::PropertyAccess inCallProp, Dynamic &outValue);


class CppiaLoadedModule_obj : public ::hx::Object
{
public:
   virtual void run() = 0;
   virtual void boot() = 0;
   virtual ::hx::Class resolveClass( ::String inName) = 0;
};
typedef ::hx::ObjectPtr<CppiaLoadedModule_obj> CppiaLoadedModule;



} // End namespace hx

void __scriptable_load_neko(String inName);
void __scriptable_load_cppia(String inCode);
::hx::CppiaLoadedModule __scriptable_cppia_from_string(String inCode);
::hx::CppiaLoadedModule __scriptable_cppia_from_data(Array<unsigned char> inBytes);
void __scriptable_load_neko_bytes(Array<unsigned char> inBytes);
void __scriptable_load_abc(Array<unsigned char> inBytes);

#if (HXCPP_API_LEVEL >= 330)

#define HX_SCRIPTABLE_REGISTER_INTERFACE(name,class) \
    hx::ScriptableRegisterInterface( HX_CSTRING(name), __scriptableFunctions, & class##_scriptable )

#else

#define HX_SCRIPTABLE_REGISTER_INTERFACE(name,class) \
    hx::ScriptableRegisterInterface( HX_CSTRING(name), __scriptableFunctions, &typeid(class), class##__scriptable::__script_create )

#endif

#define HX_SCRIPTABLE_REGISTER_CLASS(name,class) \
   hx::ScriptableRegisterClass( HX_CSTRING(name), (int)offsetof(class##__scriptable,__scriptVTable) + sizeof(void *), __scriptableFunctions, class##__scriptable::__script_create, class##__scriptable::__script_construct )


#ifdef HXCPP_VISIT_ALLOCS
#define SCRIPTABLE_VISIT_FUNCTION \
void __Visit(HX_VISIT_PARAMS) { super::__Visit(HX_VISIT_ARG); hx::ScriptableVisit(__scriptVTable[-1],this,HX_VISIT_ARG); }
#else
#define SCRIPTABLE_VISIT_FUNCTION
#endif


#define HX_DEFINE_SCRIPTABLE(ARG_LIST) \
   inline void *operator new( size_t inSize, int inExtraDataSize ) \
   { \
      return hx::InternalNew(inSize + inExtraDataSize,true); \
   } \
   inline void operator delete(void *,int) {} \
   public: \
   void **__scriptVTable; \
   static hx::Object *__script_create(void **inVTable, int inExtra) { \
    __ME *result = new (inExtra) __ME(); \
    result->__scriptVTable = inVTable; \
   return result; } \
   void ** __GetScriptVTable() { return __scriptVTable; } \
   ::String toString() {  if (__scriptVTable[0] ) \
     { hx::CppiaCtx *ctx = hx::CppiaCtx::getCurrent(); hx::AutoStack a(ctx); ctx->pushObject(this); return ctx->runString(__scriptVTable[0]); } \
      else return __superString::toString(); } \
   ::String __ToString() const { return hx::ScriptableToString(__scriptVTable[-1]); } \
   hx::Class __GetClass() const { return hx::ScriptableGetClass(__scriptVTable[-1]); } \
   int __GetType() const { return hx::ScriptableGetType(__scriptVTable[-1]); } \
   void __Mark(HX_MARK_PARAMS) { super::__Mark(HX_MARK_ARG); hx::ScriptableMark(__scriptVTable[-1],this,HX_MARK_ARG); } \
   SCRIPTABLE_VISIT_FUNCTION



#define HX_DEFINE_SCRIPTABLE_INTERFACE \
   void **__scriptVTable; \
   Dynamic mDelegate; \
   hx::Object *__GetRealObject() { return mDelegate.mPtr; } \
   SCRIPTABLE_VISIT_FUNCTION \
   void ** __GetScriptVTable() { return __scriptVTable; } \
   public: \
   static hx::Object *__script_create(void **inVTable,hx::Object *inDelegate) { \
    __ME *result = new __ME(); \
    result->__scriptVTable = inVTable; \
    result->mDelegate = inDelegate; \
    return result; }



#define HX_DEFINE_SCRIPTABLE_DYNAMIC \
 \
	hx::Val __Field(const ::String &inName,hx::PropertyAccess inCallProp) \
      { Dynamic result; if (hx::ScriptableField(this,inName,inCallProp,result)) return result; return super::__Field(inName,inCallProp); } \
	Float __INumField(int inFieldID) \
		{ Float result; if (hx::ScriptableField(this,inFieldID,hx::paccAlways,result)) return result; return super::__INumField(inFieldID); } \
	Dynamic __IField(int inFieldID) \
		{ Dynamic result; if (hx::ScriptableField(this,inFieldID,hx::paccAlways,result)) return result; return super::__IField(inFieldID); } \
   hx::Val __SetField(const ::String &inName,const hx::Val &inValue,hx::PropertyAccess inCallProp) \
   { \
      Dynamic value; \
      if (hx::ScriptableSetField(this, inName, inValue,inCallProp,value)) \
         return value; \
		return super::__SetField(inName,inValue,inCallProp); \
   } \
	void __GetFields(Array< ::String> &outFields) \
		{ super::__GetFields(outFields); hx::ScriptableGetFields(this,outFields); }




#endif
