#include <hxcpp.h>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <hx/Debug.h>
#include <hx/Thread.h>
#include <hx/Telemetry.h>
#include <hx/OS.h>
#include "QuickVec.h"

#ifdef HX_WINRT
#define DBGLOG WINRT_LOG
#elif defined(ANDROID)
#include <android/log.h>
#define DBGLOG(...) __android_log_print(ANDROID_LOG_INFO, "HXCPP", __VA_ARGS__)
#else
#include <stdio.h>
#define DBGLOG printf
#endif

#if _MSC_VER
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef HXCPP_PROFILE_EXTERNS
const char* EXTERN_CLASS_NAME = "extern";
#endif

// These should implement write and read memory barrier, but since there are
// no obvious portable implementations, they are currently left unimplemented
static void write_memory_barrier()
{
    // currently unimplemented
}

static void read_memory_barrier()
{
    // currently unimplemented
}

// Newer versions of haxe compiler will set these too (or might be null for haxe 3.0)
static const char **__all_files_fullpath = 0;
static const char **__all_classes = 0;

namespace hx
{

static void CriticalErrorHandler(String inErr, bool allowFixup);

// These are emitted elsewhere by the haxe compiler
extern const char *__hxcpp_all_files[];

// This global boolean is set whenever there are any breakpoints (normal or
// immediate), and can relatively quickly gate debugged threads from making
// more expensive breakpoint check calls when there are no breakpoints set.
// Note that there is no lock to protect this.  Volatile is used to ensure
// that within a function call, the value of gShouldCallHandleBreakpoints is
// not cached in a register and thus not properly checked within the function
// call.
volatile bool gShouldCallHandleBreakpoints;

enum ExecutionTrace
{
   exeTraceOff = 0,
   exeTraceFuncs = 1,
   exeTraceLines = 2,
};
static ExecutionTrace sExecutionTrace = exeTraceOff;

// This is the event notification handler, as registered by the debugger
// thread.
// Signature: threadNumber : Int -> status: Int  -> Void
static Dynamic g_eventNotificationHandler;
// This is the function to call to create a new Parameter
// Signature: name : String -> value : Dynamic -> Parameter : Dynamic
static Dynamic g_newParameterFunction;
// This is the function to call to create a new StackFrame
// Signature: fileName : String -> lineNumber :Int ->
//            className : String -> functionName : String ->
//            StackFrame : Dynamic
static Dynamic g_newStackFrameFunction;
// This is the function to call to create a new ThreadInfo
// Signature: number : Int -> statu s: Int -> breakpoint : Int ->
//                     ThreadInfo :  Dynamic
static Dynamic g_newThreadInfoFunction;
// This is the function to call to add a Parameter to a StackFrame.
// Signature: inStackFrame : Dynamic -> inParameter : Dynamic -> Void
static Dynamic g_addParameterToStackFrameFunction;
// This is the function to call to add a StackFrame to a ThreadInfo.
// Signature: inThreadInfo : Dynamic -> inStackFrame : Dynamic -> Void
static Dynamic g_addStackFrameToThreadInfoFunction;

// User settable String->Void
//  String  : message
//  You can 'throw' to prevent default action
static Dynamic sCriticalErrorHandler;


static void setStaticHandler(Dynamic &inStore, Dynamic inValue)
{
   if ( inStore==null() != inValue==null())
   {
      if ( inValue!=null() )
         GCAddRoot(&inStore.mPtr);
      else
         GCRemoveRoot(&inStore.mPtr);
   }
   inStore = inValue;
}

// This is the thread number of the debugger thread, extracted from
// information about the thread that called 
// __hxcpp_dbg_setEventNotificationHandler
static unsigned int g_debugThreadNumber = -1;

class Breakpoints;
class CallStack;


// Every thread has a local data pointer to its call stack
DECLARE_TLS_DATA(CallStack, tlsCallStack);
// Every thread has a local data pointer to a set of breakpoints.  Since
// the breakpoints object is shared across many threads, it is reference
// counted and the last thread to reference it deletes it.
DECLARE_TLS_DATA(Breakpoints, tlsBreakpoints);


// Profiler functionality separated into this class
class Profiler
{
public:

    Profiler(const String &inDumpFile)
        : mT0(0)
{
        mDumpFile = inDumpFile;
   
        // When a profiler exists, the profiler thread needs to exist
        gThreadMutex.Lock();
   
        gThreadRefCount += 1;
   
        if (gThreadRefCount == 1) {
            HxCreateDetachedThread(ProfileMainLoop, 0);
        }

        gThreadMutex.Unlock();
    }

    ~Profiler()
{
        gThreadMutex.Lock();

        gThreadRefCount -= 1;

        gThreadMutex.Unlock();
}

    void Sample(CallStack *stack);

    void DumpStats()
    {
        FILE *out = 0;
        if (mDumpFile.length > 0) {
            out = fopen(mDumpFile.c_str(), "wb");
            if (!out) {
                return;
            }
}

        std::vector<ResultsEntry> results;

        results.reserve(mProfileStats.size());

        int total = 0;
        std::map<const char *, ProfileEntry>::iterator iter = 
            mProfileStats.begin();
        while (iter != mProfileStats.end()) {
            ProfileEntry &pe = iter->second;
            ResultsEntry re;
            re.fullName = iter->first;
            re.self = pe.self;
            re.total = pe.total;
            re.childrenPlusSelf = re.self;
            ChildEntry internal;
            internal.fullName = "(internal)";
            internal.self = re.self;
            std::map<const char *, int>::iterator childIter =
                pe.children.begin();
            int childTotal = 0;
            while (childIter != pe.children.end()) {
                ChildEntry ce;
                ce.fullName = childIter->first;
                ce.self = childIter->second;
                re.childrenPlusSelf += ce.self;
                re.children.push_back(ce);
                childIter++;
            }
            re.children.push_back(internal);
            std::sort(re.children.begin(), re.children.end());
            results.push_back(re);
            total += re.self;
            iter++;
        }

        std::sort(results.begin(), results.end());

        double scale = total ? (100.0 / total) : 1.0;

        int size = results.size();

#ifdef HX_WINRT
#define PROFILE_PRINT WINRT_LOG
#else
#define PROFILE_PRINT(...)                      \
        if (out) {                              \
            fprintf(out, __VA_ARGS__);          \
        }                                       \
        else {                                  \
            DBGLOG(__VA_ARGS__);                \
        }
#endif
        for (int i = 0; i < size; i++) {
            ResultsEntry &re = results[i];
            PROFILE_PRINT("%s %.2f%%/%.2f%%\n", re.fullName, re.total * scale,
                          re.self * scale);
            if (re.children.size() == 1) {
                continue;
            }

            int childrenSize = re.children.size();
            for (int j = 0; j < childrenSize; j++) {
                ChildEntry &ce = re.children[j];
                PROFILE_PRINT("   %s %.1f%%\n", ce.fullName,
                              (100.0 * ce.self) / re.childrenPlusSelf);
            }
        }

        if (out) {
            fclose(out);
        }
    }

private:

struct ProfileEntry
{
        ProfileEntry()
            : self(0), total(0)
        {
        }

        int self;
        std::map<const char *, int> children;
        int total;
};

    struct ChildEntry
    {
        bool operator <(const ChildEntry &inRHS) const
{
            return self > inRHS.self;
        }

        const char *fullName;
        int self;
};

    struct ResultsEntry
    {
        bool operator <(const ResultsEntry &inRHS) const
        {
            return ((total > inRHS.total) ||
                    ((total == inRHS.total) && (self < inRHS.self)));
        }
        
        const char *fullName;
        int self;
        std::vector<ChildEntry> children;
        int total;
        int childrenPlusSelf;
    };

    static THREAD_FUNC_TYPE ProfileMainLoop(void *)
    {
        int millis = 1;

        while (gThreadRefCount > 0) { 
            HxSleep(millis);

            int count = gProfileClock + 1;
            gProfileClock = (count < 0) ? 0 : count;
        }

        THREAD_FUNC_RET
    }

    String mDumpFile;
    int mT0;
    std::map<const char *, ProfileEntry> mProfileStats;

    static HxMutex gThreadMutex;
    static int gThreadRefCount;
    static int gProfileClock;
};
/* static */ HxMutex Profiler::gThreadMutex;
/* static */ int Profiler::gThreadRefCount;
/* static */ int Profiler::gProfileClock;


#ifdef HXCPP_TELEMETRY

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

    Telemetry(bool profiler_en, bool allocs_en)
        : mT0(0)
{
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

    void StackUpdate(CallStack *stack, StackFrame *frame);
    void HXTAllocation(CallStack *stack, void* obj, size_t inSize, const char* type=0);
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

    void push_callstack_ids_into(hx::CallStack *stack, std::vector<int> *list);
    int GetNameIdx(const char *fullName);
    int ComputeCallStackId(hx::CallStack *stack);

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

#endif // HXCPP_TELEMETRY


class CallStack
{
public:

    // Gets the call stack of the calling thread
    static CallStack *GetCallerCallStack()
    {
        CallStack *stack = tlsCallStack;
 
        if (!stack) {
            int threadNumber = __hxcpp_GetCurrentThreadNumber();

            stack = new CallStack(threadNumber);

            gMutex.Lock();
            gMap[threadNumber] = stack;
            gList.push_back(stack);
            gMutex.Unlock();

            tlsCallStack = stack;
        }

        return stack;
    }

    static void RemoveCallStack(int threadNumber)
{ 
        gMutex.Lock();

        CallStack *stack = gMap[threadNumber];
        if (!stack) {
            gMap.erase(threadNumber);
            gList.remove(stack);
            delete stack;
            tlsCallStack = 0;
        }

        gMutex.Unlock();
    }

    static void EnableCurrentThreadDebugging(bool enable)
    {
        GetCallerCallStack()->mCanStop = enable;
    }
    void PushStackFrame(StackFrame *frame)
    {
        if (mProfiler)
            mProfiler->Sample(this);

#ifdef HXCPP_TELEMETRY
        if (mTelemetry) mTelemetry->StackUpdate(this, frame);
#endif

        mUnwindException = false;
        mStackFrames.push_back(frame);

        if (sExecutionTrace!=exeTraceOff)
           Trace();
    }

    void PopStackFrame()
    {
        if (mProfiler)
            mProfiler->Sample(this);

#ifdef HXCPP_TELEMETRY
        if (mTelemetry) mTelemetry->StackUpdate(this, 0);
#endif

        if (mUnwindException)
        {
           // Use default operator=
           mExceptionStack.push( *mStackFrames.back() );
        }

        mStackFrames.pop_back();
    }
    void Trace( )
    {
       StackFrame *frame = mStackFrames[mStackFrames.size()-1];
       #ifdef HXCPP_STACK_LINE
       DBGLOG("%s::%s : %d\n", frame->className, frame->functionName, frame->lineNumber);
       #else
       DBGLOG("%s::%s\n", frame->className, frame->functionName);
       #endif
    }

    #ifdef HXCPP_STACK_LINE
    void SetLine(int inLine)
    {
       mStackFrames[mStackFrames.size()-1]->lineNumber = inLine;
       if (gShouldCallHandleBreakpoints)
          __hxcpp_on_line_changed();
    }
    #endif


    // Note that the stack frames are manipulated without holding any locks.
    // This is because the manipulation of stack frames can only be done by
    // the thread that "owns" that stack frame.  The only other contention on
    // the call stack is from calls to GetThreadInfo() and GetThreadInfos(),
    // and these should only be called when the thread for which the call
    // stack is being acquired is stopped in a breakpoint anyway, thus there
    // can be no contention on the contents of the CallStack in that case
    // either.
    inline static void PushCallerStackFrame(StackFrame *frame)
    {
        CallStack *stack = GetCallerCallStack();
        stack->PushStackFrame(frame);
    }

    inline static void PopCallerStackFrame()
    {
        CallStack *stack = GetCallerCallStack();
        stack->PopStackFrame();
    }

    static void ContinueThreads(int specialThreadNumber, int count)
    {
        gMutex.Lock();

        // All threads get continued, but specialThreadNumber only for count
        std::list<CallStack *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            CallStack *stack = *iter++;
            if (stack->mThreadNumber == specialThreadNumber) {
                stack->Continue(count);
            }
            else {
                stack->Continue(1);
            }
        }

        gMutex.Unlock();
    }

    static void StepOneThread(int threadNumber, int &stackLevel)
    {
        gMutex.Lock();

        std::list<CallStack *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            CallStack *stack = *iter++;
            if (stack->mThreadNumber == threadNumber) {
                stackLevel = stack->mStackFrames.size() - 1;
                stack->Continue(1);
                break;
            }
        }

        gMutex.Unlock();
    }

    static void GetCurrentCallStackAsStrings(Array<String> result, bool skipLast)
    {
        CallStack *stack = CallStack::GetCallerCallStack();

        int n = stack->mStackFrames.size() - (skipLast ? 1 : 0);
        
        for (int i = 0; i < n; i++)
        {
            // Reverse call stack to match exception stack
            StackFrame *frame = stack->mStackFrames[n-1-i];
            result->push(frame->toString());
        }
    }

    // Gets a ThreadInfo for a thread
    static Dynamic GetThreadInfo(int threadNumber, bool unsafe)
    {
        if (threadNumber == g_debugThreadNumber)
            return null();

        CallStack *stack;

        gMutex.Lock();

        if (gMap.count(threadNumber) == 0)
        {
            gMutex.Unlock();
            return null();
        }
        else
            stack = gMap[threadNumber];

        if ((stack->mStatus == STATUS_RUNNING) && !unsafe)
        {
            gMutex.Unlock();
            return null();
        }

        // It's safe to release the mutex here, because the stack to be
        // converted is either for a thread that is not running (and thus
        // the stack cannot be altered while the conversion is in progress),
        // or unsafe mode has been invoked
        gMutex.Unlock();

        return CallStackToThreadInfo(stack);
    }

    // Gets a ThreadInfo for each Thread
    static ::Array<Dynamic> GetThreadInfos()
    {
        gMutex.Lock();

        // Latch the current thread numbers from the current list of call
        // stacks.
        std::list<int> threadNumbers;
        std::list<CallStack *>::iterator stack_iter = gList.begin();
        while (stack_iter != gList.end()) {
            CallStack *stack = *stack_iter++;
            threadNumbers.push_back(stack->mThreadNumber);
        }
        
        gMutex.Unlock();

        ::Array<Dynamic> ret = Array_obj<Dynamic>::__new();

        // Now get each thread info
        std::list<int>::iterator thread_iter = threadNumbers.begin();
        while (thread_iter != threadNumbers.end())
        {
            Dynamic info = GetThreadInfo(*thread_iter++, false);
            if (info != null())
                ret->push(info);
        }

        return ret;
    }

    static ::Array<Dynamic> GetStackVariables(int threadNumber,
                                              int stackFrameNumber,
                                              bool unsafe,
                                              Dynamic markThreadNotStopped)
    {
        ::Array<Dynamic> ret = Array_obj<Dynamic>::__new();

        gMutex.Lock();

        std::list<CallStack *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            CallStack *stack = *iter++;
            if (stack->mThreadNumber == threadNumber) {
                if ((stack->mStatus == STATUS_RUNNING) && !unsafe) {
                    ret->push(markThreadNotStopped);
                    gMutex.Unlock();
                    return ret;
                }
                // Some kind of error signalling here would be nice I guess
                if (stack->mStackFrames.size() <= stackFrameNumber) {
                    break;
                }
                StackVariable *variable = 
                    stack->mStackFrames[stackFrameNumber]->variables;
                while (variable) {
                    ret->push(String(variable->mHaxeName));
                    variable = variable->mNext;
                }
                break;
            }
        }

        gMutex.Unlock();

        return ret;
    }

    static Dynamic GetVariableValue(int threadNumber, int stackFrameNumber,
                                    String name, bool unsafe,
                                    Dynamic markNonexistent,
                                    Dynamic markThreadNotStopped)
    {
        if (threadNumber == g_debugThreadNumber) {
            return markNonexistent;
        }

        CallStack *stack;

        gMutex.Lock();

        if (gMap.count(threadNumber) == 0) {
            gMutex.Unlock();
            return markNonexistent;
        }
        else {
            stack = gMap[threadNumber];
        }

        if ((stack->mStatus == STATUS_RUNNING) && !unsafe) {
            gMutex.Unlock();
            return markThreadNotStopped;
        }

        // Don't need the lock any more, the thread is not running
        gMutex.Unlock();

        // Check to ensure that the stack frame is valid
        int size = stack->mStackFrames.size();

        if ((stackFrameNumber < 0) || (stackFrameNumber >= size)) {
            return markNonexistent;
        }
        
        const char *nameToFind = name.c_str();

        StackVariable *sv = stack->mStackFrames[stackFrameNumber]->variables;

        while (sv) {
            if (!strcmp(sv->mHaxeName, nameToFind)) {
                return (Dynamic) *sv;
            }
            sv = sv->mNext;
        }

        return markNonexistent;
    }

    static Dynamic SetVariableValue(int threadNumber, int stackFrameNumber,
                                    String name, Dynamic value,
                                    bool unsafe, Dynamic markNonexistent,
                                    Dynamic markThreadNotStopped)
    {
        if (threadNumber == g_debugThreadNumber) {
            return null();
        }

        CallStack *stack;

        gMutex.Lock();

        if (gMap.count(threadNumber) == 0) {
            gMutex.Unlock();
            return null();
        }
        else {
            stack = gMap[threadNumber];
        }

        if ((stack->mStatus == STATUS_RUNNING) && !unsafe) {
            gMutex.Unlock();
            return markThreadNotStopped;
        }

        // Don't need the lock any more, the thread is not running
        gMutex.Unlock();

        // Check to ensure that the stack frame is valid
        int size = stack->mStackFrames.size();

        if ((stackFrameNumber < 0) || (stackFrameNumber >= size)) {
            return null();
        }

        const char *nameToFind = name.c_str();

        if (!strcmp(nameToFind, "this")) {
            return markNonexistent;
        }

        StackVariable *sv = stack->mStackFrames[stackFrameNumber]->variables;

        while (sv) {
            if (!strcmp(sv->mHaxeName, nameToFind)) {
                *sv = value;
                return (Dynamic) *sv;
            }
            sv = sv->mNext;
        }
        
        return markNonexistent;
    }

    static bool BreakCriticalError(const String &inErr)
    {
        CallStack *stack = GetCallerCallStack();

        //if the thread with the critical error is the debugger one,
        //we don't break as it would block debugging since the debugger thread
        //is the only one which can wake up application threads.
        if (stack->GetThreadNumber() == g_debugThreadNumber) {
           return false;
        }

        stack->DoBreak
            (STATUS_STOPPED_CRITICAL_ERROR, -1, &inErr);

        return true;
    }

    // Make best effort to wait until all threads are stopped
    static void WaitForAllThreadsToStop()
    {
        // Make a "best effort" in the face of threads that could do arbitrary
        // things to make the "break all" not complete successfully.  Threads
        // can hang in system calls indefinitely, and can spawn new threads
        // continuously that themselves do the same.  Don't try to be perfect
        // and guarantee that all threads have stopped, as that could mean
        // waiting a long time in pathological cases or even forever in really
        // pathological cases.  Just make a best effort and if the break
        // doesn't break everything, the user will know when they go to list
        // all thread stacks and can try the break again.

        // Copy the thread numbers out.  This is because we really don't want
        // to hold the lock during the entire process as this could block
        // threads from actually evaluating breakpoints.
        std::vector<int> threadNumbers;
        gMutex.Lock();
        std::list<CallStack *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            CallStack *stack = *iter++;
            if (stack->mThreadNumber == g_debugThreadNumber) {
                continue;
            }
            threadNumbers.push_back(stack->mThreadNumber);
        }
        gMutex.Unlock();

        // Now wait no longer than 2 seconds total for all threads to
        // be stopped.  If any thread times out, then stop immediately.
        int size = threadNumbers.size();
        // Each time slice is 1/10 of a second.  Yeah there's some slop here
        // because no time is accounted for the time spent outside of sem
        // waiting.  If there were good portable time APIs easily available
        // within hxcpp I'd use them ...
        int timeSlicesLeft = 20;
        HxSemaphore timeoutSem;
        int i = 0;
        while (i < size) {
            gMutex.Lock();
            CallStack *stack = gMap[threadNumbers[i]];
            if (!stack) {
                // The thread went away while we were working!
                gMutex.Unlock();
                i += 1;
                continue;
            }
            if (stack->mWaiting) {
                gMutex.Unlock();
                i += 1;
                continue;
            }
            gMutex.Unlock();
            if (timeSlicesLeft == 0) {
                // The 2 seconds have expired, give up
                return;
            }
            // Sleep for 1/10 of a second on a semaphore that will never
            // be Set.
            timeoutSem.WaitSeconds(0.100);
            timeSlicesLeft -= 1;
            // Don't increment i, try the same thread again
        }
    }

    static bool CanBeCaught(Dynamic e)
    {
        CallStack *stack = GetCallerCallStack();

        std::vector<StackFrame *>::reverse_iterator iter = 
            stack->mStackFrames.rbegin();
        while (iter != stack->mStackFrames.rend()) {
            StackFrame *frame = *iter++;
            StackCatchable *catchable = frame->catchables;
            while (catchable) {
                if (catchable->Catches(e)) {
                    return true;
                }
                catchable = catchable->mNext;
            }
        }

        return false;
    }

    static void StartCurrentThreadProfiler(String inDumpFile)
    {
        CallStack *stack = GetCallerCallStack();

        if (stack->mProfiler) {
            delete stack->mProfiler;
        }

        stack->mProfiler = new Profiler(inDumpFile);
    }

    static void StopCurrentThreadProfiler()
    {
        CallStack *stack = GetCallerCallStack();

        if (stack->mProfiler) {
            stack->mProfiler->DumpStats();
            delete stack->mProfiler;
            stack->mProfiler = 0;
        }
    }

#ifdef HXCPP_TELEMETRY
    static void StartCurrentThreadTelemetry(bool profiler, bool allocations)
    {
        CallStack *stack = GetCallerCallStack();

        if (stack->mTelemetry) {
            delete stack->mTelemetry;
        }

        stack->mTelemetry = new Telemetry(profiler, allocations);
        //printf("Inst telemetry %016lx on stack %016lx\n", stack->mTelemetry, stack);
    }

    static void StashCurrentThreadTelemetry()
    {
        CallStack *stack = CallStack::GetCallerCallStack();
        stack->mTelemetry->Stash();
    }

    static TelemetryFrame* DumpThreadTelemetry(int thread_num)
    {
      CallStack *stack;
      gMutex.Lock();
      stack = gMap[thread_num];
      gMutex.Unlock();

      return stack->mTelemetry->Dump();
    }

    static void IgnoreAllocs(int delta)
    {
        CallStack *stack = hx::CallStack::GetCallerCallStack();
        Telemetry *telemetry = stack->mTelemetry;
        if (telemetry) {
            telemetry->IgnoreAllocs(delta);
        }
    }

    static void HXTAllocation(void* obj, size_t inSize, const char* type)
    {
        CallStack *stack = hx::CallStack::GetCallerCallStack();
        Telemetry *telemetry = stack->mTelemetry;
        if (telemetry) {
          //printf("Telemetry %016lx allocating %s at %016lx of size %d\n", telemetry, type, obj, inSize); 
          telemetry->HXTAllocation(stack, obj, inSize, type);
        }
    }

    static void HXTRealloc(void* old_obj, void* new_obj, int new_size)
    {
        CallStack *stack = hx::CallStack::GetCallerCallStack();
        Telemetry *telemetry = stack->mTelemetry;
        if (telemetry) {
          telemetry->HXTRealloc(old_obj, new_obj, new_size);
        }
    }

    static void HXTGCStart()
    {
        CallStack *stack = hx::CallStack::GetCallerCallStack();
        Telemetry *telemetry = stack->mTelemetry;
        if (telemetry) {
          telemetry->GCStart();
        }
    }

    static void HXTGCEnd()
    {
        CallStack *stack = hx::CallStack::GetCallerCallStack();
        Telemetry *telemetry = stack->mTelemetry;
        if (telemetry) {
          telemetry->GCEnd();
        }
    }

#endif

    static void GetCurrentExceptionStackAsStrings(Array<String> &result)
    {
        CallStack *stack = CallStack::GetCallerCallStack();

        int size = stack->mExceptionStack.size();

        for (int i = 0; i < size; i++) {
            result->push(stack->mExceptionStack[i].toString());
        }
    }

    // Gets the current stack frame of the calling thread
    StackFrame *GetCurrentStackFrame()
    {
        return mStackFrames.back();
    }

    int GetThreadNumber()
    {
        return mThreadNumber;
    }

    bool CanStop() const
    {
        return mCanStop;
    }

    int GetDepth() const
    {
        return mStackFrames.size() - 1;
    }

    const char *GetFullNameAtDepth(int depth) const
    {
        return mStackFrames[depth]->fullName;
    }
        

    // Wait for someone to call Continue() on this call stack.  Really only
    // the thread that owns this call stack should call Wait().
    void Break(ThreadStatus status, int breakpoint,
               const String *criticalErrorDescription)
    {
        // If break status is break immediate, then eliminate any residual
        // continue count from the last continue.
        if (status == STATUS_STOPPED_BREAK_IMMEDIATE) {
            mContinueCount = 0;
        }
        // Else break status is break in breakpoint -- but if there is a
        // continue count, just decrement the continue count
        else if (mContinueCount > 0) {
            mContinueCount -= 1;
            return;
        }

        this->DoBreak(status, breakpoint, 0);
    }

    // Continue the thread that is waiting, if it is waiting.  Only the
    // debugger thread should call this.
    void Continue(int count)
    {
        // Paranoia
        if (count < 1) {
            count = 1;
        }

        mWaitMutex.Lock();

        if (mWaiting) {
            mWaiting = false;
            mContinueCount = count - 1;
            mWaitSemaphore.Set();
        }

        mWaitMutex.Unlock();
    }

    // Called when a throw occurs
    void SetLastException()
    {
       mExceptionStack.clear();
       mUnwindException = true;
    }

    // Called when a catch block begins to be executed.  hxcpp wants to track
    // the stack back through the catches so that it can be dumped if
    // uncaught.  If inAll is true, the entire stack is captured immediately.
    // If inAll is false, only the last stack frame is captured.
    void BeginCatch(bool inAll)
    {
       if (inAll)
       {
          mExceptionStack.clear();
          // Copy remaineder of stack frames to exception stack...
          // This will use the default operator=, which will copy the pointers.
          // This is what we want, since the pointers are pointers to constant data
          for(int i=mStackFrames.size()-1;i>=0;--i)
             mExceptionStack.push( *mStackFrames[i] );
       }
       // Lock-in the excpetion stack
       mUnwindException = false;
    }

   void DumpExceptionStack()
   {
#ifdef ANDROID
#define EXCEPTION_PRINT(...) \
        __android_log_print(ANDROID_LOG_ERROR, "HXCPP", __VA_ARGS__)
#else
#define EXCEPTION_PRINT(...) \
        printf(__VA_ARGS__)
#endif
        
        int size = mExceptionStack.size();

        for (int i = size - 1; i >= 0; i--) {
            EXCEPTION_PRINT("Called from %s\n", mExceptionStack[i].toDisplay().__s);
        }
   }

private:

    CallStack(int threadNumber)
        : mThreadNumber(threadNumber), mCanStop(true), mStatus(STATUS_RUNNING),
          mBreakpoint(-1), mWaiting(false), mContinueCount(0), mProfiler(0),
          mUnwindException(false)
    {
      mProfiler = 0;
#ifdef HXCPP_TELEMETRY
      mTelemetry = 0;
#endif
    }

    void DoBreak(ThreadStatus status, int breakpoint,
                 const String *criticalErrorDescription)
    {
        // Update status
        mStatus = status;
        mBreakpoint = breakpoint;
        if (criticalErrorDescription) {
            mCriticalErrorDescription = *criticalErrorDescription;
        }

        // This thread cannot stop while making the callback
        mCanStop = false;

        // Call the handler to announce the status.
        int stackFrame = mStackFrames.size() - 1;
        StackFrame *frame = mStackFrames[stackFrame];
        g_eventNotificationHandler
            (mThreadNumber, THREAD_STOPPED, stackFrame,
             String(frame->className), String(frame->functionName),
             String(frame->fileName), frame->lineNumber);

        // Wait until the debugger thread sets mWaiting to false and signals
        // the semaphore
        mWaitMutex.Lock();
        mWaiting = true;

        while (mWaiting) {
            mWaitMutex.Unlock();
            hx::EnterGCFreeZone();
            mWaitSemaphore.Wait();
            hx::ExitGCFreeZone();
            mWaitMutex.Lock();
        }

        mWaitMutex.Unlock();

        // Save the breakpoint status in the call stack so that queries for
        // thread info will know the current status of the thread
        mStatus = STATUS_RUNNING;
        mBreakpoint = -1;

        // Announce the new status
        g_eventNotificationHandler(mThreadNumber, THREAD_STARTED);

        // Can stop again
        mCanStop = true;
    }

    static Dynamic CallStackToThreadInfo(CallStack *stack)
   {
        Dynamic ret = g_newThreadInfoFunction
            (stack->mThreadNumber, stack->mStatus, stack->mBreakpoint,
             stack->mCriticalErrorDescription);

        int size = stack->mStackFrames.size();
        for (int i = 0; i < size; i++) {
            g_addStackFrameToThreadInfoFunction
                (ret, StackFrameToStackFrame(stack->mStackFrames[i]));
        }

        return ret;
   }

   static Dynamic StackFrameToStackFrame(StackFrame *frame)
   {
        Dynamic ret = g_newStackFrameFunction
            (String(frame->fileName), String(frame->lineNumber),
             String(frame->className), String(frame->functionName));
        
        // Don't do parameters for now
        // xxx figure them out later

        return ret;
   }

    int mThreadNumber;
    bool mCanStop;
    ThreadStatus mStatus;
    int mBreakpoint;
    String mCriticalErrorDescription;
    std::vector<StackFrame *> mStackFrames;

    // Updated only when a thrown exception unwinds the stack
    bool mUnwindException;
    hx::QuickVec<StackFrame> mExceptionStack;

    int mStepLevel;
    HxMutex mWaitMutex;
    bool mWaiting;
    HxSemaphore mWaitSemaphore;
    int mContinueCount;

    // Profiling support
    Profiler *mProfiler;

#ifdef HXCPP_TELEMETRY
    // Telemetry support
    Telemetry *mTelemetry;
#endif

    // gMutex protects gMap and gList
    static HxMutex gMutex;
    static std::map<int, CallStack *> gMap;
    static std::list<CallStack *> gList;
};
/* static */ HxMutex CallStack::gMutex;
/* static */ std::map<int, CallStack *> CallStack::gMap;
/* static */ std::list<CallStack *> CallStack::gList;

#ifdef HXCPP_DEBUGGER
class Breakpoints
{
public:

#ifdef HXCPP_DEBUG_HASHES
    static int Hash(int value, const char *inString)
    {
       while(*inString)
          value = value*223 + *inString++;
       return value;
    }
#endif

    static int Add(String inFileName, int lineNumber)
    {
        // Look up the filename constant
        const char *fileName = LookupFileName(inFileName);

        if (!fileName) {
            return -1;
        }

        gMutex.Lock();

        int ret = gNextBreakpointNumber++;
        
        Breakpoints *newBreakpoints = new Breakpoints(gBreakpoints, ret, fileName, lineNumber);
        
        gBreakpoints->RemoveRef();

        // Write memory barrier ensures that newBreakpoints values are updated
        // before gBreakpoints is assigned to it
        write_memory_barrier();

        gBreakpoints = newBreakpoints;

        // Don't need a write memory barrier here, it's harmless to see
        // gShouldCallHandleBreakpoints update before gBreakpoints has updated
        gShouldCallHandleBreakpoints = true;

        gMutex.Unlock();

        return ret;
    }

    static int Add(String inClassName, String functionName)
    {
        // Look up the class name constant
        const char *className = LookupClassName(inClassName);

        if (!className) {
            return -1;
        }
        
        gMutex.Lock();

        int ret = gNextBreakpointNumber++;
        
        Breakpoints *newBreakpoints = new Breakpoints
            (gBreakpoints, ret, className, functionName);
        
        gBreakpoints->RemoveRef();

        // Write memory barrier ensures that newBreakpoints values are updated
        // before gBreakpoints is assigned to it
        write_memory_barrier();

        gBreakpoints = newBreakpoints;

        // Don't need a write memory barrier here, it's harmless to see
        // gShouldCallHandleBreakpoints update before gBreakpoints has updated
        gShouldCallHandleBreakpoints = true;

        gMutex.Unlock();

        return ret;
    }

    static void DeleteAll()
    {
        gMutex.Lock();
        
        Breakpoints *newBreakpoints = new Breakpoints();

        gBreakpoints->RemoveRef();

        // Write memory barrier ensures that newBreakpoints values are updated
        // before gBreakpoints is assigned to it
        write_memory_barrier();

        gBreakpoints = newBreakpoints;

        // Don't need a write memory barrier here, it's harmless to see
        // gShouldCallHandleBreakpoints update before gStepType has updated
        gShouldCallHandleBreakpoints = (gStepType != STEP_NONE) || (sExecutionTrace==exeTraceLines);

        gMutex.Unlock();
    }

    static void Delete(int number)
    {
        gMutex.Lock();
        
        if (gBreakpoints->HasBreakpoint(number)) {
            // Replace mBreakpoints with a copy and remove the breakpoint
            // from it
            Breakpoints *newBreakpoints = new Breakpoints(gBreakpoints, number);
            Breakpoints *toRelease = gBreakpoints;
            gBreakpoints = newBreakpoints;

            // Write memory barrier ensures that newBreakpoints values are
            // updated before gBreakpoints is assigned to it
            write_memory_barrier();

            // Only release after gBreakpoints is set
            toRelease->RemoveRef();


            if (gBreakpoints->IsEmpty()) {
                // Don't need a write memory barrier here, it's harmless to
                // see gShouldCallHandleBreakpoints update before gStepType
                // has updated
                gShouldCallHandleBreakpoints = (gStepType != STEP_NONE) || (sExecutionTrace==exeTraceLines);
            }
        }

        gMutex.Unlock();
    }

    static void BreakNow(bool wait)
    {
        gStepType = STEP_INTO;
        gStepCount = 0;
        gStepThread = -1;
        // Won't bother with a write memory barrier here, it's harmless to set
        // gShouldCallHandleBreakpoints before the step type and step thread
        // are updated xxx should consider making gStepType and gStepThread
        // atomic though by putting them into one uint32_t value ...
        gShouldCallHandleBreakpoints = true;

        // Wait for all threads to be stopped
        if (wait) {
            CallStack::WaitForAllThreadsToStop();
        }
    }

    static void ContinueThreads(int specialThreadNumber, int continueCount)
    {
        gStepType = STEP_NONE;

        gShouldCallHandleBreakpoints = !gBreakpoints->IsEmpty() || (sExecutionTrace==exeTraceLines);

        CallStack::ContinueThreads(specialThreadNumber, continueCount);
    }

    static void StepThread(int threadNumber, StepType stepType, int stepCount)
    {
        // Continue the thread, but set its step first
        gStepThread = threadNumber;
        gStepType = stepType;
        gStepCount = stepCount;
        
        CallStack::StepOneThread(threadNumber, gStepLevel);
    }

    // Note that HandleBreakpoints is called immediately after a read memory
    // barrier by the HX_STACK_LINE macro
    static void HandleBreakpoints()
    {
        // This will be set to a valid status if a stop is needed
        ThreadStatus breakStatus = STATUS_INVALID;
        int breakpointNumber = -1;

        CallStack *stack = CallStack::GetCallerCallStack();
        if (sExecutionTrace==exeTraceLines)
           stack->Trace();

        // Handle possible immediate break
        if (gStepType == STEP_NONE) {
            // No stepping
        }
        else if (gStepType == STEP_INTO) {
            if ((gStepThread == -1) ||
                (gStepThread == stack->GetThreadNumber())) {
                breakStatus = STATUS_STOPPED_BREAK_IMMEDIATE;
            }
        }
        else {
            if ((gStepThread == -1) ||
                (gStepThread == stack->GetThreadNumber())) {
                if (gStepType == STEP_OVER) {
                    if (stack->GetDepth() <= gStepLevel) {
                        breakStatus = STATUS_STOPPED_BREAK_IMMEDIATE;
                    }
                }
                else { // (gStepType == STEP_OUT)
                    if (stack->GetDepth() < gStepLevel) {
                        breakStatus = STATUS_STOPPED_BREAK_IMMEDIATE;
                    }
                }
            }
        }

        // If didn't hit any immediate breakpoints, check for set breakpoints
        if (breakStatus == STATUS_INVALID) {
            Breakpoints *breakpoints = tlsBreakpoints;
            // If the current thread has never gotten a reference to
            // breakpoints, get a reference to the current breakpoints
            if (!breakpoints) {
                gMutex.Lock();
                // Get break points and ref it
                breakpoints = gBreakpoints;
                // This read memory barrier ensures that old values within
                // gBreakpoints are not seen after gBreakpoints has been set
                // here
                read_memory_barrier();
                tlsBreakpoints = breakpoints;
                breakpoints->AddRef();
                gMutex.Unlock();
            }
            // Else if the current thread's breakpoints number is out of date,
            // release the reference on that and get the new breakpoints.
            // Note that no locking is done on the reference to gBreakpoints.
            // A thread calling GetBreakpoints will retain its old breakpoints
            // until it "sees" a newer gBreakpoints.  Without memory barriers,
            // this could theoretically be indefinitely.
            else if (breakpoints != gBreakpoints) {
                gMutex.Lock();
                // Release ref on current break points
                breakpoints->RemoveRef();
                // Get new break points and ref it
                breakpoints = gBreakpoints;
                // This read memory barrier ensures that old values within
                // gBreakpoints are not seen after gBreakpoints has been set
                // here
                read_memory_barrier();
                tlsBreakpoints = breakpoints;
                breakpoints->AddRef();
                gMutex.Unlock();
            }

            // If there are breakpoints, then may need to break in one
            if (!breakpoints->IsEmpty())
            {
               StackFrame *frame = stack->GetCurrentStackFrame();
               if (!breakpoints->QuickRejectClassFunc(frame->classFuncHash))
               {
                  // Check for class:function breakpoint if this is the
                  // first line of the stack frame
                  if (frame->lineNumber == frame->firstLineNumber)
                      breakpointNumber = breakpoints->FindClassFunctionBreakpoint(frame);
               }

               // If still haven't hit a break point, check for file:line
               // breakpoint
               if (breakpointNumber == -1 && !breakpoints->QuickRejectFileLine(frame->fileHash))
                  breakpointNumber = breakpoints->FindFileLineBreakpoint(frame);

               if (breakpointNumber != -1)
                  breakStatus = STATUS_STOPPED_BREAKPOINT;
            }
        }

        // If no breakpoint of any kind was found, then don't break
        if (breakStatus == STATUS_INVALID) {
            return;
        }

        // The debug thread never breaks
        if (stack->GetThreadNumber() == g_debugThreadNumber) {
            return;
        }

        // If the thread has been put into no stop mode, it can't stop
        if (!stack->CanStop()) {
            return;
        }

        // If the break was an immediate break, and there was a step count,
        // just decrement the step count
        if (breakStatus == STATUS_STOPPED_BREAK_IMMEDIATE) {
            if (gStepCount > 1) {
                gStepCount -= 1;
                return;
            }
        }

        // Now break, which will wait until the debugger thread continues
        // the thread
        stack->Break(breakStatus, breakpointNumber, 0);
      }

      static bool shoudBreakOnLine()
      {
         return gBreakpoints->IsEmpty() || gStepType != hx::STEP_NONE;
      }

private:

    struct Breakpoint
    {
        int number;
        int lineNumber;
        int hash;

        bool isFileLine;

        std::string fileOrClassName;
        std::string functionName;
    };

    // Creates Breakpoints object with no breakpoints and a zero version
    Breakpoints()
        : mRefCount(1), mBreakpointCount(0), mBreakpoints(0)
    {
#ifdef HXCPP_DEBUG_HASHES
       calcCombinedHash();
#endif
    }

    // Copies breakpoints from toCopy and adds a new file:line breakpoint
    Breakpoints(const Breakpoints *toCopy, int number,
                const char *fileName, int lineNumber)
        : mRefCount(1)
    {
        mBreakpointCount = toCopy->mBreakpointCount + 1;
        mBreakpoints = new Breakpoint[mBreakpointCount];
        for (int i = 0; i < toCopy->mBreakpointCount; i++)
            mBreakpoints[i] = toCopy->mBreakpoints[i];

        mBreakpoints[toCopy->mBreakpointCount].number = number;
        mBreakpoints[toCopy->mBreakpointCount].isFileLine = true;
        mBreakpoints[toCopy->mBreakpointCount].fileOrClassName = fileName;
        mBreakpoints[toCopy->mBreakpointCount].lineNumber = lineNumber;

#ifdef HXCPP_DEBUG_HASHES
        mBreakpoints[toCopy->mBreakpointCount].hash = Hash(0, fileName);
        calcCombinedHash();
#else
        mBreakpoints[toCopy->mBreakpointCount].hash = 0;
#endif
    }

    // Copies breakpoints from toCopy and adds a new class:function breakpoint
    Breakpoints(const Breakpoints *toCopy, int number,
                const char *className, String functionName)
        : mRefCount(1)
    {
        mBreakpointCount = toCopy->mBreakpointCount + 1;
        mBreakpoints = new Breakpoint[mBreakpointCount];
        for (int i = 0; i < toCopy->mBreakpointCount; i++) {
            mBreakpoints[i] = toCopy->mBreakpoints[i];
        }
        mBreakpoints[toCopy->mBreakpointCount].number = number;
        mBreakpoints[toCopy->mBreakpointCount].isFileLine = false;
        mBreakpoints[toCopy->mBreakpointCount].fileOrClassName = className;
        mBreakpoints[toCopy->mBreakpointCount].functionName = functionName.c_str();

#ifdef HXCPP_DEBUG_HASHES
        int hash = Hash(0,className);
        hash = Hash(hash,".");
        hash = Hash(hash,functionName.c_str());
        //printf("%s.%s -> %08x\n", className, functionName.c_str(), hash );
        mBreakpoints[toCopy->mBreakpointCount].hash = hash;
        calcCombinedHash();
#else
        mBreakpoints[toCopy->mBreakpointCount].hash = 0;
#endif
   }

   // Copies breakpoints from toCopy except for number
   Breakpoints(const Breakpoints *toCopy, int number)
        : mRefCount(1)
   {
     mBreakpointCount = toCopy->mBreakpointCount - 1;
     if (mBreakpointCount == 0)
        mBreakpoints = 0;
     else
     {
        mBreakpoints = new Breakpoint[mBreakpointCount];
        for(int s = 0, d = 0; s < toCopy->mBreakpointCount; s++)
        {
           Breakpoint &other = toCopy->mBreakpoints[s];
           if (other.number != number)
              mBreakpoints[d++] = toCopy->mBreakpoints[s];
         }
      }
#ifdef HXCPP_DEBUG_HASHES
      calcCombinedHash();
#endif
   }

#ifdef HXCPP_DEBUG_HASHES
   void calcCombinedHash()
   {
      int allFileLine = 0;
      int allClassFunc = 0;

      for(int i=0;i<mBreakpointCount;i++)
         if (mBreakpoints[i].isFileLine)
            allFileLine |= mBreakpoints[i].hash;
         else
            allClassFunc |= mBreakpoints[i].hash;

      mNotInAnyFileLine = ~allFileLine;
      mNotInAnyClassFunc = ~allClassFunc;
      //printf("Combined mask -> %08x %08x\n", mNotInAnyFileLine, mNotInAnyClassFunc);
   }
#endif

   ~Breakpoints()
   {
      delete[] mBreakpoints;
   }

   void AddRef()
   {
       mRefCount += 1;
   }

   void RemoveRef()
   {
      if (--mRefCount == 0)
         delete this;
   }

   bool IsEmpty() const
   {
      return (mBreakpointCount == 0);
   }

   inline bool QuickRejectClassFunc(int inHash)
   {
#ifdef HXCPP_DEBUG_HASHES
      return inHash & mNotInAnyClassFunc;
#else
      return false;
#endif
   }

   inline bool QuickRejectFileLine(int inHash)
   {
#ifdef HXCPP_DEBUG_HASHES
      return inHash & mNotInAnyFileLine;
#else
      return false;
#endif
   }

    bool HasBreakpoint(int number) const
                  {
        for (int i = 0; i < mBreakpointCount; i++) {
            if (number == mBreakpoints[i].number) {
                return true;
                  }
               }
        return false;
         }

   int FindFileLineBreakpoint(StackFrame *inFrame)
   {
      for (int i = 0; i < mBreakpointCount; i++)
      {
         Breakpoint &breakpoint = mBreakpoints[i];
         if (breakpoint.isFileLine && breakpoint.hash==inFrame->fileHash &&
             (breakpoint.lineNumber == inFrame->lineNumber) &&
             !strcmp(breakpoint.fileOrClassName.c_str(),inFrame->fileName) )
            return breakpoint.number;
      }
      return -1;
   }

   int FindClassFunctionBreakpoint(StackFrame *inFrame)
   {
      for (int i = 0; i < mBreakpointCount; i++)
      {
         Breakpoint &breakpoint = mBreakpoints[i];
         if (!breakpoint.isFileLine && breakpoint.hash==inFrame->classFuncHash &&
             !strcmp(breakpoint.fileOrClassName.c_str(), inFrame->className)  &&
             !strcmp(breakpoint.functionName.c_str(), inFrame->functionName) )
            return breakpoint.number;
      }
      return -1;
   }

    // Looks up the "interned" version of the name, for faster compares
    // when evaluating breakpoints
    static const char *LookupFileName(String fileName)
      {
        for (const char **ptr = hx::__hxcpp_all_files; *ptr; ptr++) {
            if (!strcmp(*ptr, fileName)) {
                return *ptr;
      }
      }
        return 0;
   }

   static const char *LookupClassName(String className)
   {
      if (__all_classes)
         for (const char **ptr = __all_classes; *ptr; ptr++)
         {
            if (!strcmp(*ptr, className.__s))
               return *ptr;
         }
      return 0;
   }


private:

    int mRefCount;
    int mBreakpointCount;
#ifdef HXCPP_DEBUG_HASHES
    int mNotInAnyClassFunc;
    int mNotInAnyFileLine;
#endif
    Breakpoint *mBreakpoints;

    static HxMutex gMutex;
    static int gNextBreakpointNumber;
    static Breakpoints * volatile gBreakpoints;
    static StepType gStepType;
    static int gStepLevel;
    static int gStepThread; // If -1, all threads are targeted
    static int gStepCount;
};



/* static */ HxMutex Breakpoints::gMutex;
/* static */ int Breakpoints::gNextBreakpointNumber;
/* static */ Breakpoints * volatile Breakpoints::gBreakpoints = 
    new Breakpoints();
/* static */ StepType Breakpoints::gStepType = STEP_NONE;
/* static */ int Breakpoints::gStepLevel;
/* static */ int Breakpoints::gStepThread = -1;
/* static */ int Breakpoints::gStepCount = -1;
#endif

} // namespace


#ifdef HXCPP_DEBUGGER

void __hxcpp_dbg_setEventNotificationHandler(Dynamic handler)
      {
    if (hx::g_eventNotificationHandler != null()) {
        GCRemoveRoot(&(hx::g_eventNotificationHandler.mPtr));
         }
    hx::g_debugThreadNumber = __hxcpp_GetCurrentThreadNumber();
    hx::g_eventNotificationHandler = handler;
    GCAddRoot(&(hx::g_eventNotificationHandler.mPtr));
      }
 

void __hxcpp_dbg_enableCurrentThreadDebugging(bool enable)
   {
    hx::CallStack::EnableCurrentThreadDebugging(enable);
}


int __hxcpp_dbg_getCurrentThreadNumber()
{
    return __hxcpp_GetCurrentThreadNumber();
}
 

Array< ::String> __hxcpp_dbg_getFiles()
{
    Array< ::String> ret = Array_obj< ::String>::__new();

    for (const char **ptr = hx::__hxcpp_all_files; *ptr; ptr++)
    {
       ret->push(String(*ptr));
    }

    return ret;
}

Array< ::String> __hxcpp_dbg_getFilesFullPath()
{
    Array< ::String> ret = Array_obj< ::String>::__new();

    for (const char **ptr = __all_files_fullpath;ptr && *ptr; ptr++)
    {
        ret->push(String(*ptr));
    }

    return ret;
}



Array< ::String> __hxcpp_dbg_getClasses()
{
    Array< ::String> ret = Array_obj< ::String>::__new();

    if (__all_classes)
    {
       for (const char **ptr = __all_classes; *ptr; ptr++)
         ret->push(String(*ptr));
    }

    return ret;
}


Array<Dynamic> __hxcpp_dbg_getThreadInfos()
{
    return hx::CallStack::GetThreadInfos();
}


Dynamic __hxcpp_dbg_getThreadInfo(int threadNumber, bool unsafe)
{
    return hx::CallStack::GetThreadInfo(threadNumber, unsafe);
}


int __hxcpp_dbg_addFileLineBreakpoint(String fileName, int lineNumber)
      {
    return hx::Breakpoints::Add(fileName, lineNumber);
   }


int __hxcpp_dbg_addClassFunctionBreakpoint(String className,
                                            String functionName)
   {
    return hx::Breakpoints::Add(className, functionName);
   }


void __hxcpp_dbg_deleteAllBreakpoints()
{
    hx::Breakpoints::DeleteAll();
}


void __hxcpp_dbg_deleteBreakpoint(int number)
{
    hx::Breakpoints::Delete(number);
}


void __hxcpp_dbg_breakNow(bool wait)
{
    hx::Breakpoints::BreakNow(wait);
}


void __hxcpp_dbg_continueThreads(int specialThreadNumber, int count)
{
    hx::Breakpoints::ContinueThreads(specialThreadNumber, count);
}


void __hxcpp_dbg_stepThread(int threadNumber, int stepType, int stepCount)
{
    hx::Breakpoints::StepThread(threadNumber, (hx::StepType) stepType,
                                stepCount);
}


Array<Dynamic> __hxcpp_dbg_getStackVariables(int threadNumber,
                                             int stackFrameNumber,
                                             bool unsafe,
                                             Dynamic markThreadNotStopped)
{
    return hx::CallStack::GetStackVariables(threadNumber, stackFrameNumber,
                                            unsafe, markThreadNotStopped);
}


Dynamic __hxcpp_dbg_getStackVariableValue(int threadNumber,
                                          int stackFrameNumber,
                                          String name,
                                          bool unsafe, Dynamic markNonexistent,
                                          Dynamic markThreadNotStopped)
{
    return hx::CallStack::GetVariableValue(threadNumber, stackFrameNumber, 
                                           name, unsafe, markNonexistent,
                                           markThreadNotStopped);
}


Dynamic __hxcpp_dbg_setStackVariableValue(int threadNumber,
                                          int stackFrameNumber,
                                          String name, Dynamic value,
                                          bool unsafe, Dynamic markNonexistent,
                                          Dynamic markThreadNotStopped)
{
    return hx::CallStack::SetVariableValue(threadNumber, stackFrameNumber,
                                           name, value, unsafe,
                                           markNonexistent,
                                           markThreadNotStopped);
}


void __hxcpp_dbg_setNewParameterFunction(Dynamic function)
{
    hx::g_newParameterFunction = function;
    GCAddRoot(&(hx::g_newParameterFunction.mPtr));
}


void __hxcpp_dbg_setNewStackFrameFunction(Dynamic function)
{
    hx::g_newStackFrameFunction = function;
    GCAddRoot(&(hx::g_newStackFrameFunction.mPtr));
}


void __hxcpp_dbg_setNewThreadInfoFunction(Dynamic function)
{
    hx::g_newThreadInfoFunction = function;
    GCAddRoot(&(hx::g_newThreadInfoFunction.mPtr));
}


void __hxcpp_dbg_setAddParameterToStackFrameFunction(Dynamic function)
{
    hx::g_addParameterToStackFrameFunction = function;
    GCAddRoot(&(hx::g_addParameterToStackFrameFunction.mPtr));
}


void __hxcpp_dbg_setAddStackFrameToThreadInfoFunction(Dynamic function)
{
    hx::g_addStackFrameToThreadInfoFunction = function;
    GCAddRoot(&(hx::g_addStackFrameToThreadInfoFunction.mPtr));
}


void __hxcpp_dbg_threadCreatedOrTerminated(int threadNumber, bool created)
{
    // Note that there is a race condition here.  If the debugger is
    // "detaching" at this exact moment, it might set the event handler to
    // NULL during this call.  So latch the handler variable.  This means that
    // the handler might be called even after the debugger thread has set it
    // to NULL, but this should generally be harmless.  Doing this correctly
    // would require some sophisticated locking that just doesn't seem worth
    // it, when the worst that can happen is an extra call to the handler
    // function milliseconds after it's set to NULL ...
    Dynamic handler = hx::g_eventNotificationHandler;

    if (handler == null())
        return;

    // If the thread was not created, remove its call stack
    if (!created)
        hx::CallStack::RemoveCallStack(threadNumber);

    handler(threadNumber, created ? hx::THREAD_CREATED : hx::THREAD_TERMINATED);
}



Dynamic __hxcpp_dbg_checkedThrow(Dynamic toThrow)
{
    if (!hx::CallStack::CanBeCaught(toThrow))
        hx::CriticalErrorHandler(HX_CSTRING("Uncatchable Throw: ") +
                                            toThrow->toString(), true);
    return hx::Throw(toThrow);
}


#endif // HXCPP_DEBUGGER


void __hxcpp_on_line_changed()
{
   #ifdef HXCPP_DEBUGGER
   hx::Breakpoints::HandleBreakpoints();
   #elif defined(HXCPP_STACK_LINE)
   if (hx::sExecutionTrace==hx::exeTraceLines)
   {
      hx::CallStack *stack = hx::CallStack::GetCallerCallStack();
      stack->Trace();
   }
   #endif
}



void hx::__hxcpp_register_stack_frame(hx::StackFrame *inFrame)
{
    hx::CallStack::PushCallerStackFrame(inFrame);
}

    
hx::StackFrame::~StackFrame()
{
    hx::CallStack::PopCallerStackFrame();
}

::String hx::StackFrame::toDisplay()
{
   // Not sure if the following is even possible but the old debugger did it so ...
   char buf[1024];
   if (!fileName || (fileName[0] == '?'))
   {
       snprintf(buf, sizeof(buf), "%s::%s", className, functionName);
   }
   else
   {
      #ifdef HXCPP_STACK_LINE
      // Old-style combined file::class...
      if (!className || !className[0])
          snprintf(buf, sizeof(buf), "%s %s line %d", functionName, fileName, lineNumber);
      else
          snprintf(buf, sizeof(buf), "%s::%s %s line %d",
                className, functionName,
                fileName, lineNumber);
      #else
      // Old-style combined file::class...
      if (!className || !className[0])
        snprintf(buf, sizeof(buf), "%s %s", functionName, fileName);
      else
        snprintf(buf, sizeof(buf), "%s::%s %s",
            className, functionName,
            fileName);
      #endif
   }
   return ::String(buf);
}


::String hx::StackFrame::toString()
{
   // Not sure if the following is even possible but the old debugger did it so ...
   char buf[1024];
   if (!fileName || (fileName[0] == '?'))
   {
       snprintf(buf, sizeof(buf), "%s::%s", className, functionName);
   }
   else
   {
      #ifdef HXCPP_STACK_LINE
      // Old-style combined file::class...
      if (!className || !className[0])
          snprintf(buf, sizeof(buf), "%s::%s::%d", functionName, fileName, lineNumber);
      else
          snprintf(buf, sizeof(buf), "%s::%s::%s::%d",
                className, functionName,
                fileName, lineNumber);
      #else
      // Old-style combined file::class...
      if (!className || !className[0])
        snprintf(buf, sizeof(buf), "%s::%s::0", functionName, fileName);
      else
        snprintf(buf, sizeof(buf), "%s::%s::%s::0",
            className, functionName,
            fileName);
      #endif
   }
   return ::String(buf);
}





void hx::Profiler::Sample(hx::CallStack *stack)
{
    if (mT0 == gProfileClock) {
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

    int depth = stack->GetDepth();

    std::map<const char *, bool> alreadySeen;

    // Add children time in to each stack element
    for (int i = 0; i < (depth - 1); i++) {
        const char *fullName = stack->GetFullNameAtDepth(i);
        ProfileEntry &pe = mProfileStats[fullName];
        if (!alreadySeen.count(fullName)) {
            pe.total += delta;
            alreadySeen[fullName] = true;
        }
        // For everything except the very bottom of the stack, add the time to
        // that child's total with this entry
        pe.children[stack->GetFullNameAtDepth(i + 1)] += delta;
}

    // Add the time into the actual function being executed
    if (depth > 0) {
        mProfileStats[stack->GetFullNameAtDepth(depth - 1)].self += delta;
}
}


#ifdef HXCPP_TELEMETRY
void hx::Telemetry::push_callstack_ids_into(hx::CallStack *stack, std::vector<int> *list)
{
    int size = stack->GetDepth()+1;
    for (int i = 0; i < size; i++) {
        const char *fullName = stack->GetFullNameAtDepth(i);
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

int hx::Telemetry::ComputeCallStackId(hx::CallStack *stack) {
    std::vector<int> callstack;
    int stackId;

    push_callstack_ids_into(stack, &callstack);
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

void hx::Telemetry::StackUpdate(hx::CallStack *stack, StackFrame *pushed_frame)
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

    int size = stack->GetDepth()+1;

    // Collect function names and callstacks (as indexes into the names vector)
    samples->push_back(size);
    push_callstack_ids_into(stack, samples);
    samples->push_back(delta);
}

void hx::Telemetry::HXTAllocation(CallStack *stack, void* obj, size_t inSize, const char* type)
{
    if (ignoreAllocs>0 || !allocations_enabled) return;

    // Optionally ignore from extern::cffi - very expensive to track allocs
    // for every external call, hashes for every SDL event (Lime's
    // ExternalInterface.external_handler()), etc
#ifndef HXCPP_PROFILE_EXTERNS
    int depth = stack->GetDepth();
    if (stack->GetCurrentStackFrame()->className==EXTERN_CLASS_NAME) {
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

    int stackid = ComputeCallStackId(stack);

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

#endif


// The old Debug.cpp had this here.  Why is this here????
namespace hx
{
// }
static void CriticalErrorHandler(String inErr, bool allowFixup)
{
#ifdef HXCPP_DEBUGGER
    if (allowFixup && (hx::g_eventNotificationHandler != null())) {
        if (hx::CallStack::BreakCriticalError(inErr)) {
            return;
        }
        else {
            hx::Throw(HX_CSTRING("Critical Error in the debugger thread"));
        }
    }
#endif

   if (sCriticalErrorHandler!=null())
      sCriticalErrorHandler(inErr);

#ifdef HXCPP_STACK_TRACE
    hx::CallStack::GetCallerCallStack()->BeginCatch(true);
    hx::CallStack::GetCallerCallStack()->DumpExceptionStack();
#endif

    DBGLOG("Critical Error: %s\n", inErr.__s);

#if defined(HX_WINDOWS) && !defined(HX_WINRT)
    MessageBoxA(0, inErr.__s, "Critial Error - program must terminate",
        MB_ICONEXCLAMATION|MB_OK);
#endif

    // Good when using gdb, and to collect a core ...
    #if __has_builtin(__builtin_trap)
    __builtin_trap();
    #else
    (* (int *) 0) = 0;
    #endif

    // Just in case that didn't do it ...
    exit(1);
}

void CriticalError(const String &inErr)
{
    CriticalErrorHandler(inErr, false);
}

HXCPP_EXTERN_CLASS_ATTRIBUTES void NullReference(const char *type, bool allowFixup)
{
#ifdef HXCPP_DEBUGGER
    if (allowFixup && (hx::g_eventNotificationHandler != null())) {
        if (hx::CallStack::BreakCriticalError
        (HX_CSTRING("Null ") + String(type) + HX_CSTRING(" Reference"))) {
            return;
        }
        else {
            hx::Throw(HX_CSTRING("Null pointer reference in the debugger thread"));
        }
    }
#endif

    __hxcpp_dbg_checkedThrow(HX_CSTRING("Null Object Reference"));
}

} // namespace


void __hxcpp_start_profiler(String inDumpFile)
{
#ifdef HXCPP_STACK_TRACE
    hx::CallStack::StartCurrentThreadProfiler(inDumpFile);
#endif
}


void __hxcpp_stop_profiler()
{
#ifdef HXCPP_STACK_TRACE
    hx::CallStack::StopCurrentThreadProfiler();
#endif
}

// These globals are called by HXTelemetry.hx
#ifdef HXCPP_TELEMETRY

  int __hxcpp_hxt_start_telemetry(bool profiler, bool allocations)
  {
  #ifdef HXCPP_STACK_TRACE
    // Operates on the current thread, no mutexes needed
    hx::CallStack::StartCurrentThreadTelemetry(profiler, allocations);
    return __hxcpp_GetCurrentThreadNumber();
  #endif
  }

  void __hxcpp_hxt_stash_telemetry()
  {
  #ifdef HXCPP_STACK_TRACE
    // Operates on the current thread, no mutexes needed
    hx::CallStack::StashCurrentThreadTelemetry();
  #endif
  }

  // Operates on the socket writer thread
  TelemetryFrame* __hxcpp_hxt_dump_telemetry(int thread_num)
  {
  #ifdef HXCPP_STACK_TRACE
    return hx::CallStack::DumpThreadTelemetry(thread_num);
  #endif
  }

  void __hxcpp_hxt_ignore_allocs(int delta)
  {
  #ifdef HXCPP_STACK_TRACE
      hx::CallStack::IgnoreAllocs(delta);
  #endif
  }
#endif


// These globals are called by other cpp files
#ifdef HXCPP_TELEMETRY

void __hxt_new_string(void* obj, int inSize)
{
  #ifdef HXCPP_STACK_TRACE
  hx::CallStack::HXTAllocation(obj, inSize, (const char*)"String");
  #endif
}
void __hxt_new_array(void* obj, int inSize)
{
  #ifdef HXCPP_STACK_TRACE
  hx::CallStack::HXTAllocation(obj, inSize, (const char*)"Array");
  #endif
}
void __hxt_new_hash(void* obj, int inSize)
{
  #ifdef HXCPP_STACK_TRACE
  hx::CallStack::HXTAllocation(obj, inSize, (const char*)"Hash");
  #endif
}
void __hxt_gc_new(void* obj, int inSize, const char* name)
{
  #ifdef HXCPP_STACK_TRACE
  hx::CallStack::HXTAllocation(obj, inSize, name);
  #endif
}
void __hxt_gc_realloc(void* old_obj, void* new_obj, int new_size)
{
  #ifdef HXCPP_STACK_TRACE
  hx::CallStack::HXTRealloc(old_obj, new_obj, new_size);
  #endif
}
void __hxt_gc_start()
{
  hx::CallStack::HXTGCStart();
}
void __hxt_gc_end()
{
  hx::CallStack::HXTGCEnd();
}
void __hxt_gc_after_mark(int gByteMarkID, int ENDIAN_MARK_ID_BYTE)
{
  hx::Telemetry::HXTAfterMark(gByteMarkID, ENDIAN_MARK_ID_BYTE);
}

#endif // HXCPP_TELEMETRY


void __hxcpp_execution_trace(int inLevel)
{
#ifdef HXCPP_STACK_TRACE
    hx::sExecutionTrace = (hx::ExecutionTrace)inLevel;
    #ifdef HXCPP_STACK_LINE
    hx::gShouldCallHandleBreakpoints =
        #ifdef HXCPP_DEBUGGER
          hx::Breakpoints::shoudBreakOnLine() ||
        #endif
         (hx::sExecutionTrace==hx::exeTraceLines);
    #endif
#endif
}


void __hx_dump_stack()
{
#ifdef HXCPP_STACK_TRACE
    hx::CallStack::GetCallerCallStack()->BeginCatch(false);
    hx::CallStack::GetCallerCallStack()->DumpExceptionStack();
#endif
}


void __hx_stack_set_last_exception()
{
#ifdef HXCPP_STACK_TRACE
    hx::CallStack::GetCallerCallStack()->SetLastException();
#endif
}

void __hxcpp_set_stack_frame_line(int inLine)
{
#ifdef HXCPP_STACK_LINE
    hx::CallStack::GetCallerCallStack()->SetLine(inLine);
#endif
}

void __hxcpp_stack_begin_catch()
{
#ifdef HXCPP_STACK_TRACE
    hx::CallStack::GetCallerCallStack()->BeginCatch(false);
#endif
}


Array<String> __hxcpp_get_call_stack(bool inSkipLast)
{
    Array<String> result = Array_obj<String>::__new();

#ifdef HXCPP_STACK_TRACE
    hx::CallStack::GetCurrentCallStackAsStrings(result, inSkipLast);
#endif

    return result;
}


Array<String> __hxcpp_get_exception_stack()
{
    Array<String> result = Array_obj<String>::__new();

#ifdef HXCPP_STACK_TRACE
    hx::CallStack::GetCurrentExceptionStackAsStrings(result);
#endif

    return result;
}

void __hxcpp_set_critical_error_handler(Dynamic inHandler)
{
   setStaticHandler(hx::sCriticalErrorHandler,inHandler);
}

void __hxcpp_set_debugger_info(const char **inAllClasses, const char **inFullPaths)
{
   __all_classes = inAllClasses;
   __all_files_fullpath = inFullPaths;
}
   
