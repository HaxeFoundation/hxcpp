#ifndef HX_TELEMETRY_H
#define HX_TELEMETRY_H

#include <list>
#include <map>
#include <vector>

// --- Telemetry -----------------------------------------------------------------

  struct AllocsPerStackId
  {
    const char *type;
    int stackid;
    int size;
    std::vector<unsigned long> ids; // TODO: intptr_t, __hxcpp_obj_id ?
  };

  struct TelemetryFrame
  {
    // Array<int> samples;
    std::vector<int> *samples;
    std::map<int, AllocsPerStackId> *allocations;
    std::vector<unsigned long> *collections;
    double gctime;
    std::vector<const char *> names;
    std::vector<int> stacks;

    // int namesStart;
    // int namesEnd;
    // int stackIdStart;
    // int stackIdEnd;
  };


int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations);
void __hxcpp_hxt_stash_telemetry();
TelemetryFrame* __hxcpp_hxt_dump_telemetry(int thread_num);
void __hxcpp_hxt_ignore_allocs(int delta);
int __hxcpp_hxt_dump_gctime();

int __hxcpp_gc_reserved_bytes();

#endif
