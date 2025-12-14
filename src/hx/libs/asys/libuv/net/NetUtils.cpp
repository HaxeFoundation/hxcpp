#include <hxcpp.h>
#include <memory>
#include <cstring>
#include <array>
#include "NetUtils.h"
#include "../LibuvUtils.h"

namespace
{
	hx::Anon make_address_anon(sockaddr_storage& storage)
	{
		auto name   = std::array<char, UV_IF_NAMESIZE>();
		auto port   = int(reinterpret_cast<sockaddr_in*>(&storage)->sin_port);
		auto result = 0;

		if ((result = uv_ip_name(reinterpret_cast<sockaddr*>(&storage), name.data(), name.size())) < 0)
		{
			return null();
		}

		return
			hx::Anon_obj::Create(2)
				->setFixed(0, HX_CSTRING("host"), String::create(name.data()))
				->setFixed(1, HX_CSTRING("port"), port);
	}
}

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

    std::memcpy(&ip, &addr->sin_addr, sizeof(in_addr));

    return hx::asys::libuv::create(HX_CSTRING("INET"), 0, 1)->_hx_init(0, ip);
}

hx::EnumBase hx::asys::libuv::net::ip_from_sockaddr(sockaddr_in6* addr)
{
    auto bytes = new Array_obj<uint8_t>(0, 0);

    bytes->memcpy(0, reinterpret_cast<uint8_t*>(&addr->sin6_addr), sizeof(in6_addr));

    return hx::asys::libuv::create(HX_CSTRING("INET6"), 1, 1)->_hx_init(0, bytes);
}

hx::Anon hx::asys::libuv::net::getLocalAddress(uv_tcp_t* tcp)
{
	auto storage = sockaddr_storage();
	auto length = int(sizeof(sockaddr_storage));
	auto result = uv_tcp_getsockname(tcp, reinterpret_cast<sockaddr*>(&storage), &length);

	if (result < 0)
	{
		return null();
	}
	else
	{
		return make_address_anon(storage);
	}
}

hx::Anon hx::asys::libuv::net::getRemoteAddress(uv_tcp_t* tcp)
{
	auto storage = sockaddr_storage();
	auto length = int(sizeof(sockaddr_storage));
	auto result = uv_tcp_getpeername(tcp, reinterpret_cast<sockaddr*>(&storage), &length);

	if (result < 0)
	{
		return null();
	}
	else
	{
		return make_address_anon(storage);
	}
}