#include <hxcpp.h>
#include <array>
#include <memory>
#include "../LibuvUtils.h"
#include "NetUtils.h"

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

    class ResolveRequest : public hx::asys::libuv::BaseRequest
    {
        hx::strbuf buffer;

    public:
        static void callback(uv_getaddrinfo_t* request, int status, addrinfo* addr)
        {
            auto gcZone      = hx::AutoGCZone();
            auto spRequest   = std::unique_ptr<ResolveRequest>(static_cast<ResolveRequest*>(request->data));
            auto addrCleaner = AddrInfoCleaner(addr);

            if (status < 0)
            {
                Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
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
                        ips->Add(hx::asys::libuv::net::ip_from_sockaddr(reinterpret_cast<sockaddr_in*>(info->ai_addr)));
                        break;
                    }

                    case AF_INET6: {
                        ips->Add(hx::asys::libuv::net::ip_from_sockaddr(reinterpret_cast<sockaddr_in6*>(info->ai_addr)));
                        break;
                    }

                    // TODO : What should we do if its another type?
                    }

                    info = info->ai_next;
                } while (nullptr != info);

                Dynamic(spRequest->cbSuccess.rooted)(ips);
            }
        }

        uv_getaddrinfo_t uv;

        const char* host;

        ResolveRequest(String _host, Dynamic _cbSuccess, Dynamic _cbFailure)
            : BaseRequest(_cbSuccess, _cbFailure)
            , host(_host.utf8_str(&buffer))
        {
            uv.data = this;
        }
    };

    struct ReverseRequest : public hx::asys::libuv::BaseRequest
    {
        static void callback(uv_getnameinfo_t* request, int status, const char* hostname, const char* service)
        {
            auto gcZone    = hx::AutoGCZone();
            auto spRequest = std::unique_ptr<ReverseRequest>(static_cast<ReverseRequest*>(request->data));

            if (status < 0)
            {
                Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
            }
            else
            {
                Dynamic(spRequest->cbSuccess.rooted)(String::create(hostname));
            }
        }

        uv_getnameinfo_t uv;

        ReverseRequest(Dynamic _cbSuccess, Dynamic _cbFailure)
            : BaseRequest(_cbSuccess, _cbFailure)
        {
            uv.data = this;
        }
    };
}

void hx::asys::net::dns::resolve(Context ctx, String host, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<ResolveRequest>(host, cbSuccess, cbFailure);
    auto hints    = addrinfo();

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    auto result = uv_getaddrinfo(libuvCtx->uvLoop, &request->uv, ResolveRequest::callback, host.utf8_str(), nullptr, &hints);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::net::dns::reverse(Context ctx, const Ipv4Address ip, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<ReverseRequest>(cbSuccess, cbFailure);
    auto addr     = hx::asys::libuv::net::sockaddr_from_int(ip);
    auto result   = uv_getnameinfo(libuvCtx->uvLoop, &request->uv, ReverseRequest::callback, reinterpret_cast<sockaddr*>(&addr), 0);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::net::dns::reverse(Context ctx, const Ipv6Address ip, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<ReverseRequest>(cbSuccess, cbFailure);
    auto addr     = hx::asys::libuv::net::sockaddr_from_data(ip);
    auto result   = uv_getnameinfo(libuvCtx->uvLoop, &request->uv, ReverseRequest::callback, reinterpret_cast<sockaddr*>(&addr), 0);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}