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
	class LibuvCurrentProcess final : public hx::asys::system::CurrentProcess_obj
	{
	public:
		int pid() override;

		void sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure) override;

		void setSignalAction(hx::EnumBase signal, hx::EnumBase action);
	};
}