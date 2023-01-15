#pragma once

#include <hxcpp.h>
#include <uv.h>

namespace hx::asys::libuv::net
{
    sockaddr_in sockaddr_from_int(int ip);

    sockaddr_in6 sockaddr_from_data(const Array<uint8_t> ip);
}