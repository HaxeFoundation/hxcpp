#include <hxcpp.h>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <hx/Debug.h>
#include <hx/Thread.h>
#include <hx/OS.h>
#include <hx/QuickVec.h>


// Newer versions of haxe compiler will set these too (or might be null for haxe 3.0)
static const char **__all_files_fullpath = 0;
static const char **__all_classes = 0;


#define HXCPP_DEBUG_HASHES



namespace hx
{

// These are emitted elsewhere by the haxe compiler
extern const char *__hxcpp_all_files[];

// This global boolean is set whenever there are any breakpoints (normal or
// immediate), and can relatively quickly gate debugged threads from making
// more expensive breakpoint check calls when there are no breakpoints set.
// Note that there is no lock to protect this.  Volatile is used to ensure
// that within a function call, the value of gShouldCallHandleBreakpoints is
// not cached in a register and thus not properly checked within the function
// call.
volatile bool gShouldCallHandleBreakpoints = false;

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


// This is the thread number of the debugger thread, extracted from
// information about the thread that called 
// __hxcpp_dbg_setEventNotificationHandler
static unsigned int g_debugThreadNumber = -1;

ExecutionTrace sExecutionTrace = exeTraceOff;


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


const char *_hx_dbg_find_scriptable_class_name(String className);


static HxMutex gMutex;
static std::map<int, DebuggerContext *> gMap;
static std::list<DebuggerContext *> gList;

class Breakpoints;
Breakpoints *ReleaseBreakpointsLocked(Breakpoints *inBreakpoints);

class DebuggerContext
{
public:
   int          mThreadNumber;
   StackContext *mStackContext;
   bool         mCanStop;
   int          mBreakpoint;
   // Always 'const' strings (no GC)
   String       mCriticalError;
   DebugStatus  mStatus;
   int          mStepLevel;
   Breakpoints  *mBreakpoints;

   // Waiting for continue
   bool         mWaiting;
   HxMutex      mWaitMutex;
   HxSemaphore  mWaitSemaphore;
   int          mContinueCount;
   bool         mAttached;



   DebuggerContext(StackContext *inStack)
   {
      mStackContext = inStack;
      reset();
   }
   ~DebuggerContext()
   {
      if (mAttached)
         detach();
   }

   void attach(StackContext *inStack)
   {
      mAttached = true;
      mStackContext = inStack;
      mThreadNumber = mStackContext->mThreadId;
      mStatus = DBG_STATUS_RUNNING;
      gMutex.Lock();
      gList.push_back(this);
      gMap[mThreadNumber] = this;
      gMutex.Unlock();

      // Note that there is a race condition here.  If the debugger is
      // "detaching" at this exact moment, it might set the event handler to
      // NULL during this call.  So latch the handler variable.  This means that
      // the handler might be called even after the debugger thread has set it
      // to NULL, but this should generally be harmless.  Doing this correctly
      // would require some sophisticated locking that just doesn't seem worth
      // it, when the worst that can happen is an extra call to the handler
      // function milliseconds after it's set to NULL ...

      Dynamic handler = hx::g_eventNotificationHandler;
      if (handler != null())
         handler(mThreadNumber, hx::THREAD_CREATED);
   }

   void detach()
   {
      mAttached = false;
      gMutex.Lock();
      gList.remove(this);
      gMap.erase(mThreadNumber);
      mBreakpoints = ReleaseBreakpointsLocked(mBreakpoints);
      gMutex.Unlock();
      reset();

      Dynamic handler = hx::g_eventNotificationHandler;
      if (handler != null())
         handler(mThreadNumber, hx::THREAD_TERMINATED);
   }

   void enable(bool inEnable)
   {
      mCanStop = inEnable;
   }

   void reset()
   {
      mCanStop = false;
      mBreakpoint = 0;
      mBreakpoints = 0;
      mCriticalError = null();
      mStatus = DBG_STATUS_INVALID;
      mStepLevel = 0;
      mWaiting = false;
      mContinueCount = 0;
      mThreadNumber = -1;
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
        std::list<DebuggerContext *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            DebuggerContext *stack = *iter++;
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
            DebuggerContext *stack = gMap[threadNumbers[i]];
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



    void DoBreak(DebugStatus status, int breakpoint, const String *criticalErrorDescription)
    {
        // Update status
        mStatus = status;
        mBreakpoint = breakpoint;
        if (criticalErrorDescription)
           mCriticalError = criticalErrorDescription->makePermanent();

        // This thread cannot stop while making the callback
        mCanStop = false;

        mWaitMutex.Lock();
        mWaiting = true;
        mWaitMutex.Unlock();

        // Call the handler to announce the status.
        StackFrame *frame = mStackContext->getCurrentStackFrame();
        // Record this before g_eventNotificationHandler is run, since it might change in there
        mStackContext->mDebugger->mStepLevel = mStackContext->getDepth();
        g_eventNotificationHandler
            (mThreadNumber, THREAD_STOPPED, mStackContext->mDebugger->mStepLevel,
             String(frame->position->className), String(frame->position->functionName),
             String(frame->position->fileName), frame->lineNumber);

        if (mWaiting)
        {
           // Wait until the debugger thread sets mWaiting to false and signals
           // the semaphore
           mWaitMutex.Lock();

           while (mWaiting) {
               mWaitMutex.Unlock();
               hx::EnterGCFreeZone();
               mWaitSemaphore.Wait();
               hx::ExitGCFreeZone();
               mWaitMutex.Lock();
           }

           mWaitMutex.Unlock();
        }

        // Save the breakpoint status in the call stack so that queries for
        // thread info will know the current status of the thread
        mStatus = DBG_STATUS_RUNNING;
        mBreakpoint = -1;

        // Announce the new status
        Dynamic handler = hx::g_eventNotificationHandler;
        if (handler!=null())
           handler(mThreadNumber, THREAD_STARTED);

        // Can stop again
        mCanStop = true;
    }


    // Wait for someone to call Continue() on this call stack.  Really only
    // the thread that owns this call stack should call Wait().
    void Break(DebugStatus status, int breakpoint,
               const String *criticalErrorDescription)
    {
        // If break status is break immediate, then eliminate any residual
        // continue count from the last continue.
        if (status == DBG_STATUS_STOPPED_BREAK_IMMEDIATE) {
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

};


DebuggerContext *dbgCtxCreate(StackContext *inStack) { return new DebuggerContext(inStack); }
void dbgCtxDestroy(DebuggerContext *ctx) { delete ctx; }
void dbgCtxAttach(DebuggerContext *ctx, StackContext *inStack) { ctx->attach(inStack); }
void dbgCtxDetach(DebuggerContext *ctx) { ctx->detach(); }
void dbgCtxEnable(DebuggerContext *ctx, bool inEnable) { ctx->enable(inEnable); }





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

    void RemoveRef()
    {
       if (--mRefCount == 0)
         delete this;
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
        
        Breakpoints *newBreakpoints = new Breakpoints(gBreakpoints, ret, className, functionName);
        
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
           DebuggerContext::WaitForAllThreadsToStop();
        }
    }

    static void ContinueThreads(int specialThreadNumber, int continueCount)
    {
        gStepType = STEP_NONE;

        gShouldCallHandleBreakpoints = !gBreakpoints->IsEmpty() || (sExecutionTrace==exeTraceLines);

        gMutex.Lock();

        // All threads get continued, but specialThreadNumber only for count
        std::list<DebuggerContext *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            DebuggerContext *stack = *iter++;
            if (stack->mThreadNumber == specialThreadNumber) {
                stack->Continue(continueCount);
            }
            else {
                stack->Continue(1);
            }
        }

        gMutex.Unlock();
    }

    static void StepThread(int threadNumber, StepType stepType, int stepCount)
    {
        // Continue the thread, but set its step first
        gStepThread = threadNumber;
        gStepType = stepType;
        gStepCount = stepCount;
        
        gMutex.Lock();

        std::list<DebuggerContext *>::iterator iter = gList.begin();
        while (iter != gList.end()) {
            DebuggerContext *stack = *iter++;
            if (stack->mThreadNumber == threadNumber) {
                gStepLevel = stack->mStackContext->mDebugger->mStepLevel;
                stack->Continue(1);
                break;
            }
        }
        gMutex.Unlock();

    }

    // Note that HandleBreakpoints is called immediately after a read memory
    // barrier by the HX_STACK_LINE macro
    static void HandleBreakpoints(hx::StackContext *stack)
    {
        // This will be set to a valid status if a stop is needed
        DebugStatus breakStatus = DBG_STATUS_INVALID;
        int breakpointNumber = -1;


        // The debug thread never breaks
        if (stack->mThreadId == g_debugThreadNumber) {
            return;
        }

        if (sExecutionTrace==exeTraceLines)
           stack->tracePosition();

        // Handle possible immediate break
        if (gStepType == STEP_NONE) {
            // No stepping
        }
        else if (gStepType == STEP_INTO) {
            if ((gStepThread == -1) ||
                (gStepThread == stack->mThreadId)) {
                breakStatus = DBG_STATUS_STOPPED_BREAK_IMMEDIATE;
            }
        }
        else {
            if ((gStepThread == -1) ||
                (gStepThread == stack->mThreadId)) {
                if (gStepType == STEP_OVER) {
                    if (stack->getDepth() <= gStepLevel) {
                        breakStatus = DBG_STATUS_STOPPED_BREAK_IMMEDIATE;
                    }
                }
                else { // (gStepType == STEP_OUT)
                    if (stack->getDepth() < gStepLevel) {
                        breakStatus = DBG_STATUS_STOPPED_BREAK_IMMEDIATE;
                    }
                }
            }
        }

        // If didn't hit any immediate breakpoints, check for set breakpoints
        if (breakStatus == DBG_STATUS_INVALID) {
            Breakpoints *breakpoints = stack->mDebugger->mBreakpoints;
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
                stack->mDebugger->mBreakpoints = breakpoints;
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
                stack->mDebugger->mBreakpoints = breakpoints;
                breakpoints->AddRef();
                gMutex.Unlock();
            }

            // If there are breakpoints, then may need to break in one
            if (!breakpoints->IsEmpty())
            {
               StackFrame *frame = stack->getCurrentStackFrame();
               if (!breakpoints->QuickRejectClassFunc(frame->position->classFuncHash))
               {
                  // Check for class:function breakpoint if this is the
                  // first line of the stack frame
                  if (frame->lineNumber == frame->position->firstLineNumber)
                      breakpointNumber = breakpoints->FindClassFunctionBreakpoint(frame);
               }

               // If still haven't hit a break point, check for file:line
               // breakpoint
               if (breakpointNumber == -1 && !breakpoints->QuickRejectFileLine(frame->position->fileHash))
                  breakpointNumber = breakpoints->FindFileLineBreakpoint(frame);

               if (breakpointNumber != -1)
                  breakStatus = DBG_STATUS_STOPPED_BREAKPOINT;
            }
        }

        // If no breakpoint of any kind was found, then don't break
        if (breakStatus == DBG_STATUS_INVALID) {
            return;
        }
        // If the thread has been put into no stop mode, it can't stop
        if (!stack->mDebugger->mCanStop) {
            return;
        }

        // If the break was an immediate break, and there was a step count,
        // just decrement the step count
        if (breakStatus == DBG_STATUS_STOPPED_BREAK_IMMEDIATE) {
            if (gStepCount > 1) {
                gStepCount -= 1;
                return;
            }
        }

        // Now break, which will wait until the debugger thread continues
        // the thread
        stack->mDebugger->Break(breakStatus, breakpointNumber, 0);
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
         if (breakpoint.isFileLine &&
             #ifdef HXCPP_DEBUG_HASHES
             breakpoint.hash==inFrame->position->fileHash &&
             #endif
             (breakpoint.lineNumber == inFrame->lineNumber) &&
             !strcmp(breakpoint.fileOrClassName.c_str(),inFrame->position->fileName) )
            return breakpoint.number;
      }
      return -1;
   }

   int FindClassFunctionBreakpoint(StackFrame *inFrame)
   {
      for (int i = 0; i < mBreakpointCount; i++)
      {
         Breakpoint &breakpoint = mBreakpoints[i];
         if (!breakpoint.isFileLine &&
             #ifdef HXCPP_DEBUG_HASHES
             breakpoint.hash==inFrame->position->classFuncHash &&
             #endif
             !strcmp(breakpoint.fileOrClassName.c_str(), inFrame->position->className)  &&
             !strcmp(breakpoint.functionName.c_str(), inFrame->position->functionName) )
            return breakpoint.number;
      }
      return -1;
   }

   // Looks up the "interned" version of the name, for faster compares
   // when evaluating breakpoints
   static const char *LookupFileName(String fileName)
   {
      for (const char **ptr = hx::__hxcpp_all_files; *ptr; ptr++)
      {
         if (!strcmp(*ptr, fileName))
            return *ptr;
      }

      #ifdef HXCPP_SCRIPTABLE
      Array< ::String> ret = Array_obj< ::String>::__new();
      __hxcpp_dbg_getScriptableFiles(ret);
      for(int i=0;i<ret->length;i++)
         if (ret[i]==fileName)
            return (ret[i]).makePermanent().utf8_str();

      ret = Array_obj< ::String>::__new();
      __hxcpp_dbg_getScriptableFilesFullPath(ret);
      for(int i=0;i<ret->length;i++)
         if (ret[i]==fileName)
            return (ret[i]).makePermanent().utf8_str();
      #endif

      return 0;
   }

   static const char *LookupClassName(String className)
   {
      if (__all_classes)
         for (const char **ptr = __all_classes; *ptr; ptr++)
         {
            if (!strcmp(*ptr, className.raw_ptr()))
               return *ptr;
         }

      #ifdef HXCPP_SCRIPTABLE
      Array< ::String> ret = Array_obj< ::String>::__new();
      __hxcpp_dbg_getScriptableClasses(ret);
      for(int i=0;i<ret->length;i++)
         if (ret[i]==className)
            return ret[i].makePermanent().raw_ptr();
      #endif

      return 0;
   }


private:

    int mRefCount;
    int mBreakpointCount;
    int mNotInAnyClassFunc;
    int mNotInAnyFileLine;
    Breakpoint *mBreakpoints;

    static int gNextBreakpointNumber;
    static Breakpoints * volatile gBreakpoints;
    static StepType gStepType;
    static int gStepLevel;
    static int gStepThread; // If -1, all threads are targeted
    static int gStepCount;
};



/* static */ int Breakpoints::gNextBreakpointNumber;
/* static */ Breakpoints * volatile Breakpoints::gBreakpoints = new Breakpoints();
/* static */ StepType Breakpoints::gStepType = STEP_NONE;
/* static */ int Breakpoints::gStepLevel;
/* static */ int Breakpoints::gStepThread = -1;
/* static */ int Breakpoints::gStepCount = -1;


Breakpoints *ReleaseBreakpointsLocked(Breakpoints *inBreakpoints)
{
   if (inBreakpoints)
      inBreakpoints->RemoveRef();
   return 0;
}




// Gets a ThreadInfo for a thread
static Dynamic GetThreadInfo(int threadNumber, bool unsafe)
{
    if (threadNumber == g_debugThreadNumber)
        return null();

    DebuggerContext *stack = 0;

    gMutex.Lock();

    if (gMap.count(threadNumber) == 0)
    {
        gMutex.Unlock();
        return null();
    }
    else
        stack = gMap[threadNumber];

    if ((stack->mStatus == DBG_STATUS_RUNNING) && !unsafe)
    {
        gMutex.Unlock();
        return null();
    }

    // It's safe to release the mutex here, because the stack to be
    // converted is either for a thread that is not running (and thus
    // the stack cannot be altered while the conversion is in progress),
    // or unsafe mode has been invoked
    gMutex.Unlock();


    Dynamic ret = g_newThreadInfoFunction
        (stack->mThreadNumber, stack->mStatus, stack->mBreakpoint,
         stack->mCriticalError);

    int size = stack->mStackContext->getDepth();
    for (int i = 0; i < size; i++)
    {
       StackFrame *frame = stack->mStackContext->getStackFrame(i);
       #ifdef HXCPP_STACK_LINE
         Dynamic info = g_newStackFrameFunction
              (String(frame->position->fileName), String(frame->lineNumber),
               String(frame->position->className), String(frame->position->functionName));
       #else
         Dynamic info = g_newStackFrameFunction
              (String(frame->position->fileName), String(frame->position->firstLineNumber),
               String(frame->position->className), String(frame->position->functionName));
       #endif

        g_addStackFrameToThreadInfoFunction(ret, info);
    }

    return ret;
}

// Gets a ThreadInfo for each Thread
static ::Array<Dynamic> GetThreadInfos()
{
    gMutex.Lock();

    // Latch the current thread numbers from the current list of call
    // stacks.
    std::list<int> threadNumbers;
    std::list<DebuggerContext *>::iterator stack_iter = gList.begin();
    while (stack_iter != gList.end()) {
        DebuggerContext *stack = *stack_iter++;
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

    std::list<DebuggerContext *>::iterator iter = gList.begin();
    while (iter != gList.end()) {
        DebuggerContext *ctx = *iter++;
        if (ctx->mThreadNumber == threadNumber) {
            if ((ctx->mStatus == DBG_STATUS_RUNNING) && !unsafe) {
                ret->push(markThreadNotStopped);
                gMutex.Unlock();
                return ret;
            }
            StackContext *stack = ctx->mStackContext;
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

            #ifdef HXCPP_STACK_SCRIPTABLE
            StackFrame *scriptFrame = stack->mStackFrames[stackFrameNumber];
            if (scriptFrame)
               __hxcpp_dbg_getScriptableVariables(scriptFrame, ret);
            #endif
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

    DebuggerContext *ctx;

    gMutex.Lock();

    if (gMap.count(threadNumber) == 0) {
        gMutex.Unlock();
        return markNonexistent;
    }
    else {
        ctx = gMap[threadNumber];
    }

    if ((ctx->mStatus == DBG_STATUS_RUNNING) && !unsafe) {
        gMutex.Unlock();
        return markThreadNotStopped;
    }

    // Don't need the lock any more, the thread is not running
    gMutex.Unlock();

    StackContext *stack = ctx->mStackContext;

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

    #ifdef HXCPP_STACK_SCRIPTABLE
    StackFrame *scriptFrame = stack->mStackFrames[stackFrameNumber];
    if (scriptFrame)
    {
       Dynamic result;
       if (__hxcpp_dbg_getScriptableValue(scriptFrame, name, result))
          return result;
    }
    #endif


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

    DebuggerContext *ctx;

    gMutex.Lock();

    if (gMap.count(threadNumber) == 0) {
        gMutex.Unlock();
        return null();
    }
    else {
        ctx = gMap[threadNumber];
    }

    if ((ctx->mStatus == DBG_STATUS_RUNNING) && !unsafe) {
        gMutex.Unlock();
        return markThreadNotStopped;
    }

    // Don't need the lock any more, the thread is not running
    gMutex.Unlock();

    StackContext *stack = ctx->mStackContext;
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

    #ifdef HXCPP_STACK_SCRIPTABLE
    StackFrame *scriptFrame = stack->mStackFrames[stackFrameNumber];
    if (scriptFrame)
    {
       if (__hxcpp_dbg_setScriptableValue(scriptFrame, name, value))
       {
          __hxcpp_dbg_getScriptableValue(scriptFrame, name, value);
          return value;
       }
    }
    #endif
    

    return markNonexistent;
}


static bool CanBeCaught(Dynamic e)
{
   hx::JustGcStackFrame frame;

   QuickVec<StackFrame *> &frames = frame.ctx->mStackFrames;

   for(int i=frames.size()-1; i>=0; i--)
   {
        StackCatchable *catchable = frames[i]->catchables;
        while (catchable) {
            if (catchable->Catches(e)) {
                return true;
            }
            catchable = catchable->mNext;
        }
    }

    return false;
}







} // namespace hx






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
   hx::JustGcStackFrame frame;
   frame.ctx->mDebugger->enable(enable);
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

    #ifdef HXCPP_SCRIPTABLE
    __hxcpp_dbg_getScriptableFiles(ret);
    #endif

    return ret;
}

Array< ::String> __hxcpp_dbg_getFilesFullPath()
{
    Array< ::String> ret = Array_obj< ::String>::__new();

    for (const char **ptr = __all_files_fullpath;ptr && *ptr; ptr++)
    {
        ret->push(String(*ptr));
    }

    #ifdef HXCPP_SCRIPTABLE
    __hxcpp_dbg_getScriptableFilesFullPath(ret);
    #endif

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

    #ifdef HXCPP_SCRIPTABLE
    __hxcpp_dbg_getScriptableClasses(ret);
    #endif

    return ret;
}


Array<Dynamic> __hxcpp_dbg_getThreadInfos()
{
    return hx::GetThreadInfos();
}


Dynamic __hxcpp_dbg_getThreadInfo(int threadNumber, bool unsafe)
{
    return hx::GetThreadInfo(threadNumber, unsafe);
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
    return hx::GetStackVariables(threadNumber, stackFrameNumber,
                                            unsafe, markThreadNotStopped);
}


Dynamic __hxcpp_dbg_getStackVariableValue(int threadNumber,
                                          int stackFrameNumber,
                                          String name,
                                          bool unsafe, Dynamic markNonexistent,
                                          Dynamic markThreadNotStopped)
{
    return hx::GetVariableValue(threadNumber, stackFrameNumber, 
                                           name, unsafe, markNonexistent,
                                           markThreadNotStopped);
}


Dynamic __hxcpp_dbg_setStackVariableValue(int threadNumber,
                                          int stackFrameNumber,
                                          String name, Dynamic value,
                                          bool unsafe, Dynamic markNonexistent,
                                          Dynamic markThreadNotStopped)
{
    return hx::SetVariableValue(threadNumber, stackFrameNumber,
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


Dynamic __hxcpp_dbg_checkedThrow(Dynamic toThrow)
{
    if (!hx::CanBeCaught(toThrow))
        hx::CriticalError(HX_CSTRING("Uncatchable Throw: ") + toThrow->toString(),true);

    return hx::Throw(toThrow);
}


Dynamic __hxcpp_dbg_checkedRethrow(Dynamic toThrow)
{
    if (!hx::CanBeCaught(toThrow))
        hx::CriticalError(HX_CSTRING("Uncatchable Throw: ") + toThrow->toString(),true);

    return hx::Rethrow(toThrow);
}



void __hxcpp_on_line_changed(hx::StackContext *stack)
{
   hx::Breakpoints::HandleBreakpoints(stack);
   if (hx::sExecutionTrace==hx::exeTraceLines)
      stack->tracePosition();
}


bool __hxcpp_dbg_fix_critical_error(String inErr)
{
   if (hx::g_eventNotificationHandler != null())
   {
      hx::JustGcStackFrame frame;
      hx::DebuggerContext *stack = frame.ctx->mDebugger;

      //if the thread with the critical error is the debugger one,
      //we don't break as it would block debugging since the debugger thread
      //is the only one which can wake up application threads.
      if (stack->mThreadNumber == hx::g_debugThreadNumber) {
        hx::Throw(HX_CSTRING("Critical Error in the debugger thread"));
      }

      stack->DoBreak(hx::DBG_STATUS_STOPPED_CRITICAL_ERROR, -1, &inErr);
      return true;
   }
   return false;
}


void __hxcpp_execution_trace(int inLevel)
{
    hx::sExecutionTrace = (hx::ExecutionTrace)inLevel;
    hx::gShouldCallHandleBreakpoints =
          hx::Breakpoints::shoudBreakOnLine() || (hx::sExecutionTrace==hx::exeTraceLines);
}


void __hxcpp_set_debugger_info(const char **inAllClasses, const char **inFullPaths)
{
   __all_classes = inAllClasses;
   __all_files_fullpath = inFullPaths;
}
   


