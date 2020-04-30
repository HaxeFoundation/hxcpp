#include <hxcpp.h>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <hx/Debug.h>
#include <hx/Thread.h>
#include <hx/Telemetry.h>
#include <hx/Unordered.h>
#include <hx/OS.h>


#if defined(HXCPP_CATCH_SEGV) && !defined(_MSC_VER)
#include <signal.h>
#endif



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




namespace hx
{

const char* EXTERN_CLASS_NAME = "extern";

#ifdef HXCPP_STACK_IDS
static HxMutex sStackMapMutex;
typedef UnorderedMap<int, StackContext *> StackMap;
static StackMap sStackMap;
#endif

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



static void CriticalErrorHandler(String inErr, bool allowFixup)
{
#ifdef HXCPP_DEBUGGER
   if (allowFixup && __hxcpp_dbg_fix_critical_error(inErr))
      return;
#endif

   if (sCriticalErrorHandler!=null())
      sCriticalErrorHandler(inErr);

#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->beginCatch(true);
   ctx->dumpExceptionStack();
#endif

    DBGLOG("Critical Error: %s\n", inErr.utf8_str());

#if defined(HX_WINDOWS) && !defined(HX_WINRT)
    MessageBoxA(0, inErr.utf8_str(), "Critial Error - program must terminate",
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

void CriticalError(const String &inErr, bool inAllowFixup)
{
    CriticalErrorHandler(inErr, inAllowFixup);
}


#ifdef HXCPP_CATCH_SEGV
class hxSehException : public hx::Object
{
public:
   int code;

   inline void *operator new( size_t inSize )
   {
      return hx::InternalCreateConstBuffer(0,(int)inSize);
   }
   void operator delete( void *) { }

   hxSehException(int inCode) : code(inCode) { }

   String __ToString() const { return  HX_CSTRING("hxSehException"); }

   int __GetType() const { return vtObject; }
};

static hx::Object *sException = new hxSehException(1);

#ifdef _MSC_VER
void __cdecl hxSignalFunction(unsigned int, struct _EXCEPTION_POINTERS* )
{
   hx::Throw(sException);
}
#else
void hxSignalFunction(int)
{
   hx::Throw(sException);
}
#endif

#endif


String FormatStack(const char *file, const char *clazz, const char *func, int line, bool display)
{
   // Not sure if the following is even possible but the old debugger did it so ...
   char buf[1024];
   if (!file || file[0]=='?')
   {
       if (display)
          snprintf(buf, sizeof(buf), "%s::%s", clazz, func);
       else
          snprintf(buf, sizeof(buf), "%s::%s::%d", clazz, func,line);
   }
   else
   {
      // Old-style combined file::class...
      if (!clazz || !clazz[0])
      {
          if (line>0 || !display)
             snprintf(buf, sizeof(buf), display ? "%s %s line %d" : "%s::%s::%d", func, file, line);
          else
             snprintf(buf, sizeof(buf), "%s %s", func, file);
      }
      else
      {
          if (line>0 || !display)
             snprintf(buf, sizeof(buf), display ? "%s::%s %s line %d" : "%s::%s::%s::%d", clazz, func, file, line);
          else
             snprintf(buf, sizeof(buf), "%s::%s %s", clazz, func, file);
      }
   }
   return ::String(buf);
}






StackContext::StackContext()
{
   #ifdef HXCPP_STACK_TRACE
   mIsUnwindingException = false;
   #endif

   #ifdef HXCPP_TELEMETRY
   //mTelemetry = tlmCreate(this);
   // Do not automatically start
   mTelemetry = 0;
   #endif

   #ifdef HXCPP_DEBUGGER
   mDebugger = 0;
   #endif

   #ifdef HXCPP_PROFILER
   mProfiler = 0;
   #endif

   #ifdef HXCPP_SCRIPTABLE
   stack = new unsigned char[128*1024];
   pointer = &stack[0];
   push((hx::Object *)0);
   frame = pointer;
   exception = 0;
   breakContReturn = 0;
   #endif

   #ifdef HXCPP_COMBINE_STRINGS
   stringSet = 0;
   #endif
}

StackContext::~StackContext()
{
   #ifdef HXCPP_DEBUGGER
   if (mDebugger)
      dbgCtxDestroy(mDebugger);
   #endif

   #ifdef HXCPP_TELEMETRY
   if (mTelemetry)
      tlmDestroy(mTelemetry);
   #endif

   #ifdef HXCPP_PROFILER
   if (mProfiler)
      profDestroy(mProfiler);
   #endif

   #ifdef HXCPP_SCRIPTABLE
   delete [] stack;
   #endif
}

void StackContext::onThreadAttach()
{
   #ifdef HXCPP_STACK_IDS
   mThreadId = __hxcpp_GetCurrentThreadNumber();

   sStackMapMutex.Lock();
   sStackMap[mThreadId] = this;
   sStackMapMutex.Unlock();
   #endif

   #ifdef HXCPP_DEBUGGER
   if (!mDebugger)
      mDebugger = dbgCtxCreate(this);
   if (mDebugger)
      dbgCtxAttach(mDebugger,this);
   #endif

   #ifdef HXCPP_TELEMETRY
   if (mTelemetry)
      tlmAttach(mTelemetry,this);
   #endif

   #ifdef HXCPP_PROFILER
   if (mProfiler)
      profAttach(mProfiler,this);
   #endif

   #ifdef HXCPP_CATCH_SEGV
      #ifdef _MSC_VER
      mOldSignalFunc = _set_se_translator( hxSignalFunction );
      #else
      mOldSignalFunc = signal( SIGSEGV, hxSignalFunction );
      #endif
   #endif
}

void StackContext::onThreadDetach()
{
   #ifdef HXCPP_CATCH_SEGV
      #ifdef _MSC_VER
      _set_se_translator( mOldSignalFunc );
      #else
      signal( SIGSEGV, mOldSignalFunc );
      #endif
   #endif

   #ifdef HXCPP_DEBUGGER
   if (mDebugger)
      dbgCtxDetach(mDebugger);
   #endif

   #ifdef HXCPP_TELEMETRY
   if (mTelemetry)
      tlmDetach(mTelemetry);
   #endif

   #ifdef HXCPP_PROFILER
   if (mProfiler)
      profDetach(mProfiler,this);
   #endif

   #ifdef HXCPP_STACK_IDS
   sStackMapMutex.Lock();
   sStackMap.erase(mThreadId);
   sStackMapMutex.Unlock();
   mThreadId = 0;
   #endif

   #ifdef HXCPP_COMBINE_STRINGS
   stringSet = 0;
   #endif
}

#ifdef HXCPP_STACK_IDS
void StackContext::getAllStackIds( QuickVec<int> &outIds )
{
   outIds.clear();
   sStackMapMutex.Lock();
   for(StackMap::iterator i=sStackMap.begin(); i!=sStackMap.end(); ++i)
      outIds.push(i->first);
   sStackMapMutex.Unlock();
}

StackContext *StackContext::getStackForId(int id)
{
   sStackMapMutex.Lock();
   StackContext *result = sStackMap[id];
   sStackMapMutex.Unlock();
   return result;
}
#endif


#ifdef HXCPP_STACK_TRACE // {

void StackContext::tracePosition( )
{
   StackFrame *frame = mStackFrames[mStackFrames.size()-1];
   #ifdef HXCPP_STACK_LINE
   DBGLOG("%s::%s : %d\n", frame->position->className, frame->position->functionName, frame->lineNumber);
   #else
   DBGLOG("%s::%s\n", frame->position->className, frame->position->functionName);
   #endif
}

void StackContext::getCurrentCallStackAsStrings(Array<String> result, bool skipLast)
{
   int n = mStackFrames.size() - (skipLast ? 1 : 0);

   for (int i = 0; i < n; i++)
   {
       // Reverse call stack to match exception stack
       StackFrame *frame = mStackFrames[n-1-i];
       result->push(frame->toString());
   }
}

void StackContext::getCurrentExceptionStackAsStrings(Array<String> result)
{
   int size = mExceptionStack.size();

   for (int i = 0; i < size; i++)
   {
      result->push(mExceptionStack[i].toString());
   }
}

void StackContext::beginCatch(bool inAll)
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
   mIsUnwindingException = false;
}



// Called when a throw occurs
void StackContext::setLastException()
{
   mExceptionStack.clear();
   mIsUnwindingException = true;
}


// Called when a throw occurs
void StackContext::pushLastException()
{
   mIsUnwindingException = true;
}



void StackContext::dumpExceptionStack()
{
   #ifdef ANDROID
   #define EXCEPTION_PRINT(...) \
        __android_log_print(ANDROID_LOG_ERROR, "HXCPP", __VA_ARGS__)
   #else
   #define EXCEPTION_PRINT(...) \
           printf(__VA_ARGS__)
   #endif

   int size = mExceptionStack.size();
   for(int i = size - 1; i >= 0; i--)
   {
      EXCEPTION_PRINT("Called from %s\n", mExceptionStack[i].toDisplay().utf8_str());
   }
}


// ---- StackFrame -------


::String StackFrame::toDisplay()
{
   #ifndef HXCPP_STACK_LINE
   int lineNumber=0;
   #endif
   return FormatStack(position->fileName, position->className, position->functionName, lineNumber,true);
}


::String StackFrame::toString()
{
   #ifndef HXCPP_STACK_LINE
   int lineNumber=0;
   #endif

   return FormatStack(position->fileName, position->className, position->functionName, lineNumber,false);
}




ExceptionStackFrame::ExceptionStackFrame(const StackFrame &inFrame)
{
   // It is safe to use the pointer in 331+
   #if HXCPP_API_LEVEL > 330
   position = inFrame.position;
   #else
   // Must copy the pointer values
   className =  inFrame.position->className;
   functionName =  inFrame.position->functionName;
   fileName =  inFrame.position->fileName;
   #endif

   #ifdef HXCPP_STACK_LINE
   line = inFrame.lineNumber;
   #endif
}

::String ExceptionStackFrame::format(bool inForDisplay)
{
   #ifndef HXCPP_STACK_LINE
   int line=0;
   #endif

   #if HXCPP_API_LEVEL > 330
   const char *fileName = position->fileName;
   const char *className = position->className;
   const char *functionName = position->functionName;
   #endif

   return FormatStack(fileName, className, functionName, line, inForDisplay);
}

::String ExceptionStackFrame::toDisplay() { return format(true); }
::String ExceptionStackFrame::toString() { return format(false); }


#endif // } HXCPP_STACK_TRACE


HXCPP_EXTERN_CLASS_ATTRIBUTES void NullReference(const char *type, bool allowFixup)
{
#ifdef HXCPP_DEBUGGER
   String err(HX_CSTRING("Null ") + String(type) + HX_CSTRING(" Reference"));
   if (__hxcpp_dbg_fix_critical_error(err))
      return;
#endif

   __hxcpp_dbg_checkedThrow(HX_CSTRING("Null Object Reference"));
}


} // end namspace hx

#ifndef HXCPP_DEBUGGER
void __hxcpp_execution_trace(int inLevel) { }
#endif


void __hx_dump_stack()
{
#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->beginCatch(false);
   ctx->dumpExceptionStack();
#endif
}


void __hx_stack_set_last_exception()
{
#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->setLastException();
#endif
}


void __hx_stack_push_last_exception()
{
#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->pushLastException();
#endif
}


void __hxcpp_stack_begin_catch()
{
#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->beginCatch(false);
#endif
}


Array<String> __hxcpp_get_call_stack(bool inSkipLast)
{
    Array<String> result = Array_obj<String>::__new();

#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->getCurrentCallStackAsStrings(result,inSkipLast);
#endif
   return result;
}


Array<String> __hxcpp_get_exception_stack()
{
    Array<String> result = Array_obj<String>::__new();

#ifdef HXCPP_STACK_TRACE
   hx::StackContext *ctx = hx::StackContext::getCurrent();
   ctx->getCurrentExceptionStackAsStrings(result);
#endif

    return result;
}

void __hxcpp_set_critical_error_handler(Dynamic inHandler)
{
   hx::setStaticHandler(hx::sCriticalErrorHandler,inHandler);
}

#ifndef HXCPP_DEBUGGER
void __hxcpp_set_debugger_info(const char **inAllClasses, const char **inFullPaths) { }
#endif


#ifndef HXCPP_PROFILER
void __hxcpp_start_profiler(::String inDumpFile)
{
   DBGLOG("Warning - profiler has no effect without HXCPP_PROFILER\n");
}
void __hxcpp_stop_profiler() { }
#endif


