#include <hxcpp.h>
#include <hx/asys/libuv/LibuvAsysContext.h>
#include "BaseData.h"
#include "system/LibuvCurrentProcess.h"

#include <memory>

hx::asys::Context hx::asys::Context_obj::create()
{
    auto loop   = std::make_unique<uv_loop_t>();
    auto result = uv_loop_init(loop.get());

    if (result < 0)
    {
        hx::Throw(String::create(uv_strerror(result)));
    }

    auto current = std::make_unique<hx::asys::libuv::system::LibuvCurrentProcess::Ctx>(loop.get());

    for (auto i = 0; i < current->ttys.size(); i++)
    {
        if ((result = uv_tty_init(loop.get(), &current->ttys.at(i), i, false)) < 0)
        {
            hx::Throw(HX_CSTRING("Failed to init tty : ") + hx::asys::libuv::uv_err_to_enum(result)->GetEnumName());
        }
    }

    return
        Context(
            new libuv::LibuvAsysContext_obj(
                loop.release(),
                hx::asys::system::CurrentProcess(new hx::asys::libuv::system::LibuvCurrentProcess(current.release()))));
}

hx::asys::libuv::LibuvAsysContext_obj::LibuvAsysContext_obj(uv_loop_t* _uvLoop, hx::asys::system::CurrentProcess _process)
    : hx::asys::Context_obj(_process)
    , uvLoop(_uvLoop) {}

bool hx::asys::libuv::LibuvAsysContext_obj::loop()
{
    auto freeZone = AutoGCFreeZone();
    auto result   = uv_run(uvLoop, UV_RUN_NOWAIT);

    if (result < 0)
    {
        freeZone.close();

        hx::Throw(String::create(uv_strerror(result)));
    }

    return result > 0;
}

void hx::asys::libuv::LibuvAsysContext_obj::close()
{
    // Cleanup the loop according to https://stackoverflow.com/a/25831688

    uv_stop(uvLoop);
    uv_walk(
        uvLoop,
        [](uv_handle_t* handle, void*) {
            uv_close(handle, hx::asys::libuv::clean_handle);
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
    
    delete uvLoop;
}