#include <hxcpp.h>
#include "LibuvUtils.h"

namespace
{
    void cleanupFsRequest(uv_fs_t* request)
    {
        uv_fs_req_cleanup(request);

        delete request;
    }
}

hx::asys::libuv::LibuvAsysContext hx::asys::libuv::context(Context ctx)
{
    auto casted = dynamic_cast<LibuvAsysContext_obj*>(ctx.mPtr);
    if (!casted)
    {
        hx::Throw(HX_CSTRING("Bad Libuv Context"));
    }

    return casted;
}

std::unique_ptr<uv_fs_t, uv_fs_cb> hx::asys::libuv::unique_fs_req(uv_fs_t* request)
{
    return std::unique_ptr<uv_fs_t, uv_fs_cb>(request, cleanupFsRequest);
}

hx::EnumBase hx::asys::libuv::create(const String& name, const int index, const int fields)
{
    auto result = new (fields * sizeof(cpp::Variant)) hx::EnumBase_obj;

    result->_hx_setIdentity(name, index, fields);

    return result;
}

hx::EnumBase hx::asys::libuv::uv_err_to_enum(const int code)
{
    switch (code)
    {
        case UV_ENOENT:
            return create(HX_CSTRING("FileNotFound"), 0, 0);
        case UV_EEXIST:
            return create(HX_CSTRING("FileExists"), 1, 0);
        case UV_ESRCH:
            return create(HX_CSTRING("ProcessNotFound"), 2, 0);
        case UV_EACCES:
            return create(HX_CSTRING("AccessDenied"), 3, 0);
        case UV_ENOTDIR:
            return create(HX_CSTRING("NotDirectory"), 4, 0);
        case UV_EMFILE:
            return create(HX_CSTRING("TooManyOpenFiles"), 5, 0);
        case UV_EPIPE:
            return create(HX_CSTRING("BrokenPipe"), 6, 0);
        case UV_ENOTEMPTY:
            return create(HX_CSTRING("NotEmpty"), 7, 0);
        case UV_EADDRNOTAVAIL:
            return create(HX_CSTRING("AddressNotAvailable"), 8, 0);
        case UV_ECONNRESET:
            return create(HX_CSTRING("ConnectionReset"), 9, 0);
        case UV_ETIMEDOUT:
            return create(HX_CSTRING("TimedOut"), 10, 0);
        case UV_ECONNREFUSED:
            return create(HX_CSTRING("ConnectionRefused"), 11, 0);
        case UV_EBADF:
            return create(HX_CSTRING("BadFile"), 12, 0);
        default:
            return create(HX_CSTRING("CustomError"), 13, 1)->_hx_init(0, String::create(uv_err_name(code)));
    }
}

hx::asys::libuv::BaseRequest::BaseRequest(Dynamic _cbSuccess, Dynamic _cbFailure)
    : cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr) {}

void hx::asys::libuv::basic_callback(uv_fs_t* request)
{
    auto gcZone    = hx::AutoGCZone();
    auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
    auto spRequest = hx::asys::libuv::unique_fs_req(request);

    if (spRequest->result < 0)
    {
        Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
    }
    else
    {
        Dynamic(spData->cbSuccess.rooted)();
    }
}

void hx::asys::libuv::clean_handle(uv_handle_t *handle)
{
    delete handle;
}