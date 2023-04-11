#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "LibuvSocket.h"

namespace
{
    /// <summary>
    /// Every time the user calls `Accept` on the server the callback is rooted and stored in this queue.
    /// Whenever libuv notifies us of a incoming connection the front of the queue will be popped and used.
    /// </summary>
    class ConnectionQueue
    {
    private:
        std::deque<std::unique_ptr<hx::asys::libuv::BaseRequest>> queue;

    public:
        ConnectionQueue() : queue(std::deque<std::unique_ptr<hx::asys::libuv::BaseRequest>>(0)) { }

        void Clear()
        {
            queue.clear();
        }

        void Enqueue(Dynamic cbSuccess, Dynamic cbFailure)
        {
            queue.push_back(std::make_unique<hx::asys::libuv::BaseRequest>(cbSuccess, cbFailure));
        }

        std::unique_ptr<hx::asys::libuv::BaseRequest> tryDequeue()
        {
            if (queue.empty())
            {
                return nullptr;
            }

            auto root = std::unique_ptr<hx::asys::libuv::BaseRequest>{ std::move(queue.front()) };

            queue.pop_front();

            return root;
        }
    };

    class LibuvServer final : public hx::asys::net::Server_obj
    {
    private:
        uv_stream_t* const stream;
        const std::unique_ptr<ConnectionQueue> queue;

    public:
        LibuvServer(uv_stream_t* _stream)
            : hx::asys::net::Server_obj(hx::asys::libuv::getName(reinterpret_cast<uv_handle_t*>(_stream), false))
            , stream(_stream)
            , queue(new ConnectionQueue())
        {
            hx::GCSetFinalizer(this, [](hx::Object* obj) {
                reinterpret_cast<LibuvServer*>(obj)->~LibuvServer();
            });

            stream->data = queue.get();
        }

        ~LibuvServer()
        {
            uv_close(reinterpret_cast<uv_handle_t*>(stream), hx::asys::libuv::clean_handle);
        }

        void accept(Dynamic cbSuccess, Dynamic cbFailure)
        {
            queue->Enqueue(cbSuccess, cbFailure);
        }

        void close(Dynamic cbSuccess, Dynamic cbFailure)
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
                    static_cast<ConnectionQueue*>(request->handle->data)->Clear();

                    Dynamic(spData->cbSuccess.rooted)();
                }
            };

            auto result = uv_shutdown(request.get(), stream, wrapper);

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
    };

    void onTcpConnection(uv_stream_t* server, int result)
    {
        auto queue   = static_cast<ConnectionQueue*>(server->data);
        auto request = queue->tryDequeue();
        auto gcZone  = hx::AutoGCZone();

        if (nullptr != request)
        {
            if (result < 0)
            {
                Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

                return;
            }
            
            auto client = std::make_unique<uv_tcp_t>();
            auto result = 0;

            if ((result = uv_tcp_init(server->loop, client.get())) < 0)
            {
                Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

                return;
            }

            if ((result = uv_accept(server, reinterpret_cast<uv_stream_t*>(client.get()))) != 0)
            {
                uv_close(reinterpret_cast<uv_handle_t*>(client.release()), hx::asys::libuv::clean_handle);

                Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

                return;
            }

            Dynamic(request->cbSuccess.rooted)(new hx::asys::libuv::net::LibuvSocket(reinterpret_cast<uv_stream_t*>(client.release())));
        }
    }

    void open_tcp_impl(hx::asys::libuv::LibuvAsysContext ctx, sockaddr* const address, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
    {
        auto server = std::make_unique<uv_tcp_t>();
        auto result = 0;

        if ((result = uv_tcp_init(ctx->uvLoop, server.get())) < 0)
        {
            cbFailure(hx::asys::libuv::uv_err_to_enum(result));

            return;
        }

        if ((result = uv_tcp_bind(server.get(), address, 0)) < 0)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(server.release()), hx::asys::libuv::clean_handle);

            cbFailure(hx::asys::libuv::uv_err_to_enum(result));

            return;
        }

        auto backlog = SOMAXCONN;

        if (null() != options)
        {
            auto sendBufferSize = options->__Field(HX_CSTRING("sendBuffer"), hx::PropertyAccess::paccDynamic);
            if (!sendBufferSize.isNull() && sendBufferSize.isInt())
            {
                auto size = sendBufferSize.asInt();
                auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(server.get()), &size);

                if (result < 0)
                {
                    uv_close(reinterpret_cast<uv_handle_t*>(server.get()), hx::asys::libuv::clean_handle);

                    cbFailure(hx::asys::libuv::uv_err_to_enum(result));

                    return;
                }
            }

            auto recvBufferSize = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
            if (!recvBufferSize.isNull() && recvBufferSize.isInt())
            {
                auto size = sendBufferSize.asInt();
                auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(server.get()), &size);

                if (result < 0)
                {
                    uv_close(reinterpret_cast<uv_handle_t*>(server.get()), hx::asys::libuv::clean_handle);

                    cbFailure(hx::asys::libuv::uv_err_to_enum(result));

                    return;
                }
            }

            auto backlogSize = options->__Field(HX_CSTRING("backlog"), hx::PropertyAccess::paccDynamic);
            if (!backlogSize.isNull() && backlogSize.isInt())
            {
                backlog = backlogSize.asInt();
            }
        }

        if ((result = uv_listen(reinterpret_cast<uv_stream_t*>(server.get()), backlog, onTcpConnection)) < 0)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(server.release()), hx::asys::libuv::clean_handle);

            cbFailure(hx::asys::libuv::uv_err_to_enum(result));

            return;
        }

        cbSuccess(new LibuvServer(reinterpret_cast<uv_stream_t*>(server.release())));
    }
}

void hx::asys::net::Server_obj::open_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto address = sockaddr_in();
    auto result  = uv_ip4_addr(host.utf8_str(), port, &address);
    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    open_tcp_impl(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
}

void hx::asys::net::Server_obj::open_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto address = sockaddr_in6();
    auto result  = uv_ip6_addr(host.utf8_str(), port, &address);
    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    open_tcp_impl(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
}

void hx::asys::net::Server_obj::open_ipc(Context ctx, const String path, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto pipe     = std::make_unique<uv_pipe_t>();
    auto result   = 0;

    if ((result = uv_pipe_init(libuvCtx->uvLoop, pipe.get(), false)) < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    if ((result = uv_pipe_bind(pipe.get(), path.utf8_str())) < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    if (null() != options)
    {
        auto sendBufferSize = options->__Field(HX_CSTRING("sendBuffer"), hx::PropertyAccess::paccDynamic);
        if (!sendBufferSize.isNull() && sendBufferSize.isInt())
        {
            auto size = sendBufferSize.asInt();
            auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(pipe.get()), &size);

            if (result < 0)
            {
                uv_close(reinterpret_cast<uv_handle_t*>(pipe.get()), hx::asys::libuv::clean_handle);

                cbFailure(hx::asys::libuv::uv_err_to_enum(result));

                return;
            }
        }

        auto recvBufferSize = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
        if (!recvBufferSize.isNull() && recvBufferSize.isInt())
        {
            auto size = sendBufferSize.asInt();
            auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(pipe.get()), &size);

            if (result < 0)
            {
                uv_close(reinterpret_cast<uv_handle_t*>(pipe.get()), hx::asys::libuv::clean_handle);

                cbFailure(hx::asys::libuv::uv_err_to_enum(result));

                return;
            }
        }
    }

    cbSuccess(new LibuvServer(reinterpret_cast<uv_stream_t*>(pipe.release())));
}