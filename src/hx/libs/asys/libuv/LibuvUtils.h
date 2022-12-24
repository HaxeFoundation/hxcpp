#pragma once

#include <hxcpp.h>
#include <uv.h>

namespace hx::asys::libuv
{
    hx::EnumBase create(const String& name, const int index, int fields)
    {
        auto result = new (fields * sizeof(cpp::Variant)) hx::EnumBase_obj;

        result->_hx_setIdentity(name, index, fields);

        return result;
    }

    hx::EnumBase uv_err_to_enum(int code)
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

    struct BaseRequest
    {
        const hx::RootedObject cbSuccess;
        const hx::RootedObject cbFailure;

        BaseRequest(Dynamic _cbSuccess, Dynamic _cbFailure) : cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr) {}
    };
}