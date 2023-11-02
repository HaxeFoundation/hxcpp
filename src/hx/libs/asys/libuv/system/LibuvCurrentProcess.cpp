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
