#pragma once

#include <hxcpp.h>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::net
{
	class LibuvIpcSocket final : public hx::asys::net::IpcSocket_obj
	{
	public:
		struct Ctx final : hx::asys::libuv::BaseRequest
		{
			uv_pipe_t pipe;
			uv_connect_t connection;
			uv_shutdown_t shutdown;
			hx::strbuf buffer;
			hx::asys::libuv::stream::StreamReader_obj::Ctx stream;

			Ctx(Dynamic cbSuccess, Dynamic cbFailure);

			static void onClose(uv_handle_t* handle);
			static void onShutdown(uv_shutdown_t* request, int status);
		};

		Ctx* ctx;

		LibuvIpcSocket(Ctx* ctx);

		void close(Dynamic cbSuccess, Dynamic cbFailure) override;

		void __Mark(hx::MarkContext* __inCtx) override;
#if HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}