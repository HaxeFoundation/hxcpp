#pragma once

#include <hxcpp.h>
#include <memory>
#include <deque>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::net
{
    struct QueuedRead : hx::asys::libuv::BaseRequest
    {
        const hx::RootedObject<Array_obj<uint8_t>> array;
        const int offset;
        const int length;

        QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure);
    };

    class LibuvSocket final : public hx::asys::net::Socket_obj
    {
    private:
        cpp::Pointer<uv_stream_t> handle;
        std::deque<QueuedRead> queue;
        std::unique_ptr<std::vector<uint8_t>> buffer;

        bool tryConsumeRequest(const QueuedRead& request);

    public:
        LibuvSocket(cpp::Pointer<uv_stream_t> _handle);

        ~LibuvSocket();

        void addToBuffer(const ssize_t len, const uv_buf_t* read);

        void consume();

        void reject(int error);

        void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);
        void write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);
        void flush(Dynamic cbSuccess, Dynamic cbFailure);
        void close(Dynamic cbSuccess, Dynamic cbFailure);

        hx::EnumBase socket();
        hx::EnumBase peer();
    };
}