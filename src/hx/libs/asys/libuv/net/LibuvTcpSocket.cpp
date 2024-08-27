#include <hxcpp.h>
#include "LibuvTcpSocket.h"
#include "NetUtils.h"
#include "../LibuvUtils.h"

namespace
{
	struct CtxDeleter
	{
		void operator()(hx::asys::libuv::net::LibuvTcpSocket::Ctx* ctx)
		{
			if (0 == ctx->tcp.type)
			{
				hx::asys::libuv::net::LibuvTcpSocket::Ctx::onFailure(reinterpret_cast<uv_handle_t*>(&ctx->tcp));

				return;
			}

			uv_close(reinterpret_cast<uv_handle_t*>(&ctx->tcp), hx::asys::libuv::net::LibuvTcpSocket::Ctx::onFailure);
		}
	};

	void onConnection(uv_connect_t* request, const int status)
	{
		auto ctx = std::unique_ptr<hx::asys::libuv::net::LibuvTcpSocket::Ctx, CtxDeleter>{ static_cast<hx::asys::libuv::net::LibuvTcpSocket::Ctx*>(request->data) };

		if ((ctx->status = status) < 0)
		{
			return;
		}
		
		auto gcZone = hx::AutoGCZone();

		Dynamic(ctx->cbSuccess.rooted)(new hx::asys::libuv::net::LibuvTcpSocket(ctx.release()));
	}

	void startConnection(hx::asys::libuv::LibuvAsysContext ctx, sockaddr* const address, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
	{
		auto request = std::unique_ptr<hx::asys::libuv::net::LibuvTcpSocket::Ctx, CtxDeleter>{ new hx::asys::libuv::net::LibuvTcpSocket::Ctx(cbSuccess, cbFailure) };

		if ((request->status = uv_tcp_init(ctx->uvLoop, &request->tcp)) < 0)
		{
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
				if (sendSizeValue.asInt() > 0)
				{
					if ((request->status = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&request->tcp), &sendSizeValue.valInt)) < 0)
					{
						return;
					}
				}
				else
				{
					request->status = UV_EINVAL;

					return;
				}
			}

			auto recvSizeValue = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
			if (recvSizeValue.isInt())
			{
				if (recvSizeValue.asInt() > 0)
				{
					if ((request->status = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&request->tcp), &recvSizeValue.valInt)) < 0)
					{
						return;
					}
				}
				else
				{
					request->status = UV_EINVAL;

					return;
				}
			}
		}

		if ((request->status = uv_tcp_keepalive(&request->tcp, request->keepAlive > 0, hx::asys::libuv::net::KEEP_ALIVE_VALUE) < 0))
		{
			uv_close(reinterpret_cast<uv_handle_t*>(&request.release()->tcp), hx::asys::libuv::net::LibuvTcpSocket::Ctx::onFailure);

			return;
		}

		if ((request->status = uv_tcp_connect(&request->connection, &request->tcp, address, onConnection)) < 0)
		{
			return;
		}
		else
		{
			request.release();
		}
	}

	void onAlloc(uv_handle_t* handle, const size_t suggested, uv_buf_t* buffer)
	{
		auto  ctx     = static_cast<hx::asys::libuv::net::LibuvTcpSocket::Ctx*>(handle->data);
		auto& staging = ctx->stream.staging.emplace_back(suggested);

		buffer->base = staging.data();
		buffer->len  = staging.size();
	}

	void onRead(uv_stream_t* stream, const ssize_t len, const uv_buf_t* read)
	{
		auto gc  = hx::AutoGCZone();
		auto ctx = static_cast<hx::asys::libuv::net::LibuvTcpSocket::Ctx*>(stream->data);

		if (len <= 0)
		{
			ctx->stream.reject(len);

			return;
		}

		ctx->stream.buffer.insert(ctx->stream.buffer.end(), read->base, read->base + len);
		ctx->stream.consume();
	}
}

hx::asys::libuv::net::LibuvTcpSocket::Ctx::Ctx(Dynamic cbSuccess, Dynamic cbFailure)
	: hx::asys::libuv::BaseRequest(cbSuccess, cbFailure)
	, tcp()
	, connection()
	, shutdown()
	, keepAlive(hx::asys::libuv::net::KEEP_ALIVE_VALUE)
	, status(0)
	, stream(reinterpret_cast<uv_stream_t*>(&tcp))
{
	shutdown.data   = this;
	connection.data = this;
	tcp.data        = this;
}

void hx::asys::libuv::net::LibuvTcpSocket::Ctx::onSuccess(uv_handle_t* handle)
{
	auto spData = std::unique_ptr<Ctx>(reinterpret_cast<Ctx*>(handle->data));
	auto gcZone = hx::AutoGCZone();

	Dynamic(spData->cbSuccess.rooted)();
}

void hx::asys::libuv::net::LibuvTcpSocket::Ctx::onFailure(uv_handle_t* handle)
{
	auto spData = std::unique_ptr<Ctx>(reinterpret_cast<Ctx*>(handle->data));
	auto gcZone = hx::AutoGCZone();

	Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spData->status));
}

void hx::asys::libuv::net::LibuvTcpSocket::Ctx::onShutdown(uv_shutdown_t* handle, int status)
{
	uv_close(
		reinterpret_cast<uv_handle_t*>(handle->handle),
		status < 0
			? hx::asys::libuv::net::LibuvTcpSocket::Ctx::onFailure
			: hx::asys::libuv::net::LibuvTcpSocket::Ctx::onSuccess);
}


hx::asys::libuv::net::LibuvTcpSocket::LibuvTcpSocket(Ctx* ctx) : ctx(ctx)
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	reader        = hx::asys::Readable(new hx::asys::libuv::stream::StreamReader_obj(&ctx->stream, onAlloc, onRead));
	writer        = hx::asys::Writable(new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(&ctx->tcp)));
	localAddress  = hx::asys::libuv::net::getLocalAddress(&ctx->tcp);
	remoteAddress = hx::asys::libuv::net::getRemoteAddress(&ctx->tcp);
}

void hx::asys::libuv::net::LibuvTcpSocket::getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure)
{
	cbSuccess(ctx->keepAlive > 0);
}

void hx::asys::libuv::net::LibuvTcpSocket::getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
{
	auto size   = 0;
	auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&ctx->tcp), &size);

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
	auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(&ctx->tcp), &size);

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
	auto result = uv_tcp_keepalive(&ctx->tcp, keepAlive, keepAlive ? KEEP_ALIVE_VALUE : 0);
	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		ctx->keepAlive = keepAlive ? KEEP_ALIVE_VALUE : 0;

		cbSuccess();
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&ctx->tcp), &size);

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
	auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(&ctx->tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
	}
}

void hx::asys::libuv::net::LibuvTcpSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	if ((ctx->status = uv_shutdown(&ctx->shutdown, reinterpret_cast<uv_stream_t*>(&ctx->tcp), Ctx::onShutdown)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(ctx->status));
	}
	else
	{
		ctx->cbSuccess.rooted = cbSuccess.mPtr;
		ctx->cbFailure.rooted = cbFailure.mPtr;
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
	hx::strbuf buffer;

	auto address = sockaddr_in();
	auto result  = uv_ip4_addr(host.utf8_str(&buffer), port, &address);

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
	hx::strbuf buffer;

	auto address = sockaddr_in6();
	auto result  = uv_ip6_addr(host.utf8_str(&buffer), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		startConnection(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}