#ifndef HX_MACROS_JUMBO_H
#define HX_MACROS_JUMBO_H

::foreach LOCALS::
#define HX_BEGIN_LOCAL_FUNC_S::ARG::(SUPER,name,::TYPE_ARGS::) \
   struct name : public SUPER { \
   ::TYPE_DECL::; \
   void __Mark(hx::MarkContext *__inCtx) { DoMarkThis(__inCtx); ::MARKS:: } \
   void __Visit(hx::VisitContext *__inCtx) { DoVisitThis(__inCtx); ::VISITS:: } \
   name(::CONSTRUCT_ARGS::) : ::CONSTRUCT_VARS:: {}::end::

#if (HXCPP_API_LEVEL>=330)
  #define HX_LOCAL_RUN _hx_run
#else
  #define HX_LOCAL_RUN run
#endif

::foreach LOCALS::
#define HX_END_LOCAL_FUNC::ARG::(ret) HX_DYNAMIC_CALL::ARG::(ret, HX_LOCAL_RUN ) };::end::

// For compatibility until next version of haxe is released
::foreach LOCALS::
#define HX_BEGIN_LOCAL_FUNC::ARG::(name,::TYPE_ARGS::) \
      HX_BEGIN_LOCAL_FUNC_S::ARG::(hx::LocalFunc,name,::TYPE_ARGS::)::end::


#endif


