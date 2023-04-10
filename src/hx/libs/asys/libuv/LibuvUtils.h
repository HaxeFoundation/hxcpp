#pragma once

#include <hxcpp.h>
#include <uv.h>
#include <memory>
#include <hx/asys/libuv/LibuvAsysContext.h>

namespace hx::asys::libuv
{
    LibuvAsysContext context(Context ctx);

    std::unique_ptr<uv_fs_t, uv_fs_cb> unique_fs_req(uv_fs_t* request);

    hx::EnumBase create(const String& name, const int index, int fields);

    hx::EnumBase uv_err_to_enum(int code);

    struct BaseRequest
    {
        const hx::RootedObject<hx::Object> cbSuccess;
        const hx::RootedObject<hx::Object> cbFailure;

        BaseRequest(Dynamic _cbSuccess, Dynamic _cbFailure);
    };

    void basic_callback(uv_fs_t* request);

    void clean_handle(uv_handle_t* handle);

    hx::EnumBase getName(uv_handle_t* handle, bool remote);
}