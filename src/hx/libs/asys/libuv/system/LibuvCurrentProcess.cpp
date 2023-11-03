#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"
#include "../stream/ReadablePipe.h"
#include "../stream/WritablePipe.h"
#include "../filesystem/LibuvFile.h"
#include "LibuvCurrentProcess.h"

int hx::asys::libuv::system::LibuvCurrentProcess::pid()
{
	return uv_os_getpid();
}

void hx::asys::libuv::system::LibuvCurrentProcess::sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure)
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
	if ((result = uv_kill(pid(), signum)) < 0)
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
	switch (action->_hx_getIndex())
	{
	case 0:
		break;

	case 1:
		break;

	case 2:
		break;
	}
}
