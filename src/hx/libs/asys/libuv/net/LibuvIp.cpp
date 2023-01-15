#include <hxcpp.h>
#include <array>
#include <memory>
#include "../LibuvUtils.h"
#include "NetUtils.h"

String hx::asys::net::ip::name(const int ip)
{
    auto addr   = hx::asys::libuv::net::sockaddr_from_int(ip);
    auto buffer = std::array<char, UV_IF_NAMESIZE>();
    if (uv_ip4_name(&addr, buffer.data(), buffer.size()) >= 0)
    {
        return String::create(buffer.data());
    }
    else
    {
        return null();
    }
}

String hx::asys::net::ip::name(const Array<uint8_t> ip)
{
    auto addr   = hx::asys::libuv::net::sockaddr_from_data(ip);
    auto buffer = std::array<char, UV_IF_NAMESIZE>();
    if (uv_ip6_name(&addr, buffer.data(), buffer.size()) >= 0)
    {
        return String::create(buffer.data());
    }
    else
    {
        return null();
    }
}

hx::EnumBase hx::asys::net::ip::parse(const String ip)
{
    auto str = ip.utf8_str();

    {
        auto addr   = sockaddr_in();
        if (uv_ip4_addr(str, 0, &addr) >= 0)
        {
            return hx::asys::libuv::net::ip_from_sockaddr(&addr);
        }
    }

    {
        auto addr = sockaddr_in6();
        if (uv_ip6_addr(str, 0, &addr) >= 0)
        {
            return hx::asys::libuv::net::ip_from_sockaddr(&addr);
        }
    }

    return null();
}