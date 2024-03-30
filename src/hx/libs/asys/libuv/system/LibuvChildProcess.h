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
		uv_process_t* request;
		uv_process_options_t* options;
		std::vector<char*>* arguments;
		std::vector<char*>* environment;
		std::vector<uv_stdio_container_t>* containers;
		std::optional<int64_t>* currentExitCode;

		Dynamic exitCallback;
		Dynamic closeCallback;

		LibuvChildProcess();
		~LibuvChildProcess() = default;

		Pid pid() override;

		void sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure) override;

		void exitCode(Dynamic cbSuccess, Dynamic cbFailure) override;

		void close(Dynamic cbSuccess, Dynamic cbFailure) override;

		void __Mark(hx::MarkContext* __inCtx) override;
#ifdef HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}