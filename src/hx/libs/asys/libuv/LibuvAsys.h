#pragma once

#include <hx/asys/Asys.h>
#include <hx/Thread.h>
#include <uv.h>

HX_DECLARE_CLASS2(hx, asys, Event)
HX_DECLARE_CLASS2(hx, asys, LibuvAsysContext)

namespace hx::asys
{
    class BaseData
    {
    public:
        virtual ~BaseData() = 0;
    };

    class Event_obj : public Object
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

    class LibuvAsysContext_obj : public Context_obj
    {
    private:
        HxMutex mutex;
        Array<Event> queue;

    public:
        LibuvAsysContext_obj();

        cpp::Pointer<uv_loop_t> uvLoop;
        cpp::Pointer<uv_async_t> uvAsync;

        void consume();
        void enqueue(Dynamic func);
        Dynamic enqueue(Dynamic func, int intervalMs);
        void cancel(Dynamic);
        void loop();
        void finalize();

        void __Mark(hx::MarkContext *__inCtx);
#ifdef HXCPP_VISIT_ALLOCS
        void __Visit(hx::VisitContext *__inCtx);
#endif
    };
}