#pragma once

#include <hxcpp.h>
#include "../LibuvUtils.h"
#include "StreamReader.h"

namespace hx::asys::libuv::stream
{
	class ReadablePipe final : public Readable_obj
	{
	private:
		std::unique_ptr<StreamReader> reader;

	public:
		std::unique_ptr<uv_pipe_t> pipe;

		ReadablePipe(uv_loop_t* loop);
		~ReadablePipe() = default;

		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void close(Dynamic cbSuccess, Dynamic cbFailure) override;
	};
}
