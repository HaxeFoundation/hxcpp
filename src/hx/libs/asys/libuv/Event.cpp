#include <hxcpp.h>
#include "Event.h"

hx::asys::libuv::Event_obj::Event_obj(Dynamic _func)
    : func(_func)
    , intervalMs(null())
    , timer(cpp::Pointer<uv_timer_t>()) {}

hx::asys::libuv::Event_obj::Event_obj(Dynamic _func, int _intervalMs)
    : func(_func)
    , intervalMs(_intervalMs)
    , timer(cpp::Pointer<uv_timer_t>()) {}

void hx::asys::libuv::Event_obj::__Mark(hx::MarkContext* __inCtx)
{
    HX_MARK_MEMBER(func);
}

#ifdef HXCPP_VISIT_ALLOCS

void hx::asys::libuv::Event_obj::__Visit(hx::VisitContext* __inCtx)
{
    HX_VISIT_MEMBER(func);
}

#endif