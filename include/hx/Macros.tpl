#ifndef HX_MACROS_H
#define HX_MACROS_H

// --- Functions and their parameters ----

::foreach PARAMS::
#define HX_ARR_LIST::ARG:: ::ARR_LIST::::end::

::foreach PARAMS::
#define HX_DYNAMIC_ARG_LIST::ARG:: ::DYNAMIC_ARG_LIST::::end::

::foreach PARAMS::
#define HX_ARG_LIST::ARG:: ::ARG_LIST::::end::

#define HX_DEFINE_DYNAMIC_FUNC0(class,func,ret) \
static ::NS::Dynamic __##class##func(::hx::NS::Object *inObj) \
{ \
      ret reinterpret_cast<class *>(inObj)->func(); return  ::NS::Dynamic(); \
}; \
 ::NS::Dynamic class::func##_dyn() \
{\
   return ::hx::NS::CreateMemberFunction0(#func,this,__##class##func); \
}


#define HX_DEFINE_DYNAMIC_FUNC(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
static ::NS::Dynamic __##class##func(::hx::NS::Object *inObj, dynamic_arg_list) \
{ \
      ret reinterpret_cast<class *>(inObj)->func(arg_list); return  ::NS::Dynamic(); \
}; \
 ::NS::Dynamic class::func##_dyn() \
{\
   return ::hx::NS::CreateMemberFunction##N(#func,this,__##class##func); \
}


#define HX_DEFINE_DYNAMIC_FUNC_EXTRA(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
static ::NS::Dynamic __##class##func(::hx::NS::Object *inObj, const Array< ::NS::Dynamic> &inArgs) \
{ \
      ret reinterpret_cast<class *>(inObj)->func(array_list); return  ::NS::Dynamic(); \
}; \
 ::NS::Dynamic class::func##_dyn() \
{\
   return ::hx::NS::CreateMemberFunctionVar(#func,this,__##class##func,N); \
}


#define DELEGATE_0(ret,func) ret func() { return mDelegate->func(); }
#define CDELEGATE_0(ret,func) ret func() const { return mDelegate->func(); }
#define DELEGATE_1(ret,func,arg1) ret func(arg1 _a1) { return mDelegate->func(_a1); }
#define CDELEGATE_1(ret,func,arg1) ret func(arg1 _a1) const { return mDelegate->func(_a1); }
#define DELEGATE_2(ret,func,arg1,arg2) ret func(arg1 _a1,arg2 _a2) { return mDelegate->func(_a1,_a2); }





#define HX_DECLARE_DYNAMIC_FUNC(func,dynamic_arg_list) \
    ::NS::Dynamic func##_dyn(dynamic_arg_list);

#define STATIC_HX_DECLARE_DYNAMIC_FUNC(func,dynamic_arg_list) \
   static  ::NS::Dynamic func##_dyn(dynamic_arg_list);


::foreach PARAMS::
::if (ARG>0)::::if (ARG<6)::
#define HX_DEFINE_DYNAMIC_FUNC::ARG::(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,::ARG::,func,ret,HX_ARR_LIST::ARG::,HX_DYNAMIC_ARG_LIST::ARG::,HX_ARG_LIST::ARG::)
::else::
#define HX_DEFINE_DYNAMIC_FUNC::ARG::(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC_EXTRA(class,::ARG::,func,ret,HX_ARR_LIST::ARG::,HX_DYNAMIC_ARG_LIST::ARG::,HX_ARG_LIST::ARG::)
::end::
::end::::end::


#define STATIC_HX_DEFINE_DYNAMIC_FUNC0(class,func,ret) \
static ::NS::Dynamic __##class##func() \
{ \
      ret class::func(); return  ::NS::Dynamic(); \
}; \
 ::NS::Dynamic class::func##_dyn() \
{\
   return ::hx::NS::CreateStaticFunction0(#func,__##class##func); \
}


#define STATIC_HX_DEFINE_DYNAMIC_FUNC(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
static ::NS::Dynamic __##class##func(dynamic_arg_list) \
{ \
      ret class::func(arg_list); return  ::NS::Dynamic(); \
}; \
 ::NS::Dynamic class::func##_dyn() \
{\
   return ::hx::NS::CreateStaticFunction##N(#func,__##class##func); \
}


#define STATIC_HX_DEFINE_DYNAMIC_FUNC_EXTRA(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
static ::NS::Dynamic __##class##func(const Array< ::NS::Dynamic> &inArgs) \
{ \
      ret class::func(array_list); return  ::NS::Dynamic(); \
}; \
 ::NS::Dynamic class::func##_dyn() \
{\
   return ::hx::NS::CreateStaticFunctionVar(#func,__##class##func,N); \
}



::foreach PARAMS::
::if (ARG>0)::::if (ARG<6)::
#define STATIC_HX_DEFINE_DYNAMIC_FUNC::ARG::(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,::ARG::,func,ret,HX_ARR_LIST::ARG::,HX_DYNAMIC_ARG_LIST::ARG::,HX_ARG_LIST::ARG::)
::else::
#define STATIC_HX_DEFINE_DYNAMIC_FUNC::ARG::(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC_EXTRA(class,::ARG::,func,ret,HX_ARR_LIST::ARG::,HX_DYNAMIC_ARG_LIST::ARG::,HX_ARG_LIST::ARG::)
::end::
::end::::end::


#define HX_DYNAMIC_CALL(ret,func,array_args,dyn_arg_list,arg_list) \
    ::NS::Dynamic __Run(const Array< ::NS::Dynamic> &inArgs) { ret func( array_args ); return null();} \
    ::NS::Dynamic __run(dyn_arg_list) { ret func( arg_list ); return null();}

::foreach PARAMS::
#define HX_DYNAMIC_CALL::ARG::(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST::ARG::,HX_DYNAMIC_ARG_LIST::ARG::,HX_ARG_LIST::ARG::)::end::

#define HX_BEGIN_DEFAULT_FUNC(name,t0) \
	namespace { \
   struct name : public ::hx::Object { int __GetType() const { return vtFunction; } \
   HX_IS_INSTANCE_OF enum { _hx_ClassId = ::hx::clsIdClosure }; \
   ::hx::ObjectPtr<t0> __this; \
   name(::hx::ObjectPtr<t0> __0 = null()) : __this(__0) {} \
   void __Mark(::hx::MarkContext *__inCtx) { HX_MARK_MEMBER(__this); } \
   void __Visit(::hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(__this); }


#define HX_END_DEFAULT_FUNC \
}

#define HXARGC(x) int __ArgCount() const { return x; }

#define HX_BEGIN_LOCAL_FUNC_S0(SUPER,name) \
   struct name : public SUPER { \
   HX_IS_INSTANCE_OF enum { _hx_ClassId = ::hx::clsIdClosure }; \
   void __Mark(::hx::MarkContext *__inCtx) { DoMarkThis(__inCtx); } \
   void __Visit(::hx::VisitContext *__inCtx) { DoVisitThis(__inCtx); } \
   name() {}

::foreach LOCALS::
#define HX_BEGIN_LOCAL_FUNC_S::ARG::(SUPER,name,::TYPE_ARGS::) \
   struct name : public SUPER { \
   HX_IS_INSTANCE_OF enum { _hx_ClassId = ::hx::clsIdClosure }; \
   ::TYPE_DECL::; \
   void __Mark(::hx::MarkContext *__inCtx) { DoMarkThis(__inCtx); ::MARKS:: } \
   void __Visit(::hx::VisitContext *__inCtx) { DoVisitThis(__inCtx); ::VISITS:: } \
   name(::CONSTRUCT_ARGS::) : ::CONSTRUCT_VARS:: {}::end::

#if (HXCPP_API_LEVEL>=330)
  #define HX_LOCAL_RUN _hx_run
#else
  #define HX_LOCAL_RUN run
#endif

#define HX_END_LOCAL_FUNC0(ret) HX_DYNAMIC_CALL0(ret, HX_LOCAL_RUN ) };
::foreach LOCALS::
#define HX_END_LOCAL_FUNC::ARG::(ret) HX_DYNAMIC_CALL::ARG::(ret, HX_LOCAL_RUN ) };::end::

// For compatibility until next version of haxe is released
#define HX_BEGIN_LOCAL_FUNC0(name) \
      HX_BEGIN_LOCAL_FUNC_S0(::hx::LocalFunc,name)
::foreach LOCALS::
#define HX_BEGIN_LOCAL_FUNC::ARG::(name,::TYPE_ARGS::) \
      HX_BEGIN_LOCAL_FUNC_S::ARG::(::hx::LocalFunc,name,::TYPE_ARGS::)::end::


#define HX_DECLARE_DYNAMIC_FUNCTIONS \
::foreach PARAMS:: ::if (ARG<6)::::else::  ::NS::Dynamic operator()(::DYNAMIC_ARG_LIST::); \
::end:: ::end::


#define HX_DECLARE_VARIANT_FUNCTIONS \
::foreach PARAMS:: ::if (ARG<6):: inline  ::NS::Dynamic operator()(::DYNAMIC_ARG_LIST::); \
::else::  ::NS::Dynamic operator()(::DYNAMIC_ARG_LIST::); \
::end:: ::end::


#define HX_IMPLEMENT_INLINE_VARIANT_FUNCTIONS \
::foreach PARAMS:: ::if (ARG<6):: ::NS::Dynamic Variant::NS::operator()(::DYNAMIC_ARG_LIST::) { CheckFPtr(); return valObject->__run(::ARG_LIST::); } \
::end:: ::end::


#endif


