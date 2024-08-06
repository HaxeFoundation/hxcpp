#include <hxcpp.h>
#include <Windows.h>
#include <comdef.h>
#include <array>

#include "Utils.h"

String hx::ssl::windows::utils::Win32ErrorToString(DWORD error)
{
	hx::EnterGCFreeZone();

	auto buffer = std::array<char, 4096>();

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buffer.data(),
		buffer.size(),
		nullptr);

	hx::ExitGCFreeZone();

	return String::create(buffer.data());
}

String hx::ssl::windows::utils::HResultErrorToString(HRESULT error)
{
	hx::EnterGCFreeZone();
	auto str = _com_error(error).ErrorMessage();
	hx::ExitGCFreeZone();

	return String::create(str);
}

String hx::ssl::windows::utils::NTStatusErrorToString(NTSTATUS error)
{
	return HResultErrorToString(HRESULT_FROM_NT(error));
}
