#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include <limits>
#include "../LibuvUtils.h"

HX_DECLARE_CLASS4(hx, asys, libuv, stream, StreamWriter)

namespace hx::asys::libuv::stream
{
    class StreamWriter_obj : public hx::asys::Writable_obj
    {
        uv_stream_t* stream;

    public:
        StreamWriter_obj(uv_stream_t* stream);

        void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
        void flush(Dynamic cbSuccess, Dynamic cbFailure) override;
        void close(Dynamic cbSuccess, Dynamic cbFailure) override;
    };
}