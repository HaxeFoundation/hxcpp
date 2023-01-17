#include <hxcpp.h>
#include <array>
#include <memory>
#include "../LibuvUtils.h"
#include "../stream/Streams.h"

namespace
{
    class QueuedRead : public hx::Object
    {
        //
    };

    class LibuvTcpSocket : public hx::asys::net::tcp::Socket_obj
    {
    private:
        cpp::Pointer<uv_stream_t> handle;
        Array<QueuedRead> queue;

    public:
        LibuvTcpSocket(cpp::Pointer<uv_stream_t> _handle)
            : handle(_handle)
            , queue(null()) {}

        void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            //
        }
        void write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            hx::asys::libuv::stream::write(handle, input, offset, length, cbSuccess, cbFailure);
        }
        void flush(Dynamic cbSuccess, Dynamic cbFailure)
        {
            //
        }
        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            //
        }
//         void __Mark(hx::MarkContext *__inCtx)
//         {
//             HX_MARK_MEMBER(queue);
//         }
// #ifdef HXCPP_VISIT_ALLOCS
//         void __Visit(hx::VisitContext *__inCtx)
//         {
//             HX_VISIT_MEMBER(queue);
//         }
// #endif
    };
}

void hx::asys::net::tcp::Socket_obj::connect(Context ctx, const String host, int port, const hx::Anon options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto connect  = std::make_unique<uv_connect_t>();
    auto socket   = std::make_unique<uv_tcp_t>();
    auto result   = 0;
    auto wrapper  = [](uv_connect_t* request, const int status) {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_connect_t>(request);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

        if (status < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(hx::asys::net::tcp::Socket(new LibuvTcpSocket(request->handle)));
        }
    };

    if ((result = uv_tcp_init(libuvCtx->uvLoop, socket.get())) < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    auto address = sockaddr_in();

    if ((result = uv_ip4_addr(host.utf8_str(), port, &address)) < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }

    if ((result = uv_tcp_connect(connect.get(), socket.get(), reinterpret_cast<sockaddr*>(&address), wrapper)) < 0)
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

void hx::asys::net::tcp::Socket_obj::connect(Context ctx, const String path, const hx::Anon options, Dynamic cbSuccess, Dynamic cbFailure)
{
    //
}