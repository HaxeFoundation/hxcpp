#include <hxcpp.h>
#include <stdio.h>

#include <hx/Thread.h>

#ifdef ANDROID
#define DBGLOG(...) __android_log_print(ANDROID_LOG_INFO, "hxcpp", __VA_ARGS__)
#else
#define DBGLOG printf
#endif

#include <map>
#include <vector>
#include <string>
#include <time.h>

#ifdef HX_WINDOWS
#include <windows.h>
#include <stdio.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif


void __hx_dump_stack_all();

namespace hx
{

void CriticalError(const String &inErr)
{
   #ifdef HXCPP_DEBUGGER
   if (__hxcpp_dbg_handle_error(inErr))
      return;
   #endif
   
   #ifdef HXCPP_STACK_TRACE
   __hx_dump_stack_all();
   #endif
   
   DBGLOG("Critical Error: %s\n", inErr.__s);
   
   #if defined(HX_WINDOWS) && !defined(HX_WINRT)
   MessageBoxA(0,inErr.__s,"Critial Error - program must terminate",MB_ICONEXCLAMATION|MB_OK);
   #endif
   // Good when using gdb...
   // *(int *)0=0;
   exit(1);
}

void NullObjectReference()
{
   CriticalError(HX_CSTRING("Null Object Reference"));
}

}




#ifdef HXCPP_STACK_TRACE // {

bool gProfileThreadRunning = false;

namespace hx
{

// --- Profiling ------------------------------------------

bool gIsProfiling = false;


typedef std::map<const char *,int> ChildEntry;

struct ProfileEntry
{
   ProfileEntry() : myCount(0), totalCount(0) { }
   int myCount;
   int totalCount;
   // Can we assume strings are constant, and refer to the same memory location? - we will see.
   ChildEntry children;
};

typedef std::map<const char *, ProfileEntry> ProfileStats;
struct FlatResult
{
   bool operator<(const FlatResult &inRHS) const { return totalCount > inRHS.totalCount; }
   const char *func;
   int        myCount;
   int        totalCount;
   ChildEntry *children;
};

int gProfileClock = 0;
int gSampleClock = 0;
 
// --- Debugging ------------------------------------------


MyMutex gBreakpointLock;

int  gBreakpoint = 0;

unsigned short  gBPVersion = 0;

struct Breakpoint
{ 
   const char *file;
   int line;
};

typedef std::vector<Breakpoint> Breakpoints;

static Breakpoints gBreakpoints;

} // end namespace hx



bool    dbgInit = false;
hx::CallStack   *dbgInDebugger = 0;
Dynamic dbgHandler;
Dynamic dbgThread;

enum BreakMode
{
   bmBreakThis = -2,
   bmExit      = -1,

   bmNone      = 0x00,
   bmASAP      = 0x01,
   bmStep      = 0x02,
   bmEnter     = 0x04,
   bmLeave     = 0x08,
   bmStepMask  = 0x0f,
   bmFunction  = 0x10,
};

void __hxcpp_dbg_set_handler(Dynamic inHandler)
{
   #ifndef HXCPP_DEBUGGER
   DBGLOG("WARNING: debugger being used without -DHXCPP_DEBUGGER");
   #endif
   if (!dbgInit)
   {
      dbgInit = true;
      GCAddRoot(&dbgHandler.mPtr);
      GCAddRoot(&dbgThread.mPtr);
   }
   dbgHandler = inHandler;
}


void __hxcpp_dbg_set_thread(Dynamic inThread)
{
   dbgThread = inThread;
}



namespace hx
{

// --- CallStack ---------------------------------------------

struct CallStack
{
   enum { StackSize = 1000 };

   CallStack()
   {
      mSize = 0;
      mExceptionStackSize = 0;
      mExceptionStackOverflow = false;
      mProfiling = false;
      mProfileStats = 0;
      mDebuggerStart = StackSize;
      mLastException = 0;
      #if HXCPP_DEBUGGER
      for(int i=0;i<mSize;i++)
      {
         mLocations[i].mBPVersion = 0xffff;
         mLocations[i].mFile = 0;
      }
      #endif
   }
   ~CallStack()
   {
      delete mProfileStats;
      printf("Bye-bye call stack!\n");
   }

   void StartProfile(String inDumpFile)
   {
      if (mProfileStats)
         delete mProfileStats;
      mProfileStats = new ProfileStats();

      mDumpFile = inDumpFile.__s ? inDumpFile.__s : "";

      mT0 = gProfileClock = 0;
      mProfiling = true;
   }

   void StopProfile()
   {
      mProfiling = false;
      FILE *out = 0;
      if (!mDumpFile.empty())
         out = fopen(mDumpFile.c_str(),"wb");
      std::vector<FlatResult> results;
      results.reserve(mProfileStats->size());
      int total = 0;
      for(ProfileStats::iterator i = mProfileStats->begin(); i!=mProfileStats->end(); ++i)
      {
         FlatResult r;
         r.func = i->first;
         r.totalCount = i->second.totalCount;
         r.myCount = i->second.myCount;
         total += r.myCount;
         r.children = &(i->second.children);
         results.push_back(r);
      }

      std::sort(results.begin(), results.end());

      double scale = total==0 ? 1.0 : 100.0/total;
      for(int i=0;i<results.size();i++)
      {
         FlatResult &r = results[i];
         int internal = r.totalCount;
         if (out)
            fprintf(out,"%s %.2f%%/%.2f%%\n", r.func, r.totalCount*scale, r.myCount*scale );
         else
            DBGLOG("%s %.2f%%/%.2f%%\n", r.func, r.totalCount*scale, r.myCount*scale );

         for(ChildEntry::iterator c=r.children->begin(); c!=r.children->end(); c++)
         {
            if (out)
               fprintf(out,"   %s %.1f%%\n", c->first, 100.0*c->second/r.totalCount);
            else
               DBGLOG("   %s %.1f%%\n", c->first, 100.0*c->second/r.totalCount);
            internal -= c->second;
         }
         if (internal)
         {
            if (out)
               fprintf(out,"   (internal) %.1f%%\n", 100.0*internal/r.totalCount);
            else
               DBGLOG("   (internal) %.1f%%\n", 100.0*internal/r.totalCount);
         }
      }
         
      if (out)
         fclose(out);
      delete mProfileStats;
      mProfileStats = 0;
   }

   void Sample()
   {
      if (!gProfileThreadRunning)
      {
         DBGLOG("Forced profile stop.");
         StopProfile();
      }
      else if (mT0!=gProfileClock)
      {
         int clock = gProfileClock;
         gSampleClock = gProfileClock;
         int delta = clock-mT0;
         if (delta<0) delta = 1;
         mT0 = clock;

         for(int i=1;i<=mSize;i++)
         {
            ProfileEntry &p = (*mProfileStats)[mLocations[i].mFunction];
            bool alreadyInStack = false;
            for(int j=0;j<i;j++)
               if (mLocations[i].mFunction==mLocations[j].mFunction)
               {
                  alreadyInStack = true;
                  break;
               }
            if (!alreadyInStack)
               p.totalCount += delta;
            if (i<mSize)
               p.children[mLocations[i+1].mFunction] += delta;
            else
               p.myCount += delta;
         }
      }
   }

   bool EnterDebugMode()
   {
      bool result = false;
      if (!dbgInDebugger && dbgHandler.mPtr )
      {
         if ( __hxcpp_is_current_thread(dbgThread.mPtr) )
         {
            dbgInDebugger = this;
            mDebuggerStart = mSize+1;
            hx::gBreakpoint &= ~bmStepMask;
            result = true;
            dbgHandler();
            mDebuggerStart = StackSize;
            dbgInDebugger = 0;
         }
      }
      return result;
   }


   CallLocation *Push(const char *inName, const char *inFile, int inLine)
   {
      if (mProfiling) Sample();

      mSize++;
      if (mSize<StackSize)
      {
          mLocations[mSize].mFunction = inName;
          mLocations[mSize].mFile = inFile;
          mLocations[mSize].mLine = inLine;
          #ifdef HXCPP_DEBUGGER
          if (inFile==mLocations[mSize-1].mFile)
             mLocations[mSize].mBPOnFile = mLocations[mSize-1].mBPOnFile;
          else
             mLocations[mSize].mBPVersion = gBPVersion - 1;
          #endif
          #ifdef HXCPP_STACK_VARS
          mLocations[mSize].mLocal = 0;
          #endif
          if ( hx::gBreakpoint & (bmASAP|bmEnter) )
             EnterDebugMode();
          return mLocations + mSize;
      }

      if ( hx::gBreakpoint & (bmASAP|bmEnter) )
         EnterDebugMode();
      return mLocations + StackSize-1;
   }

   void Pop()
   {
      if (mProfiling) Sample();
     --mSize;
      if ( hx::gBreakpoint & (bmASAP | bmLeave) )
         EnterDebugMode();
   }

   void CheckBreakpoint()
   {
      #ifdef HXCPP_DEBUGGER
      if (hx::gBreakpoint & bmASAP)
         EnterDebugMode();
      else if (hx::gBreakpoint & bmFunction)
      {
         CallLocation &loc = mLocations[mSize];
         if (gBPVersion!=loc.mBPVersion)
         {
            bool onFile = false;
            AutoLock lock(hx::gBreakpointLock);
            loc.mBPVersion = gBPVersion;
            for(int i=0;i<gBreakpoints.size();i++)
               if (gBreakpoints[i].file == loc.mFile)
               {
                  onFile = true;
                  if (gBreakpoints[i].line == loc.mLine)
                  {
                     lock.Unlock();
                     EnterDebugMode();
                     break;
                  }
               }
            loc.mBPOnFile = onFile;
         }
         else if (loc.mBPOnFile)
         {
            AutoLock lock(hx::gBreakpointLock);
            for(int i=0;i<gBreakpoints.size();i++)
               if (gBreakpoints[i].file == loc.mFile && gBreakpoints[i].line==loc.mLine)
               {
                  lock.Unlock();
                  EnterDebugMode();
                  return;
               }
         }
      }
      #endif
   }

   void SetLastException()
   {
      mLastException = mSize+1;
   }

   void BeginCatch(bool inAll)
   {
      int start = inAll ? 1 : mSize;
      int end = inAll ? mSize+1 : mLastException;
      int last = end < StackSize ? end : StackSize;
      mExceptionStackOverflow = end - last;
      mExceptionStackSize = last-start;
      if (mExceptionStackSize<=0)
         mExceptionStackSize = 0;
      else
         memcpy(mExceptionStack, mLocations + start , sizeof(CallLocation)*mExceptionStackSize);
   }

   void DumpExceptionStack()
   {
      for(int i=0;i<mExceptionStackSize;i++)
      {
         CallLocation loc = mExceptionStack[i];
         #ifdef ANDROID
         if (loc.mFunction==0)
             __android_log_print(ANDROID_LOG_ERROR, "HXCPP", "Called from CFunction\n");
         else
             __android_log_print(ANDROID_LOG_ERROR, "HXCPP", "Called from %s, %s %d\n", loc.mFunction, loc.mFile, loc.mLine );
         #else
         if (loc.mFunction==0)
            printf("Called from CFunction\n");
         else
            printf("Called from %s, %s %d\n", loc.mFunction, loc.mFile, loc.mLine );
         #endif
      }
      if (mExceptionStackOverflow)
      {
         #ifdef ANDROID
         __android_log_print(ANDROID_LOG_ERROR,"HXCPP","... %d functions missing ...\n", mExceptionStackOverflow);
         #else
         printf("... %d functions missing ...\n", mExceptionStackOverflow);
         #endif
      }
   }

   Array< ::String > GetLastException()
   {
      Array< ::String > result = Array_obj< ::String >::__new();

      for(int i=0 ;i<mExceptionStackSize; i++)
      {
         CallLocation &loc = mExceptionStack[i];
         if (!loc.mFile || loc.mFile[0]=='?')
            result->push( String(loc.mFunction) );
         else
         {
            char buf[1024];
            sprintf(buf,"%s::%s::%d", loc.mFunction, loc.mFile, loc.mLine );
            result->push( String(buf) );
         }
      }
      return result;
   }

   Array< ::String > GetCallStack(bool inSkipLast)
   {
      Array< ::String > result = Array_obj< ::String >::__new();
      int n = mSize - inSkipLast;

      for(int i=1;i<=n && i<mDebuggerStart;i++)
      {
         CallLocation &loc = mLocations[i];
         if (!loc.mFile || loc.mFile[0]=='?')
            result->push( String(loc.mFunction) );
         else
         {
            char buf[1024];
            sprintf(buf,"%s::%s::%d", loc.mFunction, loc.mFile, loc.mLine );
            result->push( String(buf) );
         }
      }
 
      return result;
   }

   Array< ::String > GetStackVars(int inFrame)
   {
      Array< ::String > result = Array_obj< ::String >::__new();

      #ifdef HXCPP_STACK_VARS
      int idx =  (mDebuggerStart<mSize) ? mDebuggerStart - inFrame : mSize - inFrame;

      if (idx>=0)
      {
         hx::AutoVar *var = mLocations[idx].mLocal;
         while(var)
         {
            result->push( String(var->name) );
            var = var->next;
         }
      }
      #endif
 
      return result;
   }

   Dynamic GetStackVar(int inFrame, String inName)
   {
      #ifdef HXCPP_STACK_VARS
      int idx =  (mDebuggerStart<mSize) ? mDebuggerStart - inFrame : mSize - inFrame;

      if (idx>=0)
      {
         hx::AutoVar *var = mLocations[idx].mLocal;
         while(var)
         {
            if ( String(var->name)==inName)
               return var->get();
            var = var->next;
         }
      }
      #endif
 
      return null();
   }


   void SetStackVar(int inFrame, String inName, Dynamic inValue)
   {
      #ifdef HXCPP_STACK_VARS
      int idx =  (mDebuggerStart<mSize) ? mDebuggerStart - inFrame : mSize - inFrame;

      if (idx>=0)
      {
         hx::AutoVar *var = mLocations[idx].mLocal;
         while(var)
         {
            if ( String(var->name)==inName)
            {
               var->set(inValue);
               return;
            }
            var = var->next;
         }
      }
      #endif
   }






   #ifdef HXCPP_DEBUG_HOST
   void Where()
   {
      for(int i=1;i<=mSize && i<StackSize;i++)
      {
         CallLocation loc = mLocations[i];
         if (loc.mFunction==0)
             DbgWriteData("CFunction\n");
         else
         {
            char buf[2048];
            sprintf(buf,"%s @  %s:%d\n", loc.mFunction, loc.mFile, loc.mLine );
            DbgWriteData(buf);
         }
      }
   }

   void Trace()
   {
      int last = mSize;
      if (last>StackSize)
         last = StackSize-1;
    
      CallLocation loc = mLocations[last];
      char buf[2048];
      sprintf(buf,"%s @  %s:%d", loc.mFunction, loc.mFile, loc.mLine );
      DbgWriteInfo(buf);
   }
   #endif


   int mDebuggerStart;

   int mSize;
   CallLocation mLocations[StackSize];

   int mExceptionStackSize;
   int mLastException;
   bool mExceptionStackOverflow;
   CallLocation mExceptionStack[StackSize];

   bool mProfiling;
   int  mT0;
   std::string mDumpFile;
   ProfileStats *mProfileStats;
};

// Called with lock set...
void OnBreakpointChanged()
{
   gBreakpoint = (gBreakpoint & bmStepMask) | (gBreakpoints.size()>0 ? bmFunction : 0);
   gBPVersion++;
}


DECLARE_TLS_DATA(CallStack,tlsCallStack);

CallStack *GetCallStack()
{
   CallStack *result =  tlsCallStack;
   if (!result)
   {
      result = new CallStack();
      tlsCallStack = result;
   }
   return result;
}


AutoStack::AutoStack(const char *inName, const char *inFile, int inLine)
{
   mLocation = hx::GetCallStack()->Push(inName,inFile,inLine);
}

AutoStack::~AutoStack()
{
   hx::GetCallStack()->Pop();
}

void CheckBreakpoint()
{
   hx::GetCallStack()->CheckBreakpoint();
}


} // namespace hx

void __hxcpp_dbg_set_break(int inMode)
{
   if (inMode==bmExit)
      exit(1);
   else if (inMode==bmBreakThis)
      hx::GetCallStack()->EnterDebugMode();
   else
      hx::gBreakpoint = (hx::gBreakpoint & (~bmStepMask)) | inMode;
}


void __hxcpp_stack_begin_catch_all()
{
   hx::GetCallStack()->BeginCatch(true);
}

void __hxcpp_stack_begin_catch()
{
   hx::GetCallStack()->BeginCatch(false);
}

void __hx_stack_set_last_exception()
{
   hx::GetCallStack()->SetLastException();
}

// In response to caught exception
void __hx_dump_stack()
{
   hx::GetCallStack()->BeginCatch(false);
   hx::GetCallStack()->DumpExceptionStack();
}

void __hx_dump_stack_all()
{
   __hxcpp_stack_begin_catch_all();
   hx::GetCallStack()->DumpExceptionStack();
}

Array<String> __hxcpp_get_call_stack(bool inSkipLast)
{
   return hx::GetCallStack()->GetCallStack(inSkipLast);
}

Array<String> __hxcpp_get_exception_stack()
{
   return hx::GetCallStack()->GetLastException();
}



THREAD_FUNC_TYPE profile_main_loop( void *)
{
   int millis = 1;
   hx::gSampleClock = hx::gProfileClock;

   while(gProfileThreadRunning)
   {
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
		nanosleep(&t,&tmp);
      #endif

      int count = hx::gProfileClock + 1;
      if (count>2000 && (hx::gSampleClock+100 < hx::gProfileClock) )
      {
         // Main thread seems to have forgotten about us - force stop
         gProfileThreadRunning = false;
         DBGLOG("Profiler terminated due to lack of activity");
         break;
      }
      hx::gProfileClock = count < 0 ? 0 : count;
   }
	THREAD_FUNC_RET
}


void __hxcpp_start_profiler(::String inDumpFile)
{
   if (!gProfileThreadRunning)
   {
      gProfileThreadRunning = true;
      #if defined(HX_WINDOWS)
         #ifndef HX_WINRT
         _beginthreadex(0,0,profile_main_loop,0,0,0);
         #else
         // TODO
         #endif
      #else
      pthread_t result;
      pthread_create(&result,0,profile_main_loop,0);
	   #endif
   }

   hx::CallStack *stack = hx::GetCallStack();
   stack->StartProfile(inDumpFile);
}

void __hxcpp_stop_profiler()
{
   if (gProfileThreadRunning)
   {
      gProfileThreadRunning = false;
      hx::GetCallStack()->StopProfile();
   }
}


#else // } HXCPP_STACK_TRACE {

void __hx_dump_stack()
{
   //printf("No stack in release mode.\n");
}


Array<String> __hxcpp_get_call_stack(bool inSkipLast)
{
   Array< ::String > result = Array_obj< ::String >::__new();
   return result;
}

Array<String> __hxcpp_get_exception_stack()
{
   Array< ::String > result = Array_obj< ::String >::__new();
   return result;
}



void __hxcpp_start_profiler(::String)
{
    DBGLOG("Compile with -D HXCPP_STACK_TRACE or -debug to use profiler.\n");
}

void __hxcpp_stop_profiler()
{
}

void __hxcpp_dbg_set_handler(Dynamic inHandler) { }
void __hxcpp_dbg_set_break(int) { }
void __hxcpp_dbg_set_thread(Dynamic inThread) { }


#endif // }

#ifdef HXCPP_DEBUGGER // {
namespace hx
{
   extern const char *__hxcpp_class_path[];
   extern const char *__hxcpp_all_files[];
}

Array<Dynamic> __hxcpp_dbg_get_files( )
{
   Array< ::String > result = Array_obj< ::String >::__new();
   for(const char **ptr = hx::__hxcpp_all_files; *ptr; ptr++)
      result->push(String(*ptr));
   return result;
}

void __hxcpp_breakpoints_add(int inFile, int inLine)
{
   hx::Breakpoint bp;
   bp.file = hx::__hxcpp_all_files[inFile];
   bp.line = inLine;
   AutoLock lock(hx::gBreakpointLock);
   hx::gBreakpoints.push_back(bp);
   hx::OnBreakpointChanged();
}

Array<String> __hxcpp_dbg_breakpoints_get( )
{
   Array< ::String > result = Array_obj< ::String >::__new();
   for(int i=0;i<hx::gBreakpoints.size();i++)
   {
      hx::Breakpoint &bp = hx::gBreakpoints[i];
      char buf[1024];
      sprintf(buf,"%s:%d", bp.file, bp.line );
      result->push( String(buf) );
   }
   return result;
}

void __hxcpp_dbg_breakpoints_delete(int inIndex)
{
   AutoLock lock(hx::gBreakpointLock);
   if (inIndex<0 || inIndex>=hx::gBreakpoints.size())
      return;
   hx::gBreakpoints.erase( hx::gBreakpoints.begin() + inIndex );
   hx::OnBreakpointChanged();
}

bool __hxcpp_dbg_handle_error(::String inError)
{
   if (dbgHandler.mPtr)
   {
	   hx::GetCallStack()->EnterDebugMode();
      hx::Throw(inError);
   }
   return false;
}

void __hxcpp_dbg_set_stack_var(int inFrame,String inVar, Dynamic inValue)
{
   if (dbgInDebugger)
      dbgInDebugger->SetStackVar(inFrame, inVar, inValue);
}

Dynamic __hxcpp_dbg_get_stack_var(int inFrame,String inVar)
{
   if (!dbgInDebugger)
      return null();
   return dbgInDebugger->GetStackVar(inFrame,inVar);
}

Array<String> __hxcpp_dbg_get_stack_vars(int inFrame)
{
   if (!dbgInDebugger)
      return null();
   return dbgInDebugger->GetStackVars(inFrame);
}

#else // } HXCPP_DEBUGGER {

Array<Dynamic> __hxcpp_dbg_get_files( ) { return null(); }
void __hxcpp_breakpoints_add(int inFile, int inLine) { }
Array<String> __hxcpp_dbg_breakpoints_get( ) { return null(); }
void __hxcpp_dbg_breakpoints_delete(int inIndex) { }
bool __hxcpp_debugger_handle_error(::String inError) { return false; }
void __hxcpp_dbg_set_stack_var(int inFrame,String inVar, Dynamic inValue) { }
Dynamic __hxcpp_dbg_get_stack_var(int inFrame,String inVar) { return null(); }
Array<String> __hxcpp_dbg_get_stack_vars(int inFrame) { return null(); }

#endif // }

// Debug stubs

Array<Dynamic> __hxcpp_dbg_stack_frames_get( ) { return null(); }
Array<Class> __hxcpp_dbg_get_classes( ) { return null(); }

