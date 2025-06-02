#ifndef HX_TELEMETRY_TRACY_H
#define HX_TELEMETRY_TRACY_H

#ifndef HXCPP_TRACY
	#error "Error: HXCPP_TRACY must be defined."
#endif

#define TRACY_ENABLE
#include <hxcpp.h>
#include "../../project/thirdparty/tracy-0.12.0/tracy/TracyC.h"
#include "../../project/thirdparty/tracy-0.12.0/tracy/Tracy.hpp"

#ifdef HXCPP_TRACY_MEMORY
	#ifdef HXCPP_GC_MOVING
		#error "Error: HXCPP_TRACY_MEMORY is not supported when HXCPP_GC_MOVING is active."
	#endif
	#ifdef HXCPP_GC_GENERATIONAL
		#error "Error: HXCPP_TRACY_MEMORY is not supported when HXCPP_GC_GENERATIONAL is active."
	#endif
#endif

#ifdef HXCPP_TRACY_INCLUDE_CALLSTACKS
#define HXCPP_TRACY_ZONE(name) \
	::hx::strbuf TracyConcat(_hx_tracy_str_buffer, TracyLine); \
	int TracyConcat(_hx_tracy_str_length, TracyLine); \
	const char *TracyConcat(_hx_tracy_str_buffer_ptr, TracyLine) = name.utf8_str(&TracyConcat(_hx_tracy_str_buffer, TracyLine), false, &TracyConcat(_hx_tracy_str_length, TracyLine)); \
	::tracy::ScopedZone TracyConcat(_hx_tracy_scoped_zone,TracyLine)(_hx_stackframe.lineNumber, _hx_stackframe.position->fileName, strlen(_hx_stackframe.position->fileName), _hx_stackframe.position->fullName, strlen(_hx_stackframe.position->fullName), TracyConcat(_hx_tracy_str_buffer_ptr, TracyLine), TracyConcat(_hx_tracy_str_length, TracyLine), __hxcpp_tracy_get_zone_count());
#else
#define HXCPP_TRACY_ZONE(name) \
	::hx::strbuf TracyConcat(_hx_tracy_str_buffer, TracyLine); \
	int TracyConcat(_hx_tracy_str_length, TracyLine); \
	const char *TracyConcat(_hx_tracy_str_buffer_ptr, TracyLine) = name.utf8_str(&TracyConcat(_hx_tracy_str_buffer, TracyLine), false, &TracyConcat(_hx_tracy_str_length, TracyLine)); \
	::tracy::ScopedZone TracyConcat(_hx_tracy_scoped_zone,TracyLine)(_hx_stackframe.lineNumber, _hx_stackframe.position->fileName, strlen(_hx_stackframe.position->fileName), _hx_stackframe.position->fullName, strlen(_hx_stackframe.position->fullName), TracyConcat(_hx_tracy_str_buffer_ptr, TracyLine), TracyConcat(_hx_tracy_str_length, TracyLine), -1);
#endif

void __hxcpp_tracy_framemark();
void __hxcpp_tracy_plot(::String name, ::Float val);
void __hxcpp_tracy_plot_config(::String name, uint8_t format, bool step, bool fill, int color);
void __hxcpp_tracy_message(::String msg, int color);
void __hxcpp_tracy_message_app_info(::String info);
void __hxcpp_tracy_set_thread_name_and_group(String name, int groupHint);
int __hxcpp_tracy_get_zone_count();

#endif
