#include <hxcpp.h>
#include "StreamWriter.h"
#include <cstring>

namespace
{
	class WriteRequest final : hx::asys::libuv::BaseRequest
	{
		std::unique_ptr<hx::ArrayPin> pin;

	public:
		uv_write_t request;
		uv_buf_t buffer;

		WriteRequest(hx::ArrayPin* _pin, char* _base, int _length, Dynamic _cbSuccess, Dynamic _cbFailure)
			: BaseRequest(_cbSuccess, _cbFailure)
			, pin(_pin)
			, buffer(uv_buf_init(_base, _length))
		{
			request.data = this;
		}

		static void callback(uv_write_t* request, int status)
		{
			auto gcZone = hx::AutoGCZone();
			auto spData = std::unique_ptr<WriteRequest>(static_cast<WriteRequest*>(request->data));

			if (status < 0)
			{
				Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
			}
			else
			{
				Dynamic(spData->cbSuccess.rooted)(spData->buffer.len);
			}
		}
	};
}

hx::asys::libuv::stream::StreamWriter_obj::StreamWriter_obj(uv_stream_t* stream) : stream(stream) {}

void hx::asys::libuv::stream::StreamWriter_obj::write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto request = std::make_unique<WriteRequest>(data->Pin(), data->GetBase() + offset, length, cbSuccess, cbFailure);
	auto result  = uv_write(&request->request, stream, &request->buffer, 1, WriteRequest::callback);

	if (result < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		request.release();
	}
}

void hx::asys::libuv::stream::StreamWriter_obj::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
	cbSuccess();
}

void hx::asys::libuv::stream::StreamWriter_obj::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);

	cbSuccess();
}
