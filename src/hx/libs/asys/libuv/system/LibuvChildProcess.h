#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include <optional>
#include "../LibuvUtils.h"
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::system
{
	class LibuvChildProcess final : public hx::asys::system::ChildProcess_obj
	{
	public:
		struct Ctx {
			uv_process_t request;
			uv_process_options_t options;
			std::vector<char*> arguments;
			std::vector<char*> environment;
			std::array<uv_stdio_container_t, 3> containers;
			std::optional<int64_t> currentExitCode;
			hx::RootedObject<hx::Object> exitCallback;

			Ctx();
		};

		Ctx* ctx;

		LibuvChildProcess(Ctx* ctx, Writable oStdin, Readable oStdout, Readable oStderr);

		Pid pid() override;

		void sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure) override;

		void exitCode(Dynamic cbSuccess, Dynamic cbFailure) override;

		void close(Dynamic cbSuccess, Dynamic cbFailure) override;
	};
}