#pragma once

#include <hxcpp.h>
#include <uv.h>

namespace hx::asys::libuv::net
{
    sockaddr_in sockaddr_from_int(const Ipv4Address ip, int port = 0);

    sockaddr_in6 sockaddr_from_data(const Ipv6Address ip, int port = 0);
    
    ::hx::EnumBase ip_from_sockaddr(sockaddr_in* addr);

    ::hx::EnumBase ip_from_sockaddr(sockaddr_in6* addr);

    ::hx::Anon getLocalAddress(uv_tcp_t* tcp);

    ::hx::Anon getRemoteAddress(uv_tcp_t* tcp);
}