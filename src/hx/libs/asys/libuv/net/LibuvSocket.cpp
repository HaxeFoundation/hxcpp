#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "../stream/Streams.h"
#include "LibuvSocket.h"

namespace
{
    struct SocketData
    {
        hx::RootedObject<hx::asys::libuv::net::LibuvSocket> tcp;
        std::vector<std::vector<char>> staging;

        SocketData(hx::asys::libuv::net::LibuvSocket* _tcp)
            : tcp(_tcp)
            , staging(std::vector<std::vector<char>>()) {}
    };

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

hx::asys::libuv::net::QueuedRead::QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure)
    : BaseRequest(_cbSuccess, _cbFailure)
    , array(_array.mPtr)
    , offset(_offset)
    , length(_length)
{
}

hx::asys::libuv::net::LibuvSocket::LibuvSocket(cpp::Pointer<uv_stream_t> _handle)
    : handle(_handle)
    , queue(std::deque<QueuedRead>())
    , buffer(std::make_unique<std::vector<uint8_t>>())
{
    handle->ptr->data = new SocketData(this);

    hx::GCSetFinalizer(this, [](hx::Object* obj) {
        reinterpret_cast<LibuvSocket*>(obj)->~LibuvSocket();
    });
}

hx::asys::libuv::net::LibuvSocket::~LibuvSocket()
{
    delete handle->ptr->data;

    uv_close(reinterpret_cast<uv_handle_t*>(handle->ptr), hx::asys::libuv::clean_handle);
}

bool hx::asys::libuv::net::LibuvSocket::tryConsumeRequest(const QueuedRead& request)
{
    if (request.length > buffer->size())
    {
        return false;
    }

    std::memcpy(request.array.rooted->getBase() + request.offset, buffer->data(), request.length);

    buffer->erase(buffer->begin(), buffer->begin() + request.length);

    Dynamic(request.cbSuccess.rooted)(request.length);

    return true;
}

void hx::asys::libuv::net::LibuvSocket::addToBuffer(const ssize_t len, const uv_buf_t* read)
{
    const auto oldSize = buffer->size();
    const auto newSize = oldSize + len;

    buffer->resize(newSize);
        
    std::memcpy(buffer->data() + oldSize, read->base, len);
}

void hx::asys::libuv::net::LibuvSocket::consume()
{
    auto consumed = 0;

    for (auto i = 0; i < queue.size(); i++)
    {
        if (tryConsumeRequest(queue.at(i)))
        {
            consumed++;
        }
        else
        {
            break;
        }
    }

    while (consumed > 0)
    {
        queue.pop_front();

        consumed--;
    }

    if (queue.empty())
    {
        delete static_cast<SocketData*>(handle->ptr->data);

        uv_read_stop(handle);
    }
}

void hx::asys::libuv::net::LibuvSocket::reject(int error)
{
    // In the case of an error I guess we should error all pending reads?

    for (auto i = 0; i < queue.size(); i++)
    {
        Dynamic(queue.at(i).cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(error));
    }

    while (queue.size() > 0)
    {
        queue.pop_front();
    }

    delete static_cast<SocketData*>(handle->ptr->data);

    uv_read_stop(handle);
}

void hx::asys::libuv::net::LibuvSocket::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    if (queue.empty())
    {
        auto alloc = [](uv_handle_t* handle, size_t suggested, uv_buf_t* buffer) {
            auto data = static_cast<SocketData*>(handle->data);

            data->staging.emplace_back(suggested);

            buffer->base = data->staging.back().data();
            buffer->len  = data->staging.back().size();
        };

        auto read = [](uv_stream_t* stream, ssize_t len, const uv_buf_t* read) {
            auto gcZone = hx::AutoGCZone();
            auto data   = static_cast<SocketData*>(stream->data);

            if (len <= 0)
            {
                data->tcp.rooted->reject(len);
            }
            else
            {
                data->tcp.rooted->addToBuffer(len, read);
                data->tcp.rooted->consume();
            }
        };

        uv_read_start(handle, alloc, read);
    }
        
    queue.emplace_back(output, offset, length, cbSuccess, cbFailure);

    if (tryConsumeRequest(queue.back()))
    {
        queue.pop_back();
    }
}

void hx::asys::libuv::net::LibuvSocket::write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    hx::asys::libuv::stream::write(handle, input, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvSocket::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    //
}

void hx::asys::libuv::net::LibuvSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
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

hx::EnumBase hx::asys::libuv::net::LibuvSocket::socket()
{
    return getName(reinterpret_cast<uv_handle_t*>(handle->ptr), false);
}

hx::EnumBase hx::asys::libuv::net::LibuvSocket::peer()
{
    return getName(reinterpret_cast<uv_handle_t*>(handle->ptr), true);
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