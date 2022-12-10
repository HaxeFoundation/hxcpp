#pragma once

#include <hx/asys/Asys.h>
#include <hx/Thread.h>
#include <uv.h>

HX_DECLARE_CLASS3(hx, asys, libuv, Event)
HX_DECLARE_CLASS3(hx, asys, libuv, LibuvAsysContext)

namespace hx::asys::libuv
{
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

namespace hx::asys
{
    Context Context_obj::create()
    {
        return Context(new libuv::LibuvAsysContext_obj());
    }
}