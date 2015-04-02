#ifndef HX_DEBUG_H
#define HX_DEBUG_H

#include <hxcpp.h>

// Some functions used by AdvancedDebug.cpp
// Returns the thread number of the calling thread
HXCPP_EXTERN_CLASS_ATTRIBUTES
int __hxcpp_GetCurrentThreadNumber();

// Set:
// HXCPP_STACK_VARS if stack variables need to be tracked
// HXCPP_STACK_LINE if stack line numbers need to be tracked
// HXCPP_STACK_TRACE if stack frames need to be tracked

// Track stack variables - only really needed for debugger
#if defined(HXCPP_DEBUGGER) && !defined(HXCPP_STACK_VARS)
#define HXCPP_STACK_VARS
#endif

// Keep track of lines - more accurate stack traces for exceptions, also
// needed for the debugger
#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER)) && !defined(HXCPP_STACK_LINE)
#define HXCPP_STACK_LINE
#endif

// Do we need to keep a stack trace - for basic exception handelling, also
// needed for the debugger
#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER) || defined(HXCPP_STACK_VARS) || defined(HXCPP_STACK_LINE)) && !defined(HXCPP_STACK_TRACE)
#define HXCPP_STACK_TRACE
#endif

// Do we care about the debug-breakpoint-lookup-hashes
#if (defined HXCPP_STACK_LINE) && (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER)) && (!defined(HXCPP_DEBUG_HASHES))
#define HXCPP_DEBUG_HASHES
#endif

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
void __hxcpp_on_line_changed();

HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_set_debugger_info(const char **inAllClasses, const char **inFullPaths);

namespace hx
{

// These must match the values present in cpp.vm.Debugger
enum ThreadStatus
{
    STATUS_INVALID = 0, // Not present or needed in cpp.vm.Debugger
    STATUS_RUNNING = 1,
    STATUS_STOPPED_BREAK_IMMEDIATE = 2,
    STATUS_STOPPED_BREAKPOINT = 3,
    STATUS_STOPPED_UNCAUGHT_EXCEPTION = 4,
    STATUS_STOPPED_CRITICAL_ERROR = 5
};

enum ThreadEvent
{
    THREAD_CREATED = 1,
    THREAD_TERMINATED = 2,
    THREAD_STARTED = 3,
    THREAD_STOPPED = 4
};

enum StepType
{
    STEP_NONE = 0, // Not present or needed in cpp.vm.Debugger
    STEP_INTO = 1,
    STEP_OVER = 2,
    STEP_OUT = 3
};

class StackArgument;
class StackVariable;
class StackCatchable;

void __hxcpp_register_stack_frame(class StackFrame *inFrame);

class StackFrame
{
public:

    // The constructor automatically adds the StackFrame to the list of
    // stack frames for the current thread
    inline StackFrame(const char *inClassName, const char *inFunctionName,
               #ifdef HXCPP_DEBUG_HASHES
               int inClassFunctionHash,
               #endif
               const char *inFullName, const char *inFileName
               #ifdef HXCPP_STACK_LINE
               , int inLineNumber
               #endif
               #ifdef HXCPP_DEBUG_HASHES
               , int inFileHash
               #endif
               )

       : className(inClassName), functionName(inFunctionName),
         #ifdef HXCPP_DEBUG_HASHES
         classFuncHash(inClassFunctionHash),
         fileHash(inFileHash),
         #else
         classFuncHash(0),
         fileHash(0),
         #endif
         fullName(inFullName), fileName(inFileName),
         #ifdef HXCPP_STACK_LINE
         firstLineNumber(inLineNumber),
         #endif
         #ifdef HXCPP_STACK_VARS
         variables(0),
         #endif
         catchables(0)
    {
         #ifdef HXCPP_STACK_LINE
         lineNumber = firstLineNumber;
         #endif
       __hxcpp_register_stack_frame(this);
    }

    // The destructor automatically removes the StackFrame from the list of
    // stack frames for the current thread
    ~StackFrame();

    ::String toString();
    ::String toDisplay();

    // These are constant during the lifetime of the stack frame
    const char *className;
    const char *functionName;
    const char *fullName; // this is className.functionName - used for profiler
    const char *fileName;
    int firstLineNumber;

    // Current line number, changes during the lifetime of the stack frame.
    // Only updated if HXCPP_STACK_LINE is defined.
    int lineNumber;

    // These are only used if HXCPP_DEBUG_HASHES is defined
    int fileHash;
    int classFuncHash;

    // Function arguments and local variables in reverse order of their
    // declaration.  If a variable name is in here twice, the first version is
    // the most recently scoped one and should be used.  Only updated if
    // HXCPP_STACK_VARS is defined.
    StackVariable *variables;

    // The list of types that can be currently caught in the stack frame.
    StackCatchable *catchables;
};


class StackVariable
{
public:

    const char *mHaxeName;
    bool mIsArg;
    StackVariable *mNext;

    template<typename T>
    StackVariable(StackVariable *&inHead, bool inIsArg,
                  const char *inHaxeName, T *inCppVar)
        : mHaxeName(inHaxeName), mIsArg(inIsArg), mHead(inHead),
          mCppVar((void *) inCppVar)
    {
        mGetOrSetFunction = GetOrSetFunction<T>;
        mNext = mHead;
        mHead = this;
}

    // For StackThis
    template<typename T>
    StackVariable(StackVariable *&inHead, T *inCppVar)
        : mHaxeName("this"), mIsArg(true), mHead(inHead),
          mCppVar((void *) inCppVar)
    {
        mNext = mHead;
        mHead = this;
    }

    ~StackVariable()
    {
        // Stack variables are always deleted in the reverse order that they
        // are created, so a simple pop_front is sufficient; no need to hunt
        // for and remove the variable, it's always in the front ...
        mHead = mNext;
    }

    operator Dynamic()
    {
        return mGetOrSetFunction(true, mCppVar, 0);
    }

    StackVariable &operator =(Dynamic &other)
    {
        (void) mGetOrSetFunction(false, mCppVar, &other);

        return *this;
    }

protected:

	typedef Dynamic (*GetOrSetFunctionType)(bool, void *, Dynamic *);

    GetOrSetFunctionType mGetOrSetFunction;

private:

    template<typename T>
    static Dynamic GetOrSetFunction(bool get, void *ptr, Dynamic *dynamic)
    {
        if (get) {
            return * (T *) ptr;
        }
        else {
            * (T *) ptr = *dynamic;
            return null();
        }
    }

    StackVariable *&mHead;

    void *mCppVar;

};


class StackThis : public StackVariable
{
public:

    template<typename T>
    StackThis(StackVariable *&inHead, T *inThis)
        : StackVariable(inHead, inThis)
    {
        mGetOrSetFunction = GetFunction<T>;
    }

    template<typename T>
    static Dynamic GetFunction(bool get, void *ptr, Dynamic *val)
    {
        if (get) {
            return (T *) ptr;
        }
        else {
            return null();
        }
    }
};


class StackCatchable
{
public:

    StackCatchable *mNext;

    template<typename T>
    StackCatchable(StackFrame &frame, T * /* dummy required by template*/)
        : mFrame(frame)
{
        mNext = frame.catchables;
        frame.catchables = this;
        mTestFunction = TestFunction<T>;
    }

    ~StackCatchable()
{
        mFrame.catchables = mNext;
    }

    bool Catches(Dynamic e) const
   {
        return mTestFunction(e);
   }

private:

   template<typename T>
    static bool TestFunction(Dynamic e)
   {
        return e.IsClass<T>();
   }

    StackFrame &mFrame;
    bool (*mTestFunction)(Dynamic e);
};


extern volatile bool gShouldCallHandleBreakpoints;


// Only if HXCPP_STACK_TRACE is defined do any of the stack trace macros
// do anything
#ifdef HXCPP_STACK_TRACE

// Stack frames are always pushed if HXCPP_STACK_TRACE is enabled, hashes only if debugger is
#ifdef HXCPP_STACK_LINE

   #ifdef HXCPP_DEBUG_HASHES
      #define HX_STACK_FRAME(className, functionName, classFunctionHash, fullName,fileName,     \
                          lineNumber, fileHash ) \
       hx::StackFrame __stackframe(className, functionName, classFunctionHash, fullName,      \
                                   fileName, lineNumber, fileHash);
   #else
      #define HX_STACK_FRAME(className, functionName, classFunctionHash, fullName,fileName,     \
                          lineNumber, fileHash ) \
       hx::StackFrame __stackframe(className, functionName, fullName,      \
                                   fileName, lineNumber);
   #endif
#else

   #define HX_STACK_FRAME(className, functionName, classFunctionHash, fullName,fileName,     \
                       lineNumber, fileHash ) \
    hx::StackFrame __stackframe(className, functionName, fullName, fileName);

#endif

// Emitted at the beginning of every instance fuction.  ptr is "this".
// Only if stack variables are to be tracked
#ifdef HXCPP_STACK_VARS
#define HX_STACK_THIS(ptr) hx::StackThis __stackthis                    \
     (__stackframe.variables, ptr);
#endif // HXCPP_STACK_VARS

// Emitted at the beginning of every function that takes arguments.
// name is the name of the argument.  XXX NOTE - this macro needs to be
// expanded to give a pointer to the value of the argument also, otherwise
// there is no way to get the value ...
// For the lifetime of this object, the argument will be in the [arguments]
// list of the stack frame in which the arg was declared
// Only if stack variables are to be tracked
#ifdef HXCPP_STACK_VARS
#define HX_STACK_ARG(cpp_var, haxe_name)                                \
    hx::StackVariable __stackargument_##cpp_var                         \
        (__stackframe.variables, true, haxe_name, &cpp_var);
#endif // HXCPP_STACK_VARS

// Emitted whenever a Haxe value is pushed on the stack.  cpp_var is the local
// cpp variable, haxe_name is the name that was used in haxe for it
// Only if stack variables are to be tracked
#ifdef HXCPP_STACK_VARS
#define HX_STACK_VAR(cpp_var, haxe_name)                                \
    hx::StackVariable __stackvariable_##cpp_var                         \
        (__stackframe.variables, false, haxe_name, &cpp_var);
#endif // HXCPP_STACK_VARS

// Emitted after every Haxe line.  number is the original Haxe line number.
// Only if stack lines are to be tracked
#ifdef HXCPP_STACK_LINE
// If the debugger is enabled, must check for a breakpoint at every line.
#ifdef HXCPP_DEBUGGER
#define HX_STACK_LINE(number)                                           \
    __stackframe.lineNumber = number;                                   \
    /* This is incorrect - a read memory barrier is needed here. */     \
    /* For now, just live with the exceedingly rare cases where */      \
    /* breakpoints are missed */                                        \
    if (hx::gShouldCallHandleBreakpoints) {                             \
        __hxcpp_on_line_changed();                               \
   }
#else
#define HX_STACK_LINE(number) __stackframe.lineNumber = number;
#endif // HXCPP_DEBUGGER
#endif // HXCPP_STACK_LINE

// Emitted at the beginning of every try block
// Catchables are only tracked if HXCPP_DEBUGGER is set, to enable
// entering the debugger if an uncatchable throw occurs
#ifdef HXCPP_DEBUGGER
#define HX_STACK_CATCHABLE(T, n)                                        \
    hx::StackCatchable __stackcatchable_##n                             \
        (__stackframe, reinterpret_cast<T *>(&__stackframe));
#endif // HXCPP_DEBUGGER


// Emitted at the beginning of every catch block.  Used to build up the
// catch stack.
// Catches are always tracked if HXCPP_STACK_TRACE is enabled.
#define HX_STACK_BEGIN_CATCH __hxcpp_stack_begin_catch();

// If HXCPP_DEBUGGER is enabled, then a throw is checked to see if it
// can be caught and if not, the debugger is entered.  Otherwise, the
// throw proceeds as normal.
#ifdef HXCPP_DEBUGGER
#define HX_STACK_DO_THROW(e) __hxcpp_dbg_checkedThrow(e)
#endif

#endif // HXCPP_STACK_TRACE

} // namespace hx


// Define any macros not defined already above
#ifndef HX_STACK_FRAME
#define HX_STACK_FRAME(className, functionName, classFuncHash, fullName, fileName, lineNumber, fileHash )
#endif
#ifndef HX_STACK_THIS
#define HX_STACK_THIS(ptr)
#endif
#ifndef HX_STACK_ARG
#define HX_STACK_ARG(cpp_var, haxe_name)
#endif
#ifndef HX_STACK_VAR
#define HX_STACK_VAR(cpp_var, haxe_name)
#endif
#ifndef HX_STACK_LINE
#define HX_STACK_LINE(number)
#endif
#ifndef HX_STACK_CATCHABLE
#define HX_STACK_CATCHABLE(T, n)
#endif
#ifndef HX_STACK_BEGIN_CATCH
#define HX_STACK_BEGIN_CATCH
#endif
#ifndef HX_STACK_DO_THROW
#define HX_STACK_DO_THROW(e) hx::Throw(e)
#endif

// To support older versions of the haxe compiler that emit HX_STACK_PUSH
// instead of HX_STACK_FRAME.  If the old haxe compiler is used with this
// new debugger implementation, className.functionName breakpoints will
// not work, and stack reporting will be a little weird.  If you want to
// use debugging, you really should upgrade to a newer haxe compiler.

#undef HX_STACK_PUSH
#define HX_STACK_PUSH(fullName, fileName, lineNumber)                  \
    HX_STACK_FRAME("", fullName, 0, fullName, fileName, lineNumber, 0)


#ifdef HXCPP_DEBUGGER

// The following functions are called directly, and only, by the haxe standard
// library's cpp.vm.Debugger.hx class
void __hxcpp_dbg_setEventNotificationHandler(Dynamic handler);
void __hxcpp_dbg_enableCurrentThreadDebugging(bool enable);
int __hxcpp_dbg_getCurrentThreadNumber();
Array< ::String> __hxcpp_dbg_getFiles();
Array< ::String> __hxcpp_dbg_getFilesFullPath();
Array< ::String> __hxcpp_dbg_getClasses();
Array<Dynamic> __hxcpp_dbg_getThreadInfos();
Dynamic __hxcpp_dbg_getThreadInfo(int threadNumber, bool unsafe);
int __hxcpp_dbg_addFileLineBreakpoint(String fileName, int lineNumber);
int __hxcpp_dbg_addClassFunctionBreakpoint(String className,
                                            String functionName);
void __hxcpp_dbg_deleteAllBreakpoints();
void __hxcpp_dbg_deleteBreakpoint(int number);
void __hxcpp_dbg_breakNow(bool wait);
void __hxcpp_dbg_continueThreads(int threadNumber, int count);
void __hxcpp_dbg_stepThread(int threadNumber, int stepType, int stepCount);
Array<Dynamic> __hxcpp_dbg_getStackVariables(int threadNumber,
                                             int stackFrameNumber,
                                             bool unsafe,
                                             Dynamic markThreadNotStopped);
Dynamic __hxcpp_dbg_getStackVariableValue(int threadNumber,
                                          int stackFrameNumber,
                                          String name,
                                          bool unsafe,
                                          Dynamic markNonexistent,
                                          Dynamic markThreadNotStopped);

Dynamic __hxcpp_dbg_setStackVariableValue(int threadNumber,
                                          int stackFrameNumber,
                                          String name, Dynamic value,
                                          bool unsafe,
                                          Dynamic markNonexistent,
                                          Dynamic markThreadNotStopped);
void __hxcpp_dbg_setNewParameterFunction(Dynamic function);
void __hxcpp_dbg_setNewStackFrameFunction(Dynamic function);
void __hxcpp_dbg_setNewThreadInfoFunction(Dynamic function);
void __hxcpp_dbg_setAddParameterToStackFrameFunction(Dynamic function);
void __hxcpp_dbg_setAddStackFrameToThreadInfoFunction(Dynamic function);

// The following functions are called by Thread.cpp to notify of thread
// created and terminated
void __hxcpp_dbg_threadCreatedOrTerminated(int threadNumber, bool created);

// The following is called by the stack macros, but only if
// HXCPP_DEBUGGER is set
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic __hxcpp_dbg_checkedThrow(Dynamic toThrow);

#else // !HXCPP_DEBUGGER

// If no debugger, provide empty implementations of the debugging functions

inline void __hxcpp_dbg_setEventNotificationHandler(Dynamic)
    { hx::Throw("Debugging is not enabled for this program; try\n"
                "rebuilding it with the -D HXCPP_DEBUGGER option"); }
inline void __hxcpp_dbg_enableCurrentThreadDebugging(bool) { }
inline int __hxcpp_dbg_getCurrentThreadNumber() { return -1; }
inline Array< ::String> __hxcpp_dbg_getFiles()
    { return Array_obj< String>::__new(); }
inline Array< ::String> __hxcpp_dbg_getFilesFullPath()
    { return Array_obj< String>::__new(); }
inline Array< ::String> __hxcpp_dbg_getClasses()
    { return Array_obj< String>::__new(); }
inline Array<Dynamic> __hxcpp_dbg_getThreadInfos()
    { return Array_obj< ::Dynamic>::__new(); }
inline Dynamic __hxcpp_dbg_getThreadInfo(int, bool) { return null(); }
inline int __hxcpp_dbg_addFileLineBreakpoint(String, int) { return -1; }
inline int __hxcpp_dbg_addClassFunctionBreakpoint(String, String)
    { return -1; }
inline void __hxcpp_dbg_deleteAllBreakpoints() { }
inline void __hxcpp_dbg_deleteBreakpoint(int) { }
inline void __hxcpp_dbg_breakNow(bool) { }
inline void __hxcpp_dbg_continueThreads(int, int) { }
inline void __hxcpp_dbg_stepThread(int, int, int) { }
inline Array<Dynamic> __hxcpp_dbg_getStackVariables(int, int, bool, Dynamic)
    { return Array_obj< String>::__new(); }
inline Dynamic __hxcpp_dbg_getStackVariableValue(int, int, String, bool,
                                                 Dynamic, Dynamic)
    { return null(); }
inline Dynamic __hxcpp_dbg_setStackVariableValue(int, int, String, Dynamic,
                                                 bool, Dynamic, Dynamic)
    { return null(); }
inline void __hxcpp_dbg_setNewParameterFunction(Dynamic) { }
inline void __hxcpp_dbg_setNewStackFrameFunction(Dynamic) { }
inline void __hxcpp_dbg_setNewThreadInfoFunction(Dynamic) { }
inline void __hxcpp_dbg_setAddParameterToStackFrameFunction(Dynamic) { }
inline void __hxcpp_dbg_setAddStackFrameToThreadInfoFunction(Dynamic) { }

// The following functions are called by Thread.cpp to notify of thread
// created and terminated
inline void __hxcpp_dbg_threadCreatedOrTerminated(int, bool) { }

inline Dynamic __hxcpp_dbg_checkedThrow(Dynamic toThrow) { return hx::Throw(toThrow); }

#endif // HXCPP_DEBUGGER



#endif // HX_DEBUG_H
