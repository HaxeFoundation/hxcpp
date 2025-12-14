#include <hxcpp.h>
#include "LibuvIpcSocket.h"

namespace
{
	static void onConnection(uv_connect_t* request, const int status)
	{
		auto gcZone = hx::AutoGCZone();
		auto spData = std::unique_ptr<hx::asys::libuv::net::LibuvIpcSocket::Ctx>(static_cast<hx::asys::libuv::net::LibuvIpcSocket::Ctx*>(request->data));

		if (status < 0)
		{
			Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));

			return;
		}

		Dynamic(spData->cbSuccess.rooted)(hx::asys::net::IpcSocket(new hx::asys::libuv::net::LibuvIpcSocket(spData.release())));
	}

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

hx::asys::libuv::net::LibuvIpcSocket::Ctx::Ctx(Dynamic _cbSuccess, Dynamic _cbFailure)
	: BaseRequest(_cbSuccess, _cbFailure)
	, pipe()
	, connection()
	, shutdown()
	, stream(reinterpret_cast<uv_stream_t*>(&pipe))
{
	pipe.data = this;
	connection.data = this;
	shutdown.data = this;
}

void hx::asys::libuv::net::LibuvIpcSocket::Ctx::onClose(uv_handle_t* handle)
{
	auto spData = std::unique_ptr<Ctx>(reinterpret_cast<Ctx*>(handle->data));
	auto gcZone = hx::AutoGCZone();

	Dynamic(spData->cbSuccess.rooted)();
}

void hx::asys::libuv::net::LibuvIpcSocket::Ctx::onShutdown(uv_shutdown_t* shutdown, int status)
{
	uv_close(reinterpret_cast<uv_handle_t*>(shutdown->handle), hx::asys::libuv::net::LibuvIpcSocket::Ctx::onClose);
}

hx::asys::libuv::net::LibuvIpcSocket::LibuvIpcSocket(Ctx* _ctx) : ctx(_ctx)
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	reader     = hx::asys::Readable(new hx::asys::libuv::stream::StreamReader_obj(nullptr, nullptr, nullptr));
	writer     = hx::asys::Writable(new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(&ctx->pipe)));
	socketName = getSocketName(&ctx->pipe);
	peerName   = getPeerName(&ctx->pipe);
}

void hx::asys::libuv::net::LibuvIpcSocket::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	ctx->cbSuccess.rooted = cbSuccess.mPtr;
	ctx->cbFailure.rooted = cbFailure.mPtr;

	auto result = uv_shutdown(&ctx->shutdown, reinterpret_cast<uv_stream_t*>(&ctx->pipe), hx::asys::libuv::net::LibuvIpcSocket::Ctx::onShutdown);
	if (result < 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(&ctx->pipe), hx::asys::libuv::net::LibuvIpcSocket::Ctx::onClose);
	}
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
	auto socket   = std::make_unique<hx::asys::libuv::net::LibuvIpcSocket::Ctx>(null(), null());
	auto result   = 0;

	if ((result = uv_pipe_init(libuvCtx->uvLoop, &socket->pipe, false)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));

		return;
	}

	hx::strbuf buffer;
	if ((result = uv_pipe_bind(&socket->pipe, name.utf8_str(&buffer))) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));

		return;
	}

	cbSuccess(IpcSocket(new hx::asys::libuv::net::LibuvIpcSocket(socket.release())));
}

void hx::asys::net::IpcSocket_obj::connect(Context ctx, String name, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto libuvCtx = hx::asys::libuv::context(ctx);
	auto socket   = std::make_unique<hx::asys::libuv::net::LibuvIpcSocket::Ctx>(cbSuccess, cbFailure);
	auto result   = 0;

	if ((result = uv_pipe_init(libuvCtx->uvLoop, &socket->pipe, false)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));

		return;
	}

	uv_pipe_connect(&socket->connection, &socket->pipe, name.utf8_str(&socket->buffer), onConnection);
}