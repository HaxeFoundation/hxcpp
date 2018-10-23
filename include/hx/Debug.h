#ifndef HX_DEBUG_H
#define HX_DEBUG_H

#include <hxcpp.h>

// Some functions used by AdvancedDebug.cpp
// Returns the thread number of the calling thread
HXCPP_EXTERN_CLASS_ATTRIBUTES
int __hxcpp_GetCurrentThreadNumber();

namespace hx
{

#ifdef HXCPP_DEBUGGER

template<typename T> struct StackVariableWrapper
{
   typedef T wrapper;
};
template<> struct StackVariableWrapper<size_t>
{
   #ifdef HXCPP_M64
   typedef cpp::Int64 wrapper;
   #else
   typedef int wrapper;
   #endif
};


template<typename T> struct StackVariableWrapper<T *>
{
   typedef cpp::Pointer<T> wrapper;
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

    StackVariable(StackVariable *&inHead, bool inIsArg,
                  const char *inHaxeName, hx::Object **inCppVar)
        : mHaxeName(inHaxeName), mIsArg(inIsArg), mHead(inHead),
          mCppVar((void *) inCppVar)
    {
        mGetOrSetFunction = GetOrSetFunctionHxObject;
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
       typedef typename StackVariableWrapper<T>::wrapper Wrap;

        if (get) {
            return Wrap(* (T *) ptr);
        }
        else {
            * (T *) ptr = Wrap(*dynamic);
            return null();
        }
    }

    static Dynamic GetOrSetFunctionHxObject(bool get, void *ptr, Dynamic *dynamic)
    {
        if (get) {
            return * (hx::Object **) ptr;
        }
        else {
            * (hx::Object **)ptr = dynamic->mPtr;
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
    StackThis(StackVariable *&inHead, hx::ObjectPtr<T> &inThis)
        : StackVariable(inHead, &inThis.mPtr)
    {
        mGetOrSetFunction = GetObjectPtr<T>;
    }

    template<typename T>
    static Dynamic GetObjectPtr(bool get, void *ptr, Dynamic *val)
    {
        if (get) {
            return *(hx::Object **) ptr;
        }
        else {
            return null();
        }
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



#endif // HXCPP_DEBUGGER

} // end namespace hx



void __hxcpp_dbg_getScriptableFiles( Array< ::String> ioPaths );
void __hxcpp_dbg_getScriptableFilesFullPath( Array< ::String> ioPaths );
void __hxcpp_dbg_getScriptableClasses( Array< ::String> ioClasses );



#ifdef HXCPP_DEBUGGER


namespace hx
{

// These must match the values present in cpp.vm.Debugger

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


}  // end namespace hx


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

bool __hxcpp_dbg_fix_critical_error(String inErr);

// The following functions are called by Thread.cpp to notify of thread
// created and terminated
void __hxcpp_dbg_threadCreatedOrTerminated(int threadNumber, bool created);

// The following is called by the stack macros, but only if
// HXCPP_DEBUGGER is set
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic __hxcpp_dbg_checkedThrow(Dynamic toThrow);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic __hxcpp_dbg_checkedRethrow(Dynamic toThrow);

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
inline Dynamic __hxcpp_dbg_checkedRethrow(Dynamic toThrow) { return hx::Rethrow(toThrow); }

#endif // HXCPP_DEBUGGER


#endif // HX_DEBUG_H
