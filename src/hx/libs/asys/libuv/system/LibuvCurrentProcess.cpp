#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"
#include "../filesystem/LibuvFile.h"
#include "LibuvCurrentProcess.h"

namespace
{
	int getSignalId(hx::EnumBase signal)
	{
		switch (signal->_hx_getIndex())
		{
		case 0:
			return SIGTERM;
		case 1:
			return SIGKILL;
		case 2:
			return SIGINT;
		case 3:
#if HX_WINDOWS
			hx::Throw(HX_CSTRING("Platform not supported"));
#else
			return SIGSTOP;
#endif
		case 4:
#if HX_WINDOWS
			hx::Throw(HX_CSTRING("Platform not supported"));
#else
			return SIGCONT;
#endif
		case 5:
#if HX_WINDOWS
			hx::Throw(HX_CSTRING("Platform not supported"));
#else
			return hx::asys::libuv::toPosixCode(signal->_hx_getInt(0));
#endif
		}
	}

	void onAlloc(uv_handle_t* handle, const size_t suggested, uv_buf_t* buffer)
	{
		auto  ctx     = static_cast<hx::asys::libuv::system::LibuvCurrentProcess::Ctx*>(handle->data);
		auto& staging = ctx->reader.staging.emplace_back(suggested);

		buffer->base = staging.data();
		buffer->len  = staging.size();
	}

	void onRead(uv_stream_t* stream, const ssize_t len, const uv_buf_t* read)
	{
		auto gc = hx::AutoGCZone();
		auto ctx = static_cast<hx::asys::libuv::system::LibuvCurrentProcess::Ctx*>(stream->data);

		if (len <= 0)
		{
			ctx->reader.reject(len);

			return;
		}

		ctx->reader.buffer.insert(ctx->reader.buffer.end(), read->base, read->base + len);
		ctx->reader.consume();
	}
}

hx::asys::libuv::system::LibuvCurrentProcess::Ctx::Ctx(uv_loop_t* _loop)
	: loop(_loop)
	, signals()
	, ttys()
	, reader(reinterpret_cast<uv_stream_t*>(ttys.data()))
{
	reader.stream->data = this;
}

hx::asys::libuv::system::LibuvCurrentProcess::LibuvCurrentProcess(Ctx* _ctx)
	: CurrentProcess_obj(
		hx::asys::Readable(new hx::asys::libuv::stream::StreamReader_obj(&_ctx->reader, onAlloc, onRead)),
		hx::asys::Writable(new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(&_ctx->ttys.at(1)))),
		hx::asys::Writable(new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(&_ctx->ttys.at(2)))))
	, ctx(_ctx) {}

hx::asys::Pid hx::asys::libuv::system::LibuvCurrentProcess::pid()
{
	return uv_os_getpid();
}

void hx::asys::libuv::system::LibuvCurrentProcess::sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = 0;
	auto signalId = getSignalId(signal);
	if ((result = uv_kill(pid(), signalId)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess();
	}
}

void hx::asys::libuv::system::LibuvCurrentProcess::setSignalAction(hx::EnumBase signal, hx::EnumBase action)
{
	auto signalId = getSignalId(signal);
	auto actionId = action->_hx_getIndex();

	switch (actionId)
	{
	case 0:
	case 2:
	{
		auto& handler = ctx->signals[signalId] = std::make_unique<SignalHandler>(actionId == 0 ? null() : action->_hx_getObject(0));

		auto callback = [](uv_signal_t* handle, int signum) {
			auto gcZone = hx::AutoGCZone();
			auto signal = reinterpret_cast<SignalHandler*>(handle->data);

			if (hx::IsNotNull(signal->callback.rooted))
			{
				Dynamic(signal->callback.rooted)();
			}
		};

		if (uv_signal_init(ctx->loop, handler->signal) < 0)
		{
			hx::Throw(HX_CSTRING("Failed to init signal"));
		}

		if (uv_signal_start(handler->signal, callback, signalId) < 0)
		{
			hx::Throw(HX_CSTRING("Failed to start signal"));
		}

		break;
	}

	case 1:
	{
		ctx->signals.erase(signalId);
		break;
	}
	}
}

void hx::asys::libuv::system::LibuvCurrentProcess::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(stdio_in);
	HX_MARK_MEMBER(stdio_out);
	HX_MARK_MEMBER(stdio_err);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::system::LibuvCurrentProcess::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(stdio_in);
	HX_VISIT_MEMBER(stdio_out);
	HX_VISIT_MEMBER(stdio_err);
}
#endif

hx::asys::libuv::system::LibuvCurrentProcess::SignalHandler::SignalHandler(Dynamic cb)
	: callback(cb.mPtr)
	, signal(new uv_signal_t())
{
	signal->data = this;
}

hx::asys::libuv::system::LibuvCurrentProcess::SignalHandler::~SignalHandler()
{
	uv_signal_stop(signal);
	uv_close(reinterpret_cast<uv_handle_t*>(signal), [](uv_handle_t* handle) {
		delete reinterpret_cast<uv_signal_t*>(handle);
	});
}
