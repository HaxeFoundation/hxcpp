#pragma once

#include <hxcpp.h>
#include "../LibuvUtils.h"
#include "StreamWriter.h"

namespace hx::asys::libuv::stream
{
	class WritablePipe final : virtual public Writable_obj
	{
	private:
		std::unique_ptr<StreamWriter> writer;

	public:
		std::unique_ptr<uv_pipe_t> pipe;

		WritablePipe();
		~WritablePipe() = default;

		void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void flush(Dynamic cbSuccess, Dynamic cbFailure) override;
		void close(Dynamic cbSuccess, Dynamic cbFailure) override;
	};
}
