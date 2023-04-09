#pragma once

#include <hxcpp.h>
#include <uv.h>

namespace hx::asys::libuv::stream
{
    class StreamReader final : virtual public hx::asys::Readable
    {
        void read(::cpp::Int64 pos, Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            //
        }

        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            //
        }
    };

    class StreamWriter final : virtual public hx::asys::Writable
    {
        //
    };

    void write(uv_stream_t* handle, Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);
}