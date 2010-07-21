#include <hxcpp.h>


// -------- Enums ---------------------------------

namespace hx
{

Dynamic EnumBase_obj::__Create(DynamicArray inArgs) { return new hx::EnumBase_obj; }
Dynamic EnumBase_obj::__CreateEmpty() { return new hx::EnumBase_obj; }


int EnumBase_obj::__FindIndex(String inName) { return -1; }
int EnumBase_obj::__FindArgCount(String inName) { return -1; }
Dynamic EnumBase_obj::__Field(const String &inString) { return null(); }

static Class hxEnumBase_obj__mClass;
Class &EnumBase_obj::__SGetClass() { return hxEnumBase_obj__mClass; }

//void hxEnumBase_obj::__GetFields(Array<String> &outFields) { }

void EnumBase_obj::__boot()
{
   Static(hxEnumBase_obj__mClass) = RegisterClass(HX_STRING(L"__EnumBase",10) ,TCanCast<EnumBase_obj>,
                       sNone,sNone,
                       &__CreateEmpty, &__Create, 0 );
}

void EnumBase_obj::__Mark()
{
   MarkMember(tag);
   MarkMember(mArgs);
}

String EnumBase_obj::toString() {
   if (mArgs==null() || mArgs->length==0)
      return tag;
   return tag + HX_STR(L"(") + mArgs->join(HX_STR(L",")) + HX_STR(L")");
}

}

