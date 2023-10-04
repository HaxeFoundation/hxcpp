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
		std::vector<uv_stdio_container_t> containers;
		std::optional<int64_t> currentExitCode;

		hx::Object* exitCallback;
		hx::Object* closeCallback;

		LibuvChildProcess();
		~LibuvChildProcess();

		int pid() override final;

		void exitCode(Dynamic cbSuccess, Dynamic cbFailure) override final;

		void close(Dynamic cbSuccess, Dynamic cbFailure) override final;
	};
}