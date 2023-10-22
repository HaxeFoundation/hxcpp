#include "WritablePipe.h"

hx::asys::libuv::stream::WritablePipe::WritablePipe(uv_loop_t* loop)
{
	pipe   = std::make_unique<uv_pipe_t>();
	writer = std::make_unique<StreamWriter>(reinterpret_cast<uv_stream_t*>(pipe.get()));

	pipe->data = writer.get();

	uv_pipe_init(loop, pipe.get(), false);

	hx::GCSetFinalizer(this, [](hx::Object* obj) -> void {
		reinterpret_cast<WritablePipe*>(obj)->~WritablePipe();
	});
}

void hx::asys::libuv::stream::WritablePipe::write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	writer->write(data, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::stream::WritablePipe::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
	writer->flush(cbSuccess, cbFailure);
}

void hx::asys::libuv::stream::WritablePipe::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	uv_close(reinterpret_cast<uv_handle_t*>(pipe.get()), nullptr);
}
