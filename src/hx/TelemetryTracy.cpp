#include <hxcpp.h>

#define TRACY_ENABLE
#include "../../project/thirdparty/tracy-0.11.1/tracy/TracyC.h"
#include "../../project/thirdparty/tracy-0.11.1/tracy/Tracy.hpp"
#include <vector>

namespace
{
	TracyCZoneCtx gcZone;

	___tracy_source_location_data gcSourceLocation = { "GC", "__hxt_gc_start",  TracyFile, (uint32_t)TracyLine, 0 };

	const char* sohName = "SOH";
	const char* lohName = "LOH";

	std::vector<hx::Telemetry*> created;

	bool isLargeObject(void* ptr)
	{
		auto ptrHeader = reinterpret_cast<size_t>(ptr) - sizeof(int);
		auto flags     = *reinterpret_cast<unsigned int*>(ptrHeader);

		return (flags & 0xffff) == 0;
	}
}

namespace hx
{
	class Telemetry
	{
	public:
		std::vector<TracyCZoneCtx> tracyZones;
		hx::QuickVec<void*> allocs;

		Telemetry() : tracyZones(0), allocs() {}
	};
}

int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations)
{
	hx::Throw(HX_CSTRING("Not implemented"));

	return 0;
}

TelemetryFrame* __hxcpp_hxt_dump_telemetry(int)
{
	hx::Throw(HX_CSTRING("Not implemented"));

	return 0;
}

void __hxcpp_hxt_stash_telemetry() { }

void __hxcpp_hxt_ignore_allocs(int) {}

void __hxt_new_string(void* obj, int inSize) { }

void __hxt_new_array(void* obj, int inSize) { }

void __hxt_new_hash(void* obj, int inSize) { }

void __hxt_gc_new(hx::StackContext* stack, void* obj, int inSize, const char* name) { }

void __hxt_gc_alloc(void* obj, int inSize)
{
	if (isLargeObject(obj))
	{
		TracyAllocN(obj, inSize, lohName);
	}
	else
	{
		TracyAllocN(obj, inSize, sohName);
	}

	hx::StackContext::getCurrent()->mTelemetry->allocs.push(obj);
}

void __hxt_gc_realloc(void* oldObj, void* newObj, int newSize) { }

void __hxt_gc_after_mark(int byteMarkId, int endianByteMarkId)
{
	for (auto&& telemetry : created)
	{
		hx::QuickVec<void*> retained;

		retained.safeReserveExtra(telemetry->allocs.size());

		for (auto i = 0; i < telemetry->allocs.size(); i++)
		{
			auto ptr      = telemetry->allocs[i];
			auto markByte = reinterpret_cast<unsigned char*>(ptr)[endianByteMarkId];
			if (markByte != byteMarkId)
			{
				if (isLargeObject(ptr))
				{
					TracyFreeN(ptr, lohName);
				}
				else
				{
					TracyFreeN(ptr, sohName);
				}
			}
			else
			{
				retained.push(ptr);
			}
		}

		telemetry->allocs.swap(retained);
	}
}

void __hxt_gc_start()
{
	gcZone = ___tracy_emit_zone_begin(&gcSourceLocation, true);
}

void __hxt_gc_end()
{
	___tracy_emit_zone_end(gcZone);
}

hx::Telemetry* hx::tlmCreate(StackContext* stack)
{
	auto obj = new hx::Telemetry();

	created.push_back(obj);

	return obj;
}

void hx::tlmDestroy(hx::Telemetry* telemetry)
{
	created.erase(std::find(created.begin(), created.end(), telemetry));

	delete telemetry;
}

void hx::tlmAttach(hx::Telemetry* telemetry, hx::StackContext* stack)
{
	//
}

void hx::tlmDetach(hx::Telemetry* telemetry)
{
	//
}

void hx::tlmSampleEnter(hx::Telemetry* telemetry, hx::StackFrame* frame)
{
	auto srcloc =
		___tracy_alloc_srcloc(
			frame->lineNumber,
			frame->position->fileName,
			strlen(frame->position->fileName),
			frame->position->fullName,
			strlen(frame->position->fullName),
			0);

	#if HXCPP_TRACY_INCLUDE_CALLSTACKS
		// Determine depth from tracyZones vector: +1 since we are about to add one
		auto depth = telemetry->tracyZones.size();
		telemetry->tracyZones.push_back(___tracy_emit_zone_begin_alloc_callstack(srcloc, depth, true));
	#else
		telemetry->tracyZones.push_back(___tracy_emit_zone_begin_alloc(srcloc, true));
	#endif
}

void hx::tlmSampleExit(hx::Telemetry* telemetry)
{
	if (telemetry->tracyZones.empty())
	{
		return;
	}

	___tracy_emit_zone_end(telemetry->tracyZones.back());

	telemetry->tracyZones.pop_back();
}

void __hxcpp_tracy_framemark()
{
	::tracy::Profiler::SendFrameMark(0);
}

void __hxcpp_tracy_plot(String name, float val)
{
	::tracy::Profiler::PlotData(name.c_str(), val);
}

void __hxcpp_tracy_plot_config(String name, uint8_t format, bool step, bool fill, int color)
{
	::tracy::Profiler::ConfigurePlot(name.c_str(),::tracy::PlotFormatType(format), step, fill, color);
}

void __hxcpp_tracy_message(String msg, int color)
{
	::tracy::Profiler::MessageColor(msg.c_str(), msg.length, color, 0);
}

void __hxcpp_tracy_message_app_info(String info)
{
	::tracy::Profiler::MessageAppInfo(info.c_str(), info.length);
}