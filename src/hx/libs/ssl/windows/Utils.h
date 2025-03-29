#pragma once

#include <hxcpp.h>
#include <winbase.h>

namespace hx
{
	namespace ssl
	{
		namespace windows
		{
			namespace utils
			{
				String Win32ErrorToString(DWORD error);

				String HResultErrorToString(HRESULT error);

				String NTStatusErrorToString(NTSTATUS error);
			}
		}
	}
}