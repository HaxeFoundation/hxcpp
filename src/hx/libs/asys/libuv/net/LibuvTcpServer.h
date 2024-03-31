#pragma once

#include <hxcpp.h>
#include <memory>
#include <deque>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::net
{
	/// <summary>
	/// Every time the user calls `Accept` on the server the callback is rooted and stored in this queue.
	/// Whenever libuv notifies us of a incoming connection the front of the queue will be popped and used.
	/// </summary>
	class ConnectionQueue final
	{
		std::deque<std::unique_ptr<hx::asys::libuv::BaseRequest>> queue;

	public:
		ConnectionQueue();

		void clear();
		void enqueue(Dynamic cbSuccess, Dynamic cbFailure);
		std::unique_ptr<hx::asys::libuv::BaseRequest> tryDequeue();
	};

	class LibuvTcpServerImpl final : public BaseRequest
	{
	public:
		uv_tcp_t tcp;
		uv_loop_t* loop;
		ConnectionQueue connections;
		int keepAlive;
		int sendBufferSize;
		int recvBufferSize;
		int status;

		LibuvTcpServerImpl(Dynamic cbSuccess, Dynamic cbFailure, uv_loop_t* _loop, int keepAlive, int sendBufferSize, int recvBufferSize);

		static void cleanup(uv_handle_t* handle);
	};

	class LibuvTcpServer final : public hx::asys::net::TcpServer_obj
	{
		LibuvTcpServerImpl* server;

	public:
		LibuvTcpServer(LibuvTcpServerImpl* _server);

		void accept(Dynamic cbSuccess, Dynamic cbFailure) override;
		void close(Dynamic cbSuccess, Dynamic cbFailure) override;

		void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) override;
		void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;
		void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;

		void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) override;
		void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;
		void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;

		void __Mark(hx::MarkContext* __inCtx) override;
#ifdef HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}