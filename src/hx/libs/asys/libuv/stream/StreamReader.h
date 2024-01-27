#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::stream
{
    class StreamReader
    {
    private:
        struct QueuedRead : BaseRequest
        {
            const hx::RootedObject<Array_obj<uint8_t>> array;
            const int offset;
            const int length;

            QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure);
        };

        uv_stream_t* stream;
        std::deque<QueuedRead> queue;

    public:
        std::array<char, 8192> staging;
        std::vector<uint8_t> buffer;

        StreamReader(uv_stream_t* _stream);
        virtual ~StreamReader() = default;

        void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);
        void consume();
        void reject(int code);
    };
}