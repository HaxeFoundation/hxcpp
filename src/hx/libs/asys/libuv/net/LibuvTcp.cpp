#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "../stream/StreamReader.h"
#include "../LibuvUtils.h"

namespace
{
	const int KEEP_ALIVE_VALUE = 5;

	hx::Anon getLocalAddress(uv_tcp_t* tcp)
	{
		auto storage = sockaddr_storage();
		auto length  = int(sizeof(sockaddr_storage));
		auto result  = uv_tcp_getsockname(tcp, reinterpret_cast<sockaddr*>(&storage), &length);

		if (result < 0)
		{
			return null();
		}
		else
		{
			auto name = std::array<char, UV_IF_NAMESIZE>();
			auto port = int(reinterpret_cast<sockaddr_in*>(&storage)->sin_port);

			if ((result = uv_ip_name(reinterpret_cast<sockaddr*>(&storage), name.data(), name.size())) < 0)
			{
				return null();
			}

			return
				hx::Anon(
					hx::Anon_obj::Create(2)
						->setFixed(0, HX_CSTRING("host"), String::create(name.data()))
						->setFixed(1, HX_CSTRING("port"), port));
		}
	}

	hx::Anon getRemoteAddress(uv_tcp_t* tcp)
	{
		auto storage = sockaddr_storage();
		auto length  = int(sizeof(sockaddr_storage));
		auto result  = uv_tcp_getpeername(tcp, reinterpret_cast<sockaddr*>(&storage), &length);

		if (result < 0)
		{
			return null();
		}
		else
		{
			auto name = std::array<char, UV_IF_NAMESIZE>();
			auto port = int(reinterpret_cast<sockaddr_in*>(&storage)->sin_port);

			if ((result = uv_ip_name(reinterpret_cast<sockaddr*>(&storage), name.data(), name.size())) < 0)
			{
				return null();
			}

			return
				hx::Anon(
					hx::Anon_obj::Create(2)
						->setFixed(0, HX_CSTRING("host"), String::create(name.data()))
						->setFixed(1, HX_CSTRING("port"), port));
		}
	}

	class WriteRequest final : hx::asys::libuv::BaseRequest
	{
		std::unique_ptr<hx::ArrayPin> pin;

	public:
		uv_write_t request;
		uv_buf_t buffer;

		WriteRequest(hx::ArrayPin* _pin, char* _base, int _length, Dynamic _cbSuccess, Dynamic _cbFailure)
			: BaseRequest(_cbSuccess, _cbFailure)
			, pin(_pin)
			, buffer(uv_buf_init(_base, _length)) {}

		~WriteRequest() = default;

		static void callback(uv_write_t* request, int status)
		{
			auto gcZone = hx::AutoGCZone();
			auto spData = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

			if (status < 0)
			{
				Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
			}
			else
			{
				Dynamic(spData->cbSuccess.rooted)();
			}
		}
	};

	struct TcpSocketImpl final : public hx::asys::libuv::stream::StreamReader
	{
		uv_tcp_t tcp;

		TcpSocketImpl() : hx::asys::libuv::stream::StreamReader(reinterpret_cast<uv_stream_t*>(&tcp))
		{
			tcp.data = reinterpret_cast<hx::asys::libuv::stream::StreamReader*>(this);
		}
	};

	class LibuvTcpSocket final : public hx::asys::net::TcpSocket_obj
	{
		TcpSocketImpl* socket;

	public:
		LibuvTcpSocket(TcpSocketImpl* _socket) : socket(_socket)
		{
			HX_OBJ_WB_NEW_MARKED_OBJECT(this);

			localAddress  = getLocalAddress(&socket->tcp);
			remoteAddress = getRemoteAddress(&socket->tcp);
		}

		void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			auto request = std::make_unique<WriteRequest>(data->Pin(), data->GetBase() + offset, length, cbSuccess, cbFailure);
			auto result  = uv_write(&request->request, reinterpret_cast<uv_stream_t*>(&socket->tcp), &request->buffer, 1, WriteRequest::callback);

			if (result <= 0)
			{
				cbFailure(hx::asys::libuv::uv_err_to_enum(result));
			}
			else
			{
				request.release();
			}
		}
		void flush(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess();
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			struct ShutdownRequest : hx::asys::libuv::BaseRequest
			{
				uv_shutdown_t request;

				ShutdownRequest(Dynamic cbSuccess, Dynamic cbFailure) : hx::asys::libuv::BaseRequest(cbSuccess, cbFailure)
				{
					request.data = this;
				}
			};

			auto result   = 0;
			auto request  = std::make_unique<ShutdownRequest>(cbSuccess, cbFailure);
			auto callback = [](uv_shutdown_t* request, int status) {
				auto gcZone = hx::AutoGCZone();
				auto spData = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

				if (status < 0)
				{
					Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
				}
				else
				{
					uv_close(reinterpret_cast<uv_handle_t*>(request->handle), [](uv_handle_t* handle) {
						delete static_cast<hx::asys::libuv::stream::StreamReader*>(handle->data);
					});

					Dynamic(spData->cbSuccess.rooted)();
				}
			};

			if ((result = uv_shutdown(&request->request, reinterpret_cast<uv_stream_t*>(&socket->tcp), callback)) < 0)
			{
				cbFailure(hx::asys::libuv::uv_err_to_enum(result));
			}
			else
			{
				request.release();
			}
		}
		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			socket->read(output, offset, length, cbSuccess, cbFailure);
		}

		// Inherited via TcpSocket_obj
		void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) override
		{
		}
		void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override
		{
		}
		void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override
		{
		}
		void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override
		{
		}
		void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override
		{
		}
		void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) override
		{
		}

		void __Mark(hx::MarkContext* __inCtx) override
		{
			HX_MARK_MEMBER(localAddress);
			HX_MARK_MEMBER(remoteAddress);
		}
#ifdef HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override
		{
			HX_VISIT_MEMBER(localAddress);
			HX_VISIT_MEMBER(remoteAddress);
		}
#endif
	};

	class LibuvTcpServer final : public hx::asys::net::TcpServer_obj
	{
		/// <summary>
		/// Every time the user calls `Accept` on the server the callback is rooted and stored in this queue.
		/// Whenever libuv notifies us of a incoming connection the front of the queue will be popped and used.
		/// </summary>
		class ConnectionQueue
		{
		private:
			std::deque<std::unique_ptr<hx::asys::libuv::BaseRequest>> queue;

		public:
			ConnectionQueue() : queue(std::deque<std::unique_ptr<hx::asys::libuv::BaseRequest>>(0)) { }

			void Clear()
			{
				queue.clear();
			}

			void Enqueue(Dynamic cbSuccess, Dynamic cbFailure)
			{
				queue.push_back(std::make_unique<hx::asys::libuv::BaseRequest>(cbSuccess, cbFailure));
			}

			std::unique_ptr<hx::asys::libuv::BaseRequest> tryDequeue()
			{
				if (queue.empty())
				{
					return nullptr;
				}

				auto root = std::unique_ptr<hx::asys::libuv::BaseRequest>{ std::move(queue.front()) };

				queue.pop_front();

				return root;
			}
		};

		struct TcpServerImpl final
		{
			uv_tcp_t tcp;
			uv_loop_t* loop;
			ConnectionQueue connections;
			int keepAlive;

			TcpServerImpl(uv_loop_t* _loop)
				: loop(_loop)
				, keepAlive(KEEP_ALIVE_VALUE)
			{
				tcp.data = this;
			}
		};

		TcpServerImpl* server;

	public:
		LibuvTcpServer(TcpServerImpl* _server) : server(_server)
		{
			HX_OBJ_WB_NEW_MARKED_OBJECT(this);

			localAddress = getLocalAddress(&server->tcp);
		}

		void accept(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			server->connections.Enqueue(cbSuccess, cbFailure);
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
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
				delete static_cast<hx::asys::libuv::stream::StreamReader*>(handle->data);
			});

			cbSuccess();
		}
		void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override
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
		void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			auto size = 0;
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
		void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess(server->keepAlive > 0);
		}
		void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override
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
		void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override
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
		void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) override
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
		void __Mark(hx::MarkContext* __inCtx) override
		{
			HX_MARK_MEMBER(localAddress);
		}
#ifdef HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override
		{
			HX_VISIT_MEMBER(localAddress);
		}
#endif

		static void on_open(hx::asys::libuv::LibuvAsysContext ctx, sockaddr* const address, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure)
		{
			auto server = std::make_unique<TcpServerImpl>(ctx->uvLoop);
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

			auto backlog = SOMAXCONN;

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

			if ((result = uv_listen(reinterpret_cast<uv_stream_t*>(&server->tcp), backlog, LibuvTcpServer::on_connection)) < 0)
			{
				cbFailure(hx::asys::libuv::uv_err_to_enum(result));
			}
			else
			{
				cbSuccess(hx::asys::net::TcpServer(new LibuvTcpServer(server.release())));
			}
		}

		static void on_connection(uv_stream_t* stream, int status)
		{
			auto gcZone = hx::AutoGCZone();
			auto server = static_cast<TcpServerImpl*>(stream->data);

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
					auto socket = std::make_unique<TcpSocketImpl>();

					if ((result = uv_tcp_init(server->loop, &socket->tcp)) < 0)
					{
						Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

						return;
					}

					if ((result = uv_accept(reinterpret_cast<uv_stream_t*>(&server->tcp), reinterpret_cast<uv_stream_t*>(&socket->tcp))) < 0)
					{
						Dynamic(request->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

						return;
					}

					Dynamic(request->cbSuccess.rooted)(hx::asys::net::TcpSocket(new LibuvTcpSocket(socket.release())));
				}
			}
		}
	};
}

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
		LibuvTcpServer::on_open(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
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
		LibuvTcpServer::on_open(hx::asys::libuv::context(ctx), reinterpret_cast<sockaddr*>(&address), options, cbSuccess, cbFailure);
	}
}