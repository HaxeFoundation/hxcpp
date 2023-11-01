#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "LibuvChildProcess.h"

hx::asys::libuv::system::LibuvChildProcess::LibuvChildProcess()
	: request(std::move(std::make_unique<uv_process_t>()))
	, options(std::move(std::make_unique<uv_process_options_t>()))
	, exitCallback(null())
	, closeCallback(null())
	, containers(3)
{
	hx::GCSetFinalizer(this, [](hx::Object* obj) -> void {
		reinterpret_cast<LibuvChildProcess*>(obj)->~LibuvChildProcess();
	});
}

int hx::asys::libuv::system::LibuvChildProcess::pid()
{
	return request->pid;
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
		signum = 19;
		break;
	case 4:
		signum = 18;
		break;
	default:
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));
		return;
	}

	auto result = 0;
	if ((result = uv_process_kill(request.get(), signum)) < 0)
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
	if (currentExitCode.has_value())
	{
		cbSuccess(static_cast<int>(currentExitCode.value()));
	}
	else
	{
		exitCallback = cbSuccess.mPtr;
	}
}

void hx::asys::libuv::system::LibuvChildProcess::close(Dynamic cbSuccess, Dynamic cbFailure)
{
	closeCallback = cbSuccess.mPtr;

	uv_close(reinterpret_cast<uv_handle_t*>(request.get()), [](uv_handle_t* handle) {
		auto gcZone   = hx::AutoGCZone();
		auto process  = std::unique_ptr<hx::RootedObject<hx::asys::libuv::system::LibuvChildProcess>>(reinterpret_cast<hx::RootedObject<hx::asys::libuv::system::LibuvChildProcess>*>(handle->data));
		auto callback = Dynamic(process->rooted->closeCallback);

		if (null() != callback)
		{
			callback();
		}
	});
}

void hx::asys::libuv::system::LibuvChildProcess::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(exitCallback);
	HX_MARK_MEMBER(closeCallback);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::system::LibuvChildProcess::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(exitCallback);
	HX_VISIT_MEMBER(closeCallback);
}
#endif