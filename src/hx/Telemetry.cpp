#include <hxcpp.h>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <hx/Debug.h>
#include <hx/Thread.h>
#include <hx/Telemetry.h>
#include <hx/OS.h>


namespace hx
{


inline unsigned int __hxt_ptr_id(void* obj)
{
#if defined(HXCPP_M64)
   size_t h64 = (size_t)obj;
   // Note, using >> 1 since Strings can be small, down to 2 bytes, causing collisions
   return (unsigned int)(h64>>1) ^ (unsigned int)(h64>>32);
#else
   // Note, using >> 1 since Strings can be small, down to 2 bytes, causing collisions
   return ((unsigned int)obj) >> 1;
#endif
}

struct AllocStackIdMapEntry
{
   int terminationStackId;
   std::map<int, AllocStackIdMapEntry*> children;
};


// Telemetry functionality
class Telemetry
{
public:

    Telemetry(StackContext *inStack, bool profiler_en, bool allocs_en)
        : mT0(0)
    {
        stack = inStack;
        names.push_back("1-indexed");
        namesStashed = 1;
        ignoreAllocs = allocs_en ? 0 : 1;
        allocStacksStashed = 0;
        allocStackIdNext = 0;
        allocStackIdMapRoot.terminationStackId = 0;
        gcTimer = 0;
        gcTimerTemp = 0;
        gcOverhead = 0;
        _last_obj = 0;

        profiler_enabled = profiler_en;
        allocations_enabled = profiler_en && allocs_en;

        samples = 0;
        allocation_data = 0;

        // Push a blank (destroyed on first Dump)
        Stash();

        // When a profiler exists, the profiler thread needs to exist
        gThreadMutex.Lock();
   
        gThreadRefCount += 1;
        if (gThreadRefCount == 1) {
            HxCreateDetachedThread(ProfileMainLoop, 0);
        }

        gThreadMutex.Unlock();
    }

    ~Telemetry()
    {
        gThreadMutex.Lock();

        gThreadRefCount -= 1;

        gThreadMutex.Unlock();
    }

    // todo
    void attach(StackContext *ctx) { stack = ctx; }
    void detach() { }

    void StackUpdate(StackFrame *frame);
    inline void sampleEnter(StackFrame *frame)
    {
       StackUpdate(frame);
    }
    inline void sampleExit()
    {
       StackUpdate(0);
    }

    void HXTAllocation(void* obj, size_t inSize, const char* type=0);
    void HXTRealloc(void* old_obj, void* new_obj, int new_Size);

    void Stash()
    {
      TelemetryFrame *stash = new TelemetryFrame();

      IgnoreAllocs(1);

      stash->gctime = gcTimer*1000000; // usec
      gcTimer = 0;

      stash->gcoverhead = gcOverhead*1000000; // usec
      gcOverhead = 0;

      alloc_mutex.Lock();

      if (_last_obj!=0) lookup_last_object_type();

      stash->allocation_data = allocation_data;
      stash->samples = samples;

      samples = profiler_enabled ? new std::vector<int>() : 0;
      if (allocations_enabled) {
        allocation_data = new std::vector<int>();
      }

      alloc_mutex.Unlock();

      int i,size;
      stash->names = 0;
      if (profiler_enabled) {
        stash->names = new std::vector<const char*>();
        size = names.size();
        for (i=namesStashed; i<size; i++) {
          stash->names->push_back(names.at(i));
        }
        //printf("Stash pushed %d names, %d\n", (size-namesStashed), stash->names->size());
        namesStashed = names.size();
      }

      stash->stacks = 0;
      if (allocations_enabled) {
        stash->stacks = new std::vector<int>();
        size = allocStacks.size();
        for (i=allocStacksStashed; i<size; i++) {
          stash->stacks->push_back(allocStacks.at(i));
        }
        allocStacksStashed = allocStacks.size();
      }

      gStashMutex.Lock();
      stashed.push_back(*stash);
      gStashMutex.Unlock();

      IgnoreAllocs(-1);
    }

    TelemetryFrame* Dump()
    {
      gStashMutex.Lock();
      if (stashed.size()<1) {
        gStashMutex.Unlock();
        return 0;
      }

      // Destroy item that was dumped last call
      TelemetryFrame *front = &stashed.front();
      if (front->samples!=0) delete front->samples;
      if (front->names!=0) delete front->names;
      if (front->allocation_data!=0) delete front->allocation_data;
      if (front->stacks!=0) delete front->stacks;
      //delete front; // delete happens via pop_front:
      stashed.pop_front(); // Destroy item that was Dumped last call

      front = &stashed.front();
      gStashMutex.Unlock();

      //printf(" -- dumped stash, allocs=%d, alloc[max]=%d\n", front->allocations->size(), front->allocations->size()>0 ? front->allocations->at(front->allocations->size()-1) : 0);

      return front;
    }

    void IgnoreAllocs(int delta)
    {
        ignoreAllocs += delta;
    }

    void GCStart()
    {
        gcTimerTemp = __hxcpp_time_stamp();
    }

    void GCEnd()
    {
        gcTimer += __hxcpp_time_stamp() - gcTimerTemp;
    }

    void lookup_last_object_type()
    {
      if (_last_obj==0) return;

      const char* type = "_uninitialized";

      int obj_id = __hxt_ptr_id(_last_obj);
      alloc_mutex.Lock();
      std::map<void*, hx::Telemetry*>::iterator exist = alloc_map.find(_last_obj);
      if (exist != alloc_map.end() && _last_obj!=(NULL)) {
        type = "_unknown";
        int vtt = _last_obj->__GetType();
        if (vtt==vtInt || vtt==vtFloat || vtt==vtBool) type = "_non_class";
        else if (vtt==vtObject ||
                 vtt==vtString ||
                 vtt==vtArray ||
                 vtt==vtClass ||
                 vtt==vtFunction ||
                 vtt==vtEnum ||
                 vtt==vtAbstractBase) {
          //printf("About to resolve...\n");
          type = _last_obj->__GetClass()->mName; //__CStr();
          //printf("Updating last allocation %016lx type to %s\n", _last_obj, type);
        }
      }
      alloc_mutex.Unlock();
      allocation_data->at(_last_loc+2) = GetNameIdx(type);
      _last_obj = 0;
    }

    void reclaim(int id)
    {
      if (!allocations_enabled) return;

      allocation_data->push_back(1); // collect flag
      allocation_data->push_back(id);
    }

    static void HXTReclaimInternal(void* obj)
    {
      int obj_id = __hxt_ptr_id(obj);
      std::map<void*, hx::Telemetry*>::iterator exist = alloc_map.find(obj);
      if (exist != alloc_map.end()) {
        Telemetry* telemetry = exist->second;
        if (telemetry) {
          telemetry->reclaim(obj_id);
          alloc_map.erase(exist);
          //printf("Tracking collection %016lx, id=%016lx\n", obj, obj_id);
        } else {
          printf("HXT ERR: we shouldn't get: Telemetry lookup failed!\n");
        }
      } else {
        // Ignore, assume object was already reclaimed
        //printf("HXT ERR: we shouldn't get: Reclaim a non-tracked object %016lx, id=%016lx -- was there an object ID collision?\n", obj, obj_id);
      }
    }

    static void HXTAfterMark(int gByteMarkID, int ENDIAN_MARK_ID_BYTE)
    {
      double t0 = __hxcpp_time_stamp();

      Telemetry* telemetry = 0;
      alloc_mutex.Lock();
      std::map<void*, hx::Telemetry*>::iterator iter = alloc_map.begin();
      while (iter != alloc_map.end()) {
        void* obj = iter->first;
        unsigned char mark = ((unsigned char *)obj)[ENDIAN_MARK_ID_BYTE];
        if ( mark!=gByteMarkID ) {
          // not marked, deallocated
          telemetry = iter->second;
          if (telemetry) {
            int obj_id = __hxt_ptr_id(obj);
            telemetry->reclaim(obj_id);
          }
          alloc_map.erase(iter++);
        } else {
          iter++;
        }
      }
      alloc_mutex.Unlock();

      // Report overhead on one of the telemetry instances
      // TODO: something better?
      if (telemetry) {
        telemetry->gcOverhead += __hxcpp_time_stamp() - t0;
      }
    }

private:

    void push_callstack_ids_into(std::vector<int> *list);
    int GetNameIdx(const char *fullName);
    int ComputeCallStackId();

    static THREAD_FUNC_TYPE ProfileMainLoop(void *)
    {
        int millis = 1;

        while (gThreadRefCount > 0) { 
#ifdef HX_WINDOWS
            Sleep(millis);
#else
            struct timespec t;
            struct timespec tmp;
            t.tv_sec = 0;
            t.tv_nsec = millis * 1000000;
            nanosleep(&t, &tmp);
#endif

            int count = gProfileClock + 1;
            gProfileClock = (count < 0) ? 0 : count;
        }

        THREAD_FUNC_RET
    }

    StackContext *stack;
    int mT0;

    std::list<TelemetryFrame> stashed;

    std::map<const char *, int> nameMap;
    std::vector<const char *> names;
    std::vector<int> *samples;
    int namesStashed;

    bool profiler_enabled;
    bool allocations_enabled;

    std::vector<int> allocStacks;
    int allocStacksStashed;
    int allocStackIdNext;
    AllocStackIdMapEntry allocStackIdMapRoot;

    double gcTimer;
    double gcTimerTemp;
    double gcOverhead;

    int ignoreAllocs;

    hx::Object* _last_obj;
    int _last_loc;

    std::vector<int> *allocation_data;

    static  HxMutex gStashMutex;
    static HxMutex gThreadMutex;
    static int gThreadRefCount;
    static int gProfileClock;

    static HxMutex alloc_mutex;
    static std::map<void*, Telemetry*> alloc_map;
};
/* static */ HxMutex Telemetry::gStashMutex;
/* static */ HxMutex Telemetry::gThreadMutex;
/* static */ int Telemetry::gThreadRefCount;
/* static */ int Telemetry::gProfileClock;
/* static */ HxMutex Telemetry::alloc_mutex;
/* static */ std::map<void*, Telemetry*> Telemetry::alloc_map;



Telemetry *tlmCreate(StackContext *inStack)
{
   return new Telemetry(inStack, true, false);
}

void tlmDestroy(Telemetry *inTelemetry)
{
   delete inTelemetry;
}

void tlmAttach(Telemetry *inTelemetry, StackContext *inStack)
{
   inTelemetry->attach(inStack);
}

void tlmDetach(Telemetry *inTelemetry)
{
   inTelemetry->detach();
}

void tlmSampleEnter(Telemetry *inTelemetry, StackFrame *inFrame)
{
   inTelemetry->sampleEnter(inFrame);
}

void tlmSampleExit(Telemetry *inTelemetry)
{
   inTelemetry->sampleExit();
}





void hx::Telemetry::push_callstack_ids_into(std::vector<int> *list)
{
    int size = stack->getDepth();
    for (int i = 0; i < size; i++) {
        const char *fullName = stack->getFullNameAtDepth(i);
        list->push_back(GetNameIdx(fullName));
    }
}

int hx::Telemetry::GetNameIdx(const char *fullName) {
  int idx = nameMap[fullName];
  if (idx==0) {
    idx = names.size();
    nameMap[fullName] = idx;
    names.push_back(fullName);
  }
  return idx;
}

int hx::Telemetry::ComputeCallStackId() {
    std::vector<int> callstack;
    int stackId;

    push_callstack_ids_into(&callstack);
    int size = callstack.size();

    AllocStackIdMapEntry *asime = &allocStackIdMapRoot;

    int i=0;
    while (i<size) {
        int name_id = callstack.at(i++);
        //printf("Finding child with id=%d, asime now %#010x\n", name_id, asime);
        std::map<int, AllocStackIdMapEntry*>::iterator lb = asime->children.lower_bound(name_id);
         
        if (lb != asime->children.end() && !(asime->children.key_comp()(name_id, lb->first)))
        {   // key already exists
            asime = lb->second;
        } else {
            // the key does not exist in the map, add it
            AllocStackIdMapEntry *newEntry = new AllocStackIdMapEntry();
            newEntry->terminationStackId = -1;
            asime->children.insert(lb, std::map<int, AllocStackIdMapEntry*>::value_type(name_id, newEntry));
            asime = newEntry;
        }
    }

    if (asime->terminationStackId == -1) {
        // This is a new stackId, store call stack id's in allocStacks
        stackId = asime->terminationStackId = allocStackIdNext;
        allocStacks.push_back(size);
        int i = size-1;
        while (i>=0) allocStacks.push_back(callstack.at(i--));
        //printf("new callstackid %d\n", allocStackIdNext);
        allocStackIdNext++;
    } else {
        stackId = asime->terminationStackId;
        //printf("existing callstackid %d\n", stackId);
    }

    return stackId;
}

void hx::Telemetry::StackUpdate(StackFrame *pushed_frame)
{
    if (mT0 == gProfileClock || !profiler_enabled) {
        return;
    }

    // Latch the profile clock and calculate the time since the last profile
    // clock tick
    int clock = gProfileClock;
    int delta = clock - mT0;
    if (delta < 0) {
        delta = 1;
    }
    mT0 = clock;

    int size = stack->getDepth();

    // Collect function names and callstacks (as indexes into the names vector)
    samples->push_back(size);
    push_callstack_ids_into(samples);
    samples->push_back(delta);
}

void hx::Telemetry::HXTAllocation(void* obj, size_t inSize, const char* type)
{
    if (ignoreAllocs>0 || !allocations_enabled) return;

    // Optionally ignore from extern::cffi - very expensive to track allocs
    // for every external call, hashes for every SDL event (Lime's
    // ExternalInterface.external_handler()), etc
#ifndef HXCPP_PROFILE_EXTERNS
    if (stack->getCurrentStackFrame()->position->className==hx::EXTERN_CLASS_NAME) {
      alloc_mutex.Unlock();
      return;
    }
#endif

    int obj_id = __hxt_ptr_id(obj);

    alloc_mutex.Lock();

    // HXT debug: Check for id collision
#ifdef HXCPP_TELEMETRY_DEBUG
    std::map<void*, hx::Telemetry*>::iterator exist = alloc_map.find(obj);
    if (exist != alloc_map.end()) {
      printf("HXT ERR: Object id collision! at on %016lx, id=%016lx\n", obj, obj_id);
      throw "uh oh";
      alloc_mutex.Unlock();
      return;
    }
#endif

    int stackid = ComputeCallStackId();

    if (_last_obj!=0) lookup_last_object_type();
    if (type==0) {
      _last_obj = (hx::Object*)obj;
      _last_loc = allocation_data->size();
      type = "_unresolved";
    }

    allocation_data->push_back(0); // alloc flag
    allocation_data->push_back(obj_id);
    allocation_data->push_back(GetNameIdx(type)); // defer lookup
    allocation_data->push_back((int)inSize);
    allocation_data->push_back(stackid);

    alloc_map[obj] = this;

    //__hxcpp_set_hxt_finalizer(obj, (void*)Telemetry::HXTReclaim);

    //printf("Tracking alloc %s at %016lx, id=%016lx, s=%d for telemetry %016lx, ts=%f\n", type, obj, obj_id, inSize, this, __hxcpp_time_stamp());

    alloc_mutex.Unlock();
}

void hx::Telemetry::HXTRealloc(void* old_obj, void* new_obj, int new_size)
{
    if (!allocations_enabled) return;
    int old_obj_id = __hxt_ptr_id(old_obj);
    int new_obj_id = __hxt_ptr_id(new_obj);

    alloc_mutex.Lock();

    // Only track reallocations of objects currently known to be allocated
    std::map<void*, hx::Telemetry*>::iterator exist = alloc_map.find(old_obj);
    if (exist != alloc_map.end()) {
      Telemetry* t = exist->second;
      t->allocation_data->push_back(2); // realloc flag (necessary?)
      t->allocation_data->push_back(old_obj_id);
      t->allocation_data->push_back(new_obj_id);
      t->allocation_data->push_back(new_size);

      //printf("Object at %016lx moving to %016lx, new_size = %d bytes\n", old_obj, new_obj, new_size);

      // HXT debug: Check for id collision
#ifdef HXCPP_TELEMETRY_DEBUG
      std::map<void*, hx::Telemetry*>::iterator exist_new = alloc_map.find(new_obj);
      if (exist_new != alloc_map.end()) {
        printf("HXT ERR: Object id collision (reloc)! at on %016lx, id=%016lx\n", (unsigned long)new_obj, (unsigned long)new_obj_id);
        throw "uh oh";
      }
#endif

      //__hxcpp_set_hxt_finalizer(old_obj, (void*)0); // remove old finalizer -- should GCInternal.InternalRealloc do this?
      HXTReclaimInternal(old_obj); // count old as reclaimed
    } else {
      //printf("Not tracking re-alloc of untracked %016lx, id=%016lx\n", old_obj, old_obj_id);
      alloc_mutex.Unlock();
      return;
    }

    alloc_map[new_obj] = this;
    //__hxcpp_set_hxt_finalizer(new_obj, (void*)HXTReclaim);

    //printf("Tracking re-alloc from %016lx, id=%016lx to %016lx, id=%016lx at %f\n", old_obj, old_obj_id, new_obj, new_obj_id, __hxcpp_time_stamp());

    alloc_mutex.Unlock();
}

} // end namespace hx

// These globals are called by HXTelemetry.hx

int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations)
{
#ifdef HXCPP_STACK_TRACE
  hx::StackContext *stack = hx::StackContext::getCurrent();
  delete stack->mTelemetry;
  stack->mTelemetry = new hx::Telemetry(stack, profiler, allocations);
  return stack->mThreadId;
#else
 return 0;
#endif
}

void __hxcpp_hxt_stash_telemetry()
{
#ifdef HXCPP_STACK_TRACE
 // Operates on the current thread, no mutexes needed
  hx::StackContext *stack = hx::StackContext::getCurrent();
  if (stack->mTelemetry)
     stack->mTelemetry->Stash();
#endif
}

// Operates on the socket writer thread
TelemetryFrame* __hxcpp_hxt_dump_telemetry(int thread_num)
{
#ifdef HXCPP_STACK_TRACE
  hx::StackContext *stack = hx::StackContext::getStackForId(thread_num);
  if (!stack || !stack->mTelemetry)
     return 0;
  return stack->mTelemetry->Dump();
#else
 return 0;
#endif
}

void __hxcpp_hxt_ignore_allocs(int delta)
{
#ifdef HXCPP_STACK_TRACE
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->IgnoreAllocs(delta);
#endif
}


// These globals are called by other cpp files




void __hxt_new_string(void* obj, int inSize)
{
  #ifdef HXCPP_STACK_TRACE
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->HXTAllocation(obj, inSize, (const char *)"String");
  #endif
}
void __hxt_new_array(void* obj, int inSize)
{
  #ifdef HXCPP_STACK_TRACE
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->HXTAllocation(obj, inSize, (const char *)"Array");
  #endif
}
void __hxt_new_hash(void* obj, int inSize)
{
  #ifdef HXCPP_STACK_TRACE
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->HXTAllocation(obj, inSize, (const char *)"Hash");
  #endif
}
void __hxt_gc_new(hx::StackContext *stack, void* obj, int inSize, const char* name)
{
  #ifdef HXCPP_STACK_TRACE
   if (stack->mTelemetry)
      stack->mTelemetry->HXTAllocation(obj, inSize, name);
  #endif
}
void __hxt_gc_realloc(void* old_obj, void* new_obj, int new_size)
{
  #ifdef HXCPP_STACK_TRACE
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->HXTRealloc(old_obj, new_obj, new_size);
  #endif
}
void __hxt_gc_start()
{
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->GCStart();
}
void __hxt_gc_end()
{
   hx::StackContext *stack = hx::StackContext::getCurrent();
   if (stack->mTelemetry)
      stack->mTelemetry->GCEnd();
}

void __hxt_gc_after_mark(int gByteMarkID, int ENDIAN_MARK_ID_BYTE)
{
  hx::Telemetry::HXTAfterMark(gByteMarkID, ENDIAN_MARK_ID_BYTE);
}








