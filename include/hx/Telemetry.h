#ifndef HX_TELEMETRY_H
#define HX_TELEMETRY_H

#define HX_TELEMETRY_VERSION 1

#include <hxcpp.h>
#include <vector>

struct TelemetryFrame
{
  // microseconds, always valid
  int gctime;
  int gcoverhead;

  // Valid only if profiler is enabled
  std::vector<int> *samples;
  std::vector<const char*> *names;

  // Valid only if allocations (and profiler) are enabled
  std::vector<int> *allocation_data;
  std::vector<int> *stacks;
};

// --- Telemetry -----------------------------------------------------------------

int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations);
void __hxcpp_hxt_stash_telemetry();
TelemetryFrame* __hxcpp_hxt_dump_telemetry(int thread_num);
void __hxcpp_hxt_ignore_allocs(int delta);

// expose these from GCInternal
int __hxcpp_gc_reserved_bytes();
int __hxcpp_gc_used_bytes();

#ifdef HXCPP_TRACY
  void __hxcpp_tracy_framemark();
  void __hxcpp_tracy_plot(String name, float val);
  void __hxcpp_tracy_plot_config(String name, uint8_t format, bool step, bool fill, int color);
  void __hxcpp_tracy_message(String msg, int color);
  void __hxcpp_tracy_message_app_info(String info);
  void __hxcpp_tracy_set_thread_name_and_group(String name, int groupHint);
#endif

#endif
