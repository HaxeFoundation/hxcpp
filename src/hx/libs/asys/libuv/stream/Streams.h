#pragma once

#include <hxcpp.h>
#include <uv.h>

namespace hx::asys::libuv::stream
{
    void write(uv_stream_t* handle, Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);
}