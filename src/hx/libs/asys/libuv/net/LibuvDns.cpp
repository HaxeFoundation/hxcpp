#include <hxcpp.h>
#include <array>
#include <memory>
#include "../LibuvUtils.h"

namespace
{
    class AddrInfoCleaner
    {
    public:
        addrinfo* info;

        AddrInfoCleaner(addrinfo* _info) : info(_info) {}
        ~AddrInfoCleaner()
        {
            uv_freeaddrinfo(info);
        }
    };
}

void hx::asys::net::resolve(Context ctx, String host, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto data     = std::make_unique<hx::asys::libuv::BaseRequest>(cbSuccess, cbFailure);
    auto request  = std::make_unique<uv_getaddrinfo_t>();
    auto wrapper  = [](uv_getaddrinfo_t* request, int status, addrinfo* addr) {
        auto gcZone      = hx::AutoGCZone();
        auto spRequest   = std::unique_ptr<uv_getaddrinfo_t>(request);
        auto spData      = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto addrCleaner = AddrInfoCleaner(addr);

        if (status < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
        }
        else
        {
            auto ips  = new Array_obj<hx::EnumBase>(0, 0);
            auto info = addr;

            do
            {
                switch (info->ai_addr->sa_family)
                {
                    case AF_INET: {
                        auto data = reinterpret_cast<sockaddr_in*>(info->ai_addr);
                        auto ip   = 0;

                        std::memcpy(&ip, &data->sin_addr, sizeof(IN_ADDR));

                        ips->Add(hx::asys::libuv::create(HX_CSTRING("INET"), 0, 1)->_hx_init(0, ip));
                        break;
                    }

                    case AF_INET6: {
                        auto data  = reinterpret_cast<sockaddr_in6*>(info->ai_addr);
                        auto bytes = new Array_obj<uint8_t>(0, 0);

                        bytes->memcpy(0, reinterpret_cast<uint8_t*>(&data->sin6_addr), sizeof(IN6_ADDR));

                        ips->Add(hx::asys::libuv::create(HX_CSTRING("INET6"), 1, 1)->_hx_init(0, bytes));
                        break;
                    }

                    // TODO : What should we do if its another type?
                }

                info = info->ai_next;
            } while (nullptr != info);

            Dynamic(spData->cbSuccess.rooted)(ips);
        }
    };

    struct addrinfo hints;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    auto result = uv_getaddrinfo(libuvCtx->uvLoop, request.get(), wrapper, host.utf8_str(), nullptr, &hints);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request->data = data.get();

        request.release();
        data.release();
    }
}

// void hx::asys::net::reverse(Context ctx, int ip, Dynamic cbSuccess, Dynamic cbFailure)
// {
//     //
// }

// void hx::asys::net::reverse(Context ctx, Array<uint8_t> ip, Dynamic cbSuccess, Dynamic cbFailure)
// {
//     //
// }

String hx::asys::net::ipName(const int ip)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

    std::memcpy(&addr.sin_addr, &ip, sizeof(int));

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

String hx::asys::net::ipName(const Array<uint8_t> ip)
{
    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;

    std::memcpy(&addr.sin6_addr, ip->getBase(), ip->size());

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