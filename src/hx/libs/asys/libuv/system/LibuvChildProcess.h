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
		std::optional<int64_t> currentExitCode;
		hx::Object* exitCallback;

		LibuvChildProcess();
		~LibuvChildProcess();

		int pid() override final;

		void exitCode(Dynamic cbSuccess, Dynamic cbFailure) override final;
	};
}