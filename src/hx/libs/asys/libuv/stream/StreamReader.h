#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include "../LibuvUtils.h"

HX_DECLARE_CLASS4(hx, asys, libuv, stream, StreamReader)

namespace hx::asys::libuv::stream
{
    class StreamReader_obj : public hx::asys::Readable_obj
    {
    private:
        struct QueuedRead final : BaseRequest
        {
            const hx::RootedObject<Array_obj<uint8_t>> array;
            const int offset;
            const int length;

            QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure);
        };

        struct Ctx final
        {
            uv_stream_t* stream;
            std::deque<QueuedRead> queue;
            std::vector<std::vector<char>> staging;
            std::vector<char> buffer;

            Ctx(uv_stream_t* stream);

            void consume();
            void reject(int code);
        };

    public:
        Ctx* ctx;

        StreamReader_obj(uv_stream_t* _stream);

        void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
        void close(Dynamic cbSuccess, Dynamic cbFailure) override;

        static void onAlloc(uv_handle_t* handle, size_t suggested, uv_buf_t* buffer);
        static void onRead(uv_stream_t* stream, ssize_t len, const uv_buf_t* read);
    };
}