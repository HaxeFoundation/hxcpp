#include <hxcpp.h>
#include "NetUtils.h"
#include <memory>

namespace hx::asys::libuv::net
{
    sockaddr_in sockaddr_from_int(const int ip)
    {
        auto addr = sockaddr_in();
        addr.sin_family = AF_INET;

        std::memcpy(&addr.sin_addr, &ip, sizeof(int));

        return addr;
    }

    sockaddr_in6 sockaddr_from_data(const Array<uint8_t> ip)
    {
        auto addr = sockaddr_in6();
        addr.sin6_family = AF_INET6;

        std::memcpy(&addr.sin6_addr, ip->getBase(), ip->size());

        return addr;
    }
}
