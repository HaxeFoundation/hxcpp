#pragma once

#include <hxcpp.h>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::net
{
	class LibuvIpcSocket final : public hx::asys::net::IpcSocket_obj
	{
		uv_pipe_t* pipe;
		hx::asys::libuv::stream::StreamWriter writer;
		hx::asys::libuv::stream::StreamReader reader;

	public:
		LibuvIpcSocket(uv_pipe_t* pipe);

		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void flush(Dynamic cbSuccess, Dynamic cbFailure) override;
		void close(Dynamic cbSuccess, Dynamic cbFailure) override;

		void __Mark(hx::MarkContext* __inCtx) override;
#if HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}