#ifndef HX_TELEMETRY_H
#define HX_TELEMETRY_H

#include <list>
#include <map>
#include <vector>

// --- Telemetry -----------------------------------------------------------------

int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations);
void __hxcpp_hxt_stash_telemetry();
Dynamic __hxcpp_hxt_dump_telemetry(int thread_num);
void __hxcpp_hxt_ignore_allocs(int delta);
int __hxcpp_hxt_dump_gctime();

int __hxcpp_gc_reserved_bytes();

#endif
