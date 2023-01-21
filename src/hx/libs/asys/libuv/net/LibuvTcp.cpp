#include <hxcpp.h>
#include <vector>
#include <deque>
#include <memory>
#include "../LibuvUtils.h"
#include "../stream/Streams.h"

namespace
{
    class LibuvTcpSocket;

    struct QueuedRead : hx::asys::libuv::BaseRequest
    {
        const hx::RootedObject<Array_obj<uint8_t>> array;
        const int offset;
        const int length;

        QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure)
            : BaseRequest(_cbSuccess, _cbFailure)
            , array(_array.mPtr)
            , offset(_offset)
            , length(_length)
        {}
    };

    struct TcpStream
    {
        hx::RootedObject<LibuvTcpSocket> tcp;
        std::vector<std::vector<char>> staging;

        TcpStream(LibuvTcpSocket* _tcp)
            : tcp(_tcp)
            , staging(std::vector<std::vector<char>>()) {}
    };

    class LibuvTcpSocket : public hx::asys::net::tcp::Socket_obj
    {
    private:
        cpp::Pointer<uv_stream_t> handle;
        std::deque<QueuedRead> queue;
        std::unique_ptr<std::vector<uint8_t>> buffer;

        bool tryConsumeRequest(const QueuedRead& request)
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

    public:
        LibuvTcpSocket(cpp::Pointer<uv_stream_t> _handle)
            : handle(_handle)
            , queue(std::deque<QueuedRead>())
            , buffer(std::make_unique<std::vector<uint8_t>>())
        {
            handle->ptr->data = new TcpStream(this);
        }

        void addToBuffer(const ssize_t len, const uv_buf_t* read)
        {
            const auto oldSize = buffer->size();
            const auto newSize = oldSize + len;

            buffer->resize(newSize);
            
            std::memcpy(buffer->data() + oldSize, read->base, len);
        }

        void consume()
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
                delete static_cast<TcpStream*>(handle->ptr->data);

                uv_read_stop(handle);
            }
        }

        void reject(int error)
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

            delete static_cast<TcpStream*>(handle->ptr->data);

            uv_read_stop(handle);
        }

        void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            if (queue.empty())
            {
                auto alloc = [](uv_handle_t* handle, size_t suggested, uv_buf_t* buffer) {
                    auto data = static_cast<TcpStream*>(handle->data);

                    data->staging.emplace_back(suggested);

                    buffer->base = data->staging.back().data();
                    buffer->len  = data->staging.back().size();
                };

                auto read = [](uv_stream_t* stream, ssize_t len, const uv_buf_t* read) {
                    auto gcZone = hx::AutoGCZone();
                    auto data   = static_cast<TcpStream*>(stream->data);

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
    };
}

void hx::asys::net::tcp::Socket_obj::connect(Context ctx, const String host, int port, Dynamic cbSuccess, Dynamic cbFailure)
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
