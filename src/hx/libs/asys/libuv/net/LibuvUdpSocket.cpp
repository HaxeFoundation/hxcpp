#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"
#include "LibuvUdpSocket.h"

namespace
{
    //
}

hx::asys::libuv::net::LibuvUdpSocket::LibuvUdpSocket(uv_udp_t* const udp)
    : udp(udp)
{
    //
}

void hx::asys::libuv::net::LibuvUdpSocket::bind(const String& host, int port, Dynamic cbSuccess, Dynamic cbFailure)
{
    //
}

void hx::asys::libuv::net::LibuvUdpSocket::unbind(Dynamic cbSuccess, Dynamic cbFailure)
{
}

void hx::asys::libuv::net::LibuvUdpSocket::write(Array<uint8_t> data, int offset, int length, const String& host, int port, Dynamic cbSuccess, Dynamic cbFailure)
{
}

void hx::asys::libuv::net::LibuvUdpSocket::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
}

void hx::asys::libuv::net::LibuvUdpSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
}

void hx::asys::net::UdpSocket_obj::open(Context ctx, const String& host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto udp      = std::make_unique<uv_udp_t>();
    auto result   = 0;

    if (0 != (result = uv_udp_init(libuvCtx->uvLoop, udp.get())))
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        auto address = sockaddr();
        auto field   = HX_CSTRING("reuseAddress");
        auto flags   = options->__HasField(field) ? options->__Field(field, hx::PropertyAccess::paccDynamic).asInt() : 0;

        if (0 != (result = uv_udp_bind(udp.get(), &address, flags)))
        {
            cbFailure(hx::asys::libuv::uv_err_to_enum(result));
        }
        else
        {
            cbSuccess(new hx::asys::libuv::net::LibuvUdpSocket(udp.release()));
        }
    }
}