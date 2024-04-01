#pragma once

#include <hxcpp.h>
#include <memory>
#include <deque>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::net
{
	class LibuvTcpServer final : public hx::asys::net::TcpServer_obj
	{
	public:
		class ConnectionQueue final
		{
			std::deque<std::unique_ptr<hx::asys::libuv::BaseRequest>> queue;

		public:
			ConnectionQueue();

			void clear();
			void enqueue(Dynamic cbSuccess, Dynamic cbFailure);
			std::unique_ptr<hx::asys::libuv::BaseRequest> tryDequeue();
		};

		class Ctx final : public BaseRequest
		{
		public:
			uv_tcp_t tcp;
			ConnectionQueue connections;
			int keepAlive;
			int status;

			Ctx(Dynamic cbSuccess, Dynamic cbFailure, int keepAlive);

			static void failure(uv_handle_t* handle);
			static void success(uv_handle_t* handle);
		};

		Ctx* ctx;

		LibuvTcpServer(Ctx* _server);

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