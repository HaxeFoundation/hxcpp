#include <hxcpp.h>
#include "LibuvTcpSocket.h"
#include "NetUtils.h"
#include "../LibuvUtils.h"

namespace
{
	struct ConnectionRequest final : hx::asys::libuv::BaseRequest
	{
		std::unique_ptr<uv_tcp_t> tcp;

		int keepAlive;

		uv_connect_t connect;

		ConnectionRequest(Dynamic _cbSuccess, Dynamic _cbFailure)
			: hx::asys::libuv::BaseRequest(_cbSuccess, _cbFailure)
			, tcp(std::make_unique<uv_tcp_t>())
			, keepAlive(hx::asys::libuv::net::KEEP_ALIVE_VALUE)
		{
			connect.data = this;
		}
	};

	struct ShutdownRequest final : hx::asys::libuv::BaseRequest
	{
		uv_shutdown_t request;

		ShutdownRequest(Dynamic cbSuccess, Dynamic cbFailure) : hx::asys::libuv::BaseRequest(cbSuccess, cbFailure)
		{
			request.data = this;
		}

		static void callback(uv_shutdown_t* request, int status)
		{
			auto gcZone = hx::AutoGCZone();
			auto spData = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

			if (status < 0)
			{
				Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
			}
			else
			{
				uv_close(reinterpret_cast<uv_handle_t*>(request->handle), hx::asys::libuv::clean_handle);

				Dynamic(spData->cbSuccess.rooted)();
			}
		}
	};

	static void onConnection(uv_connect_t* request, const int status)
	{
		auto gcZone = hx::AutoGCZone();
		auto spData = std::unique_ptr<ConnectionRequest>(static_cast<ConnectionRequest*>(request->data));

		if (status < 0)
		{
			Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));

			return;
		}

		Dynamic(spData->cbSuccess.rooted)(new hx::asys::libuv::net::LibuvTcpSocket(spData->tcp.release(), spData->keepAlive));
	}

	void startConnection(hx::asys::libuv::LibuvAsysContext ctx, sockaddr* const address, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
	{
		auto request = std::make_unique<ConnectionRequest>(cbSuccess, cbFailure);
		auto result  = 0;

		if ((result = uv_tcp_init(ctx->uvLoop, request->tcp.get())) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));

			return;
		}

		if (hx::IsNotNull(options))
		{
			auto keepAliveValue = options->__Field(HX_CSTRING("keepAlive"), hx::PropertyAccess::paccDynamic);
			if (keepAliveValue.isBool())
			{
				request->keepAlive = keepAliveValue.asInt() ? hx::asys::libuv::net::KEEP_ALIVE_VALUE : 0;
			}

			auto sendSizeValue = options->__Field(HX_CSTRING("sendBuffer"), hx::PropertyAccess::paccDynamic);
			if (sendSizeValue.isInt())
			{
				auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(request->tcp.get()), &sendSizeValue.valInt);
				if (result < 0)
				{
					cbFailure(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}
			}

			auto recvSizeValue = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
			if (recvSizeValue.isInt())
			{
				auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(request->tcp.get()), &recvSizeValue.valInt);
				if (result < 0)
				{
					cbFailure(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}
			}
		}

		if ((result = uv_tcp_keepalive(request->tcp.get(), request->keepAlive > 0, hx::asys::libuv::net::KEEP_ALIVE_VALUE) < 0))
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));

			return;
		}

		if ((result = uv_tcp_connect(&request->connect, request->tcp.get(), address, onConnection)) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));
		}
		else
		{
			request.release();
		}
	}
}

hx::asys::libuv::net::LibuvTcpSocket::LibuvTcpSocket(uv_tcp_t* tcp, const int keepAlive)
	: tcp(tcp)
	, keepAlive(keepAlive)
	, reader(new hx::asys::libuv::stream::StreamReader_obj(reinterpret_cast<uv_stream_t*>(tcp)))
	, writer(new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(tcp)))
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	localAddress  = hx::asys::libuv::net::getLocalAddress(tcp);
	remoteAddress = hx::asys::libuv::net::getRemoteAddress(tcp);
}

void hx::asys::libuv::net::LibuvTcpSocket::getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure)
{
	cbSuccess(keepAlive > 0);
}

void hx::asys::libuv::net::LibuvTcpSocket::getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
{
	auto size   = 0;
	auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
{
	auto size   = 0;
	auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = uv_tcp_keepalive(tcp, keepAlive, keepAlive);
	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		keepAlive = keepAlive ? KEEP_ALIVE_VALUE : 0;

		cbSuccess();
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	reader->read(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvTcpSocket::write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	writer->write(data, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvTcpSocket::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
	writer->flush(cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvTcpSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result  = 0;
	auto request = std::make_unique<ShutdownRequest>(cbSuccess, cbFailure);

	if ((result = uv_shutdown(&request->request, reinterpret_cast<uv_stream_t*>(tcp), ShutdownRequest::callback)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		request.release();
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(localAddress);
	HX_MARK_MEMBER(remoteAddress);
	HX_MARK_MEMBER(reader);
	HX_MARK_MEMBER(writer);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::net::LibuvTcpSocket::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(localAddress);
	HX_VISIT_MEMBER(remoteAddress);
	HX_VISIT_MEMBER(reader);
	HX_VISIT_MEMBER(writer);
}
#endif

void hx::asys::net::TcpSocket_obj::connect_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto address = sockaddr_in();
	auto result  = uv_ip4_addr(host.utf8_str(), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		startConnection(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}

void hx::asys::net::TcpSocket_obj::connect_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto address = sockaddr_in6();
	auto result = uv_ip6_addr(host.utf8_str(), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		startConnection(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}