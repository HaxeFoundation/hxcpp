#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include <optional>
#include <unordered_map>
#include "../LibuvUtils.h"
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::system
{
	class LibuvCurrentProcess final : public hx::asys::system::CurrentProcess_obj
	{
	public:
		std::unique_ptr<std::array<uv_tty_t, 3>> ttys;

		std::unique_ptr<std::unordered_map<int, std::unique_ptr<hx::asys::libuv::BaseRequest>>> signalActions;

		LibuvAsysContext ctx;

		LibuvCurrentProcess(LibuvAsysContext ctx, std::unique_ptr<std::array<uv_tty_t, 3>> ttys);

		~LibuvCurrentProcess();

		int pid() override;

		void sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure) override;

		void setSignalAction(hx::EnumBase signal, hx::EnumBase action);

		void __Mark(hx::MarkContext* __inCtx) override;
#ifdef HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}