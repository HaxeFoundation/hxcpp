#ifndef HX_TELEMETRY_H
#define HX_TELEMETRY_H

#include <hxcpp.h>
#include <vector>

struct TelemetryFrame
{
  // microseconds, always valid
  int gctime;

  // Valid only if profiler is enabled
  std::vector<int> *samples;
  std::vector<const char*> *names;

  // Valid only if allocations (and profiler) are enabled
  std::vector<int> *allocations;
  std::vector<int> *collections;
  std::vector<int> *stacks;
};

// --- Telemetry -----------------------------------------------------------------

int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations);
void __hxcpp_hxt_stash_telemetry();
TelemetryFrame* __hxcpp_hxt_dump_telemetry(int thread_num);
void __hxcpp_hxt_ignore_allocs(int delta);
int __hxcpp_hxt_dump_gctime();

int __hxcpp_gc_reserved_bytes();

#endif
