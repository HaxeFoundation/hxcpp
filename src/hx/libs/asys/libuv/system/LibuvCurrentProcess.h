#pragma once

#include <hxcpp.h>
#include <deque>
#include <array>
#include <optional>
#include <map>
#include "../LibuvUtils.h"
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::system
{
	class LibuvCurrentProcess final : public hx::asys::system::CurrentProcess_obj
	{
	public:
		struct SignalHandler
		{
			hx::RootedObject<hx::Object> callback;
			uv_signal_t* signal;

			SignalHandler(Dynamic cb);
			~SignalHandler();
		};

		struct Ctx
		{
			uv_loop_t* loop;
			std::array<uv_tty_t, 3> ttys;
			std::map<int, std::unique_ptr<SignalHandler>> signals;

			hx::asys::libuv::stream::StreamReader_obj::Ctx reader;

			Ctx(uv_loop_t* loop);
		};

		Ctx* ctx;

		LibuvCurrentProcess(Ctx* ctx);

		Pid pid() override;

		void sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure) override;

		void setSignalAction(hx::EnumBase signal, hx::EnumBase action);

		void __Mark(hx::MarkContext* __inCtx) override;
#ifdef HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}