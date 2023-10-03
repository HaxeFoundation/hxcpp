#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "LibuvChildProcess.h"

hx::asys::libuv::system::LibuvChildProcess::LibuvChildProcess()
	: exitCallback(nullptr)
	, closeCallback(nullptr)
{
	hx::GCAddRoot(&exitCallback);
	hx::GCAddRoot(&closeCallback);
}

hx::asys::libuv::system::LibuvChildProcess::~LibuvChildProcess()
{
	hx::GCRemoveRoot(&exitCallback);
	hx::GCRemoveRoot(&closeCallback);
}

int hx::asys::libuv::system::LibuvChildProcess::pid()
{
	return request.pid;
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

	uv_close(reinterpret_cast<uv_handle_t*>(&request), [](uv_handle_t* handle) {
		auto gcZone   = hx::AutoGCZone();
		auto process  = reinterpret_cast<LibuvChildProcess*>(handle->data);
		auto callback = Dynamic(process->closeCallback);

		if (null() != callback)
		{
			callback();
		}
	});
}
