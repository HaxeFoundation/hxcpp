#include <hxcpp.h>
#include "NetUtils.h"
#include "../LibuvUtils.h"
#include <memory>

sockaddr_in hx::asys::libuv::net::sockaddr_from_int(const Ipv4Address ip, const int port)
{
    auto addr = sockaddr_in();
    addr.sin_family = AF_INET;
    addr.sin_port   = port;

    std::memcpy(&addr.sin_addr, &ip, sizeof(int));

    return addr;
}

sockaddr_in6 hx::asys::libuv::net::sockaddr_from_data(const Ipv6Address ip, const int port)
{
    auto addr = sockaddr_in6();
    addr.sin6_family = AF_INET6;
    addr.sin6_port   = port;

    std::memcpy(&addr.sin6_addr, ip->getBase(), ip->size());

    return addr;
}

hx::EnumBase hx::asys::libuv::net::ip_from_sockaddr(sockaddr_in* addr)
{
    auto ip = 0;

    std::memcpy(&ip, &addr->sin_addr, sizeof(IN_ADDR));

    return hx::asys::libuv::create(HX_CSTRING("INET"), 0, 1)->_hx_init(0, ip);
}

hx::EnumBase hx::asys::libuv::net::ip_from_sockaddr(sockaddr_in6* addr)
{
    auto bytes = new Array_obj<uint8_t>(0, 0);

    bytes->memcpy(0, reinterpret_cast<uint8_t*>(&addr->sin6_addr), sizeof(IN6_ADDR));

    return hx::asys::libuv::create(HX_CSTRING("INET6"), 1, 1)->_hx_init(0, bytes);
}
