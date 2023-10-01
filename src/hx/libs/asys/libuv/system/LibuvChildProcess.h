#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include <optional>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::system
{
	class LibuvChildProcess : public hx::asys::system::ChildProcess
	{
	public:
		uv_process_t request;
		uv_process_options_t options;
		std::vector<char*> arguments;
		std::vector<char*> environment;
		std::optional<int64_t> exitCode;

		LibuvChildProcess() = default;
		~LibuvChildProcess() = default;
	};
}