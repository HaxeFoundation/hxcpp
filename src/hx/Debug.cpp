#include <hxcpp.h>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <hx/Debug.h>
#include <hx/Thread.h>
#include "QuickVec.h"

#ifdef ANDROID
#include <android/log.h>
#define DBGLOG(...) __android_log_print(ANDROID_LOG_INFO, "HXCPP", __VA_ARGS__)
#else
#include <stdio.h>
#define DBGLOG printf
#endif

#if _MSC_VER
#define snprintf _snprintf
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
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


namespace hx
{

static void CriticalErrorHandler(String inErr, bool allowFixup);

// These are emitted elsewhere by the haxe compiler
extern const char *__hxcpp_all_files[];
extern const char *__hxcpp_all_classes[];

// This global boolean is set whenever there are any breakpoints (normal or
// immediate), and can relatively quickly gate debugged threads from making
// more expensive breakpoint check calls when there are no breakpoints set.
// Note that there is no lock to protect this.  Volatile is used to ensure
// that within a function call, the value of gShouldCallHandleBreakpoints is
// not cached in a register and thus not properly checked within the function
// call.
volatile bool gShouldCallHandleBreakpoints;

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
#if defined(HX_WINDOWS)
#ifndef HX_WINRT
            _beginthreadex(0, 0, ProfileMainLoop, 0, 0, 0);
#else
        // TODO
#endif
#else
            pthread_t result;
            pthread_create(&result, 0, ProfileMainLoop, 0);
   #endif
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

#define PROFILE_PRINT(...)                      \
        if (out) {                              \
            fprintf(out, __VA_ARGS__);          \
        }                                       \
        else {                                  \
            DBGLOG(__VA_ARGS__);                \
        }

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
#ifdef HX_WINDOWS
#ifndef HX_WINRT
            Sleep(millis);
#else
            // TODO
#endif
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

    String mDumpFile;
    int mT0;
    std::map<const char *, ProfileEntry> mProfileStats;

    static MyMutex gThreadMutex;
    static int gThreadRefCount;
    static int gProfileClock;
};
/* static */ MyMutex Profiler::gThreadMutex;
/* static */ int Profiler::gThreadRefCount;
/* static */ int Profiler::gProfileClock;


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

        mUnwindException = false;
        mStackFrames.push_back(frame);
    }

    void PopStackFrame()
    {
        if (mProfiler)
            mProfiler->Sample(this);

        if (mUnwindException)
        {
           // Use default operator=
           mExceptionStack.push( *mStackFrames.back() );
        }

        mStackFrames.pop_back();
    }


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
            StackFrame *frame = stack->mStackFrames[i];
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
        GetCallerCallStack()->DoBreak
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
        MySemaphore timeoutSem;
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
            EXCEPTION_PRINT("Called from %s\n", mExceptionStack[i].toString().__s);
        }
   }

private:

    CallStack(int threadNumber)
        : mThreadNumber(threadNumber), mCanStop(true), mStatus(STATUS_RUNNING),
          mBreakpoint(-1), mWaiting(false), mContinueCount(0), mProfiler(0),
          mUnwindException(false)
    {
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
    MyMutex mWaitMutex;
    bool mWaiting;
    MySemaphore mWaitSemaphore;
    int mContinueCount;

    // Profiling support
    Profiler *mProfiler;

    // gMutex protects gMap and gList
    static MyMutex gMutex;
    static std::map<int, CallStack *> gMap;
    static std::list<CallStack *> gList;
};
/* static */ MyMutex CallStack::gMutex;
/* static */ std::map<int, CallStack *> CallStack::gMap;
/* static */ std::list<CallStack *> CallStack::gList;

#ifdef HXCPP_DEBUGGER
class Breakpoints
{
public:
    static int Hash(int value, const char *inString)
    {
       while(*inString)
          value = value*223 + *inString++;
       return value;
    }

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
        gShouldCallHandleBreakpoints = (gStepType != STEP_NONE);

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
                gShouldCallHandleBreakpoints = (gStepType != STEP_NONE);
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

        gShouldCallHandleBreakpoints = !gBreakpoints->IsEmpty();

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
       calcCombinedHash();
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
        mBreakpoints[toCopy->mBreakpointCount].hash = Hash(0, fileName);
        calcCombinedHash();
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
        int hash = Hash(0,className);
        hash = Hash(hash,".");
        hash = Hash(hash,functionName.c_str());
        //printf("%s.%s -> %08x\n", className, functionName.c_str(), hash );
        mBreakpoints[toCopy->mBreakpointCount].hash = hash;
        calcCombinedHash();
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
      calcCombinedHash();
   }

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
      return inHash & mNotInAnyClassFunc;
   }

   inline bool QuickRejectFileLine(int inHash)
   {
      return inHash & mNotInAnyFileLine;
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
        for (const char **ptr = hx::__hxcpp_all_classes; *ptr; ptr++) {
            if (!strcmp(*ptr, className)) {
                return *ptr;
         }
      }
        return 0;
   }

private:

    int mRefCount;
    int mBreakpointCount;
    int mNotInAnyClassFunc;
    int mNotInAnyFileLine;
    Breakpoint *mBreakpoints;

    static MyMutex gMutex;
    static int gNextBreakpointNumber;
    static Breakpoints * volatile gBreakpoints;
    static StepType gStepType;
    static int gStepLevel;
    static int gStepThread; // If -1, all threads are targeted
    static int gStepCount;
};

/* static */ MyMutex Breakpoints::gMutex;
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
 

Array<Dynamic> __hxcpp_dbg_getFiles()
   {
    Array< ::String> ret = Array_obj< ::String>::__new();

    for (const char **ptr = hx::__hxcpp_all_files; *ptr; ptr++) {
        ret->push(String(*ptr));
      }
 
    return ret;
   }


Array<Dynamic> __hxcpp_dbg_getClasses()
   {
    Array< ::String> ret = Array_obj< ::String>::__new();

    for (const char **ptr = hx::__hxcpp_all_classes; *ptr; ptr++) {
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

    if (handler == null()) {
        return;
}

    // If the thread was not created, remove its call stack
    if (!created) {
        hx::CallStack::RemoveCallStack(threadNumber);
}

    handler(threadNumber, created ? hx::THREAD_CREATED : hx::THREAD_TERMINATED);
}


void __hxcpp_dbg_HandleBreakpoints()
{
    hx::Breakpoints::HandleBreakpoints();
}


Dynamic __hxcpp_dbg_checkedThrow(Dynamic toThrow)
      {
    if (!hx::CallStack::CanBeCaught(toThrow)) {
        hx::CriticalErrorHandler(HX_CSTRING("Uncatchable Throw"), true);
      }

    return hx::Throw(toThrow);
}


#endif // HXCPP_DEBUGGER


void hx::__hxcpp_register_stack_frame(hx::StackFrame *inFrame)
{
    hx::CallStack::PushCallerStackFrame(inFrame);
}

    
hx::StackFrame::~StackFrame()
{
    hx::CallStack::PopCallerStackFrame();
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

void NullReference(const char *type, bool allowFixup)
{
    CriticalErrorHandler(String("Null ") + String(type) + String(" Reference"),
                         allowFixup);
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

