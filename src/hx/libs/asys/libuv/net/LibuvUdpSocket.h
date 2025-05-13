#pragma once

#include <hxcpp.h>
#include <memory>
#include <deque>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"
#include "../LibuvUtils.h"

namespace hx::asys::libuv::net
{
	class LibuvUdpSocket : public hx::asys::net::UdpSocket_obj
	{
	private:
		uv_udp_t* const udp;

	public:
		LibuvUdpSocket(uv_udp_t* const udp);

		void bind(const String& host, int port, Dynamic cbSuccess, Dynamic cbFailure) override;
		void unbind(Dynamic cbSuccess, Dynamic cbFailure) override;
		void write(Array<uint8_t> data, int offset, int length, const String& host, int port, Dynamic cbSuccess, Dynamic cbFailure) override;
		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void close(Dynamic cbSuccess, Dynamic cbFailure) override;
	};
}