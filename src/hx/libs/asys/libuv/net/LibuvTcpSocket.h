#pragma once

#include <hxcpp.h>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::net
{
	const int KEEP_ALIVE_VALUE = 5;

	class LibuvTcpSocket final : public hx::asys::net::TcpSocket_obj
	{
	public:
		struct Ctx final : public BaseRequest
		{
			uv_tcp_t tcp;
			uv_connect_t connection;
			uv_shutdown_t shutdown;
			int keepAlive;
			int status;
			hx::asys::libuv::stream::StreamReader_obj::Ctx stream;

			Ctx(Dynamic cbSuccess, Dynamic cbFailure);

			static void onSuccess(uv_handle_t* handle);
			static void onFailure(uv_handle_t* handle);
			static void onShutdown(uv_shutdown_t* handle, int status);
		};

		Ctx* ctx;

		LibuvTcpSocket(Ctx* ctx);

		void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) override;
		void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;
		void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;

		void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) override;
		void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;
		void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;

		void close(Dynamic cbSuccess, Dynamic cbFailure) override;

		void __Mark(hx::MarkContext* __inCtx) override;
#if HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}
