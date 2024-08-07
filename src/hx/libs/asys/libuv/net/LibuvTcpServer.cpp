#include <hxcpp.h>
#include "LibuvTcpServer.h"
#include "LibuvTcpSocket.h"
#include "NetUtils.h"
#include "../LibuvUtils.h"

namespace
{
	void onConnection(uv_stream_t* stream, int status)
	{
		auto gcZone = hx::AutoGCZone();
		auto server = static_cast<hx::asys::libuv::net::LibuvTcpServer::Ctx*>(stream->data);

		if (status < 0)
		{
			server->status = status;

			uv_close(reinterpret_cast<uv_handle_t*>(stream), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);
		}
		else
		{
			auto request = server->connections.tryDequeue();
			if (nullptr != request)
			{
				auto ctx = std::make_unique<hx::asys::libuv::net::LibuvTcpSocket::Ctx>(null(), null());

				if ((ctx->status = uv_tcp_init(server->tcp.loop, &ctx->tcp)) < 0)
				{
					Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(ctx->status));

					return;
				}

				if ((ctx->status = uv_accept(reinterpret_cast<uv_stream_t*>(&server->tcp), reinterpret_cast<uv_stream_t*>(&ctx->tcp))) < 0)
				{
					Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(ctx->status));

					return;
				}

				if ((ctx->status = uv_tcp_keepalive(&ctx->tcp, ctx->keepAlive > 0, ctx->keepAlive)) < 0)
				{
					Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(ctx->status));

					return;
				}

				Dynamic(request->cbSuccess.rooted)(hx::asys::net::TcpSocket(new hx::asys::libuv::net::LibuvTcpSocket(ctx.release())));
			}
		}
	}

	void onOpen(hx::asys::libuv::LibuvAsysContext ctx, sockaddr* const address, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
	{
		auto backlog        = SOMAXCONN;
		auto sendBufferSize = 0;
		auto recvBufferSize = 0;
		auto keepAlive      = hx::asys::libuv::net::KEEP_ALIVE_VALUE;

		if (hx::IsNotNull(options))
		{
			auto backlogSizeVal = options->__Field(HX_CSTRING("backlog"), hx::PropertyAccess::paccDynamic);
			if (backlogSizeVal.isInt())
			{
				backlog = backlogSizeVal.asInt();
			}

			auto keepAliveVal = options->__Field(HX_CSTRING("keepAlive"), hx::PropertyAccess::paccDynamic);
			if (keepAliveVal.isBool())
			{
				keepAlive = keepAliveVal.asInt() ? hx::asys::libuv::net::KEEP_ALIVE_VALUE : 0;
			}
		}

		auto server = std::make_unique<hx::asys::libuv::net::LibuvTcpServer::Ctx>(cbSuccess, cbFailure, keepAlive);
		auto result = 0;

		if ((result = uv_tcp_init(ctx->uvLoop, &server->tcp)) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));

			return;
		}

		if ((server->status = uv_tcp_bind(&server->tcp, address, 0)) < 0)
		{
			uv_close(reinterpret_cast<uv_handle_t*>(&server.release()->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);

			return;
		}

		if (hx::IsNotNull(options))
		{
			auto sendBufferVal = options->__Field(HX_CSTRING("sendBuffer"), hx::PropertyAccess::paccDynamic);
			if (sendBufferVal.isInt())
			{
				auto size = sendBufferVal.asInt();

				if (size > 0)
				{
					if ((server->status = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&server->tcp), &size)) < 0)
					{
						uv_close(reinterpret_cast<uv_handle_t*>(&server.release()->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);

						return;
					}
				}
				else
				{
					server->status = UV_EINVAL;

					uv_close(reinterpret_cast<uv_handle_t*>(&server.release()->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);

					return;
				}
			}

			auto recvBufferVal = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
			if (recvBufferVal.isInt())
			{
				auto size = recvBufferVal.asInt();

				if (size > 0)
				{
					if ((server->status = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(&server->tcp), &size)) < 0)
					{
						uv_close(reinterpret_cast<uv_handle_t*>(&server.release()->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);

						return;
					}
				}
				else
				{
					server->status = UV_EINVAL;

					uv_close(reinterpret_cast<uv_handle_t*>(&server.release()->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);

					return;
				}
			}
		}

		if ((server->status = uv_listen(reinterpret_cast<uv_stream_t*>(&server->tcp), backlog, onConnection)) < 0)
		{
			uv_close(reinterpret_cast<uv_handle_t*>(&server.release()->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::failure);

			return;
		}
		else
		{
			cbSuccess(hx::asys::net::TcpServer(new hx::asys::libuv::net::LibuvTcpServer(server.release())));
		}
	}
}

hx::asys::libuv::net::LibuvTcpServer::ConnectionQueue::ConnectionQueue() : queue(0)
{

}

void hx::asys::libuv::net::LibuvTcpServer::ConnectionQueue::clear()
{
	queue.clear();
}

void hx::asys::libuv::net::LibuvTcpServer::ConnectionQueue::enqueue(Dynamic cbSuccess, Dynamic cbFailure)
{
	queue.push_back(std::make_unique<hx::asys::libuv::BaseRequest>(cbSuccess, cbFailure));
}

std::unique_ptr<hx::asys::libuv::BaseRequest> hx::asys::libuv::net::LibuvTcpServer::ConnectionQueue::tryDequeue()
{
	if (queue.empty())
	{
		return nullptr;
	}

	auto root = std::unique_ptr<hx::asys::libuv::BaseRequest>{ std::move(queue.front()) };

	queue.pop_front();

	return root;
}

hx::asys::libuv::net::LibuvTcpServer::Ctx::Ctx(Dynamic cbSuccess, Dynamic cbFailure, const int keepAlive)
	: hx::asys::libuv::BaseRequest(cbSuccess, cbFailure)
	, keepAlive(keepAlive)
	, status(0)
	, connections()
{
	tcp.data = this;
}

void hx::asys::libuv::net::LibuvTcpServer::Ctx::failure(uv_handle_t* handle)
{
	auto spData = std::unique_ptr<Ctx>(reinterpret_cast<Ctx*>(handle->data));
	auto gcZone = hx::AutoGCZone();

	Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spData->status));
}

void hx::asys::libuv::net::LibuvTcpServer::Ctx::success(uv_handle_t* handle)
{
	auto spData = std::unique_ptr<Ctx>(reinterpret_cast<Ctx*>(handle->data));
	auto gcZone = hx::AutoGCZone();

	Dynamic(spData->cbSuccess.rooted)();
}

hx::asys::libuv::net::LibuvTcpServer::LibuvTcpServer(hx::asys::libuv::net::LibuvTcpServer::Ctx* ctx) : ctx(ctx)
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	localAddress = hx::asys::libuv::net::getLocalAddress(&ctx->tcp);
}

void hx::asys::libuv::net::LibuvTcpServer::accept(Dynamic cbSuccess, Dynamic cbFailure)
{
	ctx->connections.enqueue(cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvTcpServer::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	ctx->cbSuccess.rooted = cbSuccess.mPtr;
	ctx->cbFailure.rooted = cbFailure.mPtr;

	uv_close(reinterpret_cast<uv_handle_t*>(&ctx->tcp), hx::asys::libuv::net::LibuvTcpServer::Ctx::success);
}

void hx::asys::libuv::net::LibuvTcpServer::getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure)
{
	cbSuccess(ctx->keepAlive > 0);
}

void hx::asys::libuv::net::LibuvTcpServer::getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
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

void hx::asys::libuv::net::LibuvTcpServer::getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
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

void hx::asys::libuv::net::LibuvTcpServer::setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure)
{
	ctx->keepAlive = keepAlive ? KEEP_ALIVE_VALUE : 0;

	cbSuccess();
}

void hx::asys::libuv::net::LibuvTcpServer::setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	if (size > 0)
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
	else
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(UV_EINVAL));
	}
}

void hx::asys::libuv::net::LibuvTcpServer::setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	if (size > 0)
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
	else
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(UV_EINVAL));
	}
}

void hx::asys::libuv::net::LibuvTcpServer::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(localAddress);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::net::LibuvTcpServer::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(localAddress);
}
#endif

void hx::asys::net::TcpServer_obj::open_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto address = sockaddr_in();
	auto result  = uv_ip4_addr(host.utf8_str(), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		onOpen(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}

void hx::asys::net::TcpServer_obj::open_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto address = sockaddr_in6();
	auto result  = uv_ip6_addr(host.utf8_str(), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		onOpen(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}