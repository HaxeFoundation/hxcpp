#include "ReadablePipe.h"

hx::asys::libuv::stream::ReadablePipe::ReadablePipe()
{
	pipe   = std::make_unique<uv_pipe_t>();
	reader = std::make_unique<StreamReader>(reinterpret_cast<uv_stream_t*>(pipe.get()));

	pipe->data = reader.get();

	hx::GCSetFinalizer(this, [](hx::Object* obj) -> void {
		reinterpret_cast<ReadablePipe*>(obj)->~ReadablePipe();
	});
}

void hx::asys::libuv::stream::ReadablePipe::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	reader->read(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::stream::ReadablePipe::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	uv_close(reinterpret_cast<uv_handle_t*>(pipe.get()), nullptr);
}
