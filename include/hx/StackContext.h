#ifndef HX_STACK_CONTEXT_H
#define HX_STACK_CONTEXT_H

#include "QuickVec.h"

#ifdef HXCPP_SINGLE_THREADED_APP
  #define HX_CTX_GET ::hx::gMainThreadContext
#else
  #define HX_CTX_GET ((::hx::StackContext *)::hx::tlsStackContext)
#endif

// Set:
// HXCPP_STACK_LINE if stack line numbers need to be tracked
// HXCPP_STACK_TRACE if stack frames need to be tracked

// Keep track of lines - more accurate stack traces for exceptions, also
// needed for the debugger
#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER)) && !defined(HXCPP_STACK_LINE)
#define HXCPP_STACK_LINE
#endif

// Do we need to keep a stack trace - for basic exception handelling, also needed for the debugger
// At a minimum, you can track the functions calls and nothing else
#if (defined(HXCPP_STACK_LINE) || defined(HXCPP_TELEMETRY) || defined(HXCPP_PROFILER) || defined(HXCPP_DEBUG)) && !defined(HXCPP_STACK_TRACE)
   #define HXCPP_STACK_TRACE
#endif

#if defined(HXCPP_STACK_TRACE) && defined(HXCPP_SCRIPTABLE)
#define HXCPP_STACK_SCRIPTABLE
#endif
// HXCPP_DEBUG_HASH == HXCPP_DEBUGGER
// HXCPP_STACK_VARS == HXCPP_DEBUGGER


// HX_STACKFRAME(pos)    - tracks position according to define.  May be optimized away.
// HX_GC_STACKFRAME(pos) - tracks position according to define, but is never optimized away
// HX_JUST_GC_STACKFRAME - never tracks position, never optimized away

// Setup the _hx_stackframe variable
#ifdef HXCPP_STACK_TRACE
   // Setup the 'HX_DEFINE_STACK_FRAME' 'HX_LOCAL_STACK_FRAME' macro.
   // This will be empty, just track functions(release), track functions and lines(debug) or track everything (debugger)
   #define HX_DECLARE_STACK_FRAME(name) extern ::hx::StackPosition name;

   #ifdef HXCPP_STACK_LINE

      #ifdef HXCPP_DEBUGGER
         #define HX_DEFINE_STACK_FRAME(varName, className, functionName, classFunctionHash, fullName,fileName,     \
                             lineNumber, fileHash ) \
          ::hx::StackPosition varName(className, functionName, fullName, fileName, lineNumber, \
                                            classFunctionHash, fileHash);
      #else
         #define HX_DEFINE_STACK_FRAME(varName, className, functionName, classFunctionHash, fullName,fileName,     \
                          lineNumber, fileHash ) \
          ::hx::StackPosition varName(className, functionName, fullName, fileName, lineNumber);
      #endif
   #else

      #define HX_DEFINE_STACK_FRAME(varName, className, functionName, classFunctionHash, fullName,fileName,     \
                          lineNumber, fileHash ) \
      ::hx::StackPosition varName(className, functionName, fullName, fileName);

   #endif

   #define HX_LOCAL_STACK_FRAME(a,b,c,d,e,f,g,h) static HX_DEFINE_STACK_FRAME(a,b,c,d,e,f,g,h)

   // Haxe < 330 does not create position pointers, and we must use a local one.
   // This code will hst the 'HX_STACK_FRAME' macro
   #define HX_STACK_FRAME(className, functionName, classFunctionHash, fullName,fileName, lineNumber, fileHash ) \
      HX_DEFINE_STACK_FRAME(__stackPosition, className, functionName, classFunctionHash, fullName,fileName, lineNumber, fileHash ) \
      ::hx::StackFrame _hx_stackframe(&__stackPosition);

   // Newer code will use the HX_STACKFRAME macro
   #define HX_STACKFRAME(pos) ::hx::StackFrame _hx_stackframe(pos);
   #define HX_GC_STACKFRAME(pos) ::hx::StackFrame _hx_stackframe(pos);

   // Must record the stack state at the catch
   #define HX_STACK_BEGIN_CATCH __hxcpp_stack_begin_catch();
   #define HX_JUST_GC_STACKFRAME ::hx::JustGcStackFrame _hx_stackframe;
   #define HX_CTX _hx_stackframe.ctx
#else
   // No need to track frame
   #define HX_DECLARE_STACK_FRAME(name)
   #define HX_STACK_BEGIN_CATCH
   #define HX_DEFINE_STACK_FRAME(__stackPosition, className, functionName, classFunctionHash, fullName,fileName, lineNumber, fileHash )
   #define HX_LOCAL_STACK_FRAME(a,b,c,d,e,f,g,h)
   #define HX_STACK_FRAME(className, functionName, classFunctionHash, fullName,fileName, lineNumber, fileHash )
   #define HX_STACKFRAME(pos)
   #define HX_JUST_GC_STACKFRAME ::hx::StackContext *_hx_ctx = HX_CTX_GET;
   #define HX_GC_STACKFRAME(pos) HX_JUST_GC_STACKFRAME
   #define HX_CTX _hx_ctx
#endif

#define HX_GC_CTX HX_CTX


// Setup debugger catchable and variable macros...
#ifdef HXCPP_DEBUGGER

   // Emitted at the beginning of every instance fuction.  ptr is "this".
   // Only if stack variables are to be tracked
   #define HX_STACK_THIS(ptr) ::hx::StackThis __stackthis(_hx_stackframe.variables, ptr);

   // Emitted at the beginning of every function that takes arguments.
   // name is the name of the argument.
   // For the lifetime of this object, the argument will be in the [arguments]
   // list of the stack frame in which the arg was declared
   // Only if stack variables are to be tracked
   #define HX_STACK_ARG(cpp_var, haxe_name) \
       ::hx::StackVariable __stackargument_##cpp_var(_hx_stackframe.variables, true, haxe_name, &cpp_var);

   // Emitted whenever a Haxe value is pushed on the stack.  cpp_var is the local
   // cpp variable, haxe_name is the name that was used in haxe for it
   // Only if stack variables are to be tracked
   #define HX_STACK_VAR(cpp_var, haxe_name)                                \
       ::hx::StackVariable __stackvariable_##cpp_var(_hx_stackframe.variables, false, haxe_name, &cpp_var);

   #define HX_STACK_CATCHABLE(T, n)                                        \
       hx::StackCatchable __stackcatchable_##n                             \
           (_hx_stackframe, reinterpret_cast<T *>(&_hx_stackframe));

   // If HXCPP_DEBUGGER is enabled, then a throw is checked to see if it
   // can be caught and if not, the debugger is entered.  Otherwise, the
   // throw proceeds as normal.
   #define HX_STACK_DO_THROW(e) __hxcpp_dbg_checkedThrow(e)
   #define HX_STACK_DO_RETHROW(e) __hxcpp_dbg_checkedRethrow(e)


   #define HX_VAR(type,name) type name; HX_STACK_VAR(name, #name)
   #define HX_VARI(type,name) type name; HX_STACK_VAR(name, #name) name
   #define HX_VAR_NAME(type,name,dbgName) type name; HX_STACK_VAR(name, dbgName)
   #define HX_VARI_NAME(type,name,dbgName) type name; HX_STACK_VAR(name, dbgName) name

#else // Non-debugger versions.  Just stub-out.

   #define HX_STACK_THIS(ptr)
   #define HX_STACK_ARG(cpp_var, haxe_name)
   #define HX_STACK_VAR(cpp_var, haxe_name)
   #define HX_STACK_CATCHABLE(T, n)

   #define HX_VAR(type,name) type name
   #define HX_VARI(type,name) type name
   #define HX_VAR_NAME(type,name,dbgName) type name
   #define HX_VARI_NAME(type,name,dbgName) type name

   // Just throw - move to hx::Throw function?
   #define HX_STACK_DO_THROW(e) ::hx::Throw(e)
   #define HX_STACK_DO_RETHROW(e) ::hx::Rethrow(e)
#endif // HXCPP_STACK_VARS




// Emitted after every Haxe line.  number is the original Haxe line number.
// Only if stack lines are to be tracked
#ifdef HXCPP_STACK_LINE
   // If the debugger is enabled, must check for a breakpoint at every line.
   #ifdef HXCPP_DEBUGGER
      #define HX_STACK_LINE(number)                                           \
          _hx_stackframe.lineNumber = number;                                   \
          /* This is incorrect - a read memory barrier is needed here. */     \
          /* For now, just live with the exceedingly rare cases where */      \
          /* breakpoints are missed */                                        \
          if (::hx::gShouldCallHandleBreakpoints) {                             \
              __hxcpp_on_line_changed(_hx_stackframe.ctx);                    \
         }
      #define HX_STACK_LINE_QUICK(number) _hx_stackframe.lineNumber = number;
   #else
      // Just set it
      #define HX_STACK_LINE(number) _hx_stackframe.lineNumber = number;
      #define HX_STACK_LINE_QUICK(number) _hx_stackframe.lineNumber = number;
   #endif
#else
   #define HX_STACK_LINE(number)
   #define HX_STACK_LINE_QUICK(number)
#endif


// For tidier generated code
#define HXLINE(number) HX_STACK_LINE(number)
#define HXDLIN(number)


// To support older versions of the haxe compiler that emit HX_STACK_PUSH
// instead of HX_STACK_FRAME.  If the old haxe compiler is used with this
// new debugger implementation, className.functionName breakpoints will
// not work, and stack reporting will be a little weird.  If you want to
// use debugging, you really should upgrade to a newer haxe compiler.

#undef HX_STACK_PUSH
#define HX_STACK_PUSH(fullName, fileName, lineNumber)                  \
    HX_STACK_FRAME("", fullName, 0, fullName, fileName, lineNumber, 0)

#if defined(HXCPP_STACK_TRACE) || defined(HXCPP_TELEMETRY)
   #define HXCPP_STACK_IDS
#endif


namespace hx
{


class StackFrame;
struct StackContext;

class Profiler;
void profDestroy(Profiler *);
void profAttach(Profiler *, StackContext *);
void profDetach(Profiler *, StackContext *);
void profSample(Profiler *, StackContext *inContext);


class Telemetry;
Telemetry *tlmCreate(StackContext *);
void tlmDestroy(Telemetry *);
void tlmAttach(Telemetry *, StackContext *);
void tlmDetach(Telemetry *);
void tlmSampleEnter(Telemetry *, StackFrame *inFrame);
void tlmSampleExit(Telemetry *);


class DebuggerContext;
DebuggerContext *dbgCtxCreate(StackContext *);
void dbgCtxDestroy(DebuggerContext *);
void dbgCtxAttach(DebuggerContext *, StackContext *);
void dbgCtxDetach(DebuggerContext *);
void dbgCtxEnable(DebuggerContext *, bool inEnable);


struct scriptCallable;
class StackVariable;
class StackCatchable;

template<typename T> struct Hash;
struct TWeakStringSet;
typedef Hash<TWeakStringSet> WeakStringSet;

extern const char* EXTERN_CLASS_NAME;


#ifdef HXCPP_DEBUGGER
extern volatile bool gShouldCallHandleBreakpoints;


// These must match the values present in cpp.vm.Debugger
enum DebugStatus
{
    DBG_STATUS_INVALID = 0, // Not present or needed in cpp.vm.Debugger
    DBG_STATUS_RUNNING = 1,
    DBG_STATUS_STOPPED_BREAK_IMMEDIATE = 2,
    DBG_STATUS_STOPPED_BREAKPOINT = 3,
    DBG_STATUS_STOPPED_UNCAUGHT_EXCEPTION = 4,
    DBG_STATUS_STOPPED_CRITICAL_ERROR = 5
};

enum ExecutionTrace
{
   exeTraceOff = 0,
   exeTraceFuncs = 1,
   exeTraceLines = 2,
};

extern ExecutionTrace sExecutionTrace;

#endif





class StackPosition
{
public:
    // These are constant during the lifetime of the stack frame
    const char *className;
    const char *functionName;
    const char *fullName; // this is className.functionName - used for profiler
    const char *fileName;
    int firstLineNumber;

    #if defined(HXCPP_STACK_SCRIPTABLE)
    // Information about the current cppia function
    struct ScriptCallable *scriptCallable;
    #endif

    // These are only used if HXCPP_DEBUGGER is defined
    #ifdef HXCPP_DEBUGGER
    int fileHash;
    int classFuncHash;
    #else
    enum { fileHash = 0, classFuncHash=0 };
    #endif

    inline StackPosition() { }

    // The constructor automatically adds the StackFrame to the list of
    // stack frames for the current thread
    inline StackPosition(const char *inClassName, const char *inFunctionName,
                         const char *inFullName, const char *inFileName
                         #ifdef HXCPP_STACK_LINE
                         , int inLineNumber
                         #endif
                         #ifdef HXCPP_DEBUGGER
                         ,int inClassFunctionHash, int inFileHash
                         #endif
                  )

       : className(inClassName), functionName(inFunctionName)
         ,fullName(inFullName), fileName(inFileName)
         #ifdef HXCPP_DEBUGGER
         ,classFuncHash(inClassFunctionHash)
         ,fileHash(inFileHash)
         #endif
         #ifdef HXCPP_STACK_LINE
         ,firstLineNumber(inLineNumber)
         #endif
    {
       #if defined(HXCPP_STACK_SCRIPTABLE)
       // Information about the current cppia function
       scriptCallable = 0;
       #endif
    }

};






#ifdef HXCPP_STACK_TRACE
struct ExceptionStackFrame
{
   #ifdef HXCPP_STACK_LINE
   int line;
   #endif

   #if HXCPP_API_LEVEL > 330
   const hx::StackPosition *position;
   #else
   const char *className;
   const char *functionName;
   const char *fileName;
   #endif

   ExceptionStackFrame(const StackFrame &inFrame);
   ::String format(bool inForDisplay);
   ::String toDisplay();
   ::String toString();
};
#endif


#ifdef HXCPP_SCRIPTABLE
enum
{
   bcrBreak    = 0x01,
   bcrContinue = 0x02,
   bcrReturn   = 0x04,

   bcrLoop     = (bcrBreak | bcrContinue),
};



#endif


struct MarkChunk
{
   enum { SIZE = 62 };
   enum { OBJ_ARRAY_JOB = -1 };

   inline MarkChunk() : count(0), next(0) { }

   int        count;

   union
   {
      hx::Object *stack[SIZE];
      struct
      {
         hx::Object **arrayBase;
         int        arrayElements;
      };
   };
   MarkChunk  *next;

   inline void push(Object *inObj)
   {
      stack[count++] = inObj;
   }
   inline hx::Object *pop()
   {
      if (count)
         return stack[--count];
      return 0;
   }
   MarkChunk *swapForNew();
};



struct StackContext : public hx::ImmixAllocator
{
   #ifdef HXCPP_STACK_IDS
      int  mThreadId;
   #endif

   #ifdef HXCPP_STACK_TRACE
      hx::QuickVec<StackFrame *> mStackFrames;
      hx::QuickVec<hx::ExceptionStackFrame> mExceptionStack;
      // Updated only when a thrown exception unwinds the stack
      bool mIsUnwindingException;

      #ifdef HXCPP_STACK_SCRIPTABLE
         // TODO - combine CppaCtx and StackContext
      #endif

      #ifdef HXCPP_DEBUGGER
         DebuggerContext  *mDebugger;
      #endif

      #ifdef HXCPP_PROFILER
         // Profiling support
         Profiler *mProfiler;
      #endif

   #endif

   #ifdef HXCPP_TELEMETRY
      // Telemetry support
      Telemetry *mTelemetry;
   #endif

   #ifdef HXCPP_COMBINE_STRINGS
   WeakStringSet *stringSet;
   #endif

   #ifdef HXCPP_GC_GENERATIONAL
   MarkChunk *mOldReferrers;
   inline void pushReferrer(hx::Object *inObj)
   {
      // If collector is running on non-generational mode, mOldReferrers will be null
      if (mOldReferrers)
      {
         mOldReferrers->push(inObj);
         if (mOldReferrers->count==MarkChunk::SIZE)
            mOldReferrers = mOldReferrers->swapForNew();
      }
   }
   #endif

   #ifdef HXCPP_CATCH_SEGV
      #ifdef _MSC_VER
      _se_translator_function mOldSignalFunc;
      #else
      void (*mOldSignalFunc)(int);
      #endif
   #endif

   StackContext();
   ~StackContext();
   void onThreadAttach();
   void onThreadDetach();


   #ifdef HXCPP_STACK_TRACE // {
   void tracePosition();

   // Note that the stack frames are manipulated without holding any locks.
   // This is because the manipulation of stack frames can only be done by
   // the thread that "owns" that stack frame.  The only other contention on
   // the call stack is from calls to GetThreadInfo() and GetThreadInfos(),
   // and these should only be called when the thread for which the call
   // stack is being acquired is stopped in a breakpoint anyway, thus there
   // can be no contention on the contents of the CallStack in that case
   // either.

   inline void pushFrame(StackFrame *inFrame)
   {
      #ifdef HXCPP_PROFILER
      if (mProfiler)
         profSample(mProfiler,this);
      #endif

      #ifdef HXCPP_TELEMETRY
      if (mTelemetry)
         tlmSampleEnter(mTelemetry,inFrame);
      #endif

      mIsUnwindingException = false;
      mStackFrames.push(inFrame);

      #ifdef HXCPP_DEBUGGER
      if (sExecutionTrace!=exeTraceOff)
         tracePosition();
      #endif
   }

   inline void popFrame(StackFrame *inFrame)
   {
      #ifdef HXCPP_TELEMETRY
      if (mTelemetry)
         tlmSampleExit(mTelemetry);
      #endif

      if (mIsUnwindingException)
      {
         // Use default operator=
         mExceptionStack.push( *inFrame );
      }

      mStackFrames.pop_back();
   }

   void getCurrentCallStackAsStrings(Array<String> result, bool skipLast);
   void getCurrentExceptionStackAsStrings(Array<String> result);
   StackFrame *getCurrentStackFrame() { return mStackFrames.back(); }
   StackFrame *getStackFrame(int inIndex) { return mStackFrames[inIndex]; }
   int getDepth() const { return mStackFrames.size(); }
   inline const char *getFullNameAtDepth(int depth) const;
   void  dumpExceptionStack();

   // Called when a throw occurs
   void setLastException();
   void pushLastException();
    // Called when a catch block begins to be executed.  hxcpp wants to track
    // the stack back through the catches so that it can be dumped if
    // uncaught.  If inAll is true, the entire stack is captured immediately.
    // If inAll is false, only the last stack frame is captured.
    void beginCatch(bool inAll);

   #endif // } HXCPP_STACK_TRACE

   #ifdef HXCPP_DEBUGGER
   void enableCurrentThreadDebugging(bool inEnable)
   {
      dbgCtxEnable(mDebugger,inEnable);
   }
   #endif

   static inline StackContext *getCurrent()
   {
      return HX_CTX_GET;
   }

   #ifdef HXCPP_STACK_IDS
   static void getAllStackIds( QuickVec<int> &outIds );
   static StackContext *getStackForId(int id);
   #endif


   #ifdef HXCPP_SCRIPTABLE
   unsigned char *stack;
   unsigned char *pointer;
   unsigned char *frame;
   class Object  *exception;

   unsigned int breakContReturn;
   int  byteMarkId;

   template<typename T>
   void push(T inValue)
   {
      *(T *)pointer = inValue;
      pointer += sizeof(T);
   }
   unsigned char *stackAlloc(int inSize)
   {
      unsigned char *p = pointer;
      pointer += inSize;
      return p;
   }
   void stackFree(int inSize)
   {
      pointer -= inSize;
   }

   int getFrameSize() const { return pointer-frame; }

   int runInt(void *vtable);
   Float runFloat(void *vtable);
   String runString(void *vtable);
   void runVoid(void *vtable);
   Dynamic runObject(void *vtable);
   hx::Object *runObjectPtr(void *vtable);

   void push(bool &inValue) { *(int *)pointer = inValue; pointer += sizeof(int); }
   inline void pushBool(bool b) { *(int *)pointer = b; pointer += sizeof(int); }
   inline void pushInt(int i) { *(int *)pointer = i; pointer += sizeof(int); }

   inline void pushFloat(Float f);
   inline void pushString(const String &s);
   inline void pushObject(Dynamic d);
   inline void returnFloat(Float f);
   inline void returnString(const String &s);
   inline void returnObject(Dynamic d);
   inline hx::Object *getThis(bool inCheckPtr=true);

   inline void returnBool(bool b) { *(int *)frame = b; }
   inline void returnInt(int i) { *(int *)frame = i; }
   inline bool getBool(int inPos=0) { return *(bool *)(frame+inPos); }
   inline int getInt(int inPos=0) { return *(int *)(frame+inPos); }

   inline Float getFloat(int inPos=0);
   inline String getString(int inPos=0);
   inline Dynamic getObject(int inPos=0);
   inline hx::Object *getObjectPtr(int inPos=0) { return *(hx::Object **)(frame+inPos); }


   void breakFlag() { breakContReturn |= bcrBreak; }
   void continueFlag() { breakContReturn |= bcrContinue; }
   void returnFlag() { breakContReturn |= bcrReturn; }

   #endif

};


typedef StackContext CppiaCtx;



class StackFrame
{
public:
   StackContext        *ctx;

   #ifdef HXCPP_STACK_TRACE // {
   const StackPosition *position;

      #ifdef HXCPP_STACK_LINE
         // Current line number, changes during the lifetime of the stack frame.
         // Only updated if HXCPP_STACK_LINE is defined.
         int lineNumber;

         #ifdef HXCPP_DEBUGGER
         // Function arguments and local variables in reverse order of their
         // declaration.  If a variable name is in here twice, the first version is
         // the most recently scoped one and should be used.  Only updated if
         // HXCPP_DEBUGGER is defined.
         StackVariable *variables;

         // The list of types that can be currently caught in the stack frame.
         StackCatchable *catchables;
         #endif
      #endif

       // The constructor automatically adds the StackFrame to the list of
       // stack frames for the current thread
       inline StackFrame(const StackPosition *inPosition
              ) : position(inPosition)
       {
          #ifdef HXCPP_STACK_LINE
             lineNumber = inPosition->firstLineNumber;
             #ifdef HXCPP_DEBUGGER
             variables = 0;
             catchables = 0;
             #endif
          #endif


          ctx =  HX_CTX_GET;
          ctx->pushFrame(this);
       }


       // The destructor automatically removes the StackFrame from the list of
       // stack frames for the current thread
       ~StackFrame()
       {
          ctx->popFrame(this);
       }

       ::String toString();
       ::String toDisplay();
   #else // }  !HXCPP_STACK_TRACE {

       // Release version only has ctx
       inline StackFrame()
       {
          ctx =  HX_CTX_GET;
       }

   #endif // }

};

#ifdef HXCPP_STACK_TRACE
const char *StackContext::getFullNameAtDepth(int depth) const
{
   return mStackFrames[depth]->position->fullName;
}
#endif

class JustGcStackFrame
{
public:
   StackContext        *ctx;
   inline JustGcStackFrame() : ctx(HX_CTX_GET) { }
};




} // end namespace hx



// Some functions used by AdvancedDebug.cpp
// Returns the thread number of the calling thread
HXCPP_EXTERN_CLASS_ATTRIBUTES
int __hxcpp_GetCurrentThreadNumber();

// Called by the main function when an uncaught exception occurs to dump
// the stack leading to the exception
HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hx_dump_stack();

// The macro HX_STACK_BEGIN_CATCH, which is emitted at the beginning of every
// catch block, calls this in debug mode to let the debugging system know that
// a catch block has been entered
HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_stack_begin_catch();

// Last chance to throw an exception for null-pointer access
HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_set_critical_error_handler(Dynamic inHandler);

HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_execution_trace(int inLevel);

// Used by debug breakpoints and execution trace
HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_set_stack_frame_line(int);

HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_on_line_changed(hx::StackContext *);

HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_set_debugger_info(const char **inAllClasses, const char **inFullPaths);


void __hxcpp_dbg_getScriptableVariables(hx::StackFrame *stackFrame, ::Array< ::Dynamic> outNames);
bool __hxcpp_dbg_getScriptableValue(hx::StackFrame *stackFrame, String inName, ::Dynamic &outValue);
bool __hxcpp_dbg_setScriptableValue(hx::StackFrame *StackFrame, String inName, ::Dynamic inValue);




#endif // HX_STACK_CTX_H
