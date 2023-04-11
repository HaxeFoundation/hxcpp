#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::stream
{
    struct QueuedWrite final : public BaseRequest
    {
        const hx::RootedObject<Array_obj<uint8_t>> array;
        const int offset;
        const int length;
        int progress;
        std::vector<char> staging;

        QueuedWrite(Array<uint8_t> _array, int _offset, int _length, Dynamic _cbSuccess, Dynamic _cbFailure);
    };

    class StreamWriter final
    {
    public:
        const int CHUNK_SIZE = static_cast<int>(std::numeric_limits<uint16_t>::max());

        uv_stream_t* const stream;
        std::deque<std::unique_ptr<QueuedWrite>> queue;

        StreamWriter(uv_stream_t* _stream);

        void write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);

        void flush(Dynamic cbSuccess, Dynamic cbFailure);

        std::unique_ptr<QueuedWrite> popFront();

        void consume();
    };
}