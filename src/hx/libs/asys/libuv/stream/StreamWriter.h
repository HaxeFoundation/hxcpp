#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::stream
{
    class StreamWriter final
    {
    private:
        uv_stream_t* const stream;
    public:
        StreamWriter(uv_stream_t* _stream);

        void write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);
    };
}