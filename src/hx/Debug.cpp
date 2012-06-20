#include <hxcpp.h>
#include <stdio.h>

#include <hx/Thread.h>

#ifdef ANDROID
#define DBGLOG(...) __android_log_print(ANDROID_LOG_INFO, "hxcpp", __VA_ARGS__)
#else
#define DBGLOG printf
#endif


#ifdef HXCPP_STACK_TRACE // {

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


void __hx_stack_set_last_exception();

namespace hx
{


void CriticalError(const String &inErr)
{
   __hx_stack_set_last_exception();
   __hx_dump_stack();

   #ifdef HX_UTF8_STRINGS
   fprintf(stderr,"Critical Error: %s\n", inErr.__s);
   #else
   fprintf(stderr,"Critical Error: %S\n", inErr.__s);
   #endif

   #ifdef HX_WINDOWS
      #ifdef HX_UTF8_STRINGS
      MessageBoxA(0,inErr.__s,"Critial Error - program must terminate",MB_ICONEXCLAMATION|MB_OK);
      #else
      MessageBoxW(0,inErr.__s,L"Critial Error - program must terminate",MB_ICONEXCLAMATION|MB_OK);
      #endif
   #endif
   // Good when using gdb...
   // *(int *)0=0;
   exit(1);
}

// --- Profiling ------------------------------------------

bool gIsProfiling = false;

struct CallLocation
{
   const char *mFunction;
   const char *mFile;
   int        mLine; 
};

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
 
// --- Debugging ------------------------------------------

} // end namespace hx

bool    dbgInit = false;
bool    dbgInDebugger = false;
Dynamic dbgHandler;
Dynamic dbgThread;
enum BreakMode { bmNone, bmASAP, bmStep, bmEnter, bmLeave };
BreakMode dbgBreak = bmNone;

void __hxcpp_dbg_set_handler(Dynamic inHandler)
{
   if (!dbgInit)
   {
      dbgInit = true;
      GCAddRoot(&dbgHandler.mPtr);
      GCAddRoot(&dbgThread.mPtr);
   }
   dbgHandler = inHandler;
}

void __hxcpp_dbg_set_break(int inMode,Dynamic inThread)
{
   if (inMode==-1)
      exit(1);
   
   dbgThread = inThread;
   dbgBreak = (BreakMode)inMode;
}

void EnterDebugMode()
{
   if (!dbgInDebugger && dbgHandler.mPtr )
   {
      dbgInDebugger = true;
      if ( __hxcpp_thread_current().mPtr != dbgThread.mPtr)
      {
         dbgBreak = bmNone;
         dbgHandler();
      }
      dbgInDebugger = false;
   }
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
      mLastException = 0;
      mSinceLastException = 0;
      mProfiling = false;
      mProfileStats = 0;
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

      mT0 = gProfileClock;
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
      if (mT0!=gProfileClock)
      {
         int clock = gProfileClock;
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

   void Push(const char *inName)
   {
      if (mProfiling) Sample();

      mSize++;
      if (mSize>mSinceLastException)
         mSinceLastException = mSize;
      //mLastException = 0;
      if (mSize<StackSize)
      {
          mLocations[mSize].mFunction = inName;
          mLocations[mSize].mFile = "?";
          mLocations[mSize].mLine = 0;
      }
      if ( dbgBreak==bmASAP || dbgBreak==bmEnter)
         EnterDebugMode();
   }
   void Pop()
   {
      if (mProfiling) Sample();
     --mSize;
      if ( dbgBreak==bmASAP || dbgBreak==bmLeave)
         EnterDebugMode();
   }

   void SetSrcPos(const char *inFile, int inLine)
   {
      if (mSize<StackSize)
      {
          mLocations[mSize].mFile = inFile;
          mLocations[mSize].mLine = inLine;
      }
      if (dbgBreak==bmASAP)
         EnterDebugMode();
      
   }

   void VarPush(__AutoVar *inVar)
   {
   }

   void VarPop( )
   {
   }

   void SetLastException()
   {
      mLastException = mSize;
      mSinceLastException = 0;
   }
   void Dump()
   {
      for(int i=1;i<=mLastException && i<StackSize;i++)
      {
         CallLocation loc = mLocations[i];
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
      if (mLastException >= StackSize)
      {
         printf("... %d functions missing ...\n", mLastException + 1 - StackSize);
      }
   }

   Array< ::String > GetLastException()
   {
      Array< ::String > result = Array_obj< ::String >::__new();

      int first = mSinceLastException;
      if (mSize>first)
          first = mSize;
      for(int i=first ;i<=mLastException && i<StackSize;i++)
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

   Array< ::String > GetCallStack(bool inSkipLast)
   {
      Array< ::String > result = Array_obj< ::String >::__new();
      int n = mSize - inSkipLast;

      for(int i=1;i<=n && i<StackSize;i++)
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



   #ifdef HXCPP_DEBUG_HOST
   void Where()
   {
      int last = mSize + 1;
      if (mLastException!=0 && mLastException<last)
         last = mLastException;
      if (last>StackSize)
         last = StackSize;
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
      //if (mLastException>0 && mLastException<last)
      //   last = mLastException;
      if (last>StackSize)
         last = StackSize-1;
    
      CallLocation loc = mLocations[last];
      char buf[2048];
      sprintf(buf,"%s @  %s:%d", loc.mFunction, loc.mFile, loc.mLine );
      DbgWriteInfo(buf);
   }
   #endif


   int mSize;
   int mLastException;
   int mSinceLastException;
   CallLocation mLocations[StackSize];
   bool mProfiling;
   int  mT0;
   std::string mDumpFile;
   ProfileStats *mProfileStats;
};


TLSData<CallStack> tlsCallStack;
CallStack *GetCallStack()
{
   CallStack *result =  tlsCallStack.Get();
   if (!result)
   {
      result = new CallStack();
      tlsCallStack.Set(result);
   }
   return result;
}

}

__AutoStack::__AutoStack(const char *inName)
{
   hx::GetCallStack()->Push(inName);
}

__AutoStack::~__AutoStack()
{
   hx::GetCallStack()->Pop();
}

void __hx_set_source_pos(const char *inFile, int inLine)
{
   hx::GetCallStack()->SetSrcPos(inFile,inLine);
}

void __hx_dump_stack()
{
   hx::GetCallStack()->Dump();
}

void __hx_stack_set_last_exception()
{
   hx::GetCallStack()->SetLastException();
}


Array<String> __hxcpp_get_call_stack(bool inSkipLast)
{
   return hx::GetCallStack()->GetCallStack(inSkipLast);
}

Array<String> __hxcpp_get_exception_stack()
{
   return hx::GetCallStack()->GetLastException();
}

void __hxcpp_dbg_var_pop()
{
   return hx::GetCallStack()->VarPop();
}

void __hxcpp_dbg_var_push(__AutoVar *inVar)
{
   return hx::GetCallStack()->VarPush(inVar);
}




bool gProfileThreadRunning = false;

THREAD_FUNC_TYPE profile_main_loop( void *)
{
   int millis = 1;
   int alloc_count = 0;
   int last_thousand = 0;
   int total_count = 0;

   while(gProfileThreadRunning)
   {
      #ifdef HX_WINDOWS
	   Sleep(millis);
      #else
		struct timespec t;
		struct timespec tmp;
		t.tv_sec = 0;
		t.tv_nsec = millis * 1000000;
		nanosleep(&t,&tmp);
      #endif

      int count = hx::gProfileClock + 1;
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
      _beginthreadex(0,0,profile_main_loop,0,0,0);
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
   gProfileThreadRunning = false;
   hx::GetCallStack()->StopProfile();
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



void __hxcpp_start_profiler(::Float)
{
    DBGLOG("Compile with -D HXCPP_STACK_TRACE or -debug to use profiler.\n");
}

void __hxcpp_stop_profiler()
{
}

void __hxcpp_dbg_set_handler(Dynamic inHandler) { }
void __hxcpp_dbg_set_break(int,Dynamic inThread) { }

#endif // }

// Debug stubs

void __hxcpp_breakpoints_add(Dynamic inBreakpoint) { }
Dynamic __hxcpp_dbg_breakpoints_get( ) { return null(); }
void __hxcpp_dbg_breakpoints_delete(int inIndex) { }
Array<Dynamic> __hxcpp_dbg_stack_frames_get( ) { return null(); }
Array<Dynamic> __hxcpp_dbg_get_files( ) { return null(); }
Array<Class> __hxcpp_dbg_get_classes( ) { return null(); }

