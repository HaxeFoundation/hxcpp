#include <hxcpp.h>


// -------- Enums ---------------------------------

namespace hx
{

Dynamic EnumBase_obj::__Create(DynamicArray inArgs) { return new hx::EnumBase_obj; }
Dynamic EnumBase_obj::__CreateEmpty() { return new hx::EnumBase_obj; }


int EnumBase_obj::__FindIndex(String inName)
{
   if (inName==HX_CSTRING("__")) return 1;
   return -1;
}
int EnumBase_obj::__FindArgCount(String inName)
{
   if (inName==HX_CSTRING("__")) return 0;
   return -1;
}
hx::Val EnumBase_obj::__Field(const String &inString, hx::PropertyAccess inCallProp) { return null(); }

hx::Class hxEnumBase_obj__mClass;
hx::Class &EnumBase_obj::__SGetClass() { return hxEnumBase_obj__mClass; }

//void hxEnumBase_obj::__GetFields(Array<String> &outFields) { }

void EnumBase_obj::__boot()
{
   Static(hxEnumBase_obj__mClass) = hx::_hx_RegisterClass(HX_CSTRING("__EnumBase") ,TCanCast<EnumBase_obj>,
                       sNone,sNone,
                       &__CreateEmpty, &__Create, 0 );
}

#if (HXCPP_API_LEVEL >= 330)
DynamicArray EnumBase_obj::_hx_getParameters()
{
   if (mFixedFields==0)
      return null();
   Array<Dynamic> result = Array_obj<Dynamic>::__new(mFixedFields);
   cpp::Variant *fixed = _hx_getFixed();
   for(int i=0;i<mFixedFields;i++)
      result[i] = fixed[i];
   return result;
}
#endif

int EnumBase_obj::__Compare(const hx::Object *inRHS) const
{
   if (inRHS->__GetType()!=vtEnum) return -1;
   const EnumBase_obj *rhs = static_cast<const EnumBase_obj *>(inRHS);

   #if (HXCPP_API_LEVEL >= 330)
   if (_hx_tag!=rhs->_hx_tag || GetEnumName()!=rhs->GetEnumName()) return -1;
   if (mFixedFields!=rhs->mFixedFields) return -1;
   if (!mFixedFields) return 0;

   const cpp::Variant *f0 = _hx_getFixed();
   const cpp::Variant *f1 = rhs->_hx_getFixed();
   for(int i=0;i<mFixedFields;i++)
      if ( f0[i] != f1[i])
         return -1;
   #else
   if (tag!=rhs->tag || GetEnumName()!=rhs->GetEnumName()) return -1;
   if (mArgs==null() && rhs->mArgs==null())
      return 0;
   if (mArgs==null() || rhs->mArgs==null())
      return -1;

   int n = mArgs->__length();
   if (rhs->mArgs->__length()!=n)
      return -1;
   for(int i=0;i<n;i++)
      if ( mArgs[i] != rhs->mArgs[i] )
         return -1;
   #endif

   return 0;
}

#if (HXCPP_API_LEVEL >= 330)
bool __hxcpp_enum_eq( ::hx::EnumBase a, ::hx::EnumBase b)
{
   if (!a.mPtr || !b.mPtr)
      return !a.mPtr && !b.mPtr;
   // Known to be same type
   if (a->index != b->index)
      return 0;
   int n =  a->_hx_getParamCount();
   if (n==0)
      return true;
   cpp::Variant *fa = a->_hx_getFixed();
   cpp::Variant *fb = b->_hx_getFixed();
   for(int i=0;i<n;i++)
      if ( fa[i]!=fb[i] )
         return false;
   return true;
}
#endif



void EnumBase_obj::__Mark(hx::MarkContext *__inCtx)
{
   #if (HXCPP_API_LEVEL >= 330)
   HX_MARK_MEMBER(_hx_tag);
   if (mFixedFields>0)
   {
      cpp::Variant *v = _hx_getFixed();
      for(int i=0;i<mFixedFields;i++)
         HX_MARK_MEMBER(v[i]);
   }
   #else
   HX_MARK_MEMBER(tag);
   HX_MARK_MEMBER(mArgs);
   #endif
}

#ifdef HXCPP_VISIT_ALLOCS
void EnumBase_obj::__Visit(hx::VisitContext *__inCtx)
{

   #if (HXCPP_API_LEVEL >= 330)
   HX_VISIT_MEMBER(_hx_tag);
   if (mFixedFields>0)
   {
      cpp::Variant *v = _hx_getFixed();
      for(int i=0;i<mFixedFields;i++)
         HX_VISIT_MEMBER(v[i]);
   }
   #else
   HX_VISIT_MEMBER(tag);
   HX_VISIT_MEMBER(mArgs);
   #endif
}
#endif


String EnumBase_obj::toString() {
   #if (HXCPP_API_LEVEL >= 330)
   if (mFixedFields==0)
      return _hx_tag;
   if (mFixedFields==1)
      return _hx_tag + HX_CSTRING("(") + _hx_getFixed()->asString() + HX_CSTRING(")");

   Array<String> args = Array_obj<String>::__new(mFixedFields);
   cpp::Variant *v = _hx_getFixed();
   for(int i=0;i<mFixedFields;i++)
      args[i] = v[i].asString();

   return _hx_tag + HX_CSTRING("(") + args->join(HX_CSTRING(",")) + HX_CSTRING(")");
   #else
   if (mArgs==null() || mArgs->length==0)
      return tag;
   return tag + HX_CSTRING("(") + mArgs->join(HX_CSTRING(",")) + HX_CSTRING(")");
   #endif
}

}

