#include <hxcpp.h>
#include <array>
#include <memory>
#include "../LibuvUtils.h"
#include "NetUtils.h"

String hx::asys::net::ip::name(const int ip)
{
    auto addr   = hx::asys::libuv::net::sockaddr_from_int(ip);
    auto buffer = std::array<char, UV_IF_NAMESIZE>();
    if (0 > uv_ip4_name(&addr, buffer.data(), buffer.size()))
    {
        return null();
    }
    else
    {
        return String::create(buffer.data());
    }
}

String hx::asys::net::ip::name(const Array<uint8_t> ip)
{
    auto addr   = hx::asys::libuv::net::sockaddr_from_data(ip);
    auto buffer = std::array<char, UV_IF_NAMESIZE>();
    if (0 > uv_ip6_name(&addr, buffer.data(), buffer.size()))
    {
        return null();
    }
    else
    {
        return String::create(buffer.data());
    }
}