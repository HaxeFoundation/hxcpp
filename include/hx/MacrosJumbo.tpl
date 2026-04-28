#ifndef HX_MACROS_JUMBO_H
#define HX_MACROS_JUMBO_H

#if (HXCPP_API_LEVEL>=500)
    ::foreach LOCALS::
    #define HX_BEGIN_LOCAL_FUNC_S::ARG::(SUPER,name,::TYPE_ARGS::) \
       struct name : public SUPER { \
       ::TYPE_DECL::; \
       void __Mark(::hxNS::MarkContext *__inCtx) { ::MARKS:: } \
       void __Visit(::hxNS::VisitContext *__inCtx) { ::VISITS:: } \
       name(::CONSTRUCT_ARGS::) : ::CONSTRUCT_VARS:: {}::end::
#else
    ::foreach LOCALS::
    #define HX_BEGIN_LOCAL_FUNC_S::ARG::(SUPER,name,::TYPE_ARGS::) \
       struct name : public SUPER { \
       ::TYPE_DECL::; \
       void __Mark(::hxNS::MarkContext *__inCtx) { DoMarkThis(__inCtx); ::MARKS:: } \
       void __Visit(::hxNS::VisitContext *__inCtx) { DoVisitThis(__inCtx); ::VISITS:: } \
       name(::CONSTRUCT_ARGS::) : ::CONSTRUCT_VARS:: {}::end::
#endif

#define HX_LOCAL_RUN _hx_run

::foreach LOCALS::
#define HX_END_LOCAL_FUNC::ARG::(ret) HX_DYNAMIC_CALL::ARG::(ret, HX_LOCAL_RUN ) };::end::

// For compatibility until next version of haxe is released
::foreach LOCALS::
#define HX_BEGIN_LOCAL_FUNC::ARG::(name,::TYPE_ARGS::) \
      HX_BEGIN_LOCAL_FUNC_S::ARG::(::hxNS::LocalFunc,name,::TYPE_ARGS::)::end::


#endif


