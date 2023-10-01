#include <hxcpp.h>
#include <vector>
#include <deque>
#include <array>
#include <memory>
#include "LibuvChildProcess.h"

hx::asys::libuv::system::LibuvChildProcess::LibuvChildProcess()
	: exitCallback(nullptr)
{
	hx::GCAddRoot(&exitCallback);
}

hx::asys::libuv::system::LibuvChildProcess::~LibuvChildProcess()
{
	hx::GCRemoveRoot(&exitCallback);
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