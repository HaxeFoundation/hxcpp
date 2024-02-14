#include "LibuvTcpServer.h"
#include "LibuvTcpSocket.h"
#include "NetUtils.h"
#include "../LibuvUtils.h"

namespace
{
	void on_connection(uv_stream_t* stream, int status)
	{
		auto gcZone = hx::AutoGCZone();
		auto server = static_cast<hx::asys::libuv::net::LibuvTcpServerImpl*>(stream->data);

		if (status < 0)
		{
			auto request = std::unique_ptr<hx::asys::libuv::BaseRequest>();
			while (nullptr != (request = server->connections.tryDequeue()))
			{
				Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
			}
		}
		else
		{
			auto request = server->connections.tryDequeue();
			if (nullptr != request)
			{
				auto result = 0;
				auto socket = std::make_unique<hx::asys::libuv::net::LibuvTcpSocketImpl>();

				if ((result = uv_tcp_init(server->loop, &socket->tcp)) < 0)
				{
					Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}

				if ((result = uv_tcp_keepalive(&socket->tcp, true, socket->keepAlive)) < 0)
				{
					Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}

				if ((result = uv_accept(reinterpret_cast<uv_stream_t*>(&server->tcp), reinterpret_cast<uv_stream_t*>(&socket->tcp))) < 0)
				{
					Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}

				Dynamic(request->cbSuccess.rooted)(hx::asys::net::TcpSocket(new hx::asys::libuv::net::LibuvTcpSocket(socket.release())));
			}
		}
	}

	void on_open(hx::asys::libuv::LibuvAsysContext ctx, sockaddr* const address, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
	{
		auto server = std::make_unique<hx::asys::libuv::net::LibuvTcpServerImpl>(ctx->uvLoop);
		auto result = 0;

		if ((result = uv_tcp_init(ctx->uvLoop, &server->tcp)) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));

			return;
		}

		if ((result = uv_tcp_bind(&server->tcp, address, 0)) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));

			return;
		}

		auto backlog = 1;

		if (null() != options)
		{
			auto sendBufferSize = options->__Field(HX_CSTRING("sendBuffer"), hx::PropertyAccess::paccDynamic);
			if (sendBufferSize.isInt())
			{
				if ((result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(server.get()), &sendBufferSize.valInt)) < 0)
				{
					cbFailure(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}
			}

			auto recvBufferSize = options->__Field(HX_CSTRING("receiveBuffer"), hx::PropertyAccess::paccDynamic);
			if (recvBufferSize.isInt())
			{
				auto size = recvBufferSize.asInt();

				if ((result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(server.get()), &recvBufferSize.valInt)) < 0)
				{
					cbFailure(hx::asys::libuv::uv_err_to_enum(result));

					return;
				}
			}

			auto backlogSize = options->__Field(HX_CSTRING("backlog"), hx::PropertyAccess::paccDynamic);
			if (!backlogSize.isInt())
			{
				backlog = backlogSize.asInt();
			}
		}

		if ((result = uv_tcp_keepalive(&server->tcp, true, server->keepAlive)) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));

			return;
		}

		if ((result = uv_listen(reinterpret_cast<uv_stream_t*>(&server->tcp), backlog, on_connection)) < 0)
		{
			cbFailure(hx::asys::libuv::uv_err_to_enum(result));
		}
		else
		{
			cbSuccess(hx::asys::net::TcpServer(new hx::asys::libuv::net::LibuvTcpServer(server.release())));
		}
	}
}

hx::asys::libuv::net::ConnectionQueue::ConnectionQueue() : queue(0)
{

}

void hx::asys::libuv::net::ConnectionQueue::clear()
{
	queue.clear();
}

void hx::asys::libuv::net::ConnectionQueue::enqueue(Dynamic cbSuccess, Dynamic cbFailure)
{
	queue.push_back(std::make_unique<hx::asys::libuv::BaseRequest>(cbSuccess, cbFailure));
}

std::unique_ptr<hx::asys::libuv::BaseRequest> hx::asys::libuv::net::ConnectionQueue::tryDequeue()
{
	if (queue.empty())
	{
		return nullptr;
	}

	auto root = std::unique_ptr<hx::asys::libuv::BaseRequest>{ std::move(queue.front()) };

	queue.pop_front();

	return root;
}

hx::asys::libuv::net::LibuvTcpServerImpl::LibuvTcpServerImpl(uv_loop_t* _loop)
	: loop(_loop)
	, keepAlive(KEEP_ALIVE_VALUE)
{
	tcp.data = this;
}

hx::asys::libuv::net::LibuvTcpServer::LibuvTcpServer(LibuvTcpServerImpl* _server) : server(_server)
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	localAddress = hx::asys::libuv::net::getLocalAddress(&server->tcp);
}

void hx::asys::libuv::net::LibuvTcpServer::accept(Dynamic cbSuccess, Dynamic cbFailure)
{
	server->connections.enqueue(cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvTcpServer::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	//struct ShutdownRequest : hx::asys::libuv::BaseRequest
	//{
	//	uv_shutdown_t request;

	//	ShutdownRequest(Dynamic cbSuccess, Dynamic cbFailure) : hx::asys::libuv::BaseRequest(cbSuccess, cbFailure)
	//	{
	//		request.data = this;
	//	}
	//};

	//auto result   = 0;
	//auto request  = std::make_unique<ShutdownRequest>(cbSuccess, cbFailure);
	//auto callback = [](uv_shutdown_t* request, int status) {
	//	auto gcZone = hx::AutoGCZone();
	//	auto spData = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

	//	if (status < 0)
	//	{
	//		Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
	//	}
	//	else
	//	{
	//		uv_close(reinterpret_cast<uv_handle_t*>(request->handle), [](uv_handle_t* handle) {
	//			delete static_cast<TcpSocketImpl*>(handle->data);
	//		});

	//		Dynamic(spData->cbSuccess.rooted)();
	//	}
	//};

	//if ((result = uv_shutdown(&request->request, reinterpret_cast<uv_stream_t*>(&server->tcp), callback)) < 0 && result != UV_ENOTCONN)
	//{
	//	cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	//}
	//else
	//{
	//	request.release();
	//}

	// TODO : Not convinced we don't need the shutdown.

	uv_close(reinterpret_cast<uv_handle_t*>(&server->tcp), [](uv_handle_t* handle) {
		delete static_cast<LibuvTcpServerImpl*>(handle->data);
	});

	cbSuccess();
}

void hx::asys::libuv::net::LibuvTcpServer::getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure)
{
	cbSuccess(server->keepAlive > 0);
}

void hx::asys::libuv::net::LibuvTcpServer::getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure)
{
	auto size   = 0;
	auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&server->tcp, &size), &size);

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
	auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(&server->tcp), &size);

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
	auto result = uv_tcp_keepalive(&server->tcp, keepAlive, server->keepAlive);
	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		server->keepAlive = keepAlive ? KEEP_ALIVE_VALUE : 0;

		cbSuccess();
	}
}

void hx::asys::libuv::net::LibuvTcpServer::setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(&server->tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
	}
}

void hx::asys::libuv::net::LibuvTcpServer::setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = uv_recv_buffer_size(reinterpret_cast<uv_handle_t*>(&server->tcp), &size);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess(size);
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
	auto result = uv_ip4_addr(host.utf8_str(), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		on_open(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}

void hx::asys::net::TcpServer_obj::open_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto address = sockaddr_in6();
	auto result = uv_ip6_addr(host.utf8_str(), port, &address);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		on_open(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}