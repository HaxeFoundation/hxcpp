#ifndef INCLUDED_HX_SCRIPTABLE
#define INCLUDED_HX_SCRIPTABLE

#include <setjmp.h> 

#include <typeinfo>
#ifdef __clang__
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif

namespace hx
{

struct ScriptNamedFunction : public ScriptFunction
{
   ScriptNamedFunction(const char *inName,StackExecute inExe,const char *inSig)
      : ScriptFunction(inExe, inSig), name(inName) { }
   const char *name;
};

enum
{
   bcrBreak    = 0x01,
   bcrContinue = 0x02,
   bcrReturn   = 0x04,

   bcrLoop     = (bcrBreak | bcrContinue),
};

struct CppiaCtx
{
   CppiaCtx();
   ~CppiaCtx();

   unsigned char *stack;
   unsigned char *pointer;
   unsigned char *frame;
   class Object  *exception;

   jmp_buf *returnJumpBuf;
   jmp_buf *loopJumpBuf;

   unsigned int breakContReturn;

   template<typename T>
   void push(T inValue)
   {
      *(T *)pointer = inValue;
      pointer += sizeof(T);
   }

   int getFrameSize() const { return pointer-frame; }
   void mark(struct MarkContext *__inCtx);

   static CppiaCtx *getCurrent();

   int runInt(void *vtable);
   Float runFloat(void *vtable);
   String runString(void *vtable);
   void runVoid(void *vtable);
   Dynamic runObject(void *vtable);
   hx::Object *runObjectPtr(void *vtable);

   void push(bool &inValue)
   {
      *(int *)pointer = inValue;
      pointer += sizeof(int);
   }
   inline void pushBool(bool b)
   {
      *(int *)pointer = b;
      pointer += sizeof(int);
   }
   inline void pushInt(int i)
   {
      *(int *)pointer = i;
      pointer += sizeof(int);
   }
   inline void pushFloat(Float f)
   {
      *(Float *)pointer = f;
      pointer += sizeof(Float);
   }
   inline void pushString(const String &s)
   {
      *(String *)pointer = s;
      pointer += sizeof(String);
   }
   inline void pushObject(Dynamic d)
   {
      *(hx::Object **)pointer = d.mPtr;
      pointer += sizeof(hx::Object *);
   }

   inline void returnBool(bool b)
   {
      *(int *)frame = b;
   }
   inline void returnInt(int i)
   {
      *(int *)frame = i;
   }
   inline void returnFloat(Float f)
   {
      *(Float *)frame = f;
   }
   inline void returnString(const String &s)
   {
      *(String *)frame = s;
   }
   inline void returnObject(Dynamic d)
   {
      *(hx::Object **)frame = d.mPtr;
   }

   void longJump()
   {
      longjmp(*returnJumpBuf,1);
   }

   inline hx::Object *getThis(bool inCheckPtr=true)
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

   inline bool getBool(int inPos=0)
   {
      return *(bool *)(frame+inPos);
   }
   inline int getInt(int inPos=0)
   {
      return *(int *)(frame+inPos);
   }
   inline Float getFloat(int inPos=0)
   {
      return *(Float *)(frame+inPos);
   }
   inline String getString(int inPos=0)
   {
      return *(String *)(frame+inPos);
   }
   inline Dynamic getObject(int inPos=0)
   {
      return *(hx::Object **)(frame+inPos);
   }
   inline hx::Object *getObjectPtr(int inPos=0)
   {
      return *(hx::Object **)(frame+inPos);
   }


   void breakFlag() { breakContReturn |= bcrBreak; }
   void continueFlag() { breakContReturn |= bcrContinue; }
   void returnFlag() { breakContReturn |= bcrReturn; }

   void breakJump()
   {
      longjmp(*loopJumpBuf,2);
   }
   void continueJump()
   {
      longjmp(*loopJumpBuf,3);
   }
};

enum SignatureChar
{
   sigVoid = 'v',
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
void ScriptableRegisterInterface( String inName, ScriptNamedFunction *inFunctions,const hx::type_info *inType, ScriptableInterfaceFactory inFactory);

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


}

void __scriptable_load_neko(String inName);
void __scriptable_load_cppia(String inCode);
void __scriptable_load_neko_bytes(Array<unsigned char> inBytes);
void __scriptable_load_abc(Array<unsigned char> inBytes);


#define HX_SCRIPTABLE_REGISTER_INTERFACE(name,class) \
    hx::ScriptableRegisterInterface( HX_CSTRING(name), __scriptableFunctions, &typeid(class), class##__scriptable::__script_create )

#define HX_SCRIPTABLE_REGISTER_CLASS(name,class) \
   hx::ScriptableRegisterClass( HX_CSTRING(name), (int)sizeof(class##__scriptable), __scriptableFunctions, class##__scriptable::__script_create, class##__scriptable::__script_construct )


#define HX_DEFINE_SCRIPTABLE(ARG_LIST) \
   inline void *operator new( size_t inSize, int inExtraDataSize ) \
   { \
      return hx::InternalNew(inSize + inExtraDataSize,true); \
   } \
   inline void operator delete(void *,int) {} \
   void **__scriptVTable; \
   public: \
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


#ifdef HXCPP_VISIT_ALLOCS
#define SCRIPTABLE_VISIT_FUNCTION \
void __Visit(HX_VISIT_PARAMS) { HX_VISIT_OBJECT(mDelegate.mPtr); }
#else
#define SCRIPTABLE_VISIT_FUNCTION
#endif

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


#ifdef HXCPP_VISIT_ALLOCS
#define SCRIPTABLE_DYNAMIC_VISIT_FUNCTION \
void __Visit(HX_VISIT_PARAMS) { super::__Visit(HX_VISIT_ARG); hx::ScriptableVisit(__scriptVTable[-1],this,HX_VISIT_ARG); }
#else
#define SCRIPTABLE_DYNAMIC_VISIT_FUNCTION
#endif

#define HX_DEFINE_SCRIPTABLE_DYNAMIC \
	void __Mark(HX_MARK_PARAMS) { super::__Mark(HX_MARK_ARG); hx::ScriptableMark(__scriptVTable[-1],this,HX_MARK_ARG); } \
   SCRIPTABLE_DYNAMIC_VISIT_FUNCTION \
 \
	Dynamic __Field(const ::String &inName,hx::PropertyAccess inCallProp) \
      { Dynamic result; if (hx::ScriptableField(this,inName,inCallProp,result)) return result; return super::__Field(inName,inCallProp); } \
	Float __INumField(int inFieldID) \
		{ Float result; if (hx::ScriptableField(this,inFieldID,hx::paccAlways,result)) return result; return super::__INumField(inFieldID); } \
	Dynamic __IField(int inFieldID) \
		{ Dynamic result; if (hx::ScriptableField(this,inFieldID,hx::paccAlways,result)) return result; return super::__IField(inFieldID); } \
	Dynamic __SetField(const ::String &inName,const Dynamic &inValue,hx::PropertyAccess inCallProp) \
   { \
      Dynamic value; \
      if (hx::ScriptableSetField(this, inName, inValue,inCallProp,value)) \
         return value; \
		return super::__SetField(inName,inValue,inCallProp); \
   } \
	void __GetFields(Array< ::String> &outFields) \
		{ super::__GetFields(outFields); hx::ScriptableGetFields(this,outFields); }




#endif
