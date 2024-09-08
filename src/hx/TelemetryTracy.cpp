#include <hxcpp.h>

#define TRACY_ENABLE
#include "../../project/thirdparty/tracy-0.11.1/tracy/TracyC.h"
#include "../../project/thirdparty/tracy-0.11.1/tracy/Tracy.hpp"
#include <vector>

namespace
{
	TracyCZoneCtx gcZone;

	___tracy_source_location_data gcSourceLocation = { "GC", "__hxt_gc_start",  TracyFile, (uint32_t)TracyLine, 0 };
}

namespace hx
{
	class Telemetry
	{
	public:
		std::vector<TracyCZoneCtx> tracyZones;

		Telemetry() : tracyZones(0) {}
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

void __hxt_gc_realloc(void* oldObj, void* newObj, int newSize) { }

void __hxt_gc_after_mark(int byteMarkId, int endianByteMarkId) { }

void __hxt_gc_start()
{
	//gcZone = ___tracy_emit_zone_begin(&gcSourceLocation, true);
}

void __hxt_gc_end()
{
	//___tracy_emit_zone_end(gcZone);
}

hx::Telemetry* hx::tlmCreate(StackContext* stack)
{
	TracyNoop;

	return new hx::Telemetry();
}

void hx::tlmDestroy(hx::Telemetry* telemetry)
{
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
			frame->position->fileName,
			strlen(frame->position->fileName),
			0);

	// Hxcpp callstack depth starts at 0, but tracy expects it to start at 1.
	auto depth = frame->ctx->getDepth() + 1;

	telemetry->tracyZones.push_back(___tracy_emit_zone_begin_alloc_callstack(srcloc, depth, true));
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