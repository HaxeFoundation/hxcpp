#include <hxcpp.h>

#include <stdio.h>
#include <hxMath.h>
//#include <hxMacros.h>
#include <cpp/CppInt32__.h>
#include <map>


#ifdef _WIN32

#include <windows.h>
// Stoopid windows ...
#ifdef RegisterClass
#undef RegisterClass
#endif
#ifdef abs
#undef abs
#endif

#else

#include <wchar.h>
#ifndef EMSCRIPTEN
typedef  int64_t  __int64;
#endif

#endif

#ifdef HXCPP_SCRIPTABLE
#include <hx/Scriptable.h>
#endif


// --- hxObject -----------------------------------------

namespace hx
{

String sNone[] = { String(null()) };

#if (HXCPP_API_LEVEL>=332)
bool Object::_hx_isInstanceOf(int inClassId)
{
   return inClassId == hx::Object::_hx_ClassId;
}
#endif

Dynamic Object::__IField(int inFieldID)
{
   return __Field( __hxcpp_field_from_id(inFieldID), HX_PROP_DYNAMIC );
}

double Object::__INumField(int inFieldID)
{
   return __IField(inFieldID);
}

Dynamic *Object::__GetFieldMap() { return 0; }


int Object::__Compare(const Object *inRHS) const
{
   #if (HXCPP_API_LEVEL<331)
   hx::Object *real = const_cast<Object *>(this)->__GetRealObject();
   #else
   hx::Object *real = const_cast<Object *>(this);
   #endif
   return real < inRHS ? -1 : real==inRHS ? 0 : 1;
}


hx::Val Object::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   #if 0
   // Will be true for 'Implements dynamic'
   if (inCallProp && __GetFieldMap())
   {
      Dynamic resolve = __Field(HX_CSTRING("resolve"), false);
      if (resolve.mPtr)
         return resolve(inString);
   }
   #endif
   return null();
}

bool Object::__HasField(const String &inString)
{
   return false;
}
Dynamic Object::__Run(const Array<Dynamic> &inArgs) { return 0; }
Dynamic Object::__GetItem(int inIndex) const { return null(); }
Dynamic Object::__SetItem(int inIndex,Dynamic) { return null();  }


void Object::__SetThis(Dynamic inThis) { }

#if (HXCPP_API_LEVEL<331)
bool Object::__Is(Dynamic inClass ) const { return __Is(inClass.GetPtr()); }
#endif

hx::Class Object__mClass;

bool AlwaysCast(Object *inPtr) { return inPtr!=0; }

#if (HXCPP_API_LEVEL < 330)
DynamicArray Object::__EnumParams() { return DynamicArray(); }
String Object::__Tag() const { return HX_CSTRING("<not enum>"); }
int Object::__Index() const { return -1; }
#endif

#if (HXCPP_API_LEVEL >= 330) && !defined(HXCPP_SCRIPTABLE)
// Other implementation is in Cppia.cpp
void *hx::Object::_hx_getInterface(int inId)
{
   return 0;
}
#endif



#ifdef HXCPP_SCRIPTABLE

class Object__scriptable : public hx::Object {
   typedef Object__scriptable __ME;
   void __construct() { }
   typedef hx::Object super;
   typedef hx::Object __superString;
   HX_DEFINE_SCRIPTABLE(HX_ARR_LIST0);
   HX_DEFINE_SCRIPTABLE_DYNAMIC;
};

hx::ScriptFunction Object::__script_construct;

static void CPPIA_CALL __s_toString(hx::CppiaCtx *ctx) {
   ctx->returnString((ctx->getThis())->toString());
}
static hx::ScriptNamedFunction __scriptableFunctions[] = {
  hx::ScriptNamedFunction("toString",__s_toString,"s"),
  hx::ScriptNamedFunction(0,0,0) };

#endif

void Object::__boot()
{
   Static(Object__mClass) = hx::_hx_RegisterClass(HX_CSTRING("Dynamic"),AlwaysCast,sNone,sNone,0,0, 0, 0 );

   #ifdef HXCPP_SCRIPTABLE
   hx::ScriptableRegisterClass( HX_CSTRING("hx.Object"), (int)sizeof(hx::Object__scriptable), __scriptableFunctions, Object__scriptable::__script_create, Object::__script_construct );
   #endif
}


hx::Class &Object::__SGetClass() { return Object__mClass; }

hx::Class Object::__GetClass() const { return Object__mClass; }

hx::FieldRef Object::__FieldRef(const String &inString) { return hx::FieldRef(this,inString); }

String Object::__ToString() const { return HX_CSTRING("Object"); }

const char * Object::__CStr() const { return __ToString().__CStr(); }


hx::Val Object::__SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp)
{
   hx::Throw( HX_CSTRING("Invalid field:") + inField );
   return null();
}

Dynamic Object::__run()
{
   return __Run(Array_obj<Dynamic>::__new());
}

Dynamic Object::__run(D a)
{
   return __Run( Array_obj<Dynamic>::__new(0,1) << a );
}

Dynamic Object::__run(D a,D b)
{
   return __Run( Array_obj<Dynamic>::__new(0,2) << a << b );
}

Dynamic Object::__run(D a,D b,D c)
{
   return __Run( Array_obj<Dynamic>::__new(0,3) << a << b << c);
}
Dynamic Object::__run(D a,D b,D c,D d)
{
   return __Run( Array_obj<Dynamic>::__new(0,4) << a << b << c << d);
}
Dynamic Object::__run(D a,D b,D c,D d,D e)
{
   return __Run( Array_obj<Dynamic>::__new(0,5) << a << b << c << d << e);
}

void Object::__GetFields(Array<String> &outFields)
{
}


String Object::toString()
{
   Dynamic *m = __GetFieldMap();
   if (m)
   {
      Dynamic func;
      if (FieldMapGet(m,HX_CSTRING("toString"),func))
         return func();
   }
   return __ToString();
}


} // end namespace hx




