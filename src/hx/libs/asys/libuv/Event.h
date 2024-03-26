#pragma once

#include <uv.h>

namespace hx::asys::libuv
{
    class Event_obj : public hx::Object
    {
    public:
        Dynamic func;
        Null<int> intervalMs;
        cpp::Pointer<uv_timer_t> timer;

        Event_obj(Dynamic func);
        Event_obj(Dynamic func, int intervalMs);

        void __Mark(hx::MarkContext *__inCtx);
#ifdef HXCPP_VISIT_ALLOCS
        void __Visit(hx::VisitContext *__inCtx);
#endif
    };
}