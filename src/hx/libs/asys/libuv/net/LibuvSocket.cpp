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
    struct ConnectRequest : public hx::asys::libuv::BaseRequest
    {
        const hx::RootedObject<hx::Object> options;

        ConnectRequest(Dynamic _options, Dynamic _cbSuccess, Dynamic _cbFailure)
            : BaseRequest(_cbSuccess, _cbFailure)
            , options(_options.mPtr) { }
    };

    void on_connection(uv_connect_t* request, const int status)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_connect_t>(request);
        auto spData    = std::unique_ptr<ConnectRequest>(static_cast<ConnectRequest*>(request->data));

        if (status < 0)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(request->handle), hx::asys::libuv::clean_handle);

            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
        }
        else
        {
            auto options = Dynamic(spData->options.rooted);
            if (null() != options)
            {
                auto sendBufferSize = options->__Field(HX_CSTRING("sendBuffer"), hx::PropertyAccess::paccDynamic);
                if (!sendBufferSize.isNull() && sendBufferSize.isInt())
                {
                    auto size = sendBufferSize.asInt();
                    auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(request->handle), &size);

                    if (result < 0)
                    {
                        uv_close(reinterpret_cast<uv_handle_t*>(request->handle), hx::asys::libuv::clean_handle);

                        Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

                        return;
                    }
                }

                auto recvBufferSize = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
                if (!recvBufferSize.isNull() && recvBufferSize.isInt())
                {
                    auto size = sendBufferSize.asInt();
                    auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(request->handle), &size);

                    if (result < 0)
                    {
                        uv_close(reinterpret_cast<uv_handle_t*>(request->handle), hx::asys::libuv::clean_handle);

                        Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

                        return;
                    }
                }

                if (uv_handle_get_type(reinterpret_cast<uv_handle_t*>(request->handle)) == uv_handle_type::UV_TCP)
                {
                    auto keepAlive = options->__Field(HX_CSTRING("keepAlive"), hx::PropertyAccess::paccDynamic);
                    if (!keepAlive.isNull() && keepAlive.isBool())
                    {
                        auto result = uv_tcp_keepalive(reinterpret_cast<uv_tcp_t*>(request->handle), keepAlive.valBool, 5);

                        if (result < 0)
                        {
                            uv_close(reinterpret_cast<uv_handle_t*>(request->handle), hx::asys::libuv::clean_handle);

                            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

                            return;
                        }
                    }
                }
            }

            Dynamic(spData->cbSuccess.rooted)(hx::asys::net::Socket(new hx::asys::libuv::net::LibuvSocket(request->handle)));
        }
    }

    void connect_impl(hx::asys::Context ctx, const sockaddr* address, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
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
            connect->data = new ConnectRequest(options, cbSuccess, cbFailure);

            connect.release();
            socket.release();
        }
    }
}

hx::asys::libuv::net::LibuvSocket::LibuvSocket(uv_stream_t* const _handle)
    : hx::asys::net::Socket_obj(
        getName(reinterpret_cast<uv_handle_t*>(_handle), false),
        getName(reinterpret_cast<uv_handle_t*>(_handle), true))
    , handle(_handle)
    , reader(new hx::asys::libuv::stream::StreamReader(_handle))
    , writer(new hx::asys::libuv::stream::StreamWriter(_handle))
    , keepAliveEnabled(false)
{
    handle->data = reader.get();

    hx::GCSetFinalizer(this, [](hx::Object* obj) {
        reinterpret_cast<LibuvSocket*>(obj)->~LibuvSocket();
    });
}

hx::asys::libuv::net::LibuvSocket::~LibuvSocket()
{
    uv_close(reinterpret_cast<uv_handle_t*>(handle), hx::asys::libuv::clean_handle);
}

void hx::asys::libuv::net::LibuvSocket::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    reader->read(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvSocket::write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    writer->write(input, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvSocket::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    writer->flush(cbSuccess, cbFailure);
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

void hx::asys::libuv::net::LibuvSocket::setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto result = uv_tcp_keepalive(reinterpret_cast<uv_tcp_t*>(handle), keepAlive, 5);
    if (result < 0)
    {
        cbFailure(uv_err_to_enum(result));
    }
    else
    {
        keepAliveEnabled = keepAlive;

        cbSuccess();
    }
}

void hx::asys::libuv::net::LibuvSocket::setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(handle), &size);
    if (result < 0)
    {
        cbFailure(uv_err_to_enum(result));
    }
    else
    {
        cbSuccess();
    }
}

void hx::asys::libuv::net::LibuvSocket::setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(handle), &size);
    if (result < 0)
    {
        cbFailure(uv_err_to_enum(result));
    }
    else
    {
        cbSuccess();
    }
}

void hx::asys::libuv::net::LibuvSocket::getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure)
{
    cbSuccess(keepAliveEnabled);
}

void hx::asys::libuv::net::LibuvSocket::getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto size   = 0;
    auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(handle), &size);
    if (result < 0)
    {
        cbFailure(uv_err_to_enum(result));
    }
    else
    {
        cbSuccess(size);
    }
}

void hx::asys::libuv::net::LibuvSocket::getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto size   = 0;
    auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(handle), &size);
    if (result < 0)
    {
        cbFailure(uv_err_to_enum(result));
    }
    else
    {
        cbSuccess(size);
    }
}
//
//void hx::asys::net::Socket_obj::connect_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
//{
//    auto address = sockaddr_in();
//    auto result  = uv_ip4_addr(host.utf8_str(), port, &address); 
//
//    if (result < 0)
//    {
//        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
//
//        return;
//    }
//
//    connect_impl(ctx, reinterpret_cast<sockaddr*>(&address), port, options, cbSuccess, cbFailure);
//}
//
//void hx::asys::net::Socket_obj::connect_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
//{
//    auto address = sockaddr_in6();
//    auto result  = uv_ip6_addr(host.utf8_str(), port, &address); 
//
//    if (result < 0)
//    {
//        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
//
//        return;
//    }
//
//    connect_impl(ctx, reinterpret_cast<sockaddr*>(&address), port, options, cbSuccess, cbFailure);
//}
//
//void hx::asys::net::Socket_obj::connect_ipc(Context ctx, const String path, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
//{
//    auto libuvCtx = hx::asys::libuv::context(ctx);
//    auto connect  = std::make_unique<uv_connect_t>();
//    auto pipe     = std::make_unique<uv_pipe_t>();
//    auto result   = 0;
//
//    if ((result = uv_pipe_init(libuvCtx->uvLoop, pipe.get(), false)) < 0)
//    {
//        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
//
//        return;
//    }
//    else
//    {
//        connect->data = new ConnectRequest(options, cbSuccess, cbFailure);
//
//        connect.release();
//        pipe.release();
//
//        uv_pipe_connect(connect.get(), pipe.get(), path.utf8_str(), &on_connection);
//    }
//}