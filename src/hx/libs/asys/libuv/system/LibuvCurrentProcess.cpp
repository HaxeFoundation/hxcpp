#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"
#include "../stream/ReadablePipe.h"
#include "../stream/WritablePipe.h"
#include "../filesystem/LibuvFile.h"

int hx::asys::system::CurrentProcess_obj::pid()
{
	return uv_os_getpid();
}

//void hx::asys::system::CurrentProcess_obj::setSignalAction(hx::EnumBase signal, hx::EnumBase action)
//{
//	//
//}