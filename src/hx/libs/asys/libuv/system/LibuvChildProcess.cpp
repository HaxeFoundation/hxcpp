#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "LibuvChildProcess.h"

hx::asys::libuv::system::LibuvChildProcess::Ctx::Ctx()
	: request()
	, options()
	, arguments()
	, environment()
	, containers()
	, currentExitCode()
	, exitCallback(null())
{
}

hx::asys::libuv::system::LibuvChildProcess::LibuvChildProcess(Ctx* ctx, Writable oStdin, Readable oStdout, Readable oStderr) : ctx(ctx)
{
	HX_OBJ_WB_NEW_MARKED_OBJECT(this);

	stdio_in  = oStdin;
	stdio_out = oStdout;
	stdio_err = oStderr;
}

hx::asys::Pid hx::asys::libuv::system::LibuvChildProcess::pid()
{
	return ctx->request.pid;
}

void hx::asys::libuv::system::LibuvChildProcess::sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto signum = 0;
	switch (signal->_hx_getIndex())
	{
	case 0:
		signum = SIGTERM;
		break;
	case 1:
		signum = SIGKILL;
		break;
	case 2:
		signum = SIGINT;
		break;
	case 3:
#if HX_WINDOWS
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));

		return;
#else
		signum = SIGSTOP;
#endif
		break;
	case 4:
#if HX_WINDOWS
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));

		return;
#else
		signum = SIGCONT;
#endif
		break;
	case 5:
#if HX_WINDOWS
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));

		return;
#else
		signum = hx::asys::libuv::toPosixCode(signal->_hx_getInt(5));
#endif
		break;
	default:
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));
		return;
	}

	auto result = 0;
	if ((result = uv_process_kill(&ctx->request, signum)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess();
	}
}

void hx::asys::libuv::system::LibuvChildProcess::exitCode(Dynamic cbSuccess, Dynamic cbFailure)
{
	if (ctx->currentExitCode.has_value())
	{
		cbSuccess(static_cast<int>(ctx->currentExitCode.value()));
	}
	else
	{
		ctx->exitCallback.rooted = cbSuccess.mPtr;
	}
}

void hx::asys::libuv::system::LibuvChildProcess::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	uv_close(reinterpret_cast<uv_handle_t*>(&ctx->request), [](uv_handle_t* handle) {
		delete reinterpret_cast<hx::asys::libuv::system::LibuvChildProcess::Ctx*>(handle->data);
	});

	cbSuccess();
}