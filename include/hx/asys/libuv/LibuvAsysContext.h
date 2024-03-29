#pragma once

#include <hx/asys/Asys.h>
#include <hx/Thread.h>
#include <uv.h>

HX_DECLARE_CLASS3(hx, asys, libuv, LibuvAsysContext)

namespace hx::asys::libuv
{
    class LibuvAsysContext_obj final : public Context_obj
    {
    public:
        uv_loop_t* uvLoop;

        LibuvAsysContext_obj(uv_loop_t* uvLoop);

        bool loop();
        void close();
    };
}