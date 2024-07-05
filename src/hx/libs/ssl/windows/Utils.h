#pragma once
#include <hxcpp.h>
#include <Windows.h>

namespace hx::ssl::windows::utils
{
	String Win32ErrorToString(DWORD error)
	{
		auto messageBuffer = LPSTR{ nullptr };

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer,
			0,
			NULL);

		auto hxStr = String::create(messageBuffer);

		LocalFree(messageBuffer);

		return hxStr;
	}
}