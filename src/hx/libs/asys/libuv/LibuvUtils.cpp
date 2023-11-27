#include <hxcpp.h>
#include <array>
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
        case UV_EISDIR:
            return create(HX_CSTRING("IsDirectory"), 13, 0);
        default:
            return create(HX_CSTRING("CustomError"), 14, 1)->_hx_init(0, String::create(uv_err_name(code)));
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

hx::EnumBase hx::asys::libuv::getName(uv_handle_t* handle, bool remote)
{
    switch (uv_handle_get_type(handle))
    {
        case uv_handle_type::UV_TCP:
        {
            auto storage = sockaddr_storage();
            auto length = int(sizeof(sockaddr_storage));
            auto result =
                remote
                    ? uv_tcp_getpeername(reinterpret_cast<uv_tcp_t*>(handle), reinterpret_cast<sockaddr*>(&storage), &length)
                    : uv_tcp_getsockname(reinterpret_cast<uv_tcp_t*>(handle), reinterpret_cast<sockaddr*>(&storage), &length);

            if (result < 0)
            {
                return null();
            }
            else
            {
                auto name = std::array<char, UV_IF_NAMESIZE>();
                auto port = reinterpret_cast<sockaddr_in*>(&storage)->sin_port;

                if ((result = uv_ip_name(reinterpret_cast<sockaddr*>(&storage), name.data(), name.size())) < 0)
                {
                    return null();
                }

                return
                    hx::asys::libuv::create(HX_CSTRING("INET"), 0, 2)
                        ->_hx_init(0, String::create(name.data()))
                        ->_hx_init(1, int(port));
            }
        }
        case uv_handle_type::UV_NAMED_PIPE:
        {
            auto name = std::array<char, 1024>();
            auto size = name.size();
            auto result =
                remote
                ? uv_pipe_getpeername(reinterpret_cast<uv_pipe_t*>(handle), name.data(), &size)
                : uv_pipe_getsockname(reinterpret_cast<uv_pipe_t*>(handle), name.data(), &size);

            return
                (result < 0)
                ? null()
                : hx::asys::libuv::create(HX_CSTRING("PIPE"), 1, 1)->_hx_init(0, String::create(name.data(), size));
        }
        default:
            return null();
    }
}

#if (!HX_WINDOWS)
int toPosixCode(const int hxCode)
{
    switch (hxCode)
    {
    case 0:
        return SIGHUP;
    case 1:
        return SIGINT;
    case 2:
        return SIGQUIT;
    case 3:
        return SIGILL;
    case 4:
        return SIGABRT;
    case 5:
        return SIGFPE;
    case 6:
        return SIGKILL;
    case 7:
        return SIGSEGV;
    case 8:
        return SIGPIPE;
    case 9:
        return SIGALRM;
    case 10:
        return SIGTERM;
    case 11:
        return SIGUSR1;
    case 12:
        return SIGUSR2;
    case 13:
        return SIGCHLD;
    case 14:
        return SIGCONT;
    case 15:
        return SIGSTOP;
    case 16:
        return SIGTSTP;
    case 17:
        return SIGTTIN;
    case 18:
        return SIGTTOU;
    case 19:
        return SIGBUS;
    case 20:
        return SIGPOLL;
    case 21:
        return SIGPROF;
    case 22:
        return SIGSYS;
    case 23:
        return SIGTRAP;
    case 24:
        return SIGURG;
    case 25:
        return SIGVTALRM;
    case 26:
        return SIGXCPU;
    case 27:
        return SIGXFSZ;
    }
}
#endif