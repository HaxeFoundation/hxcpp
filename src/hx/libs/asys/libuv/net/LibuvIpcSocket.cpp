#include <hxcpp.h>
#include "LibuvIpcSocket.h"

namespace
{
	class ConnectionRequest final : hx::asys::libuv::BaseRequest
	{
		hx::strbuf buffer;

	public:
		std::unique_ptr<uv_pipe_t> pipe;

		uv_connect_t handle;

		const char* name;

		ConnectionRequest(String _name, Dynamic _cbSuccess, Dynamic _cbFailure, std::unique_ptr<uv_pipe_t> pipe)
			: hx::asys::libuv::BaseRequest(_cbSuccess, _cbFailure)
			, pipe(std::move(pipe))
			, name(_name.utf8_str(&buffer))
		{
			handle.data = this;
		}

		static void onConnection(uv_connect_t* request, const int status)
		{
			auto gcZone = hx::AutoGCZone();
			auto spData = std::unique_ptr<ConnectionRequest>(static_cast<ConnectionRequest*>(request->data));

			if (status < 0)
			{
				Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));

				return;
			}

			Dynamic(spData->cbSuccess.rooted)(hx::asys::net::IpcSocket(new hx::asys::libuv::net::LibuvIpcSocket(spData->pipe.release())));
		}
	};

	String getSocketName(const uv_pipe_t* pipe)
	{
		auto buffer = std::vector<char>(1024);
		auto length = buffer.size();
		auto result = 0;

		if ((result = uv_pipe_getsockname(pipe, buffer.data(), &length)) < 0)
		{
			if (result == UV_ENOBUFS)
			{
				buffer.resize(length);

				if ((result = uv_pipe_getsockname(pipe, buffer.data(), &length)) < 0)
				{
					return HX_CSTRING("");
				}
			}
		}

		return String::create(buffer.data(), length);
	}

	String getPeerName(uv_pipe_t* pipe)
	{
		auto buffer = std::vector<char>(1024);
		auto length = buffer.size();
		auto result = 0;

		if ((result = uv_pipe_getpeername(pipe, buffer.data(), &length)) < 0)
		{
			if (result == UV_ENOBUFS)
			{
				buffer.resize(length);

				if ((result = uv_pipe_getsockname(pipe, buffer.data(), &length)) < 0)
				{
					return HX_CSTRING("");
				}
			}
		}

		return String::create(buffer.data(), length);
	}
}

hx::asys::libuv::net::LibuvIpcSocket::LibuvIpcSocket(uv_pipe_t* pipe)
	: pipe(pipe)
	, reader(new hx::asys::libuv::stream::StreamReader_obj(reinterpret_cast<uv_stream_t*>(pipe)))
	, writer(new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(pipe)))
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	socketName = getSocketName(pipe);
	peerName   = getPeerName(pipe);
}

void hx::asys::libuv::net::LibuvIpcSocket::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	reader->read(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvIpcSocket::write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
	writer->write(data, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvIpcSocket::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
	writer->flush(cbSuccess, cbFailure);
}

void hx::asys::libuv::net::LibuvIpcSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	uv_close(reinterpret_cast<uv_handle_t*>(pipe), nullptr);
}

void hx::asys::libuv::net::LibuvIpcSocket::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(reader);
	HX_MARK_MEMBER(writer);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::net::LibuvIpcSocket::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(reader);
	HX_VISIT_MEMBER(writer);
}
#endif

void hx::asys::net::IpcSocket_obj::bind(Context ctx, String name, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto libuvCtx = hx::asys::libuv::context(ctx);
	auto pipe     = std::make_unique<uv_pipe_t>();
	auto result   = 0;

	if ((result = uv_pipe_init(libuvCtx->uvLoop, pipe.get(), false)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));

		return;
	}

	hx::strbuf buffer;
	if ((result = uv_pipe_bind(pipe.get(), name.utf8_str(&buffer))) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));

		return;
	}

	cbSuccess(IpcSocket(new hx::asys::libuv::net::LibuvIpcSocket(pipe.release())));
}

void hx::asys::net::IpcSocket_obj::connect(Context ctx, String name, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto libuvCtx = hx::asys::libuv::context(ctx);
	auto pipe     = std::make_unique<uv_pipe_t>();
	auto result   = 0;

	if ((result = uv_pipe_init(libuvCtx->uvLoop, pipe.get(), false)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));

		return;
	}

	auto request = new ConnectionRequest(name, cbSuccess, cbFailure, std::move(pipe));

	uv_pipe_connect(&request->handle, request->pipe.get(), request->name, ConnectionRequest::onConnection);
}