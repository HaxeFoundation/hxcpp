#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"
#include "LibuvSocket.h"

namespace
{
    hx::EnumBase getName(uv_handle_t* handle, bool remote)
    {
        switch (uv_handle_get_type(handle))
        {
            case uv_handle_type::UV_TCP:
                {
                    auto storage = sockaddr_storage();
                    auto length  = int(sizeof(sockaddr_storage));
                    auto result  =
                        remote
                            ? uv_tcp_getpeername(reinterpret_cast<uv_tcp_t*>(handle), reinterpret_cast<sockaddr*>(&storage), &length)
                            : uv_tcp_getsockname(reinterpret_cast<uv_tcp_t*>(handle), reinterpret_cast<sockaddr*>(&storage), &length);

                    if (result < 0)
                    {
                        return null();
                    }
                    else
                    {
                        auto name = std::array<char, 1024>();

                        return
                            ((result = uv_ip_name(reinterpret_cast<sockaddr*>(&storage), name.data(), name.size())) < 0)
                                ? null()
                                : hx::asys::libuv::create(HX_CSTRING("INET"), 0, 2)
                                    ->_hx_init(0, String::create(name.data()))
                                    ->_hx_init(1, int(reinterpret_cast<sockaddr_in*>(&storage)->sin_port));
                    }
                }
            case uv_handle_type::UV_NAMED_PIPE:
                {
                    auto name   = std::array<char, 1024>();
                    auto size   = name.size();
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

    void on_connection(uv_connect_t* request, const int status)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_connect_t>(request);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

        if (status < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(hx::asys::net::Socket(new hx::asys::libuv::net::LibuvSocket(request->handle)));
        }
    }

    void connect_impl(hx::asys::Context ctx, const sockaddr* address, int port, Dynamic cbSuccess, Dynamic cbFailure)
    {
        auto libuvCtx = hx::asys::libuv::context(ctx);
        auto connect  = std::make_unique<uv_connect_t>();
        auto socket   = std::make_unique<uv_tcp_t>();
        auto result   = 0;

        if ((result = uv_tcp_init(libuvCtx->uvLoop, socket.get())) < 0)
        {
            cbFailure(hx::asys::libuv::uv_err_to_enum(result));

            return;
        }

        if ((result = uv_tcp_connect(connect.get(), socket.get(), address, &on_connection)) < 0)
        {
            cbFailure(hx::asys::libuv::uv_err_to_enum(result));
        }
        else
        {
            connect->data = new hx::asys::libuv::BaseRequest(cbSuccess, cbFailure);

            connect.release();
            socket.release();
        }
    }
}

hx::asys::libuv::net::LibuvSocket::LibuvSocket(cpp::Pointer<uv_stream_t> _handle)
    : hx::asys::net::Socket_obj(
        getName(reinterpret_cast<uv_handle_t*>(_handle->ptr), false),
        getName(reinterpret_cast<uv_handle_t*>(_handle->ptr), true))
    , handle(_handle)
    , reader(new hx::asys::libuv::stream::StreamReader(_handle->ptr))
    , writer(new hx::asys::libuv::stream::StreamWriter(_handle->ptr))
{
    handle->ptr->data = reader->ptr;

    hx::GCSetFinalizer(this, [](hx::Object* obj) {
        reinterpret_cast<LibuvSocket*>(obj)->~LibuvSocket();
    });
}

hx::asys::libuv::net::LibuvSocket::~LibuvSocket()
{
    reader->destroy();

    uv_close(reinterpret_cast<uv_handle_t*>(handle->ptr), hx::asys::libuv::clean_handle);
}

void hx::asys::libuv::net::LibuvSocket::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    reader->ptr->read(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvSocket::write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    writer->ptr->write(input, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvSocket::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    //
}

void hx::asys::libuv::net::LibuvSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
    uv_read_stop(handle);

    auto request = std::make_unique<uv_shutdown_t>();
    auto wrapper = [](uv_shutdown_t* request, int status) {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_shutdown_t>(request);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

        if (status < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)();
        }
    };

    auto result = uv_shutdown(request.get(), handle, wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request->data = new hx::asys::libuv::BaseRequest(cbSuccess, cbFailure);
        request.release();
    }
}

void hx::asys::net::Socket_obj::connect_ipv4(Context ctx, const String host, int port, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto address = sockaddr_in();
    auto result  = uv_ip4_addr(host.utf8_str(), port, &address); 

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    connect_impl(ctx, reinterpret_cast<sockaddr*>(&address), port, cbSuccess, cbFailure);
}

void hx::asys::net::Socket_obj::connect_ipv6(Context ctx, const String host, int port, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto address = sockaddr_in6();
    auto result  = uv_ip6_addr(host.utf8_str(), port, &address); 

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    connect_impl(ctx, reinterpret_cast<sockaddr*>(&address), port, cbSuccess, cbFailure);
}

void hx::asys::net::Socket_obj::connect_ipc(Context ctx, const String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto connect  = std::make_unique<uv_connect_t>();
    auto pipe     = std::make_unique<uv_pipe_t>();
    auto result   = 0;

    if ((result = uv_pipe_init(libuvCtx->uvLoop, pipe.get(), false)) < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }
    else
    {
        connect->data = new hx::asys::libuv::BaseRequest(cbSuccess, cbFailure);

        connect.release();
        pipe.release();

        uv_pipe_connect(connect.get(), pipe.get(), path.utf8_str(), &on_connection);
    }
}