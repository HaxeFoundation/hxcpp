#include <hxcpp.h>
#include "LibuvAsysContext.h"
#include "Event.h"
#include "BaseData.h"

#include <memory>

hx::asys::libuv::LibuvAsysContext_obj::LibuvAsysContext_obj()
    : uvLoop(new uv_loop_t())
    , uvAsync(new uv_async_t())
    , mutex(HxMutex())
    , queue(Array<Event>(0, 0))
{
    _hx_add_finalizable(LibuvAsysContext(this), false);

    uv_loop_init(uvLoop);
    uv_async_init(uvLoop, uvAsync, [](uv_async_t* async) {
        auto ctx  = LibuvAsysContext(static_cast<LibuvAsysContext_obj*>(async->data));
        auto zone = AutoGCZone();

        ctx->consume();
    });

    uvAsync->ptr->data = this;

    uv_unref(uvAsync.rawCast());
}

void hx::asys::libuv::LibuvAsysContext_obj::enqueue(Dynamic func)
{
    auto lock   = AutoLock(mutex);
    auto result = uv_async_send(uvAsync);
    if (result < 0)
    {
        hx::Throw(String::create(uv_strerror(result)));
    }

    queue->Add(Event(new Event_obj(func)));
}

Dynamic hx::asys::libuv::LibuvAsysContext_obj::enqueue(Dynamic func, int intervalMs)
{
    auto lock   = AutoLock(mutex);
    auto result = uv_async_send(uvAsync);
    if (result < 0)
    {
        hx::Throw(String::create(uv_strerror(result)));
    }

    auto event = Event(new Event_obj(func, intervalMs));

    queue->Add(event);

    return event;
}

void hx::asys::libuv::LibuvAsysContext_obj::cancel(Dynamic obj)
{
    class Callback : public hx::LocalFunc
    {
    public:
        Event event;

        Callback(Event _event) : event(_event) {}

        void HX_LOCAL_RUN()
        {
            uv_timer_stop(event->timer);
            uv_close(
                event->timer.rawCast(),
                [](uv_handle_t* handle) {
                    auto data = static_cast<BaseData*>(handle->data);

                    if (data)
                    {
                        delete data;
                    }

                    delete handle;
                });
        }

        void __Mark(hx::MarkContext* __inCtx)
        {
            HX_MARK_MEMBER(event);
        }

#ifdef HXCPP_VISIT_ALLOCS
        void __Visit(hx::VisitContext* __inCtx)
        {
            HX_VISIT_MEMBER(event);
        }
#endif
    };

    enqueue(Dynamic(new Callback(obj.Cast<Event>())));
}

void hx::asys::libuv::LibuvAsysContext_obj::loop()
{
    consume();

    auto freeZone = AutoGCFreeZone();
    auto result   = uv_run(uvLoop, UV_RUN_DEFAULT);

    if (result < 0)
    {
        freeZone.close();

        hx::Throw(String::create(uv_strerror(result)));
    }
}

void hx::asys::libuv::LibuvAsysContext_obj::consume()
{
    class RunData : public BaseData
    {
    private:
        RootedObject task;
        cpp::Pointer<uv_timer_t> timer;

    public:
        RunData(Dynamic _task, uv_timer_t* _timer) : task(_task.mPtr), timer(_timer) {}
        ~RunData()
        {
            uv_close(timer.rawCast(), [](uv_handle_t* handle) { delete handle; });
        }

        Dynamic callback()
        {
            return Dynamic(task.rooted);
        }
    };

    auto lock = AutoLock(mutex);

    for (auto i = 0; i < queue->length; i++)
    {
        auto event  = queue[i];
        auto timer  = std::make_unique<uv_timer_t>();
        auto result = 0;

        if ((result = uv_timer_init(uvLoop, timer.get())) < 0)
        {
            hx::Throw(String::create(uv_err_name(result)));
        }

        if (event->intervalMs.isNull)
        {
            auto callback = [](uv_timer_t* timer) {
                auto gcZone = AutoGCZone();
                auto data   = std::unique_ptr<RunData>(static_cast<RunData*>(timer->data));
                auto task   = data->callback();

                task();
            };

            if ((result = uv_timer_start(timer.get(), callback, 0, 0)) < 0)
            {
                hx::Throw(String::create(uv_err_name(result)));
            }
        }
        else
        {
            auto callback = [](uv_timer_t* timer) {
                auto gcZone = AutoGCZone();
                auto data   = static_cast<RunData*>(timer->data);
                auto task   = data->callback();

                task();
            };

            if ((result = uv_timer_start(timer.get(), callback, event->intervalMs.value, event->intervalMs.value)) < 0)
            {
                hx::Throw(String::create(uv_err_name(result)));
            }
        }

        auto ptr = timer.release();

        ptr->data = new RunData(event->func, ptr);
    }

    queue->resize(0);
}

void hx::asys::libuv::LibuvAsysContext_obj::finalize()
{
    // Cleanup the loop according to https://stackoverflow.com/a/25831688
    // TODO : See if this could try and invoke haxe callbacks,
    // this would be bad as we're in a GC finaliser.
    uv_stop(uvLoop);
    uv_walk(
        uvLoop,
        [](uv_handle_t* handle, void*) {
            uv_close(handle, [](uv_handle_t* handle) { delete handle; });
        },
        nullptr);

    if (uv_run(uvLoop, uv_run_mode::UV_RUN_DEFAULT) < 0)
    {
        // TODO : what should be do if our run failed.
    }

    if (uv_loop_close(uvLoop) < 0)
    {
        // TODO : what should be do if there are still outstanding handles after trying to close them all.
    }
    else
    {
        uvLoop.destroy();
    }
}

void hx::asys::libuv::LibuvAsysContext_obj::__Mark(hx::MarkContext* __inCtx)
{
    HX_MARK_MEMBER(queue);
}

#ifdef HXCPP_VISIT_ALLOCS

void hx::asys::libuv::LibuvAsysContext_obj::__Visit(hx::VisitContext* __inCtx)
{
    HX_VISIT_MEMBER(queue);
}

#endif