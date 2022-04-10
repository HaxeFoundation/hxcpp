#include <hxcpp.h>

#include <hx/GC.h>
#include <hx/Memory.h>
#include <hx/Thread.h>
#include "../Hash.h"
#include "GcRegCapture.h"
#include <hx/Unordered.h>

#include <stdlib.h>


static bool sgIsCollecting = false;

namespace hx
{
   int gByteMarkID = 0x10;
   int gRememberedByteMarkID = 0x10 | HX_GC_REMEMBERED;


int gFastPath = 0;
int gSlowPath = 0;


}

using hx::gByteMarkID;
using hx::gRememberedByteMarkID;

// #define HXCPP_SINGLE_THREADED_APP

namespace hx
{
#ifdef HXCPP_GC_DEBUG_ALWAYS_MOVE

enum { gAlwaysMove = true };
typedef hx::UnorderedSet<void *> PointerMovedSet;
PointerMovedSet sgPointerMoved;

#else
enum { gAlwaysMove = false };
#endif
}

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef HX_WINDOWS
#include <windows.h>
#endif

#include <vector>
#include <stdio.h>

#include <hx/QuickVec.h>

// #define HXCPP_GC_BIG_BLOCKS


#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
namespace {
void DebuggerTrap()
{
   static bool triggeredOnce = false;

   if (!triggeredOnce)
   {
      triggeredOnce = true;

      #if __has_builtin(__builtin_trap)
      __builtin_trap();
      #else
      *(int *)0=0;
      #endif
   }
}
}




static bool sgAllocInit = 0;
static bool sgInternalEnable = true;
static void *sgObject_root = 0;
// With virtual inheritance, stack pointers can point to the middle of an object
#ifdef _MSC_VER
// MSVC optimizes by taking the address of an initernal data member
static int sgCheckInternalOffset = sizeof(void *)*2;
static int sgCheckInternalOffsetRows = 1;
#else
static int sgCheckInternalOffset = 0;
static int sgCheckInternalOffsetRows = 0;
#endif

int gInAlloc = false;

// This is recalculated from the other parameters
static size_t sWorkingMemorySize          = 10*1024*1024;

#ifdef HXCPP_GC_MOVING
// Just not sure what this shold be
static size_t sgMaximumFreeSpace  = 1024*1024*1024;
#else
static size_t sgMaximumFreeSpace  = 1024*1024*1024;
#endif

#ifdef EMSCRIPTEN
#define HXCPP_STACK_UP
#endif


// #define HXCPP_GC_DEBUG_LEVEL 1

#if HXCPP_GC_DEBUG_LEVEL>1
  #define PROFILE_COLLECT
  #if HXCPP_GC_DEBUG_LEVEL>2
     #define SHOW_FRAGMENTATION
     #if HXCPP_GC_DEBUG_LEVEL>3
        #define SHOW_MEM_EVENTS
     #endif
  #endif
#endif

// #define SHOW_FRAGMENTATION_BLOCKS
#if defined SHOW_FRAGMENTATION_BLOCKS
  #define SHOW_FRAGMENTATION
#endif

#define RECYCLE_LARGE

// HXCPP_GC_DYNAMIC_SIZE

//#define HXCPP_GC_SUMMARY
//#define PROFILE_COLLECT
//#define PROFILE_THREAD_USAGE
//#define HX_GC_VERIFY
//#define HX_GC_VERIFY_ALLOC_START
//#define SHOW_MEM_EVENTS
//#define SHOW_MEM_EVENTS_VERBOSE
//#define SHOW_FRAGMENTATION
//
// Setting this can make it easier to reproduce this problem since it takes
//  the timing of the zeroing out of the equation
//#define HX_GC_ZERO_EARLY

// Setting this on windows64 will mean this objects are allocated in the same place,
//  which can make native debugging easier
//#define HX_GC_FIXED_BLOCKS
//
// If the problem happens on the same object, you can print info about the object
//  every collect so you can see where it goes wrong (usually requires HX_GC_FIXED_BLOCKS)
//#define HX_WATCH

#if defined(HX_GC_VERIFY) && defined(HXCPP_GC_GENERATIONAL)
#define HX_GC_VERIFY_GENERATIONAL
#endif


#ifdef HX_WATCH
void *hxWatchList[] = {
  (void *)0x0000000100000200,
  (void *)0
};
bool hxInWatchList(void *watch)
{
   for(void **t = hxWatchList; *t; t++)
      if (*t==watch)
         return true;
   return false;
}
#endif

#ifdef HX_GC_FIXED_BLOCKS
#ifdef HX_WINDOWS
#include <Memoryapi.h>
#endif
#endif

#ifdef PROFILE_COLLECT
   #define HXCPP_GC_SUMMARY
#endif

#ifdef HX_GC_VERIFY_GENERATIONAL
static bool sGcVerifyGenerational = false;
#endif


#if HX_HAS_ATOMIC && (HXCPP_GC_DEBUG_LEVEL==0) && !defined(HX_GC_VERIFY)
  #if defined(HX_MACOS) || defined(HX_WINDOWS) || defined(HX_LINUX)
  enum { MAX_GC_THREADS = 4 };
  #else
  enum { MAX_GC_THREADS = 2 };
  #endif
#else
  enum { MAX_GC_THREADS = 1 };
#endif

#if (MAX_GC_THREADS>1)
   // You can uncomment this for better call stacks if it crashes while collecting
   #define HX_MULTI_THREAD_MARKING
#endif

#ifdef PROFILE_THREAD_USAGE
static int sThreadMarkCountData[MAX_GC_THREADS+1];
static int sThreadArrayMarkCountData[MAX_GC_THREADS+1];
static int *sThreadMarkCount = sThreadMarkCountData + 1;
static int *sThreadArrayMarkCount = sThreadArrayMarkCountData + 1;
static int sThreadChunkPushCount;
static int sThreadChunkWakes;
static int sSpinCount = 0;
static int sThreadZeroWaits = 0;
static int sThreadZeroPokes = 0;
static int sThreadBlockZeroCount = 0;
static volatile int sThreadZeroMisses = 0;
#endif

enum { MARK_BYTE_MASK = 0x0f };
enum { FULL_MARK_BYTE_MASK = 0x3f };


enum
{
   MEM_INFO_USAGE = 0,
   MEM_INFO_RESERVED = 1,
   MEM_INFO_CURRENT = 2,
   MEM_INFO_LARGE = 3,
};

enum GcMode
{
   gcmFull,
   gcmGenerational,
};

// Start with full gc - since all objects will be new
static GcMode sGcMode = gcmFull;



#ifndef HXCPP_GC_MOVING
  #ifdef HXCPP_GC_DEBUG_ALWAYS_MOVE
  #define HXCPP_GC_MOVING
  #endif
// Enable moving collector...
//#define HXCPP_GC_MOVING
#endif

// Allocate this many blocks at a time - this will increase memory usage % when rounding to block size must be done.
// However, a bigger number makes it harder to release blocks due to pinning
#define IMMIX_BLOCK_GROUP_BITS  5


#ifdef HXCPP_DEBUG
static hx::Object *gCollectTrace = 0;
static bool gCollectTraceDoPrint = false;
static int gCollectTraceCount = 0;
static int sgSpamCollects = 0;
#endif

#if defined(HXCPP_DEBUG) || defined(HXCPP_GC_DEBUG_ALWAYS_MOVE)
volatile int sgAllocsSinceLastSpam = 0;
#endif

#ifdef ANDROID
#define GCLOG(...) __android_log_print(ANDROID_LOG_INFO, "gclog", __VA_ARGS__)
#else
#define GCLOG printf
#endif

#ifdef PROFILE_COLLECT
   #define STAMP(t) double t = __hxcpp_time_stamp();
   #define MEM_STAMP(t) t = __hxcpp_time_stamp();
   static double sLastCollect = __hxcpp_time_stamp();
   static int sObjectMarks =0;
   static int sAllocMarks =0;

#else
   #define STAMP(t)
   #define MEM_STAMP(t)
#endif

#if defined(HXCPP_GC_SUMMARY) || defined(HXCPP_GC_DYNAMIC_SIZE)
struct ProfileCollectSummary
{
   enum { COUNT = 10 };
   double timeWindow[COUNT];
   int    windowIdx;
   double startTime;
   double lastTime;
   double totalCollecting;
   double maxStall;
   double spaceFactor;

   ProfileCollectSummary()
   {
      startTime = __hxcpp_time_stamp();
      lastTime = startTime;
      totalCollecting = 0;
      maxStall = 0;
      windowIdx = 0;
      spaceFactor = 1.0;
      for(int i=0;i<COUNT;i++)
         timeWindow[i] = 0.1;
   }
   ~ProfileCollectSummary()
   {
      #ifdef HXCPP_GC_SUMMARY
      double time = __hxcpp_time_stamp() - startTime;
      GCLOG("Total time     : %.2fms\n", time*1000.0);
      GCLOG("Collecting time: %.2fms\n", totalCollecting*1000.0);
      GCLOG("Max Stall time : %.2fms\n", maxStall*1000.0);
      if (time==0) time = 1;
      GCLOG(" Fraction      : %.2f%%\n",totalCollecting*100.0/time);

      #ifdef HXCPP_GC_DYNAMIC_SIZE
      GCLOG("Space factor   : %.2fx\n",spaceFactor);
      #endif
      #endif
   }

   void addTime(double inCollectStart)
   {
      double now = __hxcpp_time_stamp();
      double dt =  now-inCollectStart;
      if (dt>maxStall)
         maxStall = dt;
      totalCollecting += dt;
      #ifdef HXCPP_GC_DYNAMIC_SIZE
      double wholeTime = now - lastTime;
      if (wholeTime)
      {
         lastTime = now;
         double ratio = dt/wholeTime;
         windowIdx = (windowIdx +1)%COUNT;
         timeWindow[windowIdx] = ratio;
         double sum = 0;
         for(int i=0;i<COUNT;i++)
            sum += timeWindow[i];
         ratio = sum/COUNT;

         if (ratio<0.05)
         {
            if (spaceFactor>1.0)
            {
               spaceFactor -= 0.2;
               if (spaceFactor<1.0)
                  spaceFactor = 1.0;
            }
         }
         else if (ratio>0.12)
         {
            if (ratio>0.2)
               spaceFactor += 0.5;
            else
               spaceFactor += 0.2;

            if (spaceFactor>5.0)
               spaceFactor = 5.0;
         }
      }
      #endif
   }

};

static ProfileCollectSummary profileCollectSummary;
#define PROFILE_COLLECT_SUMMARY_START double collectT0 = __hxcpp_time_stamp();
#define PROFILE_COLLECT_SUMMARY_END profileCollectSummary.addTime(collectT0);

#else
#define PROFILE_COLLECT_SUMMARY_START 
#define PROFILE_COLLECT_SUMMARY_END
#endif



// TODO: Telemetry.h ?
#ifdef HXCPP_TELEMETRY
extern void __hxt_gc_realloc(void* old_obj, void* new_obj, int new_size);
extern void __hxt_gc_start();
extern void __hxt_gc_end();
extern void __hxt_gc_after_mark(int gByteMarkID, int markIdByte);
#endif

static int sgTimeToNextTableUpdate = 1;




HxMutex  *gThreadStateChangeLock=0;
HxMutex  *gSpecialObjectLock=0;

class LocalAllocator;
enum LocalAllocState { lasNew, lasRunning, lasStopped, lasWaiting, lasTerminal };


static void MarkLocalAlloc(LocalAllocator *inAlloc,hx::MarkContext *__inCtx);
#ifdef HXCPP_VISIT_ALLOCS
static void VisitLocalAlloc(LocalAllocator *inAlloc,hx::VisitContext *__inCtx);
#endif
static void WaitForSafe(LocalAllocator *inAlloc);
static void ReleaseFromSafe(LocalAllocator *inAlloc);
static void ClearPooledAlloc(LocalAllocator *inAlloc);
static void CollectFromThisThread(bool inMajor,bool inForceCompact);

namespace hx
{
int gPauseForCollect = 0x00000000;

StackContext *gMainThreadContext = 0;

unsigned int gImmixStartFlag[128];

int gMarkID = 0x10 << 24;
int gMarkIDWithContainer = (0x10 << 24) | IMMIX_ALLOC_IS_CONTAINER;

int gPrevByteMarkID = 0x2f;
unsigned int gPrevMarkIdMask = ((~0x2f000000) & 0x30000000) | HX_GC_CONST_ALLOC_BIT;




void ExitGCFreeZoneLocked();


DECLARE_FAST_TLS_DATA(StackContext, tlsStackContext);

#ifdef HXCPP_SCRIPTABLE
extern void scriptMarkStack(hx::MarkContext *);
#endif
}

//#define DEBUG_ALLOC_PTR ((char *)0xb68354)


#ifdef HX_WINDOWS
#define ZERO_MEM(ptr, n) ZeroMemory(ptr,n)
#else
#define ZERO_MEM(ptr, n) memset(ptr,0,n)
#endif


// ---  Internal GC - IMMIX Implementation ------------------------------



// Some inline implementations ...
// Use macros to allow for mark/move



/*
  IMMIX block size, and various masks for converting addresses

*/

#ifdef HXCPP_GC_BIG_BLOCKS
   #define IMMIX_BLOCK_BITS      16
   typedef unsigned int BlockIdType;
#else
   #define IMMIX_BLOCK_BITS      15
   typedef unsigned short BlockIdType;
#endif

#define IMMIX_BLOCK_SIZE        (1<<IMMIX_BLOCK_BITS)
#define IMMIX_BLOCK_OFFSET_MASK (IMMIX_BLOCK_SIZE-1)
#define IMMIX_BLOCK_BASE_MASK   (~(size_t)(IMMIX_BLOCK_OFFSET_MASK))
#define IMMIX_LINE_COUNT_BITS   (IMMIX_BLOCK_BITS-IMMIX_LINE_BITS)
#define IMMIX_LINES             (1<<IMMIX_LINE_COUNT_BITS)


#define IMMIX_HEADER_LINES      (IMMIX_LINES>>IMMIX_LINE_BITS)
#define IMMIX_USEFUL_LINES      (IMMIX_LINES - IMMIX_HEADER_LINES)

#define IMMIX_MAX_ALLOC_GROUPS_SIZE  (1<<IMMIX_BLOCK_GROUP_BITS)


// Every second line used
#define MAX_HOLES (IMMIX_USEFUL_LINES>>1)

/*

 IMMIX Alloc Header - 32 bits


 The header is placed in the 4 bytes before the object pointer, and it is placed there using a uint32.
 When addressed as the uint32, you can use the bitmasks below to extract the various data.

 The "mark id" can conveniently be interpreted as a byte, however the offsets well
  be different depending on whether the system is little endian or big endian.


  Little endian - lsb first

 7   0 15 8 23 16 31 24
 -----------------------
 |    |    |     | MID  | obj start here .....
 -----------------------



  Big endian - msb first

 31 24 23 16 15 8 7   0
 -----------------------
 |MID |    |     |      | obj start here .....
 -----------------------

MID = HX_ENDIAN_MARK_ID_BYTE = is measured from the object pointer
      HX_ENDIAN_MARK_ID_BYTE_HEADER = is measured from the header pointer (4 bytes before object)


*/

#define HX_ENDIAN_MARK_ID_BYTE_HEADER (HX_ENDIAN_MARK_ID_BYTE + 4)


// Used by strings
// HX_GC_CONST_ALLOC_BIT  0x80000000

//#define HX_GC_REMEMBERED          0x40000000
#define IMMIX_ALLOC_MARK_ID         0x3f000000
//#define IMMIX_ALLOC_IS_CONTAINER  0x00800000
//#define IMMIX_ALLOC_IS_PINNED     not used at object level
//#define HX_GX_STRING_EXTENDED     0x00200000
//#define HX_GC_STRING_HASH         0x00100000
// size will shift-right IMMIX_ALLOC_SIZE_SHIFT (6).  Low two bits are 0
#define IMMIX_ALLOC_SIZE_MASK       0x000fff00
#define IMMIX_ALLOC_ROW_COUNT       0x000000ff

#define IMMIX_HEADER_PRESERVE       0x00f00000


#define IMMIX_OBJECT_HAS_MOVED 0x000000fe


// Bigger than this, and they go in the large object pool
#define IMMIX_LARGE_OBJ_SIZE 4000

#ifdef allocString
#undef allocString
#endif


void CriticalGCError(const char *inMessage)
{
   // Can't perfrom normal handling because it needs the GC system
   #ifdef ANDROID
   __android_log_print(ANDROID_LOG_ERROR, "HXCPP", "Critical Error: %s", inMessage);
   #elif defined(HX_WINRT)
      WINRT_LOG("HXCPP Critical Error: %s\n", inMessage);
   #else
   printf("Critical Error: %s\n", inMessage);
   #endif

   DebuggerTrap();
}





enum AllocType { allocNone, allocString, allocObject, allocMarked };

struct BlockDataInfo *gBlockStack = 0;
typedef hx::QuickVec<hx::Object *> ObjectStack;


typedef HxMutex ThreadPoolLock;

static ThreadPoolLock sThreadPoolLock;

#if !defined(HX_WINDOWS) && !defined(EMSCRIPTEN) && \
   !defined(__SNC__) && !defined(__ORBIS__)
#define HX_GC_PTHREADS
typedef pthread_cond_t ThreadPoolSignal;
inline void WaitThreadLocked(ThreadPoolSignal &ioSignal)
{
   pthread_cond_wait(&ioSignal, &sThreadPoolLock.mMutex);
}
#else
typedef HxSemaphore ThreadPoolSignal;
#endif

typedef TAutoLock<ThreadPoolLock> ThreadPoolAutoLock;

// For threaded marking/block reclaiming
static unsigned int sRunningThreads = 0;
static unsigned int sAllThreads = 0;
static bool sLazyThreads = false;
static bool sThreadPoolInit = false;

enum ThreadPoolJob
{
   tpjNone,
   tpjMark,
   tpjReclaim,
   tpjReclaimFull,
   tpjCountRows,
   tpjAsyncZero,
   tpjAsyncZeroJit,
   tpjGetStats,
   tpjVisitBlocks,
};

int sgThreadCount = 0;
static ThreadPoolJob sgThreadPoolJob = tpjNone;
static bool sgThreadPoolAbort = false;

// Pthreads enters the sleep state while holding a mutex, so it no cost to update
//  the sleeping state and thereby avoid over-signalling the condition
bool             sThreadSleeping[MAX_GC_THREADS];
ThreadPoolSignal sThreadWake[MAX_GC_THREADS];
bool             sThreadJobDoneSleeping = false;
ThreadPoolSignal sThreadJobDone;


static inline void SignalThreadPool(ThreadPoolSignal &ioSignal, bool sThreadSleeping)
{
   #ifdef HX_GC_PTHREADS
   if (sThreadSleeping)
      pthread_cond_signal(&ioSignal);
   #else
   ioSignal.Set();
   #endif
}

static void wakeThreadLocked(int inThreadId)
{
   sRunningThreads |= (1<<inThreadId);
   sLazyThreads = sRunningThreads != sAllThreads;
   SignalThreadPool(sThreadWake[inThreadId],sThreadSleeping[inThreadId]);
}

union BlockData
{
   // First 2/4 bytes are not needed for row markers (first 2/4 rows are for flags)
   BlockIdType mId;

   // First 2/4 rows contain a byte-flag-per-row 
   unsigned char  mRowMarked[IMMIX_LINES];
   // Row data as union - don't use first 2/4 rows
   unsigned char  mRow[IMMIX_LINES][IMMIX_LINE_LEN];

};

struct BlockDataStats
{
   void clear() { ZERO_MEM(this, sizeof(BlockDataStats)); }
   void add(const BlockDataStats &inOther)
   {
      rowsInUse += inOther.rowsInUse;
      bytesInUse += inOther.bytesInUse;
      emptyBlocks += inOther.emptyBlocks;
      fraggedBlocks += inOther.fraggedBlocks;
      fragScore += inOther.fragScore;
      fraggedRows += inOther.fraggedRows;
   }

   int rowsInUse;
   size_t bytesInUse;
   int emptyBlocks;
   int fragScore;
   int fraggedBlocks;
   int fraggedRows;
};

static BlockDataStats sThreadBlockDataStats[MAX_GC_THREADS];
#ifdef HXCPP_VISIT_ALLOCS
static hx::VisitContext *sThreadVisitContext = 0;
#endif
   


namespace hx { void MarkerReleaseWorkerLocked(); }




struct GroupInfo
{
   int  blocks;
   char *alloc;

   bool pinned;
   bool isEmpty;
   int  usedBytes;
   int  usedSpace;

   void clear()
   {
      pinned = false;
      usedBytes = 0;
      usedSpace = 0;
      isEmpty = true;
   }
   int getMoveScore()
   {
      return pinned ?  0 : usedSpace-usedBytes;
   }

};
 
hx::QuickVec<GroupInfo> gAllocGroups;



struct HoleRange
{
   unsigned short start;
   unsigned short length;
};


hx::QuickVec<struct BlockDataInfo *> *gBlockInfo = 0;
static int gBlockInfoEmptySlots = 0;

#define FRAG_THRESH 14

#define ZEROED_NOT    0
#define ZEROED_THREAD 1
#define ZEROED_AUTO   2

struct BlockDataInfo
{
   int             mId;
   int             mGroupId;
   BlockData       *mPtr;

   unsigned int allocStart[IMMIX_LINES];

   HoleRange    mRanges[MAX_HOLES];
   int          mHoles;

   int          mUsedRows;
   int          mMaxHoleSize;
   int          mMoveScore;
   int          mUsedBytes;
   int          mFraggedRows;
   bool         mPinned;
   unsigned char mZeroed;
   bool         mReclaimed;
   bool         mOwned;
   #ifdef HXCPP_GC_GENERATIONAL
   bool         mHasSurvivor;
   #endif
   volatile int mZeroLock;


   BlockDataInfo(int inGid, BlockData *inData)
   {
      if (gBlockInfoEmptySlots)
      {
         for(int i=0;i<gBlockInfo->size();i++)
            if ( !(*gBlockInfo)[i] )
            {
               gBlockInfoEmptySlots--;
               mId = i;
               (*gBlockInfo)[i] = this;
               break;
            }
      }
      else
      {
         if (gBlockInfo==0)
            gBlockInfo = new hx::QuickVec<BlockDataInfo *>;
         mId = gBlockInfo->size();
         gBlockInfo->push( this );
      }


      mZeroLock = 0;
      mOwned = false;
      mGroupId = inGid;
      mPtr     = inData;
      inData->mId = mId;
      #ifdef SHOW_MEM_EVENTS
      //GCLOG("  create block %d : %p -> %p\n",  mId, this, mPtr );
      #endif
      clear();
   }

   void clear()
   {
      mUsedRows = 0;
      mUsedBytes = 0;
      mFraggedRows = 0;
      mPinned = false;
      ZERO_MEM(allocStart,sizeof(int)*IMMIX_LINES);
      ZERO_MEM(mPtr->mRowMarked+IMMIX_HEADER_LINES, IMMIX_USEFUL_LINES); 
      mRanges[0].start = IMMIX_HEADER_LINES << IMMIX_LINE_BITS;
      mRanges[0].length = IMMIX_USEFUL_LINES << IMMIX_LINE_BITS;
      mMaxHoleSize = mRanges[0].length;
      mMoveScore = 0;
      mHoles = 1;
      mZeroed = ZEROED_NOT;
      mReclaimed = true;
      mZeroLock = 0;
      mOwned = false;
   }

   void makeFull()
   {
      mUsedRows = IMMIX_USEFUL_LINES;
      mUsedBytes = mUsedRows<<IMMIX_LINE_BITS;
      mFraggedRows = 0;
      memset(mPtr->mRowMarked+IMMIX_HEADER_LINES, 1,IMMIX_USEFUL_LINES); 
      mRanges[0].start = 0;
      mRanges[0].length = 0;
      mMaxHoleSize = 0;
      mMoveScore = 0;
      mHoles = 0;
      mZeroed = ZEROED_AUTO;
      mReclaimed = true;
      mZeroLock = 0;
      mOwned = false;
   }


   void clearBlockMarks()
   {
      mPinned = false;
      #ifdef HXCPP_GC_GENERATIONAL
      mHasSurvivor = false;
      #endif
   }

   void clearRowMarks()
   {
      clearBlockMarks();
      ZERO_MEM((char *)mPtr+IMMIX_HEADER_LINES, IMMIX_USEFUL_LINES);
   }

   inline int GetFreeRows() const { return (IMMIX_USEFUL_LINES - mUsedRows); }
   inline int GetFreeData() const { return (IMMIX_USEFUL_LINES - mUsedRows)<<IMMIX_LINE_BITS; }


   bool zeroAndUnlock()
   {
      bool doZero = !mZeroed;
      if (doZero)
      {
         if (!mReclaimed)
            reclaim<false>(0);

         for(int i=0;i<mHoles;i++)
             ZERO_MEM( (char *)mPtr+mRanges[i].start, mRanges[i].length );
         mZeroed = ZEROED_THREAD;
      }
      mZeroLock = 0;
      return doZero;
   }

   bool tryZero()
   {
      if (mZeroed)
         return false;

      if (_hx_atomic_compare_exchange(&mZeroLock, 0,1) == 0)
         return zeroAndUnlock();

      return false;
   }

   void destroy()
   {
      #ifdef SHOW_MEM_EVENTS_VERBOSE
      GCLOG("  release block %d : %p\n",  mId, this );
      #endif
      (*gBlockInfo)[mId] = 0;
      gBlockInfoEmptySlots++;
      delete this;
   }

   bool isEmpty() const { return mUsedRows == 0; }
   int getUsedRows() const { return mUsedRows; }

   void getStats(BlockDataStats &outStats)
   {
      outStats.rowsInUse += mUsedRows;
      outStats.bytesInUse += mUsedBytes;
      outStats.fraggedRows += mFraggedRows;
      //outStats.bytesInUse += 0;
      outStats.fragScore += mPinned ? (mMoveScore>0?1:0) : mMoveScore;

      if (mUsedRows==0)
         outStats.emptyBlocks++;
      if (mMoveScore> FRAG_THRESH )
         outStats.fraggedBlocks++;
   }

   #ifdef HX_GC_VERIFY_ALLOC_START
   void verifyAllocStart()
   {
      unsigned char *rowMarked = mPtr->mRowMarked;

      for(int r = IMMIX_HEADER_LINES; r<IMMIX_LINES; r++)
      {
         if (!rowMarked[r] && allocStart[r])
         {
            printf("allocStart set without marking\n");
            DebuggerTrap();
         }
      }
   }
   #endif

   void countRows(BlockDataStats &outStats)
   {
      unsigned char *rowMarked = mPtr->mRowMarked;
      unsigned int *rowTotals = ((unsigned int *)rowMarked) + 1;

      // TODO - sse/neon
      #ifdef HXCPP_GC_BIG_BLOCKS
      unsigned int total = 0;
      #else
      unsigned int total = rowMarked[2] + rowMarked[3];
      #endif

      total +=
       rowTotals[0]  + rowTotals[1]  + rowTotals[2]  + rowTotals[3]  + rowTotals[4] +
       rowTotals[5]  + rowTotals[6]  + rowTotals[7]  + rowTotals[8]  + rowTotals[9] +
       rowTotals[10] + rowTotals[11] + rowTotals[12] + rowTotals[13] + rowTotals[14] +
       rowTotals[15] + rowTotals[16] + rowTotals[17] + rowTotals[18] + rowTotals[19] +
       rowTotals[20] + rowTotals[21] + rowTotals[22] + rowTotals[23] + rowTotals[24] +
       rowTotals[25] + rowTotals[26] + rowTotals[27] + rowTotals[28] + rowTotals[29] +
       rowTotals[30] + rowTotals[31] + rowTotals[32] + rowTotals[33] + rowTotals[34] +
       rowTotals[35] + rowTotals[36] + rowTotals[37] + rowTotals[38] + rowTotals[39] +
       rowTotals[40] + rowTotals[41] + rowTotals[42] + rowTotals[43] + rowTotals[44] +
       rowTotals[45] + rowTotals[46] + rowTotals[47] + rowTotals[48] + rowTotals[49] +
       rowTotals[50] + rowTotals[51] + rowTotals[52] + rowTotals[53] + rowTotals[54] +
       rowTotals[55] + rowTotals[56] + rowTotals[57] + rowTotals[58] + rowTotals[59] +
       rowTotals[60] + rowTotals[61] + rowTotals[62];


      #ifdef HXCPP_GC_BIG_BLOCKS
      rowTotals += 63;
      total +=
       rowTotals[0]  + rowTotals[1]  + rowTotals[2]  + rowTotals[3]  + rowTotals[4] +
       rowTotals[5]  + rowTotals[6]  + rowTotals[7]  + rowTotals[8]  + rowTotals[9] +
       rowTotals[10] + rowTotals[11] + rowTotals[12] + rowTotals[13] + rowTotals[14] +
       rowTotals[15] + rowTotals[16] + rowTotals[17] + rowTotals[18] + rowTotals[19] +
       rowTotals[20] + rowTotals[21] + rowTotals[22] + rowTotals[23] + rowTotals[24] +
       rowTotals[25] + rowTotals[26] + rowTotals[27] + rowTotals[28] + rowTotals[29] +
       rowTotals[30] + rowTotals[31] + rowTotals[32] + rowTotals[33] + rowTotals[34] +
       rowTotals[35] + rowTotals[36] + rowTotals[37] + rowTotals[38] + rowTotals[39] +
       rowTotals[40] + rowTotals[41] + rowTotals[42] + rowTotals[43] + rowTotals[44] +
       rowTotals[45] + rowTotals[46] + rowTotals[47] + rowTotals[48] + rowTotals[49] +
       rowTotals[50] + rowTotals[51] + rowTotals[52] + rowTotals[53] + rowTotals[54] +
       rowTotals[55] + rowTotals[56] + rowTotals[57] + rowTotals[58] + rowTotals[59] +
       rowTotals[60] + rowTotals[61] + rowTotals[62] + rowTotals[63];

      #endif

      mUsedRows = (total & 0xff) + ((total>>8) & 0xff) + ((total>>16)&0xff) + ((total>>24)&0xff);
      mUsedBytes = mUsedRows<<IMMIX_LINE_BITS;

      mZeroLock = 0;
      mOwned = false;
      outStats.rowsInUse += mUsedRows;
      outStats.bytesInUse += mUsedBytes;
      outStats.fraggedRows += mFraggedRows;
      mFraggedRows = 0;
      mHoles = 0;

      if (mUsedRows==IMMIX_USEFUL_LINES)
      {
         // All rows used - write the block off
         mMoveScore = 0;
         mZeroed = ZEROED_AUTO;
         mReclaimed = true;
      }
      else
      {
         mZeroed = ZEROED_NOT;
         mReclaimed = false;
      }

      int left = (IMMIX_USEFUL_LINES - mUsedRows) << IMMIX_LINE_BITS;
      if (left<mMaxHoleSize)
         mMaxHoleSize = left;
   }

   template<bool FULL>
   void reclaim(BlockDataStats *outStats)
   {
      HoleRange *ranges = mRanges;
      ranges[0].length = 0;
      mZeroed = ZEROED_NOT;
      int usedBytes = 0;

      unsigned char *rowMarked = mPtr->mRowMarked;

      int r = IMMIX_HEADER_LINES;
      // Count unused rows ....
     
      // start on 4-byte boundary...
      #ifdef HXCPP_ALIGN_ALLOC
      while(r<4 && rowMarked[r]==0)
         r++;
      if (!rowMarked[r])
      #endif
      {
         while(r<(IMMIX_LINES-4) && *(int *)(rowMarked+r)==0 )
            r += 4;
         while(r<(IMMIX_LINES) && rowMarked[r]==0)
            r++;
      }

      if (r==IMMIX_LINES)
      {
         ranges[0].start  = IMMIX_HEADER_LINES<<IMMIX_LINE_BITS;
         ranges[0].length = (IMMIX_USEFUL_LINES)<<IMMIX_LINE_BITS;
         mMaxHoleSize = (IMMIX_USEFUL_LINES)<<IMMIX_LINE_BITS;
         mUsedRows = 0;
         mHoles = 1;
         mMoveScore = 0;
         ZERO_MEM(allocStart+IMMIX_HEADER_LINES, IMMIX_USEFUL_LINES*sizeof(int));
      }
      else
      {
         int hole = 0;

         if (r>IMMIX_HEADER_LINES)
         {
            ranges[hole].start  = IMMIX_HEADER_LINES;
            ranges[hole].length = r-IMMIX_HEADER_LINES;
            hole++;
         }

         while(r<IMMIX_LINES)
         {
            if (rowMarked[r])
            {
               if (FULL)
               {
                  unsigned int &starts = allocStart[r];
                  if (starts)
                  {
                     unsigned int *headerPtr = ((unsigned int *)mPtr->mRow[r]);
                     #define CHECK_FLAG(i,byteMask) \
                     { \
                        unsigned int mask = 1<<i; \
                        if ( starts & mask ) \
                        { \
                           unsigned int header = headerPtr[i]; \
                           if ( (header & IMMIX_ALLOC_MARK_ID) != hx::gMarkID ) \
                           { \
                              starts ^= mask; \
                              if (!(starts & byteMask)) \
                                 break; \
                           } \
                           else \
                              usedBytes += sizeof(int) + ((header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT); \
                        } \
                     }

                     if (starts & 0x000000ff)
                        for(int i=0;i<8;i++)
                           CHECK_FLAG(i,0x000000ff);

                     if (starts & 0x0000ff00)
                        for(int i=8;i<16;i++)
                           CHECK_FLAG(i,0x0000ff00);

                     if (starts & 0x00ff0000)
                        for(int i=16;i<24;i++)
                           CHECK_FLAG(i,0x00ff0000);

                     if (starts & 0xff000000)
                        for(int i=24;i<32;i++)
                           CHECK_FLAG(i,0xff000000);
                  }
               }
               r++;
            }
            else
            {
               int start = r;
               ranges[hole].start = start;

               #ifdef HXCPP_ALIGN_ALLOC
               int alignR = (r+3) & ~3;
               while(r<alignR && rowMarked[r]==0)
                  r++;
               if (!rowMarked[r])
               #endif
               {
                  while(r<(IMMIX_LINES-4) && *(int *)(rowMarked+r)==0 )
                     r += 4;
                  while(r<(IMMIX_LINES) && rowMarked[r]==0)
                     r++;
               }
               ranges[hole].length = r-start;
               hole++;
            }
         }


         int freeLines = 0;
         // Convert hole rows to hole bytes...
         mMaxHoleSize = 0;
         for(int h=0;h<hole;h++)
         {
            int s = ranges->start;
            int l = ranges->length;
            freeLines += l;
            ZERO_MEM(allocStart+s, l*sizeof(int));

            int sBytes = s<<IMMIX_LINE_BITS;
            ranges->start = sBytes;

            int lBytes = l<<IMMIX_LINE_BITS;
            ranges->length = lBytes;

            if (lBytes>mMaxHoleSize)
              mMaxHoleSize = lBytes;

            ranges++;
         }
         mUsedRows = IMMIX_USEFUL_LINES - freeLines;
         mHoles = hole;
      }

      mUsedBytes =  FULL ? usedBytes : (mUsedRows<<IMMIX_LINE_BITS);
      mMoveScore = calcFragScore();
      mReclaimed = true;

      if (outStats)
      {
         outStats->rowsInUse += mUsedRows;
         outStats->bytesInUse += mUsedBytes;
         outStats->fragScore += mMoveScore;
         outStats->fraggedRows += mFraggedRows;

         if (mUsedRows==0)
            outStats->emptyBlocks++;
         if (mMoveScore> FRAG_THRESH )
            outStats->fraggedBlocks++;
      }

      mFraggedRows = 0;
   }

   int calcFragScore()
   {
      return mPinned ? 0 : (mHoles>3 ? mHoles-3 : 0) + 8 * (mUsedRows<<IMMIX_LINE_BITS) / (mUsedBytes+IMMIX_LINE_LEN);
   }


   // When known to be an actual object start...
   AllocType GetAllocTypeChecked(int inOffset, bool allowPrevious)
   {
      char time = mPtr->mRow[0][inOffset+HX_ENDIAN_MARK_ID_BYTE_HEADER];
      if ( ((time+1) & MARK_BYTE_MASK) != (gByteMarkID & MARK_BYTE_MASK)  )
      {
         // Object is either out-of-date, or already marked....
         return time==gByteMarkID ? allocMarked : allocNone;
      }

      if (!allowPrevious)
         return allocNone;

      if (*(unsigned int *)(mPtr->mRow[0] + inOffset) & IMMIX_ALLOC_IS_CONTAINER)
      {
         // See if object::new has been called, but not constructed yet ...
         void **vtable = (void **)(mPtr->mRow[0] + inOffset + sizeof(int));
         if (vtable[0]==0)
         {
            // GCLOG("Partially constructed object.");
            return allocString;
         }
         return allocObject;
      }

      return allocString;
   }

   #ifdef HXCPP_GC_NURSERY
   AllocType GetEnclosingNurseryType(int inOffset, void **outPtr)
   {
      // The block did not get used in the previous cycle, so allocStart is invalid and
      //  no new objects should be in here
      if (!mZeroed)
         return allocNone;
      // For the nursery(generational) case, the allocStart markers are not set
      // So trace tne new object links through the new allocation holes
      for(int h=0;h<mHoles;h++)
      {
         int scan = mRanges[h].start;
         if (inOffset<scan)
            break;

         int size = 0;
         int last = scan + mRanges[h].length;
         if (inOffset<last)
         {
            // Found hole that the object was possibly allocated in
            while(scan<=inOffset)
            {
               // Trace along the hole...
               unsigned int header = *(unsigned int *)(mPtr->mRow[0]+scan);
               if (!(header & 0xff000000))
                  size = header & 0x0000ffff;
               else
                  size = (header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT;

               int end = scan+size+sizeof(int);
               if (!size || end > last)
                  return allocNone;

               if (inOffset>=scan && inOffset<end)
               {
                  if (inOffset>scan+sgCheckInternalOffset)
                     return allocNone;

                  *outPtr = mPtr->mRow[0] + scan + sizeof(int);

                  if (header & IMMIX_ALLOC_IS_CONTAINER)
                  {
                     // See if object::new has been called, but not constructed yet ...
                     void **vtable = (void **)(mPtr->mRow[0] + scan + sizeof(int));
                     if (vtable[0]==0)
                     {
                        // GCLOG("Partially constructed object.");
                        return allocString;
                     }
                     return allocObject;
                  }
                  return allocString;
               }
               scan = end;
            }
            break;
         }
      }
      return allocNone;
}
   #endif

   AllocType GetAllocType(int inOffset,bool inAllowPrevious)
   {
      // Row that the header would be on
      int r = inOffset >> IMMIX_LINE_BITS;

      // Out of bounds - can't be a new object start
      if (r < IMMIX_HEADER_LINES || r >= IMMIX_LINES)
      {
         return allocNone;
      }

      // Does a live object start on this row
      if ( !( allocStart[r] & hx::gImmixStartFlag[inOffset &127]) )
      {
         #ifdef HXCPP_GC_NURSERY
         void *ptr;
         return GetEnclosingNurseryType(inOffset,&ptr);
         #endif
         //Not a actual start...
         return allocNone;
      }

      return GetAllocTypeChecked(inOffset,inAllowPrevious);
   }

   AllocType GetEnclosingAllocType(int inOffset,void **outPtr,bool inAllowPrevious)
   {
      for(int dx=0;dx<=sgCheckInternalOffset;dx+=4)
      {
         int blockOffset = inOffset - dx;
         if (blockOffset >= 0)
         {
            int r = blockOffset >> IMMIX_LINE_BITS;
            if (r >= IMMIX_HEADER_LINES && r < IMMIX_LINES)
            {
               // Normal, good alloc
               int rowPos = hx::gImmixStartFlag[blockOffset &127];
               if ( allocStart[r] & rowPos )
               {
                  // Found last valid object - is it big enough?
                  unsigned int header =  *(unsigned int *)((char *)mPtr + blockOffset);
                  int size = (header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT;
                  // Valid object not big enough...
                  if (blockOffset + size +sizeof(int) <= inOffset )
                     break;

                  // If the object is old, it could be the tail end of an old row that
                  //  thinks is is covering this row, but it is not because this row is reused.
                  // So we can say there is no other object that could be covering
                  //  this spot, but not that it is not a nursery object.
                  AllocType result = GetAllocTypeChecked(blockOffset,inAllowPrevious);
                  if (result!=allocNone)
                  {
                     *outPtr = (void *)(mPtr->mRow[0] + blockOffset + sizeof(int));
                     return result;
                  }
                  break;
               }
            }
          }
      }

      #ifdef HXCPP_GC_NURSERY
      return GetEnclosingNurseryType(inOffset,outPtr);
      #endif
      // Not a actual start...
      return allocNone;
   }


   void pin() { mPinned = true; }


   #ifdef HXCPP_VISIT_ALLOCS
   void VisitBlock(hx::VisitContext *inCtx)
   {
      if (isEmpty())
         return;

      unsigned char *rowMarked = mPtr->mRowMarked;
      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         if (rowMarked[r])
         {
            unsigned int starts = allocStart[r];
            if (!starts)
               continue;
            unsigned char *row = mPtr->mRow[r];
            for(int i=0;i<32;i++)
            {
               int pos = i<<2;
               if ( (starts & (1<<i)) &&
                   (row[pos+HX_ENDIAN_MARK_ID_BYTE_HEADER]&FULL_MARK_BYTE_MASK) == gByteMarkID)
                  {
                     if ( (*(unsigned int *)(row+pos)) & IMMIX_ALLOC_IS_CONTAINER )
                     {
                        hx::Object *obj = (hx::Object *)(row+pos+4);
                        // May be partially constructed
                        if (*(void **)obj)
                           obj->__Visit(inCtx);
                     }
                  }
            }
         }
      }
   }
   #endif

   #ifdef HX_GC_VERIFY
   void verify(const char *inWhere)
   {
      for(int i=IMMIX_HEADER_LINES;i<IMMIX_LINES;i++)
      {
         for(int j=0;j<32;j++)
            if (allocStart[i] & (1<<j))
            {
               unsigned int header = *(unsigned int *)( (char *)mPtr + i*IMMIX_LINE_LEN + j*4 );
               if ((header & IMMIX_ALLOC_MARK_ID) == hx::gMarkID )
               {
                  int rows = header & IMMIX_ALLOC_ROW_COUNT;
                  if (rows==0)
                  {
                     printf("BAD ROW0 %s\n", inWhere);
                     DebuggerTrap();
                  }
                  for(int r=0;r<rows;r++)
                     if (!mPtr->mRowMarked[r+i])
                     {
                        printf("Unmarked row %dx %d/%d %s, t=%d!\n", i, r, rows, inWhere,sgTimeToNextTableUpdate);
                        DebuggerTrap();
                     }
               }
            }
         }

   }
   #endif
};





bool MostUsedFirst(BlockDataInfo *inA, BlockDataInfo *inB)
{
   return inA->getUsedRows() > inB->getUsedRows();
}

bool BiggestFreeFirst(BlockDataInfo *inA, BlockDataInfo *inB)
{
   return inA->mMaxHoleSize > inB->mMaxHoleSize;
}
bool SmallestFreeFirst(BlockDataInfo *inA, BlockDataInfo *inB)
{
   return inA->mMaxHoleSize < inB->mMaxHoleSize;
}


bool LeastUsedFirst(BlockDataInfo *inA, BlockDataInfo *inB)
{
   return inA->getUsedRows() < inB->getUsedRows();
}



bool SortMoveOrder(BlockDataInfo *inA, BlockDataInfo *inB)
{
   return inA->mMoveScore > inB->mMoveScore;
}


namespace hx
{



void BadImmixAlloc()
{

   #ifdef HX_WINRT
   WINRT_LOG("Bad local allocator - requesting memory from unregistered thread!");
   #else
   #ifdef ANDROID
   __android_log_print(ANDROID_LOG_ERROR, "hxcpp",
   #else
   fprintf(stderr,
   #endif

   "Bad local allocator - requesting memory from unregistered thread!"

   #ifdef ANDROID
   );
   #else
   );
   #endif
   #endif

   DebuggerTrap();
}



void GCCheckPointer(void *inPtr)
{
   #ifdef HXCPP_GC_DEBUG_ALWAYS_MOVE
   if (hx::sgPointerMoved.find(inPtr)!=hx::sgPointerMoved.end())
   {
      GCLOG("Accessing moved pointer %p\n", inPtr);
      DebuggerTrap();
   }
   #endif

   unsigned char&mark = ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE];
   #ifdef HXCPP_GC_NURSERY
   if (mark)
   #endif
   if ( !(mark & HX_GC_CONST_ALLOC_MARK_BIT) && (mark&FULL_MARK_BYTE_MASK)!=gByteMarkID  )
   {
      GCLOG("Old object access %p\n", inPtr);
      NullReference("Object", false);
   }
}


void GCOnNewPointer(void *inPtr)
{
   #ifdef HXCPP_GC_DEBUG_ALWAYS_MOVE
   hx::sgPointerMoved.erase(inPtr);
   _hx_atomic_add(&sgAllocsSinceLastSpam, 1);
   #endif
}

// --- Marking ------------------------------------

struct MarkInfo
{
   const char *mClass;
   const char *mMember;
};

struct GlobalChunks
{
   volatile MarkChunk *processList;
   volatile int       processListPopLock;
   volatile MarkChunk *freeList;
   volatile int       freeListPopLock;

   GlobalChunks()
   {
      processList = 0;
      freeList = 0;
      freeListPopLock = 0;
      processListPopLock = 0;
   }

   MarkChunk *pushJobNoWake(MarkChunk *inChunk)
   {
      while(true)
      {
         MarkChunk *head = (MarkChunk *)processList;
         inChunk->next = head;
         if (_hx_atomic_compare_exchange_cast_ptr(&processList, head, inChunk) == head)
            break;
      }

      return alloc();
   }

   MarkChunk *pushJob(MarkChunk *inChunk,bool inAndAlloc)
   {
      while(true)
      {
         MarkChunk *head = (MarkChunk *)processList;
         inChunk->next = head;
         if (_hx_atomic_compare_exchange_cast_ptr(&processList, head, inChunk) == head)
            break;
      }

      #ifdef PROFILE_THREAD_USAGE
      _hx_atomic_add(&sThreadChunkPushCount, 1);
      #endif

      if (MAX_GC_THREADS>1 && sLazyThreads)
      {
         ThreadPoolAutoLock l(sThreadPoolLock);

         #ifdef PROFILE_THREAD_USAGE
           #define CHECK_THREAD_WAKE(tid) \
            if (MAX_GC_THREADS >tid && sgThreadCount>tid && (!(sRunningThreads & (1<<tid)))) { \
            wakeThreadLocked(tid); \
            sThreadChunkWakes++; \
           } 
         #else
           #define CHECK_THREAD_WAKE(tid)  \
            if (MAX_GC_THREADS >tid && sgThreadCount>tid && (!(sRunningThreads & (1<<tid)))) { \
            wakeThreadLocked(tid); \
           }
         #endif


         CHECK_THREAD_WAKE(0)
         else CHECK_THREAD_WAKE(1)
         else CHECK_THREAD_WAKE(2)
         else CHECK_THREAD_WAKE(3)
         else CHECK_THREAD_WAKE(4)
         else CHECK_THREAD_WAKE(5)
         else CHECK_THREAD_WAKE(6)
         else CHECK_THREAD_WAKE(7)
      }

      if (inAndAlloc)
         return alloc();
      return 0;
   }

   void addLocked(MarkChunk *inChunk)
   {
      inChunk->next = (MarkChunk *)processList;
      processList = (volatile MarkChunk *)inChunk;
   }

   void copyPointers( QuickVec<hx::Object *> &outPointers,bool andFree=false)
   {
      int size = 0;
      for(MarkChunk *c =(MarkChunk *)processList; c; c=c->next )
         size += c->count;

      outPointers.setSize(size);
      int idx = 0;
      if (andFree)
      {
         while(processList)
         {
            MarkChunk *c = (MarkChunk *)processList;
            processList = c->next;

            for(int i=0;i<c->count;i++)
               outPointers[idx++] = c->stack[i];
            c->count = 0;
            c->next = (MarkChunk *)freeList;
            freeList = c;
         }
      }
      else
      {
         for(MarkChunk *c = (MarkChunk *)processList; c; c=c->next )
         {
            for(int i=0;i<c->count;i++)
               outPointers[idx++] = c->stack[i];
         }
      }
   }

   int takeArrayJob(hx::Object **inPtr, int inLen)
   {
      if (sLazyThreads)
      {
         int n = (inLen/2) & ~15;

         if (n)
         {
            MarkChunk *chunk = alloc();
            chunk->count = MarkChunk::OBJ_ARRAY_JOB;
            chunk->arrayBase = inPtr;
            chunk->arrayElements = n;

            pushJob(chunk,false);
         }

         // Return how many to skip
         return n;
      }

      return 0;
   }

   inline void release(MarkChunk *inChunk)
   {
      while(true)
      {
         MarkChunk *head = (MarkChunk *)freeList;
         inChunk->next = head;
         if (_hx_atomic_compare_exchange_cast_ptr(&freeList, head, inChunk) == head)
            return;
      }
   }


   MarkChunk *popJobLocked(MarkChunk *inChunk)
   {
      if (inChunk)
         release(inChunk);

      while(_hx_atomic_compare_exchange(&processListPopLock, 0, 1) != 0)
      {
         // Spin
         #ifdef PROFILE_THREAD_USAGE
         _hx_atomic_add(&sSpinCount, 1);
         #endif
      }

      while(true)
      {
         MarkChunk *head = (MarkChunk *)processList;
         if (!head)
         {
            processListPopLock = 0;
            return 0;
         }
         MarkChunk *next = head->next;
         if (_hx_atomic_compare_exchange_cast_ptr(&processList, head, next) == head)
         {
            processListPopLock = 0;

            head->next = 0;
            return head;
         }
      }
      return 0;
   }


   void completeThreadLocked(int inThreadId)
   {
      if (!(sRunningThreads & (1<<inThreadId)))
      {
         printf("Complete non-running thread?\n");
         DebuggerTrap();
      }
      sRunningThreads &= ~(1<<inThreadId);
      sLazyThreads = sRunningThreads != sAllThreads;

      if (!sRunningThreads)
         SignalThreadPool(sThreadJobDone,sThreadJobDoneSleeping);
   }


   // Optionally returns inChunk to empty pool (while we have the lock),
   //  and returns a new job if there is one
   MarkChunk *popJobOrFinish(MarkChunk *inChunk,int inThreadId)
   {
      #ifdef HX_MULTI_THREAD_MARKING
      if (sAllThreads)
      {
         MarkChunk *result =  popJobLocked(inChunk);
         if (!result)
         {
            for(int spinCount = 0; spinCount<10000; spinCount++)
            {
               if ( sgThreadPoolAbort || sAllThreads == (1<<inThreadId) )
                  break;
               if (processList)
               {
                  result =  popJobLocked(0);
                  if (result)
                     return result;
               }
            }
            ThreadPoolAutoLock l(sThreadPoolLock);
            completeThreadLocked(inThreadId);
         }
         return result;
      }
      #endif

      return popJobLocked(inChunk);
   }


   void free(MarkChunk *inChunk)
   {
      release(inChunk);
   }

   inline MarkChunk *alloc()
   {
      while(_hx_atomic_compare_exchange(&freeListPopLock, 0, 1) != 0)
      {
         // Spin
         #ifdef PROFILE_THREAD_USAGE
         _hx_atomic_add(&sSpinCount, 1);
         #endif
      }

      while(true)
      {
         MarkChunk *head = (MarkChunk *)freeList;
         if (!head)
         {
            freeListPopLock = 0;
            return new MarkChunk;
         }
         MarkChunk *next = head->next;
         if (_hx_atomic_compare_exchange_cast_ptr(&freeList, head, next) == head)
         {
            freeListPopLock = 0;

            head->next = 0;
            return head;
         }
      }
   }




   MarkChunk *getInitJob()
   {
      MarkChunk *result = popJobLocked(0);
      if (result)
      {
         if (result->count==MarkChunk::OBJ_ARRAY_JOB)
         {
            GCLOG("Popped array job?\n");
            pushJob(result,false);
         }
         else
            return result;
      }
      return alloc();
   }

};

GlobalChunks sGlobalChunks;

class MarkContext
{
    #ifdef HXCPP_DEBUG
    MarkInfo  *mInfo;
    int       mPos;
    #endif

    int       mThreadId;
    MarkChunk *marking;


public:
    enum { StackSize = 8192 };

    bool isGenerational;

    MarkContext(int inThreadId = -1)
    {
       #ifdef HXCPP_DEBUG
       mInfo = new MarkInfo[StackSize];
       mPos = 0;
       #endif
       mThreadId = inThreadId;
       marking = sGlobalChunks.alloc();

       isGenerational = false;
    }
    ~MarkContext()
    {
       if (marking) sGlobalChunks.free(marking);
       #ifdef HXCPP_DEBUG
       delete [] mInfo;
       #endif
       // TODO: Free slabs
    }


    #ifdef HXCPP_DEBUG
    void PushClass(const char *inClass)
    {
       if (mPos<StackSize-1)
       {
          mInfo[mPos].mClass = inClass;
          mInfo[mPos].mMember = 0;
       }
       mPos++;
    }


    void SetMember(const char *inMember)
    {
       // Should not happen...
       if (mPos==0)
          return;
       if (mPos<StackSize)
          mInfo[mPos-1].mMember = inMember ? inMember : "Unknown";
    }
    void PopClass() { mPos--; }

    void Trace()
    {
       int n = mPos < StackSize ? mPos : StackSize;
       #ifdef ANDROID
       __android_log_print(ANDROID_LOG_ERROR, "trace", "Class referenced from");
       #else
       printf("Class referenced from:\n");
       #endif

       for(int i=0;i<n;i++)
          #ifdef ANDROID
          __android_log_print(ANDROID_LOG_INFO, "trace", "%s.%s",  mInfo[i].mClass, mInfo[i].mMember );
          #else
          printf("%s.%s\n",  mInfo[i].mClass, mInfo[i].mMember );
          #endif

       if (mPos>=StackSize)
       {
          #ifdef ANDROID
          __android_log_print(ANDROID_LOG_INFO, "trace", "... + deeper");
          #else
          printf("... + deeper\n");
          #endif
       }
    }
    #endif


    void pushObj(hx::Object *inObject)
    {
       if (marking->count < MarkChunk::SIZE)
       {
          marking->push(inObject);
       }
       else
       {
          marking = sGlobalChunks.pushJob(marking,true);
          marking->push(inObject);
       }
    }

    void init()
    {
       if (!marking)
          marking = sGlobalChunks.getInitJob();
    }

    void releaseJobs()
    {
       if (marking && marking->count)
       {
          sGlobalChunks.pushJob(marking,false);
       }
       else if (marking)
       {
          sGlobalChunks.release(marking);
       }
       marking = 0;
    }

    void processMarkStack()
    {
       while(true)
       {
          if (!marking || !marking->count)
          {
             #ifdef HX_MULTI_THREAD_MARKING
             if (sgThreadPoolAbort)
             {
                releaseJobs();
                return;
             }
             #endif

             marking = sGlobalChunks.popJobOrFinish(marking,mThreadId);
             if (!marking)
                break;

             if (marking->count==MarkChunk::OBJ_ARRAY_JOB)
             {
                int n = marking->arrayElements;
                hx::Object **elems = marking->arrayBase;
                marking->count = 0;
                MarkObjectArray(elems, n, this);
                continue;
             }
          }

          while(marking)
          {
             hx::Object *obj = marking->pop();
             if (obj)
             {
                obj->__Mark(this);
                #if HX_MULTI_THREAD_MARKING
                // Load balance
                if (sLazyThreads && marking->count>32)
                {
                   MarkChunk *c = sGlobalChunks.alloc();
                   marking->count -= 16;
                   c->count = 16;
                   memcpy( c->stack, marking->stack + marking->count, 16*sizeof(hx::Object *));
                   sGlobalChunks.pushJob(c,false);
                }
                #ifdef PROFILE_THREAD_USAGE
                sThreadMarkCount[mThreadId]++;
                #endif
                #endif
             }
             else
                break;
          }
       }
    }
};


/*
void MarkerReleaseWorkerLocked( )
{
   //printf("Release...\n");
   for(int i=0;i<sAllThreads;i++)
   {
      if ( ! (sRunningThreads & (1<<i) ) )
      {
         //printf("Wake %d\n",i);
         sRunningThreads |= (1<<i);
         sThreadWake[i]->Set();
         return;
      }
   }
}
*/



#ifdef HXCPP_DEBUG
static bool breakOnce = 1;
void MarkSetMember(const char *inName,hx::MarkContext *__inCtx)
{
   if (gCollectTrace)
      __inCtx->SetMember(inName);
}

void MarkPushClass(const char *inName,hx::MarkContext *__inCtx)
{
   if (gCollectTrace)
      __inCtx->PushClass(inName);
}

void MarkPopClass(hx::MarkContext *__inCtx)
{
   if (gCollectTrace)
      __inCtx->PopClass();
}
#endif




struct AutoMarkPush
{
   hx::MarkContext *mCtx;
   AutoMarkPush(hx::MarkContext *ctx, const char *cls, const char *member)
   {
      #ifdef HXCPP_DEBUG
      mCtx = ctx;
      MarkPushClass(cls,mCtx);
      MarkSetMember(member,mCtx);
      #endif
   }
   ~AutoMarkPush()
   {
      #ifdef HXCPP_DEBUG
      MarkPopClass(mCtx);
      #endif
   }
};






void MarkAllocUnchecked(void *inPtr,hx::MarkContext *__inCtx)
{
   #ifdef PROFILE_COLLECT
   sAllocMarks++;
   #endif

   size_t ptr_i = ((size_t)inPtr)-sizeof(int);
   unsigned int flags =  *((unsigned int *)ptr_i);

   #ifdef HXCPP_GC_NURSERY
   if (!(flags & 0xff000000))
   {
      #ifdef HX_GC_VERIFY_GENERATIONAL
      if (sGcVerifyGenerational)
      {
         printf("Nursery alloc escaped generational collection %p\n", inPtr);
         DebuggerTrap();
      }
      #endif

      int size = flags & 0xffff;
      // Size will be 0 for large allocs -> no need to mark block
      if (size)
      {
         int start = (int)(ptr_i & IMMIX_BLOCK_OFFSET_MASK);
         int startRow = start>>IMMIX_LINE_BITS;
         int blockId = *(BlockIdType *)(ptr_i & IMMIX_BLOCK_BASE_MASK);
         BlockDataInfo *info = (*gBlockInfo)[blockId];

         int endRow = (start + size + sizeof(int) + IMMIX_LINE_LEN-1)>>IMMIX_LINE_BITS;
         *(unsigned int *)ptr_i = flags = (flags & IMMIX_HEADER_PRESERVE) |
                                          (endRow -startRow) |
                                          (size<<IMMIX_ALLOC_SIZE_SHIFT) |
                                          gMarkID;

         unsigned int *pos = info->allocStart + startRow;
         unsigned int val = *pos;
         while(_hx_atomic_compare_exchange((volatile int *)pos, val,val|gImmixStartFlag[start&127]) != val)
            val = *pos;

         #ifdef HXCPP_GC_GENERATIONAL
         info->mHasSurvivor = true;
         #endif
      }
      else
      {
         // Large nursury object
         ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE] = gByteMarkID;
      }
   }
   else
   #endif
   {
      #ifdef HX_GC_VERIFY_GENERATIONAL
      if (sGcVerifyGenerational && ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE] != gPrevByteMarkID)
      {
         printf("Alloc missed int generational collection %p\n", inPtr);
         DebuggerTrap();
      }
      #endif
      ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE] = gByteMarkID;
   }

   int rows = flags & IMMIX_ALLOC_ROW_COUNT;
   if (rows)
   {
      #if HXCPP_GC_DEBUG_LEVEL>0
      if ( ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS) + rows > IMMIX_LINES) DebuggerTrap();
      #endif

      char *block = (char *)(ptr_i & IMMIX_BLOCK_BASE_MASK);
      char *rowMark = block + ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS);
      *rowMark = 1;
      if (rows>1)
      {
         rowMark[1] = 1;
         if (rows>2)
         {
            rowMark[2] = 1;
            if (rows>3)
            {
               rowMark[3] = 1;
               for(int r=4; r<rows; r++)
                  rowMark[r]=1;
            }
         }
      }

   // MARK_ROWS_UNCHECKED_END
   }
}

void MarkObjectAllocUnchecked(hx::Object *inPtr,hx::MarkContext *__inCtx)
{
   size_t ptr_i = ((size_t)inPtr)-sizeof(int);
   unsigned int flags =  *((unsigned int *)ptr_i);
   #ifdef HXCPP_GC_NURSERY
   if (!(flags & 0xff000000))
   {
      #if defined(HX_GC_VERIFY_GENERATIONAL)
         if (sGcVerifyGenerational)
         {
            printf("Nursery object escaped generational collection %p\n", inPtr);
            DebuggerTrap();
         }
      #endif


      int size = flags & 0xffff;
      int start = (int)(ptr_i & IMMIX_BLOCK_OFFSET_MASK);
      int startRow = start>>IMMIX_LINE_BITS;
      int blockId = *(BlockIdType *)(ptr_i & IMMIX_BLOCK_BASE_MASK);
      BlockDataInfo *info = (*gBlockInfo)[blockId];

      int endRow = (start + size + sizeof(int) + IMMIX_LINE_LEN-1)>>IMMIX_LINE_BITS;
      *(unsigned int *)ptr_i = flags = (flags & IMMIX_HEADER_PRESERVE) |
                                       (endRow -startRow) |
                                       (size<<IMMIX_ALLOC_SIZE_SHIFT) |
                                       gMarkID;

      unsigned int *pos = info->allocStart + startRow;
      unsigned int val = *pos;
      while(_hx_atomic_compare_exchange( (volatile int *)pos, val, val|gImmixStartFlag[start&127]) != val)
         val = *pos;
      #ifdef HXCPP_GC_GENERATIONAL
      info->mHasSurvivor = true;
      #endif
   }
   else
   #endif
      ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE] = gByteMarkID;

   int rows = flags & IMMIX_ALLOC_ROW_COUNT;
   if (rows)
   {
      char *block = (char *)(ptr_i & IMMIX_BLOCK_BASE_MASK);
      char *rowMark = block + ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS);
      #if HXCPP_GC_DEBUG_LEVEL>0
      if ( ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS) + rows > IMMIX_LINES) DebuggerTrap();
      #endif

      *rowMark = 1;
      if (rows>1)
      {
         rowMark[1] = 1;
         if (rows>2)
         {
            rowMark[2] = 1;
            if (rows>3)
            {
               rowMark[3] = 1;
               for(int r=4; r<rows; r++)
                  rowMark[r]=1;
            }
         }
      }

      if (flags & IMMIX_ALLOC_IS_CONTAINER)
      {
         #ifdef PROFILE_COLLECT
         sObjectMarks++;
         #endif

         #ifdef HXCPP_DEBUG
            if (gCollectTrace && gCollectTrace==inPtr->__GetClass().GetPtr())
            {
               gCollectTraceCount++;
               if (gCollectTraceDoPrint)
                   __inCtx->Trace();
            }

            // Recursive mark so stack stays intact..
            #if (HXCPP_GC_DEBUG_LEVEL>0)
            inPtr->__Mark(__inCtx);
            #else
            if (gCollectTrace)
               inPtr->__Mark(__inCtx);
            else
               __inCtx->pushObj(inPtr);
            #endif
         #else // Not debug

            // There is a slight performance gain by calling recursively, but you
            //   run the risk of stack overflow.  Also, a parallel mark algorithm could be
            //   done when the marking is stack based.
            //inPtr->__Mark(__inCtx);
            #if (HXCPP_GC_DEBUG_LEVEL>0)
            inPtr->__Mark(__inCtx);
            #else
            __inCtx->pushObj(inPtr);
            #endif
        #endif
      }
      else
      {
         #ifdef PROFILE_COLLECT
         sAllocMarks++;
         #endif
      }

   // MARK_ROWS_UNCHECKED_END
   }
}


void MarkObjectArray(hx::Object **inPtr, int inLength, hx::MarkContext *__inCtx)
{
   hx::Object *tmp;

   int extra = inLength & 0x0f;
   for(int i=0;i<extra;i++)
      if (inPtr[i]) MarkObjectAlloc(inPtr[i],__inCtx);
   if (inLength==extra)
      return;

   inLength -= extra;
   hx::Object **ptrI = inPtr + extra;
   hx::Object **end = ptrI + inLength;


   #define MARK_PTR_I \
   tmp = *ptrI++; \
   if (tmp) MarkObjectAlloc(tmp,__inCtx);



   #ifdef HX_MULTI_THREAD_MARKING
   if (sAllThreads && inLength>4096)
   {
      hx::Object **dishOffEnd = end - 4096;
      while(ptrI<end)
      {
         // Are the other threads slacking off?
         if ((sRunningThreads != sAllThreads) && ptrI<dishOffEnd)
         {
            ptrI += sGlobalChunks.takeArrayJob(ptrI, end-ptrI);
         }
         else
         {
            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;

            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;

            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;

            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;
            MARK_PTR_I;
         }
      }
   }
   else
   #endif
   {
      while(ptrI<end)
      {
         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;

         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;

         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;

         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;
         MARK_PTR_I;
      }
   }
}

void MarkStringArray(String *inPtr, int inLength, hx::MarkContext *__inCtx)
{
   #if 0
   if (MAX_GC_THREADS>1 && sAllThreads && inLength>4096)
   {
      int extra = inLength & 0x0f;
      for(int i=0;i<extra;i++)
         if (inPtr[i]) MarkObjectAlloc(inPtr[i],__inCtx);

      hx::Object **ptrI = inPtr + extra;
      hx::Object **end = ptrI + (inLength& ~0x0f);
      hx::Object **dishOffEnd = end - 4096;
      while(ptrI<end)
      {
         // Are the other threads slacking off?
         if ((sRunningThreads != sAllThreads) && ptrI<dishOffEnd)
         {
            ptrI += sGlobalChunks.takeArrayJob(ptrI, end-ptrI);
         }

         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
         if (*ptrI) MarkObjectAlloc(*ptrI,__inCtx); ptrI++;
      }
   }
   else
   #endif
   {
      for(int i=0;i<inLength;i++)
      {
         const char *str = inPtr[i].raw_ptr();
         HX_MARK_STRING(str);
      }
   }
}

#ifdef HXCPP_DEBUG
#define FILE_SCOPE
#else
#define FILE_SCOPE static
#endif

// --- Roots -------------------------------

FILE_SCOPE HxMutex *sGCRootLock = 0;
typedef hx::UnorderedSet<hx::Object **> RootSet;
static RootSet sgRootSet;

typedef hx::UnorderedMap<void *,int> OffsetRootSet;
static OffsetRootSet *sgOffsetRootSet=0;

void GCAddRoot(hx::Object **inRoot)
{
   AutoLock lock(*sGCRootLock);
   sgRootSet.insert(inRoot);
}

void GCRemoveRoot(hx::Object **inRoot)
{
   AutoLock lock(*sGCRootLock);
   sgRootSet.erase(inRoot);
}


void GcAddOffsetRoot(void *inRoot, int inOffset)
{
   AutoLock lock(*sGCRootLock);
   if (!sgOffsetRootSet)
      sgOffsetRootSet = new OffsetRootSet();
   (*sgOffsetRootSet)[inRoot] = inOffset;
}

void GcSetOffsetRoot(void *inRoot, int inOffset)
{
   AutoLock lock(*sGCRootLock);
   (*sgOffsetRootSet)[inRoot] = inOffset;
}

void GcRemoveOffsetRoot(void *inRoot)
{
   AutoLock lock(*sGCRootLock);
   OffsetRootSet::iterator r = sgOffsetRootSet->find(inRoot);
   sgOffsetRootSet->erase(r);
}




// --- Finalizers -------------------------------


class WeakRef;
typedef hx::QuickVec<WeakRef *> WeakRefs;

FILE_SCOPE HxMutex *sFinalizerLock = 0;
FILE_SCOPE WeakRefs sWeakRefs;

class WeakRef : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdWeakRef };

   WeakRef(Dynamic inRef)
   {
      mRef = inRef;
      if (mRef.mPtr)
      {
         sFinalizerLock->Lock();
         sWeakRefs.push(this);
         sFinalizerLock->Unlock();
      }
   }

   // Don't mark our ref !

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mRef); }
   #endif

   Dynamic mRef;
};



typedef hx::QuickVec<InternalFinalizer *> FinalizerList;

FILE_SCOPE FinalizerList *sgFinalizers = 0;

typedef hx::UnorderedMap<hx::Object *,hx::finalizer> FinalizerMap;
FILE_SCOPE FinalizerMap sFinalizerMap;

typedef void (*HaxeFinalizer)(Dynamic);
typedef hx::UnorderedMap<hx::Object *,HaxeFinalizer> HaxeFinalizerMap;
FILE_SCOPE HaxeFinalizerMap sHaxeFinalizerMap;

hx::QuickVec<int> sFreeObjectIds;
typedef hx::UnorderedMap<hx::Object *,int> ObjectIdMap;
typedef hx::QuickVec<hx::Object *> IdObjectMap;
FILE_SCOPE ObjectIdMap sObjectIdMap;
FILE_SCOPE IdObjectMap sIdObjectMap;

typedef hx::UnorderedSet<hx::Object *> MakeZombieSet;
FILE_SCOPE MakeZombieSet sMakeZombieSet;

typedef hx::QuickVec<hx::Object *> ZombieList;
FILE_SCOPE ZombieList sZombieList;

typedef hx::QuickVec<hx::HashRoot *> WeakHashList;
FILE_SCOPE WeakHashList sWeakHashList;


InternalFinalizer::InternalFinalizer(hx::Object *inObj, finalizer inFinalizer)
{
   mValid = true;
   mObject = inObj;
   mFinalizer = inFinalizer;

   // Ensure this survives generational collect
   AutoLock lock(*gSpecialObjectLock);
   sgFinalizers->push(this);
}

#ifdef HXCPP_VISIT_ALLOCS
void InternalFinalizer::Visit(VisitContext *__inCtx)
{
   HX_VISIT_OBJECT(mObject);
}
#endif

void InternalFinalizer::Detach()
{
   mValid = false;
}

void FindZombies(MarkContext &inContext)
{
   for(MakeZombieSet::iterator i=sMakeZombieSet.begin(); i!=sMakeZombieSet.end(); )
   {
      hx::Object *obj = *i;
      MakeZombieSet::iterator next = i;
      ++next;

      unsigned char mark = ((unsigned char *)obj)[HX_ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         sZombieList.push(obj);
         sMakeZombieSet.erase(i);

         // Mark now to prevent secondary zombies...
         inContext.init();
         hx::MarkObjectAlloc(obj , &inContext );
         inContext.processMarkStack();
      }

      i = next;
   }
}

bool IsWeakRefValid(const HX_CHAR *inPtr)
{
   unsigned char mark = ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE];

    // Special case of member closure - check if the 'this' pointer is still alive
   return  mark==gByteMarkID;
}

bool IsWeakRefValid(hx::Object *inPtr)
{
   unsigned char mark = ((unsigned char *)inPtr)[HX_ENDIAN_MARK_ID_BYTE];

    // Special case of member closure - check if the 'this' pointer is still alive
    bool isCurrent = mark==gByteMarkID;
    if ( !isCurrent && inPtr->__GetType()==vtFunction)
    {
        hx::Object *thiz = (hx::Object *)inPtr->__GetHandle();
        if (thiz)
        {
            mark = ((unsigned char *)thiz)[HX_ENDIAN_MARK_ID_BYTE];
            if (mark==gByteMarkID)
            {
               // The object is still alive, so mark the closure and continue
               MarkAlloc(inPtr,0);
               return true;
            }
         }
    }
    return isCurrent;
}

static void someHackyFunc(hx::Object *)
{
}

static void (*hackyFunctionCall)(hx::Object *) = someHackyFunc;

struct Finalizable
{
   union
   {
      _hx_member_finalizer member;
      _hx_alloc_finalizer  alloc;
   };
   void *base;
   bool pin;
   bool isMember;

   Finalizable(hx::Object *inBase, _hx_member_finalizer inMember, bool inPin)
   {
      base = inBase;
      member = inMember;
      pin = inPin;
      isMember = true;
   }

   Finalizable(void *inBase, _hx_alloc_finalizer inAlloc, bool inPin)
   {
      base = inBase;
      alloc = inAlloc;
      pin = inPin;
      isMember = false;
   }
   void run()
   {
      if (isMember)
      {
         hx::Object *object = (hx::Object *)base;
         // I can't tell if it is msvc over-optimizing this code, to I am not
         //  quite calling things right, but this seems to fix it...
         hackyFunctionCall(object);
         (object->*member)();
      }
      else
         alloc( base );
   }
};
typedef hx::QuickVec< Finalizable > FinalizableList;
FILE_SCOPE FinalizableList sFinalizableList;


static double tFinalizers;
static int finalizerCount;
static int localCount;
static int localObjects;
static int localAllocs;
static int rootObjects;
static int rootAllocs;

void RunFinalizers()
{
   finalizerCount = 0;

   FinalizerList &list = *sgFinalizers;
   int idx = 0;
   while(idx<list.size())
   {
      InternalFinalizer *f = list[idx];
      if (!f->mValid)
      {
         list.qerase(idx);
         delete f;
      }
      else if (((unsigned char *)(f->mObject))[HX_ENDIAN_MARK_ID_BYTE] != gByteMarkID)
      {
         if (f->mFinalizer)
         {
            f->mFinalizer(f->mObject);
            finalizerCount++;
         }
         list.qerase(idx);
         delete f;
      }
      else
      {
         idx++;
      }
   }

   idx = 0;
   while(idx<sFinalizableList.size())
   {
      Finalizable &f = sFinalizableList[idx];
      unsigned char mark = ((unsigned char *)f.base)[HX_ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         finalizerCount++;
         f.run();
         sFinalizableList.qerase(idx);
      }
      else
         idx++;
   }

   for(FinalizerMap::iterator i=sFinalizerMap.begin(); i!=sFinalizerMap.end(); )
   {
      hx::Object *obj = i->first;
      FinalizerMap::iterator next = i;
      ++next;

      unsigned char mark = ((unsigned char *)obj)[HX_ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         finalizerCount++;
         (*i->second)(obj);
         sFinalizerMap.erase(i);
      }

      i = next;
   }


   for(HaxeFinalizerMap::iterator i=sHaxeFinalizerMap.begin(); i!=sHaxeFinalizerMap.end(); )
   {
      hx::Object *obj = i->first;
      HaxeFinalizerMap::iterator next = i;
      ++next;

      unsigned char mark = ((unsigned char *)obj)[HX_ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         finalizerCount++;
         (*i->second)(obj);
         sHaxeFinalizerMap.erase(i);
      }

      i = next;
   }

   MEM_STAMP(hx::tFinalizers);

   for(ObjectIdMap::iterator i=sObjectIdMap.begin(); i!=sObjectIdMap.end(); )
   {
      ObjectIdMap::iterator next = i;
      next++;

      hx::Object *o = i->first;
      unsigned char mark = ((unsigned char *)o)[HX_ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID && !(((unsigned int *)o)[-1] & HX_GC_CONST_ALLOC_BIT))
      {
         sFreeObjectIds.push(i->second);
         sIdObjectMap[i->second] = 0;
         sObjectIdMap.erase(i);
      }

      i = next;
   }

   for(int i=0;i<sWeakHashList.size();    )
   {
      HashRoot *ref = sWeakHashList[i];
      unsigned char mark = ((unsigned char *)ref)[HX_ENDIAN_MARK_ID_BYTE];
      // Object itself is gone - no need to worry about that again
      if ( mark!=gByteMarkID )
      {
         sWeakHashList.qerase(i);
         // no i++ ...
      }
      else
      {
         ref->updateAfterGc();
         i++;
      }
   }



   for(int i=0;i<sWeakRefs.size();    )
   {
      WeakRef *ref = sWeakRefs[i];
      unsigned char mark = ((unsigned char *)ref)[HX_ENDIAN_MARK_ID_BYTE];
      // Object itself is gone ...
      if ( mark!=gByteMarkID )
      {
         sWeakRefs.qerase(i);
         // no i++ ...
      }
      else
      {
         // what about the reference?
         hx::Object *r = ref->mRef.mPtr;
         unsigned char mark = ((unsigned char *)r)[HX_ENDIAN_MARK_ID_BYTE];

         // Special case of member closure - check if the 'this' pointer is still alive
         if ( mark!=gByteMarkID && r->__GetType()==vtFunction)
         {
            hx::Object *thiz = (hx::Object *)r->__GetHandle();
            if (thiz)
            {
               mark = ((unsigned char *)thiz)[HX_ENDIAN_MARK_ID_BYTE];
               if (mark==gByteMarkID)
               {
                  // The object is still alive, so mark the closure and continue
                  MarkAlloc(r,0);
               }
            }
         }

         if ( mark!=gByteMarkID )
         {
            ref->mRef.mPtr = 0;
            sWeakRefs.qerase(i);
         }
         else
            i++;
      }
   }
}

// Callback finalizer on non-abstract type;
void  GCSetFinalizer( hx::Object *obj, hx::finalizer f )
{
   if (!obj)
      throw Dynamic(HX_CSTRING("set_finalizer - invalid null object"));
   if (((unsigned int *)obj)[-1] & HX_GC_CONST_ALLOC_BIT)
      throw Dynamic(HX_CSTRING("set_finalizer - invalid const object"));

   AutoLock lock(*gSpecialObjectLock);
   if (f==0)
   {
      FinalizerMap::iterator i = sFinalizerMap.find(obj);
      if (i!=sFinalizerMap.end())
         sFinalizerMap.erase(i);
   }
   else
      sFinalizerMap[obj] = f;
}


// Callback finalizer on non-abstract type;
void  GCSetHaxeFinalizer( hx::Object *obj, HaxeFinalizer f )
{
   if (!obj)
      throw Dynamic(HX_CSTRING("set_finalizer - invalid null object"));
   if (((unsigned int *)obj)[-1] & HX_GC_CONST_ALLOC_BIT)
      throw Dynamic(HX_CSTRING("set_finalizer - invalid const object"));

   AutoLock lock(*gSpecialObjectLock);
   if (f==0)
   {
      HaxeFinalizerMap::iterator i = sHaxeFinalizerMap.find(obj);
      if (i!=sHaxeFinalizerMap.end())
         sHaxeFinalizerMap.erase(i);
   }
   else
      sHaxeFinalizerMap[obj] = f;
}

void GCDoNotKill(hx::Object *inObj)
{
   if (!inObj)
      throw Dynamic(HX_CSTRING("doNotKill - invalid null object"));
   if (((unsigned int *)inObj)[-1] & HX_GC_CONST_ALLOC_BIT)
      throw Dynamic(HX_CSTRING("doNotKill - invalid const object"));

   AutoLock lock(*gSpecialObjectLock);
   sMakeZombieSet.insert(inObj);
}

hx::Object *GCGetNextZombie()
{
   AutoLock lock(*gSpecialObjectLock);
   if (sZombieList.empty())
      return 0;
   hx::Object *result = sZombieList.pop();
   return result;
}

void RegisterWeakHash(HashBase<String> *inHash)
{
   AutoLock lock(*gSpecialObjectLock);
   sWeakHashList.push(inHash);
}

void RegisterWeakHash(HashBase<Dynamic> *inHash)
{
   AutoLock lock(*gSpecialObjectLock);
   sWeakHashList.push(inHash);
}



void InternalEnableGC(bool inEnable)
{
   //printf("Enable %d\n", sgInternalEnable);
   sgInternalEnable = inEnable;
}

bool IsConstAlloc(const void *inData)
{
   unsigned int *header = (unsigned int *)inData;
   return header[-1] & HX_GC_CONST_ALLOC_BIT;
}

void *InternalCreateConstBuffer(const void *inData,int inSize,bool inAddStringHash)
{
   bool addHash = inAddStringHash && inSize>0;

   int *result = (int *)HxAlloc(inSize + sizeof(int) + (addHash ? sizeof(int):0) );
   if (addHash)
   {
      unsigned int hash = 0;
      if (inData)
         for(int i=0;i<inSize-1;i++)
            hash = hash*223 + ((unsigned char *)inData)[i];

      //*((unsigned int *)((char *)result + inSize + sizeof(int))) = hash;
      *result++ = hash;
      *result++ = HX_GC_CONST_ALLOC_BIT | HX_GC_STRING_HASH;
   }
   else
   {
      *result++ = HX_GC_CONST_ALLOC_BIT;
   }

   if (inData)
   {
      memcpy(result,inData,inSize);
   }
   else if (inSize)
   {
      ZERO_MEM(result,inSize);
   }

   return result;
}

// Used when the allocation size is zero for a non-null pointer
void *emptyAlloc = InternalCreateConstBuffer(0,0,false);


} // namespace hx


hx::Object *__hxcpp_weak_ref_create(Dynamic inObject)
{
   return new hx::WeakRef(inObject);
}

hx::Object *__hxcpp_weak_ref_get(Dynamic inRef)
{
   #ifdef HXCPP_DEBUG
   hx::WeakRef *ref = dynamic_cast<hx::WeakRef *>(inRef.mPtr);
   #else
   hx::WeakRef *ref = static_cast<hx::WeakRef *>(inRef.mPtr);
   #endif
   return ref->mRef.mPtr;
}


// --- GlobalAllocator -------------------------------------------------------

typedef hx::QuickVec<BlockDataInfo *> BlockList;

typedef hx::QuickVec<unsigned int *> LargeList;

enum MemType { memUnmanaged, memBlock, memLarge };




#ifdef HXCPP_VISIT_ALLOCS
class AllocCounter : public hx::VisitContext
{
public:
   int count;

   AllocCounter() { count = 0; }

   void visitObject(hx::Object **ioPtr) { count ++; }
   void visitAlloc(void **ioPtr) { count ++; }
};


#endif


class GlobalAllocator *sGlobalAlloc = 0;


inline bool SortByBlockPtr(BlockDataInfo *inA, BlockDataInfo *inB)
{
   return inA->mPtr < inB->mPtr;
}

struct MoveBlockJob
{
   BlockList blocks;
   int from;
   int to;

   MoveBlockJob(BlockList &inAllBlocks)
   {
      blocks.setSize(inAllBlocks.size());
      memcpy(&blocks[0], &inAllBlocks[0], inAllBlocks.size()*sizeof(void*));

      std::sort(&blocks[0], &blocks[0]+blocks.size(), SortMoveOrder );
      from = 0;
      to = blocks.size()-1;
   }

   BlockDataInfo *getFrom()
   {
      while(true)
      {
         if (from>=to)
         {
            //printf("Caught up!\n");
            return 0;
         }
         #ifndef HXCPP_GC_DEBUG_ALWAYS_MOVE
         // Pinned / full
         if (blocks[from]->mMoveScore<2)
         {
            //printf("All other blocks good!\n");
            while(from<to)
            {
               //printf("Ignore DEFRAG %p ... %p\n", blocks[from]->mPtr, blocks[from]->mPtr+1);
               from++;
            }
            return 0;
         }
         #endif
         if (blocks[from]->mPinned)
         {
            //printf("PINNED DEFRAG %p ... %p\n", blocks[from]->mPtr, blocks[from]->mPtr+1);
            from++;
         }
         else // Found one...
            break;
      }
      //printf("From block %d (id=%d)\n", from, blocks[from]->mId);
      BlockDataInfo *result = blocks[from++];
      ////printf("FROM DEFRAG %p ... %p\n", result->mPtr, result->mPtr + 1 );
      return result;
   }
   BlockDataInfo *getTo()
   {
      if (from>=to)
      {
         //printf("No more room!\n");
         return 0;
      }
      //printf("To block %d (id=%d)\n", to, blocks[to]->mId);
      BlockDataInfo *result = blocks[to--];
      //printf("TO DEFRAG %p ... %p\n", result->mPtr, result->mPtr + 1 );
      return result;
   }
};


#ifndef HX_MEMORY_H_OVERRIDE

#if 0
static hx::QuickVec<void *> sBlockPool;
static int gcBlockAllocs = 0;

void *HxAllocGCBlock(size_t size)
{
   if (sBlockPool.size())
      return sBlockPool.pop();
   gcBlockAllocs++;
   GCLOG("===========================================> New Chunk (%d)\n", gcBlockAllocs);
   return HxAlloc(size);
}

void HxFreeGCBlock(void *p)
{
   sBlockPool.push(p);
}

#else

#ifdef HX_GC_FIXED_BLOCKS
unsigned char *chunkData = 0;
void *HxAllocGCBlock(size_t inSize)
{
   if (!chunkData)
   {
      size_t size = 65536;
      size *= IMMIX_BLOCK_SIZE;
      #if defined(HX_WINDOWS) && defined(HXCPP_M64)
      chunkData = (unsigned char *)0x100000000;
      VirtualAlloc(chunkData,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
      #else
      chunkData = (unsigned char *)HxAlloc(size);
      size_t ptr_i = (size_t)(chunkData+0xffff) & ~((size_t)0xffff);
      chunkData = (unsigned char *)ptr_i;
      #endif
      //printf("Using fixed blocks %p...%p\n", chunkData, chunkData+size);
   }
   void *result = chunkData;
   //printf(" %p\n", result);
   chunkData += inSize;
   return result;
}
void HxFreeGCBlock(void *p)
{
   printf("todo - HX_GC_FIXED_BLOCKS %p\n",p);
}
#else
void *HxAllocGCBlock(size_t size) { return HxAlloc(size); }
void HxFreeGCBlock(void *p) { HxFree(p); }
#endif

#endif

#endif // HX_MEMORY_H_OVERRIDE


//#define VERIFY_STACK_READ

#ifdef VERIFY_STACK_READ
// Just have something to do...
int gVerifyVoidCount = 0;
void VerifyStackRead(int *inBottom, int *inTop)
{
   #ifdef HXCPP_STACK_UP
   int *start = inTop-1;
   inTop = inBottom+1;
   inBottom = start;
   #endif

   int n = inTop - inBottom;
   int check = std::min(n,5);
   for(int c=0;c<check;c++)
   {
      if (inBottom[c]==0)
         gVerifyVoidCount++;
      if (inTop[-1-c]==0)
         gVerifyVoidCount++;
   }
}
#endif

// TODO - work out best size based on cache size?
#ifdef HXCPP_GC_BIG_BLOCKS
static int sMinZeroQueueSize = 4;
static int sMaxZeroQueueSize = 16;
#else
static int sMinZeroQueueSize = 8;
static int sMaxZeroQueueSize = 32;
#endif

#define BLOCK_OFSIZE_COUNT 12


class GlobalAllocator
{
   enum { LOCAL_POOL_SIZE = 2 };

public:
   GlobalAllocator()
   {
      memset((void *)mNextFreeBlockOfSize,0,sizeof(mNextFreeBlockOfSize));
      mRowsInUse = 0;
      mLargeAllocated = 0;
      mLargeAllocSpace = 40 << 20;
      mLargeAllocForceRefresh = mLargeAllocSpace;
      // Start at 1 Meg...
      mTotalAfterLastCollect = 1<<20;
      mCurrentRowsInUse = 0;
      mAllBlocksCount = 0;
      mGenerationalRetainEstimate = 0.5;
      for(int p=0;p<LOCAL_POOL_SIZE;p++)
         mLocalPool[p] = 0;

      createFreeList();
   }
   void AddLocal(LocalAllocator *inAlloc)
   {
      if (!gThreadStateChangeLock)
      {
         gThreadStateChangeLock = new HxMutex();
         gSpecialObjectLock = new HxMutex();
      }
      // Until we add ourselves, the collector will not wait
      //  on us - ie, we are assumed ot be in a GC free zone.
      AutoLock lock(*gThreadStateChangeLock);
      mLocalAllocs.push(inAlloc);
      // TODO Attach debugger
   }

   bool ReturnToPoolLocked(LocalAllocator *inAlloc)
   {
      // Until we add ourselves, the colled will not wait
      //  on us - ie, we are assumed ot be in a GC free zone.
      for(int p=0;p<LOCAL_POOL_SIZE;p++)
      {
         if (!mLocalPool[p])
         {
            mLocalPool[p] = inAlloc;
            return true;
         }
      }
      // I told him we already got one
      return false;
   }

   LocalAllocator *GetPooledAllocator()
   {
      AutoLock lock(*gThreadStateChangeLock);
      for(int p=0;p<LOCAL_POOL_SIZE;p++)
      {
         if (mLocalPool[p])
         {
            LocalAllocator *result = mLocalPool[p];
            mLocalPool[p] = 0;
            return result;
         }
      }
      return 0;
   }

   void RemoveLocalLocked(LocalAllocator *inAlloc)
   {
      // You should be in the GC free zone before you call this...
      if (!mLocalAllocs.qerase_val(inAlloc))
      {
         CriticalGCError("LocalAllocator removed without being added");
      }
   }

   void FreeLarge(void *inLarge)
   {
      ((unsigned char *)inLarge)[HX_ENDIAN_MARK_ID_BYTE] = 0;
      // AllocLarge will not lock this list unless it decides there is a suitable
      //  value, so we can't doa realloc without potentially crashing it.
      if (largeObjectRecycle.hasExtraCapacity(1))
      {
         unsigned int *blob = ((unsigned int *)inLarge) - 2;
         unsigned int size = *blob;
         mLargeListLock.Lock();
         mLargeAllocated -= size;
         // Could somehow keep it in the list, but mark as recycled?
         mLargeList.qerase_val(blob);
         // We could maybe free anyhow?
         if (!largeObjectRecycle.hasExtraCapacity(1))
         {
            mLargeListLock.Unlock();
            HxFree(blob);
            return;
         }
         largeObjectRecycle.push(blob);
         mLargeListLock.Unlock();
      }
   }

   void *AllocLarge(int inSize, bool inClear)
   {
      if (hx::gPauseForCollect)
         __hxcpp_gc_safe_point();

      //Should we force a collect ? - the 'large' data are not considered when allocating objects
      // from the blocks, and can 'pile up' between smalll object allocations
      if ((inSize+mLargeAllocated > mLargeAllocForceRefresh) && sgInternalEnable)
      {
         #ifdef SHOW_MEM_EVENTS
         //GCLOG("Large alloc causing collection");
         #endif
         CollectFromThisThread(false,false);
      }

      inSize = (inSize +3) & ~3;

      if (inSize<<1 > mLargeAllocSpace)
         mLargeAllocSpace = inSize<<1;

      unsigned int *result = 0;
      #ifndef HXCPP_SINGLE_THREADED_APP
      bool do_lock = true;
      #else
      bool do_lock = false;
      #endif
      bool isLocked = false;


      if (largeObjectRecycle.size())
      {
         for(int i=0;i<largeObjectRecycle.size();i++)
         {
            if ( largeObjectRecycle[i][0] == inSize )
            {
               if (do_lock && !isLocked)
               {
                  mLargeListLock.Lock();
                  isLocked = true;
                  if (  i>=largeObjectRecycle.size() || largeObjectRecycle[i][0] != inSize )
                     continue;
               }

               result = largeObjectRecycle[i];
               largeObjectRecycle.qerase(i);
               // You can use this to test race condition
               //Sleep(1);
               break;
            }
         }
      }

      if (!result)
         result = (unsigned int *)HxAlloc(inSize + sizeof(int)*2);

      if (!result)
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Large alloc failed - forcing collect\n");
         #endif

         if (isLocked)
         {
            mLargeListLock.Unlock();
            isLocked = false;
         }

         CollectFromThisThread(true,true);
         result = (unsigned int *)HxAlloc(inSize + sizeof(int)*2);
      }

      if (!result)
      {
         GCLOG("Memory Exhausted!\n");
         DebuggerTrap();
      }

      if (inClear)
         ZERO_MEM(result, inSize + sizeof(int)*2);

      result[0] = inSize;
      #ifdef HXCPP_GC_NURSERY
      result[1] = 0;
      #else
      result[1] = hx::gMarkID;
      #endif

      if (do_lock && !isLocked)
         mLargeListLock.Lock();

      mLargeList.push(result);
      mLargeAllocated += inSize;

      if (do_lock)
         mLargeListLock.Unlock();

      return result+2;
   }

   void onMemoryChange(int inDelta, const char *inWhy)
   {
      if (hx::gPauseForCollect)
         __hxcpp_gc_safe_point();

      if (inDelta>0)
      {
         //Should we force a collect ? - the 'large' data are not considered when allocating objects
         // from the blocks, and can 'pile up' between smalll object allocations
         if (inDelta>0 && (inDelta+mLargeAllocated > mLargeAllocForceRefresh) && sgInternalEnable)
         {
            //GCLOG("onMemoryChange alloc causing collection");
            CollectFromThisThread(false,false);
         }

         int rounded = (inDelta +3) & ~3;
   
         if (rounded<<1 > mLargeAllocSpace)
            mLargeAllocSpace = rounded<<1;
      }

      #ifndef HXCPP_SINGLE_THREADED_APP
      bool do_lock = true;
      #else
      bool do_lock = false;
      #endif

      if (do_lock)
         mLargeListLock.Lock();

      mLargeAllocated += inDelta;

      if (do_lock)
         mLargeListLock.Unlock();
   }

   // Gets a block with the 'zeroLock' acquired, which means the zeroing thread
   //  will leave it alone from now on
   //
   // This contains some "lock free" code that triggers the thread sanitizer.
   //  * mOwned can only be changed with the mZeroLock
   //  * mFreeBlocks may increase size, but it is not critical whether the mNextFreeBlockOfSize
   //     is increased since skipping only leads to a bit of extra searhing for the
   //     next allocation
   #if defined(__has_feature)
     #if __has_feature(thread_sanitizer)
      __attribute__((no_sanitize("thread")))
     #endif
   #endif
   BlockDataInfo *GetNextFree(int inRequiredBytes)
   {
      bool failedLock = true;
      int sizeSlot = inRequiredBytes>>IMMIX_LINE_BITS;
      if (sizeSlot>=BLOCK_OFSIZE_COUNT)
         sizeSlot = BLOCK_OFSIZE_COUNT-1;
      //volatile int &nextFreeBlock = mNextFreeBlockOfSize[sizeSlot];
      int nextFreeBlock = mNextFreeBlockOfSize[sizeSlot];
      while(failedLock && nextFreeBlock<mFreeBlocks.size())
      {
         failedLock = false;

         for(int i=nextFreeBlock; i<mFreeBlocks.size(); i++)
         {
             BlockDataInfo *info = mFreeBlocks[i];
             if (!info->mOwned && info->mMaxHoleSize>=inRequiredBytes)
             {
                // Acquire the zero-lock
                if (_hx_atomic_compare_exchange(&info->mZeroLock, 0, 1) == 0)
                {
                   // Acquire ownership...
                   if (info->mOwned)
                   {
                      // Someone else got it...
                      info->mZeroLock = 0;
                   }
                   else
                   {
                      info->mOwned = true;

                      // Increase the mNextFreeBlockOfSize
                      int idx = nextFreeBlock;
                      while(idx<mFreeBlocks.size() && mFreeBlocks[idx]->mOwned)
                      {
                         _hx_atomic_compare_exchange(mNextFreeBlockOfSize+sizeSlot, idx, idx+1);
                         idx++;
                      }

                      if (sgThreadPoolJob==tpjAsyncZeroJit)
                      {
                         if (info->mZeroed==ZEROED_THREAD)
                            onZeroedBlockDequeued();
                         #ifdef PROFILE_THREAD_USAGE
                         else
                         {
                            if (!info->mZeroed)
                               _hx_atomic_add(&sThreadZeroMisses, 1);
                         }
                         #endif
                       }

                      return info;
                   }
                 }
                 else if (!info->mOwned)
                 {
                    // Zeroing thread is currently working on this block
                    // Go to next one or spin around again
                    failedLock = true;
                 }
             }
         }
      }

      return 0;
   }

   inline size_t GetWorkingMemory()
   {
       return ((size_t)mAllBlocks.size()) << IMMIX_BLOCK_BITS;
   }

   // Making this function "virtual" is actually a (big) performance enhancement!
   // On the iphone, sjlj (set-jump-long-jump) exceptions are used, which incur a
   //  performance overhead.  It seems that the overhead in only in routines that call
   //  malloc/new.  This is not called very often, so the overhead should be minimal.
   //  However, gcc inlines this function!  requiring every alloc the have sjlj overhead.
   //  Making it virtual prevents the overhead.
   virtual bool AllocMoreBlocks(bool &outForceCompact, bool inJustBorrowing)
   {
      enum { newBlockCount = 1<<(IMMIX_BLOCK_GROUP_BITS) };

      #ifdef HXCPP_GC_MOVING
      if (!inJustBorrowing)
      {
         // Do compact next collect sooner
         if (sgTimeToNextTableUpdate>1)
         {
            #ifdef SHOW_FRAGMENTATION
              #ifdef SHOW_MEM_EVENTS_VERBOSE
                GCLOG("  alloc -> enable full collect\n");
              #endif
            #endif
            //sgTimeToNextTableUpdate = 1;
         }
      }
      #endif

      #ifndef HXCPP_GC_BIG_BLOCKS
      // Currently, we only have 2 bytes for a block header
      if (mAllBlocks.size()+newBlockCount >= 0xfffe )
      {
         #if defined(SHOW_MEM_EVENTS) || defined(SHOW_FRAGMENTATION)
         GCLOG("Block id count used - collect");
         #endif
         // The problem is we are out of blocks, not out of memory
         outForceCompact = false;
         return false;
      }
      #endif

      // Find spare group...
      int gid = -1;
      for(int i=0;i<gAllocGroups.size();i++)
         if (!gAllocGroups[i].alloc)
         {
            gid = i;
            break;
         }
      if (gid<0)
      {
         if (!gAllocGroups.safeReserveExtra(1))
         {
            outForceCompact = true;
            return false;
         }
         gid = gAllocGroups.next();
         gAllocGroups[gid].alloc = 0;
      }

      int n = 1<<IMMIX_BLOCK_GROUP_BITS;


      if (!mAllBlocks.safeReserveExtra(n) || !mFreeBlocks.hasExtraCapacity(n))
      {
         outForceCompact = true;
         return false;
      }

      char *chunk = (char *)HxAllocGCBlock( 1<<(IMMIX_BLOCK_GROUP_BITS + IMMIX_BLOCK_BITS) );
      if (!chunk)
      {
         //DebuggerTrap();
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Alloc failed - try collect\n");
         #endif
         outForceCompact = true;
         return false;
      }

      char *aligned = (char *)( (((size_t)chunk) + IMMIX_BLOCK_SIZE-1) & IMMIX_BLOCK_BASE_MASK);
      if (aligned!=chunk)
         n--;
      gAllocGroups[gid].alloc = chunk;
      gAllocGroups[gid].blocks = n;
      gAllocGroups[gid].clear();

      int newSize = mFreeBlocks.size();
      for(int i=0;i<n;i++)
      {
         BlockData *block = (BlockData *)(aligned + i*IMMIX_BLOCK_SIZE);
         BlockDataInfo *info = new BlockDataInfo(gid,block);

         mAllBlocks.push(info);
         mFreeBlocks.push(info);
      }
      std::stable_sort(&mAllBlocks[0], &mAllBlocks[0] + mAllBlocks.size(), SortByBlockPtr );
      mAllBlocksCount = mAllBlocks.size();
      for(int i=0;i<BLOCK_OFSIZE_COUNT;i++)
         mNextFreeBlockOfSize[i] = newSize;

      #ifdef HX_GC_VERIFY
      VerifyBlockOrder();
      #endif

      #if defined(SHOW_MEM_EVENTS_VERBOSE) || defined(SHOW_FRAGMENTATION_BLOCKS)
      if (inJustBorrowing)
      {
         GCLOG("Borrow Blocks %d = %d k\n", mAllBlocks.size(), (mAllBlocks.size() << IMMIX_BLOCK_BITS)>>10);
      }
      else
      {
         GCLOG("Blocks %d = %d k\n", mAllBlocks.size(), (mAllBlocks.size() << IMMIX_BLOCK_BITS)>>10);
      }
      #endif


      return true;
   }

   bool allowMoreBlocks()
   {
      #ifdef HXCPP_GC_GENERATIONAL
      return sGcMode==gcmFull;
      #else
      return true;
      #endif
   }

   void repoolReclaimedBlock(BlockDataInfo *block)
   {
      // The mMaxHoleSize has changed - possibly return to one of the pools
   }


   BlockDataInfo *GetFreeBlock(int inRequiredBytes, hx::ImmixAllocator *inAlloc)
   {
      while(true)
      {
         BlockDataInfo *result = GetNextFree(inRequiredBytes);
         if (result)
         {
            result->zeroAndUnlock();

            // After zero/reclaim, it might be that the hole size is smaller than we thought.
            if (result->mMaxHoleSize>=inRequiredBytes)
               return result;

            repoolReclaimedBlock(result);
            continue;
         }

         if (hx::gPauseForCollect)
         {
            hx::PauseForCollect();
            continue;
         }

         #ifndef HXCPP_SINGLE_THREADED_APP
         hx::EnterGCFreeZone();
         gThreadStateChangeLock->Lock();
         hx::ExitGCFreeZoneLocked();

         result = GetNextFree(inRequiredBytes);
         #endif

         bool forceCompact = false;
         if (!result && allowMoreBlocks() && (!sgInternalEnable || GetWorkingMemory()<sWorkingMemorySize))
         {
            if (AllocMoreBlocks(forceCompact,false))
               result = GetNextFree(inRequiredBytes);
         }

         if (!result)
         {
            inAlloc->SetupStackAndCollect(false,forceCompact,true,true);
            result = GetNextFree(inRequiredBytes);
         }

         if (!result && !forceCompact)
         {
            // Try with compact this time...
            forceCompact = true;
            inAlloc->SetupStackAndCollect(false,forceCompact,true,true);
            result = GetNextFree(inRequiredBytes);
         }

         if (!result)
         {
            if (AllocMoreBlocks(forceCompact,false))
               result = GetNextFree(inRequiredBytes);
         }

         if (!result)
         {
            GCLOG("Memory exhausted.\n");
            #ifndef HXCPP_M64
            GCLOG(" try 64 bit build.\n");
            #endif
            #ifndef HXCPP_GC_BIG_BLOCKS
            GCLOG(" try HXCPP_GC_BIG_BLOCKS.\n");
            #endif
            DebuggerTrap();
         }

         // Assume all wil be used
         mCurrentRowsInUse += result->GetFreeRows();

         #ifndef HXCPP_SINGLE_THREADED_APP
         gThreadStateChangeLock->Unlock();
         #endif


         result->zeroAndUnlock();

         // After zero/reclaim, it might be that the hole size is smaller than we thought.
         if (result->mMaxHoleSize>=inRequiredBytes)
            return result;

         repoolReclaimedBlock(result);
      }
  }

   #if  defined(HXCPP_VISIT_ALLOCS) // {

  void MoveSpecial(hx::Object *inTo, hx::Object *inFrom, int size)
   {
      #ifdef HX_WATCH
      if (hxInWatchList(inFrom))
         GCLOG("****** watch MOVE from %p\n",inFrom);
      if (hxInWatchList(inTo))
         GCLOG("****** watch MOVE to %p\n",inTo);
      #endif
       // FinalizerList will get visited...

       hx::FinalizerMap::iterator i = hx::sFinalizerMap.find(inFrom);
       if (i!=hx::sFinalizerMap.end())
       {
          hx::finalizer f = i->second;
          hx::sFinalizerMap.erase(i);
          hx::sFinalizerMap[inTo] = f;
       }

       hx::HaxeFinalizerMap::iterator h = hx::sHaxeFinalizerMap.find(inFrom);
       if (h!=hx::sHaxeFinalizerMap.end())
       {
          hx::HaxeFinalizer f = h->second;
          hx::sHaxeFinalizerMap.erase(h);
          hx::sHaxeFinalizerMap[inTo] = f;
       }

       hx::MakeZombieSet::iterator mz = hx::sMakeZombieSet.find(inFrom);
       if (mz!=hx::sMakeZombieSet.end())
       {
          hx::sMakeZombieSet.erase(mz);
          hx::sMakeZombieSet.insert(inTo);
       }

       hx::ObjectIdMap::iterator id = hx::sObjectIdMap.find(inFrom);
       if (id!=hx::sObjectIdMap.end())
       {
          hx::sIdObjectMap[id->second] = inTo;
          hx::sObjectIdMap[inTo] = id->second;
          hx::sObjectIdMap.erase(id);
       }

       // Maybe do something faster with weakmaps

#ifdef HXCPP_TELEMETRY
       //printf(" -- moving %018x to %018x, size %d\n", inFrom, inTo, size);
       __hxt_gc_realloc(inFrom, inTo, size);
#endif

   }



   bool MoveBlocks(MoveBlockJob &inJob,BlockDataStats &ioStats)
   {
      BlockData *dest = 0;
      BlockDataInfo *destInfo = 0;
      unsigned int *destStarts = 0;
      int hole = -1;
      int holes = 0;
      int destPos = 0;
      int destLen = 0;

      int moveObjs = 0;
      int clearedBlocks = 0;

      while(true)
      {
         BlockDataInfo *from = inJob.getFrom();
         if (!from)
            break;
         if (from->calcFragScore()>FRAG_THRESH)
            ioStats.fraggedBlocks--;
         ioStats.rowsInUse -= from->mUsedRows;
         if (!from->isEmpty())
            ioStats.emptyBlocks ++;

         unsigned int *allocStart = from->allocStart;
         #ifdef SHOW_MEM_EVENTS
         //GCLOG("Move from %p (%d x %d)\n", from, from->mUsedRows, from->mHoles );
         #endif

         const unsigned char *rowMarked = from->mPtr->mRowMarked;
         for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
         {
            if (rowMarked[r])
            {
               unsigned int starts = allocStart[r];
               if (!starts)
                  continue;
               for(int i=0;i<32;i++)
               {
                  if ( starts & (1<<i))
                  {
                     unsigned int *row = (unsigned int *)from->mPtr->mRow[r];
                     unsigned int &header = row[i];

                     if ((header&IMMIX_ALLOC_MARK_ID) == hx::gMarkID)
                     {
                        int size = ((header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT);
                        int allocSize = size + sizeof(int);

                        while(allocSize>destLen)
                        {
                           hole++;
                           if (hole<holes)
                           {
                              destPos = destInfo->mRanges[hole].start;
                              destLen = destInfo->mRanges[hole].length;
                           }
                           else
                           {
                              if (destInfo)
                                 destInfo->makeFull();
                              do
                              {
                                 destInfo = inJob.getTo();
                                 if (!destInfo)
                                    goto all_done;
                              } while(destInfo->mHoles==0);


                              ioStats.rowsInUse += IMMIX_USEFUL_LINES - destInfo->mUsedRows;
                              //destInfo->zero();
                              dest = destInfo->mPtr;
                              destStarts = destInfo->allocStart;

                              hole = 0;
                              holes = destInfo->mHoles;
                              destPos = destInfo->mRanges[hole].start;
                              destLen = destInfo->mRanges[hole].length;
                           }
                        }

                        int startRow = destPos>>IMMIX_LINE_BITS;

                        destStarts[ startRow ] |= hx::gImmixStartFlag[destPos&127];

                        unsigned int *buffer = (unsigned int *)((char *)dest + destPos);

                        unsigned int headerPreserve = header & IMMIX_HEADER_PRESERVE;

                        int end = destPos + allocSize;

                        *buffer++ =  (( (end+(IMMIX_LINE_LEN-1))>>IMMIX_LINE_BITS) -startRow) |
                                        (size<<IMMIX_ALLOC_SIZE_SHIFT) |
                                        headerPreserve |
                                        hx::gMarkID;
                        destPos = end;
                        destLen -= allocSize;

                        unsigned int *src = row + i + 1;

                        MoveSpecial((hx::Object *)buffer,(hx::Object *)src, size);

                        // Result has moved - store movement in original position...
                        memcpy(buffer, src, size);
                        //GCLOG("   move %p -> %p %d (%08x %08x )\n", src, buffer, size, buffer[-1], buffer[1] );

                        *(unsigned int **)src = buffer;
                        header = IMMIX_OBJECT_HAS_MOVED;
                        moveObjs++;

                        starts &= ~(1<<i);
                        if (!starts)
                           break;
                     }
                  }
               }
            }
         }

         all_done:
         if (destInfo)
         {
            // Still have space, which means from finished.
            destInfo->makeFull();
            from->clear();
            clearedBlocks++;
            // Could remove some used rows from stats
         }
         else
         {
            // Partialy cleared, then ran out of to blocks - remove from allocation this round
            if (from)
            {
               // Could maybe be a bit smarter here
               ioStats.rowsInUse += from->mUsedRows;
               from->makeFull();
            }
         }
      }

      #ifdef SHOW_FRAGMENTATION
      GCLOG("Moved %d objects (%d/%d blocks)\n", moveObjs, clearedBlocks, mAllBlocks.size());
      #endif

      if (moveObjs)
      {
         AdjustPointers(0);
      }


      return moveObjs;
   }

   void AdjustPointers(hx::QuickVec<hx::Object *> *inRemembered)
   {
      class AdjustPointer : public hx::VisitContext
      {
         GlobalAllocator *mAlloc;
      public:
         AdjustPointer(GlobalAllocator *inAlloc) : mAlloc(inAlloc) {  }
      
         void visitObject(hx::Object **ioPtr)
         {
            if ( ((*(unsigned int **)ioPtr)[-1]) == IMMIX_OBJECT_HAS_MOVED )
            {
               //GCLOG("  patch object to  %p -> %p\n", *ioPtr,  (*(hx::Object ***)ioPtr)[0]);
               *ioPtr = (*(hx::Object ***)ioPtr)[0];
               //GCLOG("    %08x %08x ...\n", ((int *)(*ioPtr))[0], ((int *)(*ioPtr))[1] ); 
            }
         }

         void visitAlloc(void **ioPtr)
         {
            if ( ((*(unsigned int **)ioPtr)[-1]) == IMMIX_OBJECT_HAS_MOVED )
            {
               //GCLOG("  patch reference to  %p -> %p\n", *ioPtr,  (*(void ***)ioPtr)[0]);
               *ioPtr = (*(void ***)ioPtr)[0];
               //GCLOG("    %08x %08x ...\n", ((int *)(*ioPtr))[0], ((int *)(*ioPtr))[1] ); 
            }
         }
      };


      AdjustPointer adjust(this);
      VisitAll(&adjust,true, inRemembered);
   }


   #ifdef HXCPP_GC_GENERATIONAL
   BlockDataInfo *getNextBlockWithoutSurvivors(int &ioPosition)
   {
      while(ioPosition<mAllBlocks.size())
      {
         BlockDataInfo *b = mAllBlocks[ioPosition++];
         if (b->mHoles>0 && !b->mHasSurvivor)
            return b;
      }
      return 0;
   }

   int MoveSurvivors(hx::QuickVec<hx::Object *> *inRemembered)
   {
      int sourceScan = 0;
      int destScan = 0;
      int moveObjs = 0;
      int clearedBlocks = 0;

      BlockDataInfo *destInfo = 0;
      BlockData *dest = 0;
      int destHole = 0;
      int destPos = 0;
      int destLen = 0;
      unsigned int *destStarts = 0;

      BlockDataInfo *from = 0;

      int tested = 0;

      int all = mAllBlocks.size();
      for(int srcBlock=0; srcBlock<all; srcBlock++)
      {
         from = mAllBlocks[srcBlock];
         if (from->mPinned || !from->mHasSurvivor)
         {
            from = 0;
            continue;
         }
         tested++;
         unsigned int *srcStart = from->allocStart;

         // Scan nursery for survivors
         for(int hole = 0; hole<from->mHoles; hole++)
         {
            int start = from->mRanges[hole].start;
            int len = from->mRanges[hole].length;
            int startRow = start >> IMMIX_LINE_BITS;
            int end = startRow + (len >> IMMIX_LINE_BITS);
            for(int r = startRow; r<end;r++)
            {
               unsigned int &startFlags = srcStart[r];
               if (startFlags)
               {
                  for(int loc=0;loc<32;loc++)
                     if (startFlags & (1<<loc))
                     {
                        unsigned int *row = (unsigned int *)from->mPtr->mRow[r];
                        unsigned int &header = row[loc];

                        if ((header&IMMIX_ALLOC_MARK_ID) == hx::gMarkID)
                        {
                           int size = ((header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT);
                           int allocSize = size + sizeof(int);

                           // Find dest reqion ...
                           while(destHole==0 || destLen<allocSize)
                           {
                              if (destHole==0)
                              {
                                 if (destInfo)
                                    destInfo->makeFull();
                                 destInfo = getNextBlockWithoutSurvivors(destScan);
                                 if (!destInfo)
                                    goto no_more_moves;

                                 destHole = -1;
                                 destLen = 0;
                              }
                              if (destLen<allocSize)
                              {
                                 destHole++;
                                 if (destHole>=destInfo->mHoles)
                                    destHole = 0;
                                 else
                                 {
                                    dest = destInfo->mPtr;
                                    destPos = destInfo->mRanges[destHole].start;
                                    destLen = destInfo->mRanges[destHole].length;
                                    destStarts = destInfo->allocStart;
                                 }
                              }
                           }

                           // TODO - not copy + paste

                           int startRow = destPos>>IMMIX_LINE_BITS;

                           destStarts[ startRow ] |= hx::gImmixStartFlag[destPos&127];

                           unsigned int *buffer = (unsigned int *)((char *)dest + destPos);

                           unsigned int headerPreserve = header & IMMIX_HEADER_PRESERVE;

                           int end = destPos + allocSize;

                           *buffer++ =  (( (end+(IMMIX_LINE_LEN-1))>>IMMIX_LINE_BITS) -startRow) |
                                           (size<<IMMIX_ALLOC_SIZE_SHIFT) |
                                           headerPreserve |
                                           hx::gMarkID;
                           destPos = end;
                           destLen -= allocSize;

                           unsigned int *src = row + loc + 1;

                           MoveSpecial((hx::Object *)buffer,(hx::Object *)src, size);

                           // Result has moved - store movement in original position...
                           memcpy(buffer, src, size);
                           //GCLOG("   move %p -> %p %d (%08x %08x )\n", src, buffer, size, buffer[-1], buffer[1] );

                           *(unsigned int **)src = buffer;
                           header = IMMIX_OBJECT_HAS_MOVED;
                           moveObjs++;

                           startFlags &= ~(1<<loc);
                           if (!startFlags)
                              break;
                        }
                     }
               }
            }
         }

         no_more_moves:
            if (destInfo)
            {
               destInfo->makeFull();
               clearedBlocks++;
            }
            else
            {
               // Partialy cleared, then ran out of to blocks - remove from allocation this round
               if (from && moveObjs)
               {
                  // Could maybe be a bit smarter here
                  //ioStats.rowsInUse += from->mUsedRows;
                  from->makeFull();
               }
            }
      }

      #ifdef SHOW_FRAGMENTATION
      //GCLOG("Compacted nursery %d objects (%d/%d blocks)\n", moveObjs, clearedBlocks, mAllBlocks.size());
      #endif

      if (moveObjs)
      {
         AdjustPointers(inRemembered);
      }

      return moveObjs;
   }
   #endif

   int releaseEmptyGroups(BlockDataStats &outStats, size_t releaseSize)
   {
      int released=0;
      int groups=0;
      for(int i=0; i<mAllBlocks.size(); i++ )
      {
         if (!mAllBlocks[i]->isEmpty())
             gAllocGroups[mAllBlocks[i]->mGroupId].isEmpty=false;
      }

      #ifdef HX_GC_VERIFY
      typedef std::pair< void *, void * > ReleasedRange;
      std::vector<ReleasedRange> releasedRange;
      std::vector<int> releasedGids;
      #endif

      for(int i=0;i<gAllocGroups.size();i++)
      {
         GroupInfo &g = gAllocGroups[i];
         if (g.alloc && g.isEmpty && !g.pinned)
         {
            size_t groupBytes = g.blocks << (IMMIX_BLOCK_BITS);

            #ifdef SHOW_MEM_EVENTS_VERBOSE
            GCLOG("Release group %d: %p -> %p\n", i, g.alloc, g.alloc+groupBytes);
            #endif
            #ifdef HX_GC_VERIFY
            releasedRange.push_back( ReleasedRange(g.alloc, g.alloc+groupBytes) );
            releasedGids.push_back(i);
            #endif

            HxFreeGCBlock(g.alloc);
            g.alloc = 0;

            groups++;

            if (groupBytes > releaseSize)
               break;

            releaseSize -= groupBytes;
         }
      }

      // Release blocks
      for(int i=0; i<mAllBlocks.size();  )
      {
         if (!gAllocGroups[mAllBlocks[i]->mGroupId].alloc)
         {
            outStats.emptyBlocks--;
            released++;
            mAllBlocks[i]->destroy();
            mAllBlocks.erase(i);
         }
         else
         {
            #ifdef HX_GC_VERIFY
            for(int g=0;g<releasedGids.size();g++)
               if (mAllBlocks[i]->mGroupId == releasedGids[g])
               {
                  printf("Group %d should be released.\n", mAllBlocks[i]->mGroupId);
                  DebuggerTrap();
               }
            #endif
            i++;
         }
      }

      #ifdef HX_GC_VERIFY
      for(int i=0;i<mAllBlocks.size();i++)
      {
         BlockDataInfo *info = mAllBlocks[i];
         void *ptr = info->mPtr;
         for(int r=0;r<releasedRange.size();r++)
         if ( ptr>=releasedRange[r].first && ptr<releasedRange[r].second )
         {
            printf("Released block %p(%d:%p) still in list\n", info, info->mId, info->mPtr );
            DebuggerTrap();
         }
      }

      VerifyBlockOrder();
      #endif


      #if defined(SHOW_MEM_EVENTS) || defined(SHOW_FRAGMENTATION)
      GCLOG("Release %d blocks, %d groups,  %s\n", released, groups, formatBytes((size_t)released<<(IMMIX_BLOCK_BITS)).c_str());
      #endif
      return released;
   }


   void calcMoveOrder()
   {
      for(int i=0;i<gAllocGroups.size();i++)
         gAllocGroups[i].clear();

      for(int i=0; i<mAllBlocks.size(); i++ )
      {
         BlockDataInfo &block = *mAllBlocks[i];
         GroupInfo &g = gAllocGroups[block.mGroupId];
         if (block.mPinned)
            g.pinned = true;
         g.usedBytes += block.mUsedBytes;
         g.usedSpace += block.mUsedRows<<IMMIX_LINE_BITS;
      }

      for(int i=0; i<mAllBlocks.size(); i++ )
      {
         BlockDataInfo &block = *mAllBlocks[i];
         GroupInfo &g = gAllocGroups[block.mGroupId];
         // Base on group wasted space (when not pinned)
         block.mMoveScore = g.getMoveScore();
      }
   }

   #endif // } defined(HXCPP_VISIT_ALLOCS)
 
   void *GetIDObject(int inIndex)
   {
      AutoLock lock(*gSpecialObjectLock);
      if (inIndex<0 || inIndex>hx::sIdObjectMap.size())
         return 0;
      return hx::sIdObjectMap[inIndex];
   }

   int GetObjectID(void * inPtr)
   {
      AutoLock lock(*gSpecialObjectLock);
      hx::ObjectIdMap::iterator i = hx::sObjectIdMap.find( (hx::Object *)inPtr );
      if (i!=hx::sObjectIdMap.end())
         return i->second;

      int id = 0;
      if (hx::sFreeObjectIds.size()>0)
      {
         id = hx::sFreeObjectIds.pop();
      }
      else
      {
         id = hx::sObjectIdMap.size();
         hx::sIdObjectMap.push(0);
      }
      hx::sObjectIdMap[(hx::Object *)inPtr] = id;
      hx::sIdObjectMap[id] = (hx::Object *)inPtr;
      return id;
   }


   void ClearRowMarks()
   {
      for(int i=0;i<mAllBlocks.size();i++)
         mAllBlocks[i]->clearRowMarks();
   }


   void ClearBlockMarks()
   {
      for(int i=0;i<mAllBlocks.size();i++)
         mAllBlocks[i]->clearBlockMarks();
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void VisitAll(hx::VisitContext *inCtx,bool inMultithread = false,hx::QuickVec<hx::Object *> *inRemembered = 0)
   {
      if (inRemembered)
      {
         int n = inRemembered->size();
         for(int i=0;i<n;i++)
            (*inRemembered)[i]->__Visit(inCtx);
      }
      else if (MAX_GC_THREADS>1 && inMultithread && mAllBlocks.size())
      {
         sThreadVisitContext = inCtx;
         StartThreadJobs(tpjVisitBlocks, mAllBlocks.size(), true);
         sThreadVisitContext = 0;
      }
      else
         for(int i=0;i<mAllBlocks.size();i++)
            mAllBlocks[i]->VisitBlock(inCtx);

      for(int i=0;i<mLocalAllocs.size();i++)
         VisitLocalAlloc(mLocalAllocs[i], inCtx);

      hx::VisitClassStatics(inCtx);

      for(hx::RootSet::iterator i = hx::sgRootSet.begin(); i!=hx::sgRootSet.end(); ++i)
      {
         hx::Object **obj = *i;
         if (*obj)
            inCtx->visitObject(obj);
      }


      if (hx::sgOffsetRootSet)
         for(hx::OffsetRootSet::iterator i = hx::sgOffsetRootSet->begin(); i!=hx::sgOffsetRootSet->end(); ++i)
         {
            char *ptr = *(char **)(i->first);
            int offset = i->second;
            hx::Object *obj = (hx::Object *)(ptr - offset);

            if (obj)
            {
               hx::Object *obj0 = obj;
               inCtx->visitObject(&obj);
               if (obj!=obj0)
                  *(char **)(i->first) = (char *)(obj) + offset;
            }
         }

      if (hx::sgFinalizers)
         for(int i=0;i<hx::sgFinalizers->size();i++)
            (*hx::sgFinalizers)[i]->Visit(inCtx);

      for(int i=0;i<hx::sFinalizableList.size();i++)
         inCtx->visitAlloc( &hx::sFinalizableList[i].base );

      for(int i=0;i<hx::sZombieList.size();i++)
         inCtx->visitObject( &hx::sZombieList[i] );

      for(int i=0;i<hx::sWeakRefs.size(); i++)
         inCtx->visitObject( (hx::Object **) &hx::sWeakRefs[i] );

      for(int i=0;i<hx::sWeakHashList.size();i++)
         inCtx->visitObject( (hx::Object **) &hx::sWeakHashList[i] );

   }

   #endif

   void ReclaimAsync(BlockDataStats &outStats)
   {
      while(!sgThreadPoolAbort)
      {
         int blockId = _hx_atomic_add(&mThreadJobId, 1);
         if (blockId>=mAllBlocks.size())
            break;

         if ( sgThreadPoolJob==tpjReclaimFull)
            mAllBlocks[blockId]->reclaim<true>(&outStats);
         else
            mAllBlocks[blockId]->reclaim<false>(&outStats);
      }
   }

   void CountAsync(BlockDataStats &outStats)
   {
      while(!sgThreadPoolAbort)
      {
         int blockId = _hx_atomic_add(&mThreadJobId, 1);
         if (blockId>=mAllBlocks.size())
            break;

         mAllBlocks[blockId]->countRows(outStats);
      }
   }


   void GetStatsAsync(BlockDataStats &outStats)
   {
      while(!sgThreadPoolAbort)
      {
         int blockId = _hx_atomic_add(&mThreadJobId, 1);
         if (blockId>=mAllBlocks.size())
            break;

         mAllBlocks[blockId]->getStats( outStats );
      }
   }

  

   #ifdef HXCPP_VISIT_ALLOCS
   void VisitBlockAsync(hx::VisitContext *inCtx)
   {
      while(!sgThreadPoolAbort)
      {
         int blockId = _hx_atomic_add(&mThreadJobId, 1);
         if (blockId>=mAllBlocks.size())
            break;

         mAllBlocks[blockId]->VisitBlock(inCtx);
      }
   }
   #endif


   void ZeroAsync()
   {
      while(!sgThreadPoolAbort)
      {
         int zeroListId = _hx_atomic_add(&mThreadJobId, 1);
         if (zeroListId>=mZeroList.size())
            break;

         BlockDataInfo *info = mZeroList[zeroListId];
         info->tryZero();
      }
   }

   bool ZeroAsyncJit()
   {
      int spinCount = 0;
      while(!sgThreadPoolAbort)
      {
         if (mZeroListQueue>=sMaxZeroQueueSize)
         {
            spinCount++;
            if (spinCount<10000)
               // Spin
               continue;
            // Full for now, so sleep...
            return true;
         }
         spinCount = 0;

         // Look at next block...
         int zeroListId = _hx_atomic_add(&mThreadJobId, 1);
         if (zeroListId>=mZeroList.size())
         {
            // Done, so sleep...
            return true;
         }

         BlockDataInfo *info = mZeroList[zeroListId];
         if (info->tryZero())
         {
            // We zeroed it, so increase queue count
            _hx_atomic_add(&mZeroListQueue, 1);
            #ifdef PROFILE_THREAD_USAGE
            sThreadBlockZeroCount++;
            #endif
         }
      }

      return true;
   }

   // Try to maintain between sMinZeroQueueSize and sMaxZeroQueueSize pre-zeroed blocks
   void onZeroedBlockDequeued()
   {
      // Wake the thread?
      if (_hx_atomic_sub(&mZeroListQueue, 1)<sMinZeroQueueSize && !sRunningThreads)
      {
         if (mZeroListQueue + mThreadJobId < mZeroList.size())
         {
            // Wake zeroing thread
            ThreadPoolAutoLock l(sThreadPoolLock);
            if (!(sRunningThreads & 0x01))
            {
               #ifdef PROFILE_THREAD_USAGE
               sThreadZeroPokes++;
               #endif
               wakeThreadLocked(0);
            }
         }
      }
   }


   void finishThreadJob(int inId)
   {
      ThreadPoolAutoLock l(sThreadPoolLock);
      if (sRunningThreads & (1<<inId))
      {
         sRunningThreads &= ~(1<<inId);
         sLazyThreads = sRunningThreads != sAllThreads;

         if (!sRunningThreads)
            SignalThreadPool(sThreadJobDone,sThreadJobDoneSleeping);
      }
      else
      {
         printf("Finishe non-runnning thread?\n");
         DebuggerTrap();
      }
   }

   void waitForThreadWake(int inId)
   {
      #ifdef HX_GC_PTHREADS
      {
         ThreadPoolAutoLock l(sThreadPoolLock);
         int count = 0;

         // May be woken multiple times if sRunningThreads is set to 0 then 1 before we sleep
         sThreadSleeping[inId] = true;
         // Spurious wake?
         while( !(sRunningThreads & (1<<inId) ) )
            WaitThreadLocked(sThreadWake[inId]);
         sThreadSleeping[inId] = false;
      }
      #else
      while( !(sRunningThreads & (1<<inId) ) )
         sThreadWake[inId].Wait();
      #endif
   }


   void ThreadLoop(int inId)
   {
      hx::MarkContext context(inId);

      while(true)
      {
         waitForThreadWake(inId);

         #ifdef HX_GC_VERIFY
         if (! (sRunningThreads & (1<<inId)) )
            printf("Bad running threads!\n");
         #endif

         if (sgThreadPoolJob==tpjMark)
         {
            context.processMarkStack();
         }
         else if (sgThreadPoolJob==tpjAsyncZeroJit)
         {
            #ifdef PROFILE_THREAD_USAGE
            sThreadZeroWaits++;
            #endif
            if (ZeroAsyncJit())
               finishThreadJob(inId);
         }
         else
         {
            if (sgThreadPoolJob==tpjReclaimFull || sgThreadPoolJob==tpjReclaim)
               ReclaimAsync(sThreadBlockDataStats[inId]);

            else if (sgThreadPoolJob==tpjCountRows)
               CountAsync(sThreadBlockDataStats[inId]);

            else if (sgThreadPoolJob==tpjGetStats)
               GetStatsAsync(sThreadBlockDataStats[inId]);

            #ifdef HXCPP_VISIT_ALLOCS
            else if (sgThreadPoolJob==tpjVisitBlocks)
               VisitBlockAsync(sThreadVisitContext);
            #endif

            else if (sgThreadPoolJob==tpjAsyncZero)
               ZeroAsync();

            finishThreadJob(inId);
         }
      }
   }

   static THREAD_FUNC_TYPE SThreadLoop( void *inInfo )
   {
      sGlobalAlloc->ThreadLoop((int)(size_t)inInfo);
      THREAD_FUNC_RET;
   }

   void CreateWorker(int inId)
   {
      void *info = (void *)(size_t)inId;

      #ifdef HX_GC_PTHREADS
         pthread_cond_init(&sThreadWake[inId],0);
         sThreadSleeping[inId] = false;
         if (inId==0)
            pthread_cond_init(&sThreadJobDone,0);

         pthread_t result = 0;
         int created = pthread_create(&result,0,SThreadLoop,info);
         bool ok = created==0;
      #elif defined(EMSCRIPTEN)
         // Only one thread
      #else
         bool ok = HxCreateDetachedThread(SThreadLoop, info);
      #endif
   }

   void StopThreadJobs(bool inKill)
   {
      bool maybeExtraWake = false;

      if (sAllThreads)
      {
         if (inKill)
         {
            sgThreadPoolAbort = true;
            if (sgThreadPoolJob==tpjAsyncZeroJit)
            {
               ThreadPoolAutoLock l(sThreadPoolLock);
               // Thread will be waiting, but not finished
               if (sRunningThreads & 0x1)
               {
                  sgThreadPoolJob = tpjNone;
                  maybeExtraWake = true;
                  wakeThreadLocked(0);
               }
            }
         }


         #ifdef HX_GC_PTHREADS
         ThreadPoolAutoLock lock(sThreadPoolLock);
         sThreadJobDoneSleeping = true;
         while(sRunningThreads)
             WaitThreadLocked(sThreadJobDone);
         sThreadJobDoneSleeping = false;
         #else
         while(sRunningThreads)
            sThreadJobDone.Wait();
         #endif
         sgThreadPoolAbort = false;
         sAllThreads = 0;
         sgThreadPoolJob = tpjNone;
         sLazyThreads = 0;
      }
   }


   void StartThreadJobs(ThreadPoolJob inJob, int inWorkers, bool inWait, int inThreadLimit = -1)
   {
      mThreadJobId = 0;

      if (!inWorkers)
         return;

      if (!sThreadPoolInit)
      {
         sThreadPoolInit = true;
         for(int i=0;i<MAX_GC_THREADS;i++)
            CreateWorker(i);
      }

      #ifdef HX_GC_PTHREADS
      ThreadPoolAutoLock lock(sThreadPoolLock);
      #endif


      sgThreadPoolJob = inJob;

      sgThreadCount = inThreadLimit<0 ? MAX_GC_THREADS : std::min((int)MAX_GC_THREADS, inThreadLimit) ;

      int start = std::min(inWorkers, sgThreadCount );

      sAllThreads = (1<<sgThreadCount) - 1;

      sRunningThreads = (1<<start) - 1;

      sLazyThreads = sRunningThreads != sAllThreads;

      for(int i=0;i<start;i++)
         SignalThreadPool(sThreadWake[i],sThreadSleeping[i]);

      if (inWait)
      {
         // Join the workers...
         #ifdef HX_GC_PTHREADS
         sThreadJobDoneSleeping = true;
         while(sRunningThreads)
            WaitThreadLocked(sThreadJobDone);
         sThreadJobDoneSleeping = false;
         #else
         while(sRunningThreads)
            sThreadJobDone.Wait();
         #endif

         sAllThreads = 0;
         sgThreadPoolJob = tpjNone;
         sLazyThreads = 0;

         if (sRunningThreads)
         {
            printf("Bad thread stop %d\n", sRunningThreads);
            DebuggerTrap();
         }
      }
   }


   #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
   std::string formatBytes(size_t bytes)
   {
      size_t k = 1<<10;
      size_t meg = 1<<20;

      char strBuf[100];
      if (bytes<k)
         sprintf(strBuf,"%d", (int)bytes);
      else if (bytes<meg)
         sprintf(strBuf,"%.2fk", (double)bytes/k);
      else
         sprintf(strBuf,"%.2fmb", (double)bytes/meg);
      return strBuf;
   }
   #endif


   double tMarkInit;
   double tMarkLocal;
   double tMarkLocalEnd;
   double tMarked;
   void MarkAll(bool inGenerational)
   {
      if (!inGenerational)
      {
         hx::gPrevByteMarkID = hx::gByteMarkID;

         // The most-significant header byte looks like:
         // C nH Odd Even  c c c c
         //  C = "is const alloc bit" - in this case Odd and Even will be false
         // nH = non-hashed const string bit
         // Odd = true if cycle is odd
         // Even = true if cycle is even
         // c c c c = 4 bit cycle code
         //
         hx::gPrevMarkIdMask = ((~hx::gMarkID) & 0x30000000) | HX_GC_CONST_ALLOC_BIT;

         // 4 bits of cycle
         gByteMarkID = (gByteMarkID + 1) & 0x0f;
         if (gByteMarkID & 0x1)
            gByteMarkID |= 0x20;
         else
            gByteMarkID |= 0x10;

         hx::gMarkID = gByteMarkID << 24;
         hx::gMarkIDWithContainer = (gByteMarkID << 24) | IMMIX_ALLOC_IS_CONTAINER;
         gRememberedByteMarkID = gByteMarkID | HX_GC_REMEMBERED;

         #ifdef HX_WATCH
         GCLOG(" non-gen mark byte -> %02x\n", hx::gByteMarkID);
         #endif
         gBlockStack = 0;

         ClearRowMarks();
      }
      else
      {
         #ifdef HX_WATCH
         GCLOG(" generational mark byte -> %02x\n", hx::gByteMarkID);
         #endif
         ClearBlockMarks();
      }

      MEM_STAMP(tMarkInit);

      #ifdef PROFILE_THREAD_USAGE
      for(int i=-1;i<MAX_GC_THREADS;i++)
         sThreadChunkPushCount = sThreadChunkWakes = sThreadMarkCount[i] = sThreadArrayMarkCount[i] = 0;
      #endif



      mMarker.init();

      hx::MarkClassStatics(&mMarker);

      {
      hx::AutoMarkPush info(&mMarker,"Roots","root");

      for(hx::RootSet::iterator i = hx::sgRootSet.begin(); i!=hx::sgRootSet.end(); ++i)
      {
         hx::Object *&obj = **i;
         if (obj)
            hx::MarkObjectAlloc(obj , &mMarker );
      }

      if (hx::sgOffsetRootSet)
         for(hx::OffsetRootSet::iterator i = hx::sgOffsetRootSet->begin(); i!=hx::sgOffsetRootSet->end(); ++i)
         {
            char *ptr = *(char **)(i->first);
            int offset = i->second;
            hx::Object *obj = (hx::Object *)(ptr - offset);

            if (obj)
               hx::MarkObjectAlloc(obj , &mMarker );
         }
      } // automark

      #ifdef PROFILE_COLLECT
      hx::rootObjects = sObjectMarks;
      hx::rootAllocs = sAllocMarks;
      #endif


      {
      hx::AutoMarkPush info(&mMarker,"Zombies","zombie");
      // Mark zombies too....
      for(int i=0;i<hx::sZombieList.size();i++)
         hx::MarkObjectAlloc(hx::sZombieList[i] , &mMarker );
      } // automark

      MEM_STAMP(tMarkLocal);
      hx::localCount = 0;

      mMarker.isGenerational = inGenerational;

      // Mark local stacks
      for(int i=0;i<mLocalAllocs.size();i++)
         MarkLocalAlloc(mLocalAllocs[i] , &mMarker);

      #ifdef PROFILE_COLLECT
      hx::localObjects = sObjectMarks;
      hx::localAllocs = sAllocMarks;
      #endif

      MEM_STAMP(tMarkLocalEnd);

      #ifdef HX_MULTI_THREAD_MARKING
         mMarker.releaseJobs();

         // Unleash the workers...
         StartThreadJobs(tpjMark, MAX_GC_THREADS, true);
      #else
         mMarker.processMarkStack();
      #endif



      MEM_STAMP(tMarked);

      hx::FindZombies(mMarker);

      hx::RunFinalizers();

      #ifdef HX_GC_VERIFY
      for(int i=0;i<mAllBlocks.size();i++)
         mAllBlocks[i]->verify("After mark");
      #endif

      #ifdef HX_WATCH
      for(void **watch = hxWatchList; *watch; watch++)
      {
         GCLOG("********* Watch mark : %p %08x\n",*watch, ((unsigned int *)*watch)[-1]);
         GCLOG(" ******** is marked  : %d\n", (((unsigned char *)(*watch))[HX_ENDIAN_MARK_ID_BYTE]== gByteMarkID));
      }
      #endif
   }


   
   #ifdef HX_GC_VERIFY_ALLOC_START
   void verifyAllocStart()
   {
      for(int i=1;i<mAllBlocks.size();i++)
         mAllBlocks[i]->verifyAllocStart();
   }
   #endif

   #ifdef HX_GC_VERIFY
   void VerifyBlockOrder()
   {
      for(int i=1;i<mAllBlocks.size();i++)
      {
         if ( mAllBlocks[i-1]->mPtr >= mAllBlocks[i]->mPtr)
         {
            printf("Bad block order block[%d]=%p >= block[%d]=%p / %d\n", i-1, mAllBlocks[i-1]->mPtr,
                    i, mAllBlocks[i]->mPtr, mAllBlocks.size() );
            DebuggerTrap();
         }
      }
   }
   #endif

   void Collect(bool inMajor, bool inForceCompact, bool inLocked,bool inFreeIsFragged)
   {
      PROFILE_COLLECT_SUMMARY_START;

      #ifndef HXCPP_SINGLE_THREADED_APP
      // If we set the flag from 0 -> 0xffffffff then we are the collector
      //  otherwise, someone else is collecting at the moment - so wait...
      if (_hx_atomic_compare_exchange((volatile int *)&hx::gPauseForCollect, 0, 0xffffffff) != 0)
      {
         if (inLocked)
         {
            gThreadStateChangeLock->Unlock();

            hx::PauseForCollect();

            hx::EnterGCFreeZone();
            gThreadStateChangeLock->Lock();
            hx::ExitGCFreeZoneLocked();
         }
         else
         {
            hx::PauseForCollect();
         }
         return;
      }
      #endif

      STAMP(t0)

      // We are the collector - all must wait for us
      LocalAllocator *this_local = 0;
      #ifndef HXCPP_SINGLE_THREADED_APP
      this_local = (LocalAllocator *)(hx::ImmixAllocator *)hx::tlsStackContext;

      if (!inLocked)
         gThreadStateChangeLock->Lock();

      for(int i=0;i<mLocalAllocs.size();i++)
         if (mLocalAllocs[i]!=this_local)
            WaitForSafe(mLocalAllocs[i]);
      #endif

      sgIsCollecting = true;

      StopThreadJobs(true);
      #ifdef HXCPP_DEBUG
      sgAllocsSinceLastSpam = 0;
      #endif

      HX_STACK_FRAME("GC", "collect", 0, "GC::collect", __FILE__, __LINE__,0)
      #ifdef SHOW_MEM_EVENTS
      int here = 0;
      GCLOG("=== Collect === %p\n",&here);
      #endif


      #ifdef PROFILE_THREAD_USAGE
      GCLOG("Thread zero waits %d/%d/%d, misses=%d, hits=%d\n", sThreadZeroPokes, sThreadZeroWaits, mFreeBlocks.size(), sThreadZeroMisses, sThreadBlockZeroCount);
      sThreadZeroWaits = 0;
      sThreadZeroPokes = 0;
      sThreadZeroMisses = 0;
      sThreadBlockZeroCount = 0;
      #endif



      #ifdef HXCPP_TELEMETRY
      __hxt_gc_start();
      #endif

      size_t freeFraggedRows = 0; 
      if (inFreeIsFragged)
      {
         for(int i=mNextFreeBlockOfSize[0]; i<mFreeBlocks.size(); i++)
            freeFraggedRows += mFreeBlocks[i]->GetFreeRows();
      }

      // Now all threads have mTopOfStack & mBottomOfStack set.
      bool generational = false; 

      #ifdef HXCPP_GC_GENERATIONAL
      bool compactSurviors = false;

      if (sGcMode==gcmGenerational)
      {
         for(int i=0;i<mLocalAllocs.size();i++)
         {
            hx::StackContext *ctx = (hx::StackContext *)mLocalAllocs[i];
            if( ctx->mOldReferrers->count )
                hx::sGlobalChunks.addLocked( ctx->mOldReferrers );
            else
                hx::sGlobalChunks.free( ctx->mOldReferrers );
            ctx->mOldReferrers = 0;
         }
      }

      hx::QuickVec<hx::Object *> rememberedSet;
      generational = !inMajor && !inForceCompact && sGcMode == gcmGenerational;
      if (sGcMode==gcmGenerational)
      {
         hx::sGlobalChunks.copyPointers(rememberedSet,!generational);
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Patch remembered set marks %d\n", rememberedSet.size());
         #endif
         for(int i=0;i<rememberedSet.size();i++)
            ((unsigned char *)rememberedSet[i])[HX_ENDIAN_MARK_ID_BYTE] = gByteMarkID;
      }
      #endif

      STAMP(t1)

      MarkAll(generational);

      #ifdef HX_GC_VERIFY_GENERATIONAL
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("verify generational [\n");
         #endif
         sGcVerifyGenerational = true;
         sgTimeToNextTableUpdate--;
         MarkAll(false);
         sGcVerifyGenerational = false;
         #ifdef SHOW_MEM_EVENTS
         GCLOG("] verify generational\n");
         #endif
      }
      #endif

      STAMP(t2)


      // Sweep blocks

      // Update table entries?  This needs to be done before the gMarkID count clocks
      //  back to the same number
      if (!generational)
         sgTimeToNextTableUpdate--;

      bool full = inMajor || (sgTimeToNextTableUpdate<=0) || inForceCompact;

      // Setup memory target ...
      // Count free rows, and prep blocks for sorting
      BlockDataStats stats;

      /*
       This reduces the stall time, but adds a bit of background cpu usage
       Might be good to just countRows for non-generational too
      */
      if (!full && generational)
      {
         countRows(stats);
         size_t currentRows = stats.rowsInUse + stats.fraggedRows + freeFraggedRows;
         double filled = (double)(currentRows) / (double)(mAllBlocks.size()*IMMIX_USEFUL_LINES);
         if (filled>0.85)
         {
            // Failure of generational estimation
            int retained = currentRows - mRowsInUse;
            int space = mAllBlocks.size()*IMMIX_USEFUL_LINES - mRowsInUse;
            if (space<retained)
               space = retained;
            if (space<1)
               space = 1;
            mGenerationalRetainEstimate = (double)retained/(double)space;
            #ifdef SHOW_MEM_EVENTS
            GCLOG("Generational retention/fragmentation too high %f, do normal collect\n", mGenerationalRetainEstimate);
            #endif

            #ifdef HX_GC_VERIFY_ALLOC_START
            verifyAllocStart();
            #endif

            generational = false;
            MarkAll(generational);

            sgTimeToNextTableUpdate--;
            full = sgTimeToNextTableUpdate<=0;

            stats.clear();
            reclaimBlocks(full,stats);
         }
      }
      else
      {
         reclaimBlocks(full,stats);
      }


      #ifdef HXCPP_GC_GENERATIONAL
      if (compactSurviors)
      {
         MoveSurvivors(&rememberedSet);
      }
      #endif


      #ifdef HXCPP_GC_MOVING
      if (!full)
      {
         double useRatio = (double)(mRowsInUse<<IMMIX_LINE_BITS) / (sWorkingMemorySize);
         #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
            GCLOG("Row use ratio:%f\n", useRatio);
         #endif
         // Could be either expanding, or fragmented...
         if (useRatio>0.75)
         {
            #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
               GCLOG("Do full stats\n", useRatio);
            #endif
            full = true;
            stats.clear();
            reclaimBlocks(full,stats);
         }
      }
      #endif

      if (full)
      {
         #ifdef HXCPP_GC_MOVING
         sgTimeToNextTableUpdate = 7;
         #else
         sgTimeToNextTableUpdate = 15;
         #endif
      }

      size_t oldRowsInUse = mRowsInUse;
      mRowsInUse = stats.rowsInUse + stats.fraggedRows + freeFraggedRows;

      bool moved = false;

      #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
      GCLOG("Total memory : %s\n", formatBytes(GetWorkingMemory()).c_str());
      GCLOG("  reserved bytes : %s\n", formatBytes(mRowsInUse*IMMIX_LINE_LEN).c_str());
      if (full)
      {
         GCLOG("  active bytes   : %s\n", formatBytes(stats.bytesInUse).c_str());
         GCLOG("  active ratio   : %f\n", (double)stats.bytesInUse/( mRowsInUse*IMMIX_LINE_LEN));
         GCLOG("  fragged blocks : %d (%.1f%%)\n", stats.fraggedBlocks, stats.fraggedBlocks*100.0/mAllBlocks.size() );
         GCLOG("  fragged score  : %f\n", (double)stats.fragScore/mAllBlocks.size() );
      }
      GCLOG("  large size   : %s\n", formatBytes(mLargeAllocated).c_str());
      GCLOG("  empty blocks : %d (%.1f%%)\n", stats.emptyBlocks, stats.emptyBlocks*100.0/mAllBlocks.size());
      #endif

      size_t bytesInUse = mRowsInUse<<IMMIX_LINE_BITS;

      STAMP(t3)


      #ifdef HXCPP_TELEMETRY
      // Detect deallocations - TODO: add STAMP() ?
      __hxt_gc_after_mark(gByteMarkID, HX_ENDIAN_MARK_ID_BYTE);
      #endif

      // Sweep large

      // Manage recycle size ?
      //  clear old frames recycle objects
      int l2 = largeObjectRecycle.size();
      for(int i=0;i<largeObjectRecycle.size();i++)
         HxFree(largeObjectRecycle[i]);
      largeObjectRecycle.setSize(0);

      size_t recycleRemaining = 0;
      #ifdef RECYCLE_LARGE
      if (!inForceCompact)
         recycleRemaining = mLargeAllocForceRefresh;
      #endif

      int idx = 0;
      int l0 = mLargeList.size();
      while(idx<mLargeList.size())
      {
         unsigned int *blob = mLargeList[idx];
         if ( (blob[1] & IMMIX_ALLOC_MARK_ID) != hx::gMarkID )
         {
            unsigned int size = *blob;
            mLargeAllocated -= size;
            if (size < recycleRemaining)
            {
               recycleRemaining -= size;
               largeObjectRecycle.push(blob);
            }
            else
            {
               HxFree(blob);
            }

            mLargeList.qerase(idx);
         }
         else
            idx++;
      }

      int l1 = mLargeList.size();


      STAMP(t4)

      bool defragged = false;

      // Compact/Defrag?
      #if defined(HXCPP_GC_MOVING) && defined(HXCPP_VISIT_ALLOCS)
      if (full)
      {
         bool doRelease = false;

         if (inForceCompact)
            doRelease = true;
         else
         {
            size_t mem = mRowsInUse<<IMMIX_LINE_BITS;
            size_t targetFree = std::max((size_t)hx::sgMinimumFreeSpace, mem/100 * (size_t)hx::sgTargetFreeSpacePercentage );
            targetFree = std::min(targetFree, (size_t)sgMaximumFreeSpace );
            sWorkingMemorySize = std::max( mem + targetFree, (size_t)hx::sgMinimumWorkingMemory);

            size_t allMem = GetWorkingMemory();
            // 8 Meg too much?
            size_t allowExtra = std::max( (size_t)8*1024*1024, sWorkingMemorySize*5/4 );

            if ( allMem > sWorkingMemorySize + allowExtra )
            {
               #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
               int releaseGroups = (int)((allMem - sWorkingMemorySize) / (IMMIX_BLOCK_SIZE<<IMMIX_BLOCK_GROUP_BITS));
               if (releaseGroups)
                  GCLOG("Try to release %d groups\n", releaseGroups );
               #endif
               doRelease = true;
            }
         }


         bool isFragged = stats.fragScore > mAllBlocks.size()*FRAG_THRESH;
         if (doRelease || isFragged || hx::gAlwaysMove)
         {
            if (isFragged && sgTimeToNextTableUpdate>3)
               sgTimeToNextTableUpdate = 3;
            calcMoveOrder( );

            // Borrow some blocks to ensuure space to defrag into
            int workingBlocks = mAllBlocks.size()*3/2 - stats.emptyBlocks;
            int borrowed = 0;
            while( mAllBlocks.size()<workingBlocks )
            {
               bool dummy = false;
               if (!AllocMoreBlocks(dummy, true))
                  break;
               doRelease = true;
               borrowed++;
            }
            stats.emptyBlocks += borrowed;
            #if defined(SHOW_FRAGMENTATION)
               GCLOG("Borrowed %d groups for %d target blocks\n", borrowed, workingBlocks);
            #endif

            MoveBlockJob job(mAllBlocks);

            if (MoveBlocks(job,stats) || doRelease)
            {
               if (doRelease)
               {
                  size_t mem = mRowsInUse<<IMMIX_LINE_BITS;
                  size_t targetFree = std::max((size_t)hx::sgMinimumFreeSpace, bytesInUse/100 *hx::sgTargetFreeSpacePercentage );
                  targetFree = std::min(targetFree, (size_t)sgMaximumFreeSpace );
                  size_t targetMem = std::max( mem + targetFree, (size_t)hx::sgMinimumWorkingMemory) +
                                        (2<<(IMMIX_BLOCK_GROUP_BITS+IMMIX_BLOCK_BITS));

                  if (inForceCompact)
                     targetMem = 0;
                  size_t have = GetWorkingMemory();
                  if (targetMem<have)
                  {
                     size_t releaseSize = have - targetMem;
                     #ifdef SHOW_FRAGMENTATION
                     GCLOG(" Release %s bytes to leave %s\n", formatBytes(releaseSize).c_str(), formatBytes(targetMem).c_str() );
                     #endif

                     int releasedBlocks = releaseEmptyGroups(stats, releaseSize);
                     #ifdef SHOW_FRAGMENTATION
                     int releasedGroups = releasedBlocks >> IMMIX_BLOCK_GROUP_BITS;
                     GCLOG(" Released %s, %d groups\n", formatBytes((size_t)releasedBlocks<<IMMIX_BLOCK_BITS).c_str(), releasedGroups );
                     #endif
                  }
               }

               // Reduce sWorkingMemorySize now we have defragged
               #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
               GCLOG("After compacting---\n");
               GCLOG("  total memory : %s\n", formatBytes(GetWorkingMemory()).c_str() );
               GCLOG("  total needed : %s\n", formatBytes((size_t)mRowsInUse*IMMIX_LINE_LEN).c_str() );
               GCLOG("  for bytes    : %s\n", formatBytes(bytesInUse).c_str() );
               GCLOG("  empty blocks : %d (%.1f%%)\n", stats.emptyBlocks, stats.emptyBlocks*100.0/mAllBlocks.size());
               GCLOG("  fragged blocks : %d (%.1f%%)\n", stats.fraggedBlocks, stats.fraggedBlocks*100.0/mAllBlocks.size() );
               #endif
            }

            std::stable_sort(&mAllBlocks[0], &mAllBlocks[0] + mAllBlocks.size(), SortByBlockPtr );

            #ifdef HX_GC_VERIFY
            VerifyBlockOrder();
            #endif
         }
      }
      #endif


      STAMP(t5)

      size_t mem = mRowsInUse<<IMMIX_LINE_BITS;
      size_t baseMem = full ? bytesInUse : mem;
      #ifdef HXCPP_GC_DYNAMIC_SIZE
      size_t targetFree = std::max((size_t)hx::sgMinimumFreeSpace, (size_t)(baseMem * profileCollectSummary.spaceFactor ) );
      #else
      size_t targetFree = std::max((size_t)hx::sgMinimumFreeSpace, baseMem/100 *hx::sgTargetFreeSpacePercentage );
      #endif
      targetFree = std::min(targetFree, (size_t)sgMaximumFreeSpace );
      // Only adjust if non-generational
      if (!generational)
         sWorkingMemorySize = std::max( mem + targetFree, (size_t)hx::sgMinimumWorkingMemory);

      #if defined(SHOW_FRAGMENTATION) || defined(SHOW_MEM_EVENTS)
      GCLOG("Target memory %s, using %s\n",  formatBytes(sWorkingMemorySize).c_str(), formatBytes(mem).c_str() );
      #endif

      // Large alloc target
      int blockSize =  mAllBlocks.size()<<IMMIX_BLOCK_BITS;
      if (blockSize > mLargeAllocSpace)
         mLargeAllocSpace = blockSize;
      mLargeAllocForceRefresh = mLargeAllocated + mLargeAllocSpace;

      mTotalAfterLastCollect = MemUsage();

      #ifdef HXCPP_GC_GENERATIONAL
      if (generational)
      {
         // TODO - include large too?
         int retained = mRowsInUse - oldRowsInUse;
         int space = mAllBlocks.size()*IMMIX_USEFUL_LINES - oldRowsInUse;
         if (space<retained)
            space = retained;

         mGenerationalRetainEstimate = (double)retained/(double)space;
      }
      else
      {
         // move towards 0.2
         mGenerationalRetainEstimate += (0.2-mGenerationalRetainEstimate)*0.25;
      }

      double filled_ratio = (double)mRowsInUse/(double)(mAllBlocksCount*IMMIX_USEFUL_LINES);
      double after_gen = filled_ratio + (1.0-filled_ratio)*mGenerationalRetainEstimate;

      if (after_gen<0.75)
      {
         sGcMode = gcmGenerational;
      }
      else
      {
         sGcMode = gcmFull;
         // What was I thinking here?  This breaks #851
         //gByteMarkID |= 0x30;
      }

      #ifdef SHOW_MEM_EVENTS
      GCLOG("filled=%.2f%% + estimate = %.2f%% = %.2f%% -> %s\n",
            filled_ratio*100, mGenerationalRetainEstimate*100, after_gen*100, sGcMode==gcmFull?"Full":"Generational");
      #endif

      #endif

      createFreeList();

      // This saves some running/stall time, but increases the total CPU usage
      // Delaying it until just before the block is used to improve the cache locality
      backgroundProcessFreeList(true);

      mAllBlocksCount   = mAllBlocks.size();
      mCurrentRowsInUse = mRowsInUse;

      #ifdef SHOW_MEM_EVENTS
      GCLOG("Collect Done\n");
      #endif

      #ifdef PROFILE_COLLECT
      STAMP(t6)
      double period = t6-sLastCollect;
      sLastCollect=t6;
      GCLOG("Collect time %s total=%.2fms =%.1f%%\n  setup=%.2f\n  %s=%.2f(init=%.2f/roots=%.2f %d+%d/loc=%.2f*%d %d+%d/mark=%.2f %d+%d/fin=%.2f*%d/ids=%.2f)\n  reclaim=%.2f\n  large(%d->%d, recyc %d)=%.2f\n  defrag=%.2f\n",
               generational ? "gen" : "std",
              (t6-t0)*1000, (t6-t0)*100.0/period, // total %
              (t1-t0)*1000, // sync/setup
              generational ? "mark gen" : "mark", (t2-t1)*1000,
                      (tMarkInit-t1)*1000,
                      (tMarkLocal-tMarkInit)*1000,  hx::rootObjects, hx::rootAllocs,
                      (tMarkLocalEnd-tMarkLocal)*1000, hx::localCount, hx::localObjects-hx::rootObjects, hx::localAllocs-hx::rootAllocs,
                      (tMarked-tMarkLocalEnd)*1000, sObjectMarks-hx::localObjects, sAllocMarks-hx::localAllocs,
                      (hx::tFinalizers-tMarked)*1000, hx::finalizerCount,
                      (t2-hx::tFinalizers)*1000,
              (t3-t2)*1000, // reclaim
              l0, l1, l2, (t4-t3)*1000, // large
              (t5-t4)*1000 // defrag
              );
      sObjectMarks = sAllocMarks = 0;

      #endif

      #ifdef PROFILE_THREAD_USAGE
      GCLOG("Thread chunks:%d, wakes=%d\n", sThreadChunkPushCount, sThreadChunkWakes);
      for(int i=-1;i<MAX_GC_THREADS;i++)
        GCLOG(" thread %d] %d + %d\n", i, sThreadMarkCount[i], sThreadArrayMarkCount[i]);
      GCLOG("Locking spins  : %d\n", sSpinCount);
      sSpinCount = 0;
      #endif

      #ifdef HXCPP_GC_GENERATIONAL
      if (sGcMode==gcmGenerational)
         for(int i=0;i<mLocalAllocs.size();i++)
         {
            hx::StackContext *ctx = (hx::StackContext *)mLocalAllocs[i];
            ctx->mOldReferrers = hx::sGlobalChunks.alloc();
         }
      #endif


      #ifdef HX_GC_VERIFY
      VerifyBlockOrder();
      #endif

      for(int i=0;i<LOCAL_POOL_SIZE;i++)
      {
         LocalAllocator *l = mLocalPool[i];
         if (l)
            ClearPooledAlloc(l);
      }

      #ifdef HXCPP_TELEMETRY
      __hxt_gc_end();
      #endif

      sgIsCollecting = false;


      hx::gPauseForCollect = 0x00000000;
      #ifndef HXCPP_SINGLE_THREADED_APP
         for(int i=0;i<mLocalAllocs.size();i++)
         {
            #ifdef HXCPP_SCRIPTABLE
            ((hx::StackContext *)mLocalAllocs[i])->byteMarkId = hx::gByteMarkID;
            #endif
            if (mLocalAllocs[i]!=this_local)
               ReleaseFromSafe(mLocalAllocs[i]);
         }

         if (!inLocked)
            gThreadStateChangeLock->Unlock();
      #else
        #ifdef HXCPP_SCRIPTABLE
        hx::gMainThreadContext->byteMarkId = hx::gByteMarkID;
        #endif
      #endif


      PROFILE_COLLECT_SUMMARY_END;
   }

   void reclaimBlocks(bool full, BlockDataStats &outStats)
   {
      if (MAX_GC_THREADS>1)
      {
         for(int i=0;i<MAX_GC_THREADS;i++)
            sThreadBlockDataStats[i].clear();
         StartThreadJobs(full ? tpjReclaimFull : tpjReclaim, mAllBlocks.size(), true);
         outStats = sThreadBlockDataStats[0];
         for(int i=1;i<MAX_GC_THREADS;i++)
            outStats.add(sThreadBlockDataStats[i]);
      }
      else
      {
         outStats.clear();
         for(int i=0;i<mAllBlocks.size();i++)
         {
            if (full)
               mAllBlocks[i]->reclaim<true>(&outStats);
            else
               mAllBlocks[i]->reclaim<false>(&outStats);
         }
      }
   }


   void countRows(BlockDataStats &outStats)
   {
      if (MAX_GC_THREADS>1)
      {
         for(int i=0;i<MAX_GC_THREADS;i++)
            sThreadBlockDataStats[i].clear();
         StartThreadJobs(tpjCountRows, mAllBlocks.size(), true);
         outStats = sThreadBlockDataStats[0];
         for(int i=1;i<MAX_GC_THREADS;i++)
            outStats.add(sThreadBlockDataStats[i]);
      }
      else
      {
         outStats.clear();
         for(int i=0;i<mAllBlocks.size();i++)
            mAllBlocks[i]->countRows(outStats);
      }
   }


   // buils mFreeBlocks and maybe starts the async-zero process on mZeroList
   void createFreeList()
   {
      mFreeBlocks.clear();

      for(int i=0;i<mAllBlocks.size();i++)
      {
         BlockDataInfo *info = mAllBlocks[i];
         if (info->GetFreeRows() > 0 && info->mMaxHoleSize>256)
         {
            info->mOwned = false;
            mFreeBlocks.push(info);
         }
      }

      int extra = std::max( mAllBlocks.size(), 8<<IMMIX_BLOCK_GROUP_BITS);
      mFreeBlocks.safeReserveExtra(extra);

      std::sort(&mFreeBlocks[0], &mFreeBlocks[0] + mFreeBlocks.size(), SmallestFreeFirst );

      for(int i=0;i<BLOCK_OFSIZE_COUNT;i++)
         mNextFreeBlockOfSize[i] = mFreeBlocks.size();

      for(int i=mFreeBlocks.size()-1;i>=0;i--)
      {
         int slot = mFreeBlocks[i]->mMaxHoleSize >> IMMIX_LINE_BITS;
         if (slot>=BLOCK_OFSIZE_COUNT)
            slot = BLOCK_OFSIZE_COUNT-1;
         mNextFreeBlockOfSize[slot] = i;
      }

      for(int i=BLOCK_OFSIZE_COUNT-2;i>=0;i--)
         if (mNextFreeBlockOfSize[i]>mNextFreeBlockOfSize[i+1])
            mNextFreeBlockOfSize[i] = mNextFreeBlockOfSize[i+1];

      mZeroList.clear();
   }

   void backgroundProcessFreeList(bool inJit)
   {
      mZeroListQueue = 0;
      #ifdef HX_GC_ZERO_EARLY
      mZeroList.setSize(mFreeBlocks.size());
      memcpy( &mZeroList[0], &mFreeBlocks[0], mFreeBlocks.size()*sizeof(void *));

      StartThreadJobs(tpjAsyncZero, mZeroList.size(),true);
      #else
      if ( MAX_GC_THREADS>1 && mFreeBlocks.size()>4)
      {
         mZeroList.setSize(mFreeBlocks.size());
         memcpy( &mZeroList[0], &mFreeBlocks[0], mFreeBlocks.size()*sizeof(void *));

         // Only use one thread for parallel zeroing.  Try to get though the work wihout
         //  slowing down the main thread
         StartThreadJobs(inJit ? tpjAsyncZeroJit : tpjAsyncZero, mZeroList.size(), false, 1);
      }
      #endif
   }




   size_t MemLarge()
   {
      return mLargeAllocated;
   }

   size_t MemReserved()
   {
      return mLargeAllocated + (mAllBlocksCount*IMMIX_USEFUL_LINES<<IMMIX_LINE_BITS);
   }

   size_t MemCurrent()
   {
      return mLargeAllocated + (mCurrentRowsInUse<<IMMIX_LINE_BITS);
   }

   size_t MemUsage()
   {
      return mLargeAllocated + (mRowsInUse<<IMMIX_LINE_BITS);
   }

   bool IsAllBlock(BlockData *block)
   {
      if (mAllBlocks.size())
      {
         int min = 0;
         int max = mAllBlocks.size()-1;
         if (block==mAllBlocks[0]->mPtr)
            return true;
         if (block==mAllBlocks[max]->mPtr)
            return true;
         if (block>mAllBlocks[0]->mPtr && block<mAllBlocks[max]->mPtr)
         {
            while(min<max-1)
            {
               int mid = (max+min)>>1;
               if (mAllBlocks[mid]->mPtr==block)
                  return true;

               if (mAllBlocks[mid]->mPtr<block)
                  min = mid;
               else
                  max = mid;
            }
         }
      }
      return false;
   }

   MemType GetMemType(void *inPtr)
   {
      BlockData *block = (BlockData *)( ((size_t)inPtr) & IMMIX_BLOCK_BASE_MASK);

      bool isBlock = IsAllBlock(block);
      /*
      bool found = false;
      for(int i=0;i<mAllBlocks.size();i++)
      {
         if (mAllBlocks[i]==block)
         {
            found = true;
            break;
         }
      }
      */

      if (isBlock)
         return memBlock;

      for(int i=0;i<mLargeList.size();i++)
      {
         unsigned int *blob = mLargeList[i] + 2;
         if (blob==inPtr)
            return memLarge;
      }

      return memUnmanaged;
   }


   size_t mRowsInUse;
   size_t mCurrentRowsInUse;
   size_t mLargeAllocSpace;
   size_t mLargeAllocForceRefresh;
   size_t mLargeAllocated;
   size_t mTotalAfterLastCollect;
   size_t mAllBlocksCount;
   double mGenerationalRetainEstimate;

   hx::MarkContext mMarker;

   volatile int mNextFreeBlockOfSize[BLOCK_OFSIZE_COUNT];
   volatile int mThreadJobId;

   BlockList mAllBlocks;
   BlockList mFreeBlocks;
   BlockList mZeroList;
   volatile int mZeroListQueue;

   LargeList mLargeList;
   HxMutex    mLargeListLock;
   hx::QuickVec<LocalAllocator *> mLocalAllocs;
   LocalAllocator *mLocalPool[LOCAL_POOL_SIZE];
   hx::QuickVec<unsigned int *> largeObjectRecycle;
};



namespace hx
{

MarkChunk *MarkChunk::swapForNew()
{
   return sGlobalChunks.pushJobNoWake(this);
}



void MarkConservative(int *inBottom, int *inTop,hx::MarkContext *__inCtx)
{
   #ifdef VERIFY_STACK_READ
   VerifyStackRead(inBottom, inTop);
   #endif

   #ifdef SHOW_MEM_EVENTS
   GCLOG("Mark conservative %p...%p (%d) [...", inBottom, inTop, (int)(inTop-inBottom) );
     #ifdef HX_WATCH
     GCLOG("\n");
     #endif
   #endif

   #ifdef HXCPP_STACK_UP
   int *start = inTop-1;
   inTop = inBottom+1;
   inBottom = start;
   #endif

   if (sizeof(int)==4 && sizeof(void *)==8)
   {
      // Can't start pointer on last integer boundary...
      inTop--;
   }

   void *prev = 0;
   void *lastPin = 0;
   #ifdef HX_WATCH
   void *lastWatch = 0;
   bool isWatch = false;
   #endif

   #ifdef HXCPP_GC_GENERATIONAL
   // If this is a generational mark, then the byte marker has not been increased.
   // Previous mark Ids are therfore from more than 1 collection ago
   bool allowPrevious = !__inCtx->isGenerational;
   #else
   const bool allowPrevious = true;
   #endif


   for(int *ptr = inBottom ; ptr<inTop; ptr++)
   {
      void *vptr = *(void **)ptr;

      MemType mem;
      if (vptr && !((size_t)vptr & 0x03) && vptr!=prev && vptr!=lastPin)
      {

         #ifdef PROFILE_COLLECT
         hx::localCount++;
         #endif
         MemType mem = sGlobalAlloc->GetMemType(vptr);

         #ifdef HX_WATCH
         isWatch = false;
         if (hxInWatchList(vptr) && vptr!=lastWatch)
         {
            isWatch = true;
            lastWatch = vptr;
            GCLOG("********* Watch location conservative mark %p:%d\n",vptr,mem);
         }
         #endif

         if (mem!=memUnmanaged)
         {
            if (mem==memLarge)
            {
               unsigned char &mark = ((unsigned char *)(vptr))[HX_ENDIAN_MARK_ID_BYTE];
               if (mark!=gByteMarkID)
                  mark = gByteMarkID;
            }
            else
            {
               BlockData *block = (BlockData *)( ((size_t)vptr) & IMMIX_BLOCK_BASE_MASK);
               BlockDataInfo *info = (*gBlockInfo)[block->mId];

               int pos = (int)(((size_t)vptr) & IMMIX_BLOCK_OFFSET_MASK);
               AllocType t = sgCheckInternalOffset ?
                     info->GetEnclosingAllocType(pos-sizeof(int),&vptr, allowPrevious):
                     info->GetAllocType(pos-sizeof(int), allowPrevious);

               #ifdef HX_WATCH
               if (!isWatch && hxInWatchList(vptr))
               {
                  isWatch = true;
                  GCLOG("********* Watch location conservative mark offset %p:%d\n",vptr,mem);
               }
               #endif


               if ( t==allocObject )
               {
                  #ifdef HX_WATCH
                  if (isWatch)
                  {
                     GCLOG(" Mark object %p (%p)\n", vptr,ptr);
                  }
                  #endif
                  hx::MarkObjectAlloc( ((hx::Object *)vptr), __inCtx );
                  lastPin = vptr;
                  info->pin();
               }
               else if (t==allocString)
               {
                  #ifdef HX_WATCH
                  if (isWatch)
                     GCLOG(" Mark string %p (%p)\n", vptr,ptr);
                  #endif
                  HX_MARK_STRING(vptr);
                  lastPin = vptr;
                  info->pin();
               }
               else if (t==allocMarked)
               {
                  #ifdef HX_WATCH
                  if (isWatch)
                     GCLOG(" pin alloced %p (%p)\n", vptr,ptr);
                  #endif
                  lastPin = vptr;
                  info->pin();
               }
               #ifdef HX_WATCH
               else // memBlock
               {
                  if (isWatch)
                  {
                     GCLOG(" missed watch %p:%d\n", vptr,t);
                     int x = info->GetAllocType(pos-sizeof(int),allowPrevious);
                     int y = info->GetEnclosingAllocType(pos-sizeof(int),&vptr,allowPrevious);
                     #ifdef HXCPP_GC_NURSERY
                     void *nptr;
                     int z = info->GetEnclosingNurseryType(pos-sizeof(int),&nptr);
                     #else
                     int z = 0;
                     #endif
                     printf("but got alloc type=%d, enclosing=%d nurs=%d o=%d\n",x,y,z,sgCheckInternalOffset);
                  }
               }
               #endif
            }
         }
         // GCLOG(" rejected %p %p %d %p %d=%d\n", ptr, vptr, !((size_t)vptr & 0x03), prev,
         // sGlobalAlloc->GetMemType(vptr) , memUnmanaged );
      }
   }
   #ifdef SHOW_MEM_EVENTS
   GCLOG("...]\n");
   #endif
}

} // namespace hx


// --- LocalAllocator -------------------------------------------------------
//
// One per thread ...

static int sFragIgnore=0;

class LocalAllocator : public hx::StackContext
{
   int            mCurrentHole;
   int            mCurrentHoles;
   HoleRange     *mCurrentRange;
   int           *mFraggedRows;

   bool           mMoreHoles;

   int *mTopOfStack;
   int *mBottomOfStack;

   hx::RegisterCaptureBuffer mRegisterBuf;
   int                   mRegisterBufSize;

   #ifndef HXCPP_SINGLE_THREADED_APP
   bool            mGCFreeZone;
   HxSemaphore     mReadyForCollect;
   HxSemaphore     mCollectDone;
   #endif

   int             mID;


   // Must be called locked
   ~LocalAllocator()
   {
   }

public:
   bool            mGlobalStackLock;
   int             mStackLocks;

public:
   LocalAllocator(int *inTopOfStack=0)
   {
      Reset();

      #ifdef HXCPP_GC_GENERATIONAL
      mOldReferrers = 0;
      #endif

      AttachThread(inTopOfStack);
   }


   void AttachThread(int *inTopOfStack)
   {
      mTopOfStack = mBottomOfStack = inTopOfStack;

      mRegisterBufSize = 0;
      mStackLocks = 0;
      mGlobalStackLock = false;
      #ifdef HX_WINDOWS
      mID = GetCurrentThreadId();
      #endif

      #ifdef HXCPP_GC_GENERATIONAL
      if (mOldReferrers)
      {
         GCLOG("Uncleaned referrers\n");
      }

      if (sGcMode==gcmGenerational)
         mOldReferrers = hx::sGlobalChunks.alloc();
      else
         mOldReferrers = 0;
      #endif


      // It is in the free zone - wait for 'SetTopOfStack' to activate
      #ifndef HXCPP_SINGLE_THREADED_APP
      mGCFreeZone = true;
      mReadyForCollect.Set();
      #endif
      sGlobalAlloc->AddLocal(this);
   }

   void Release()
   {
      mStackLocks = 0;

      onThreadDetach();

      #ifndef HXCPP_SINGLE_THREADED_APP
      if (!mGCFreeZone)
         EnterGCFreeZone();
      #endif

      AutoLock lock(*gThreadStateChangeLock);

      #ifdef HX_WINDOWS
      mID = 0;
      #endif

      #ifdef HXCPP_GC_GENERATIONAL
      if (mOldReferrers)
      {
         if ( mOldReferrers->count )
            hx::sGlobalChunks.pushJob( mOldReferrers, false );
         else
            hx::sGlobalChunks.free( mOldReferrers );
         mOldReferrers = 0;
      }
      #endif

      mTopOfStack = mBottomOfStack = 0;

      sGlobalAlloc->RemoveLocalLocked(this);

      hx::tlsStackContext = 0;

      if (!sGlobalAlloc->ReturnToPoolLocked(this))
         delete this;
   }

   void Reset()
   {
      allocBase = 0;
      mCurrentHole = 0;
      mCurrentHoles = 0;
      mFraggedRows = 0;
      #ifdef HXCPP_GC_NURSERY
      spaceFirst = 0;
      spaceOversize = 0;
      #else
      spaceEnd = 0;
      spaceStart = 0;
      #endif
      mMoreHoles = false;
   }

   // The main of haxe calls SetTopOfStack(top,false)
   //   via hxcpp_set_top_of_stack or HX_TOP_OF_STACK in HxcppMain.cpp/Macros.h
   // The means "register current thread indefinitely"
   // Other places may call this to ensure the the current thread is registered
   //  indefinitely (until forcefully revoked)
   //
   // Normally liraries/mains will then let this dangle.
   //
   // However after the main, on android it calls SetTopOfStack(0,true), to unregister the thread,
   // because it is likely to be the ui thread, and the remaining call will be from
   // callbacks from the render thread.
   // 
   // When threads want to attach temporarily, they will call
   // gc_set_top_of_stack(top,true)
   //  -> SetTopOfStack(top,true)
   //    do stuff...
   // gc_set_top_of_stack(0,true)
   //  -> SetTopOfStack(0,true)
   //
   //  OR
   //
   //  PushTopOfStack(top)
   //   ...
   //  PopTopOfStack
   //
   //  However, we really want the gc_set_top_of_stack(top,true) to allow recursive locks so:
   //
   //  SetTopOfStack(top,false) -> ensure global stack lock exists
   //  SetTopOfStack(top,true) -> add stack lock
   //  SetTopOfStack(0,_) -> pop stack lock. If all gone, clear global stack lock
   //
   void SetTopOfStack(int *inTop,bool inPush)
   {
      if (inTop)
      {
         if (!mTopOfStack)
            mTopOfStack = inTop;
         // EMSCRIPTEN the stack grows upwards
         // It could be that the main routine was called from deep with in the stack,
         //  then some callback was called from a higher location on the stack
         #ifdef HXCPP_STACK_UP
         else if (inTop < mTopOfStack)
            mTopOfStack = inTop;
         #else
         else if (inTop > mTopOfStack)
            mTopOfStack = inTop;
         #endif

         if (inPush)
            mStackLocks++;
         else
            mGlobalStackLock = true;

         #ifndef HXCPP_SINGLE_THREADED_APP
         if (mGCFreeZone)
            ExitGCFreeZone();
         #endif
      }
      else
      {
         if (mStackLocks>0)
            mStackLocks--;
         else
            mGlobalStackLock = false;

         if (!mStackLocks && !mGlobalStackLock)
         {
            Release();
         }
      }

      #ifdef VerifyStackRead
      VerifyStackRead(mBottomOfStack, mTopOfStack)
      #endif
   }


   void PushTopOfStack(void *inTop)
   {
      SetTopOfStack((int *)inTop,true);
   }


   void PopTopOfStack()
   {
      mStackLocks--;
      if (mStackLocks<=0 && !mGlobalStackLock)
      {
         Release();
      }
   }



   void SetBottomOfStack(int *inBottom)
   {
      mBottomOfStack = inBottom;
      #ifdef VerifyStackRead
      VerifyStackRead(mBottomOfStack, mTopOfStack)
      #endif
   }

   virtual void SetupStackAndCollect(bool inMajor, bool inForceCompact, bool inLocked=false,bool inFreeIsFragged=false)
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
        #if HXCPP_DEBUG
        if (mGCFreeZone)
           CriticalGCError("Collecting from a GC-free thread");
        #endif
      #endif

      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;

      CAPTURE_REGS;

      if (!mTopOfStack)
         mTopOfStack = mBottomOfStack;
      // EMSCRIPTEN the stack grows upwards
      #ifdef HXCPP_STACK_UP
      if (mBottomOfStack < mTopOfStack)
         mTopOfStack = mBottomOfStack;
      #else
      if (mBottomOfStack > mTopOfStack)
         mTopOfStack = mBottomOfStack;
      #endif

      #ifdef VerifyStackRead
      VerifyStackRead(mBottomOfStack, mTopOfStack)
      #endif


      sGlobalAlloc->Collect(inMajor, inForceCompact, inLocked, inFreeIsFragged);
   }


   void PauseForCollect()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      CAPTURE_REGS;
      #ifdef VerifyStackRead
      VerifyStackRead(mBottomOfStack, mTopOfStack)
      #endif

      if (sgIsCollecting)
         CriticalGCError("Bad Allocation while collecting - from finalizer?");

      mReadyForCollect.Set();
      mCollectDone.Wait();
      #endif
   }

   void EnterGCFreeZone()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      if (mTopOfStack)
      {
         CAPTURE_REGS;
      }
      #ifdef VerifyStackRead
      VerifyStackRead(mBottomOfStack, mTopOfStack)
      #endif

      mGCFreeZone = true;
      mReadyForCollect.Set();
      #endif
   }

   bool TryGCFreeZone()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      if (mGCFreeZone)
         return false;
      EnterGCFreeZone();
      #endif
      return true;
   }

   bool TryExitGCFreeZone()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      if (!mGCFreeZone)
         return false;
      ExitGCFreeZone();
      return true;
      #endif
      return false;
   }


   void ExitGCFreeZone()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      if (!mGCFreeZone)
         CriticalGCError("GCFree Zone mismatch");

      AutoLock lock(*gThreadStateChangeLock);
      mReadyForCollect.Reset();
      mGCFreeZone = false;
      #endif
   }
        // For when we already hold the lock
   void ExitGCFreeZoneLocked()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      mReadyForCollect.Reset();
      mGCFreeZone = false;
      #endif
   }



   // Called by the collecting thread to make sure this allocator is paused.
   // The collecting thread has the lock, and will not be releasing it until
   //  it has finished the collect.
   //
   //  The mGCFreeZone is set without a lock in the EnterGCFreeZone code, and then
   //   mReadyForCollect is set.  So it is possible the mGCFreeZone check may or may not
   //   trigger.  If this call happens first, mGCFreeZone will be zero, and mReadyForCollect
   //   will wait.  By this time mGCFreeZone will be set and the next call not check
   //   mReadyForCollect again.  If this one happens later, it is possible mReadyForCollect
   //   will not be waited on. mReadyForCollect will be cleared when the zone is left.
   //
   //  The mMoreHoles/spaceOversize/spaceEnd get zeroed without a lock.  The timing should
   //   not be critical since the allocation code shold expect that these are volatile.
   //   If the allocation works, all is good.  If it fails then the collection will happed soon.
   #if defined(__has_feature)
     #if __has_feature(thread_sanitizer)
      __attribute__((no_sanitize("thread")))
     #endif
   #endif
   void WaitForSafe()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      if (!mGCFreeZone)
      {
         // Cause allocation routines to fail ...
         mMoreHoles = false;
         #ifdef HXCPP_GC_NURSERY
         spaceOversize = 0;
         #else
         spaceEnd = 0;
         #endif
         mReadyForCollect.Wait();
      }
      #endif
   }

   void ReleaseFromSafe()
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      if (!mGCFreeZone)
         mCollectDone.Set();
      #endif
   }

   void ExpandAlloc(int &ioSize)
   {
      #ifdef HXCPP_GC_NURSERY
      int spaceStart = spaceFirst - allocBase - 4;
      int spaceEnd = spaceOversize - allocBase - 4;
      #endif


      int size = ioSize + sizeof(int);
      #ifdef HXCPP_ALIGN_ALLOC
      // If we start in even-int offset, we need to skip 8 bytes to get alloc on even-int
      if (size+spaceStart>spaceEnd || !(spaceStart & 7))
         size += 4;
      #endif
      int end = spaceStart + size;
      if (end <= spaceEnd)
      {
         int linePad = IMMIX_LINE_LEN - (end & (IMMIX_LINE_LEN-1));
         if (linePad>0 && linePad<=64)
            ioSize += linePad;
      }
   }


   void *CallAlloc(int inSize,unsigned int inObjectFlags)
   {
      #ifndef HXCPP_SINGLE_THREADED_APP
      #if HXCPP_DEBUG
      if (mGCFreeZone)
         CriticalGCError("Allocating from a GC-free thread");
      #endif
      if (hx::gPauseForCollect)
         PauseForCollect();
      #endif

      if (inSize==0)
         return hx::emptyAlloc;

      #if defined(HXCPP_VISIT_ALLOCS) && defined(HXCPP_M64)
      // Make sure we can fit a relocation pointer
      int allocSize = sizeof(int) + std::max(8,inSize);
      #else
      int allocSize = sizeof(int) + inSize;
      #endif

      #ifdef HXCPP_ALIGN_ALLOC
      // If we start in even-int offset, we need to skip 8 bytes to get alloc on even-int
      int skip4 = allocSize+spaceStart>spaceEnd || !(spaceStart & 7) ? 4 : 0;
      #else
      enum { skip4 = 0 };
      #endif

      #if HXCPP_GC_DEBUG_LEVEL>0
      if (inSize & 3) DebuggerTrap();
      #endif

      while(1)
      {
         #ifdef HXCPP_GC_NURSERY
            unsigned char *buffer = spaceFirst;
            unsigned char *end = buffer + allocSize;

            if ( end <= spaceOversize )
            {
               spaceFirst = end;

               int size = allocSize - 4;
               ((unsigned int *)buffer)[-1] = size | inObjectFlags;

               #if defined(HXCPP_GC_CHECK_POINTER) && defined(HXCPP_GC_DEBUG_ALWAYS_MOVE)
               hx::GCOnNewPointer(buffer);
               #endif

               return buffer;
            }
            // spaceOversize might have been set to zero for quick-termination of alloc.
            unsigned char *s = spaceOversize;
            if (s>spaceFirst && mFraggedRows)
               *mFraggedRows += (s - spaceFirst)>>IMMIX_LINE_BITS;
         #else
            int end = spaceStart + allocSize + skip4;
            if (end <= spaceEnd)
            {
               #ifdef HXCPP_ALIGN_ALLOC
               spaceStart += skip4;
               #endif

               unsigned int *buffer = (unsigned int *)(allocBase + spaceStart);

               int startRow = spaceStart>>IMMIX_LINE_BITS;
               allocStartFlags[ startRow ] |= hx::gImmixStartFlag[spaceStart &127];

               int endRow = (end+(IMMIX_LINE_LEN-1))>>IMMIX_LINE_BITS;

               *buffer++ = inObjectFlags | hx::gMarkID |
                     (inSize<<IMMIX_ALLOC_SIZE_SHIFT) | (endRow-startRow);

               spaceStart = end;

               #if defined(HXCPP_GC_CHECK_POINTER) && defined(HXCPP_GC_DEBUG_ALWAYS_MOVE)
               hx::GCOnNewPointer(buffer);
               #endif

               return buffer;
            }
            if (mFraggedRows)
            {
               int frag = spaceEnd-spaceStart;
               if (frag>0)
                  *mFraggedRows += frag>>IMMIX_LINE_BITS;
            }
         #endif


         if (mMoreHoles)
         {
            #ifdef HXCPP_GC_NURSERY
            spaceFirst = allocBase + mCurrentRange[mCurrentHole].start + sizeof(int);
            spaceOversize = spaceFirst + mCurrentRange[mCurrentHole].length;
            #else
            spaceStart = mCurrentRange[mCurrentHole].start;
            spaceEnd = spaceStart + mCurrentRange[mCurrentHole].length;
            #endif
            mCurrentHole++;
            mMoreHoles = mCurrentHole<mCurrentHoles;

         }
         else
         {
            // For opmtimized windows 64 builds, this dummy var technique does not
            //  quite work, since the compiler might recycle one of the earlier stack
            //  slots, and place dummy behind the stack values we are actually trying to
            //  capture.  Moving the dummy into the GetFreeBlock seems to have fixed this.
            //  Not 100% sure this is the best answer, but it is working.
            //volatile int dummy = 1;
            //mBottomOfStack = (int *)&dummy;
            //CAPTURE_REGS;

            BlockDataInfo *info = sGlobalAlloc->GetFreeBlock(allocSize,this);

            allocBase = (unsigned char *)info->mPtr;
            mCurrentRange = info->mRanges;
            allocStartFlags = info->allocStart;
            mCurrentHoles = info->mHoles;
            mFraggedRows = &info->mFraggedRows;
            #ifdef HXCPP_GC_NURSERY
            spaceFirst = allocBase + mCurrentRange->start + sizeof(int);
            spaceOversize = spaceFirst + mCurrentRange->length;
            #else
            spaceStart = mCurrentRange->start;
            spaceEnd = spaceStart + mCurrentRange->length;
            #endif
            mCurrentHole = 1;
            mMoreHoles = mCurrentHole<mCurrentHoles;
         }

         // Other thread may have started collect, in which case we may just
         //  overwritted the 'mMoreHoles' and 'spaceEnd' termination attempt
         if (hx::gPauseForCollect)
         {
            mMoreHoles = 0;
            #ifdef HXCPP_GC_NURSERY
            spaceOversize = 0;
            #else
            spaceEnd = 0;
            #endif
         }
      }
      return 0;
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void Visit(hx::VisitContext *__inCtx)
   {
      #ifdef HXCPP_COMBINE_STRINGS
      if (stringSet)
      {
         __inCtx->visitObject( (hx::Object **)&stringSet);
      }
      #endif
   }
   #endif


   void Mark(hx::MarkContext *__inCtx)
   {
      if (!mTopOfStack)
      {
         Reset();
         return;
      }

      #ifdef SHOW_MEM_EVENTS
      //int here = 0;
      //GCLOG("=========== Mark Stack ==================== %p ... %p (%p)\n",mBottomOfStack,mTopOfStack,&here);
      #endif

      #ifdef HXCPP_DEBUG
         MarkPushClass("Stack",__inCtx);
         MarkSetMember("Stack",__inCtx);
         if (mTopOfStack && mBottomOfStack)
            hx::MarkConservative(mBottomOfStack, mTopOfStack , __inCtx);
         #ifdef HXCPP_SCRIPTABLE
            MarkSetMember("ScriptStack",__inCtx);
            hx::MarkConservative((int *)(stack), (int *)(pointer),__inCtx);
         #endif
         MarkSetMember("Registers",__inCtx);
         hx::MarkConservative(CAPTURE_REG_START, CAPTURE_REG_END, __inCtx);
         MarkPopClass(__inCtx);
      #else
         if (mTopOfStack && mBottomOfStack)
            hx::MarkConservative(mBottomOfStack, mTopOfStack , __inCtx);
         hx::MarkConservative(CAPTURE_REG_START, CAPTURE_REG_END, __inCtx);
         #ifdef HXCPP_SCRIPTABLE
            hx::MarkConservative((int *)(stack), (int *)(pointer),__inCtx);
         #endif
      #endif

      #ifdef HXCPP_COMBINE_STRINGS
         if (stringSet)
            MarkMember( *(hx::Object **)&stringSet, __inCtx);
      #endif

      Reset();

   }

};




inline LocalAllocator *GetLocalAlloc(bool inAllowEmpty=false)
{
   #ifndef HXCPP_SINGLE_THREADED_APP
      LocalAllocator *result = (LocalAllocator *)(hx::ImmixAllocator *)hx::tlsStackContext;
      if (!result && !inAllowEmpty)
         hx::BadImmixAlloc();

      return result;
   #else
      return (LocalAllocator *)hx::gMainThreadContext;
   #endif
}

void WaitForSafe(LocalAllocator *inAlloc)
{
   inAlloc->WaitForSafe();
}

void ReleaseFromSafe(LocalAllocator *inAlloc)
{
   inAlloc->ReleaseFromSafe();
}

void MarkLocalAlloc(LocalAllocator *inAlloc,hx::MarkContext *__inCtx)
{
   inAlloc->Mark(__inCtx);
}

void ClearPooledAlloc(LocalAllocator *inAlloc)
{
   inAlloc->Reset();
}

#ifdef HXCPP_VISIT_ALLOCS
void VisitLocalAlloc(LocalAllocator *inAlloc,hx::VisitContext *__inCtx)
{
   inAlloc->Visit(__inCtx);
}
#endif



void CollectFromThisThread(bool inMajor,bool inForceCompact)
{
   LocalAllocator *la = GetLocalAlloc();
   la->SetupStackAndCollect(inMajor,inForceCompact);
}

namespace hx
{

void *ImmixAllocator::CallAlloc(int inSize,unsigned int inObjectFlags)
{
   return reinterpret_cast<LocalAllocator *>(this)->CallAlloc(inSize, inObjectFlags);
}



void PauseForCollect()
{
   GetLocalAlloc()->PauseForCollect();
}



void EnterGCFreeZone()
{
   #ifndef HXCPP_SINGLE_THREADED_APP
      LocalAllocator *tla = GetLocalAlloc();
      tla->EnterGCFreeZone();
   #endif
}


bool TryGCFreeZone()
{
   #ifndef HXCPP_SINGLE_THREADED_APP
      LocalAllocator *tla = GetLocalAlloc();
      return tla->TryGCFreeZone();
   #else
      return false;
   #endif
}

bool TryExitGCFreeZone()
{
   #ifndef HXCPP_SINGLE_THREADED_APP
      LocalAllocator *tla = GetLocalAlloc(true);
      if (!tla)
         return 0;
      bool left = tla->TryExitGCFreeZone();
      return left;
   #else
      return false;
   #endif
}


void ExitGCFreeZone()
{
   #ifndef HXCPP_SINGLE_THREADED_APP
      LocalAllocator *tla = GetLocalAlloc();
      tla->ExitGCFreeZone();
   #endif
}

void ExitGCFreeZoneLocked()
{
   #ifndef HXCPP_SINGLE_THREADED_APP
      LocalAllocator *tla = GetLocalAlloc();
      tla->ExitGCFreeZoneLocked();
   #endif
}

void InitAlloc()
{
   for(int i=0;i<IMMIX_LINE_LEN;i++)
      gImmixStartFlag[i] = 1<<( i>>2 ) ;

   hx::CommonInitAlloc();
   sgAllocInit = true;
   sGlobalAlloc = new GlobalAllocator();
   sgFinalizers = new FinalizerList();
   sFinalizerLock = new HxMutex();
   sGCRootLock = new HxMutex();
   hx::Object tmp;
   void **stack = *(void ***)(&tmp);
   sgObject_root = stack[0];

   //GCLOG("__root pointer %p\n", sgObject_root);
   gMainThreadContext =  new LocalAllocator();

   tlsStackContext = gMainThreadContext;

   ExitGCFreeZone();

   // Setup main thread ...
   __hxcpp_thread_current();

   gMainThreadContext->onThreadAttach();
}


void GCPrepareMultiThreaded()
{
   #ifdef HXCPP_SINGLE_THREADED_APP
   CriticalGCError("GCPrepareMultiThreaded called with HXCPP_SINGLE_THREADED_APP");
   #endif
}



void SetTopOfStack(int *inTop,bool inForce)
{
   bool threadAttached = false;
   if (inTop)
   {
      if (!sgAllocInit)
         InitAlloc();
      else
      {
         if (tlsStackContext==0)
         {
            GCPrepareMultiThreaded();
            RegisterCurrentThread(inTop);
            threadAttached = true;
         }
      }
   }

   LocalAllocator *tla = (LocalAllocator *)(hx::ImmixAllocator *)tlsStackContext;

   if (tla)
   {
      tla->SetTopOfStack(inTop,inForce);
      if (threadAttached)
         tla->onThreadAttach();
   }
}


void *InternalNew(int inSize,bool inIsObject)
{
   //HX_STACK_FRAME("GC", "new", 0, "GC::new", "src/hx/GCInternal.cpp", __LINE__, 0)
   HX_STACK_FRAME("GC", "new", 0, "GC::new", "src/hx/GCInternal.cpp", inSize, 0)

   #ifdef HXCPP_DEBUG
   if (sgSpamCollects && sgAllocsSinceLastSpam>=sgSpamCollects)
   {
      //GCLOG("InternalNew spam\n");
      CollectFromThisThread(false,false);
   }
   _hx_atomic_add(&sgAllocsSinceLastSpam, 1);
   #endif

   if (inSize>=IMMIX_LARGE_OBJ_SIZE)
   {
      void *result = sGlobalAlloc->AllocLarge(inSize, true);
      return result;
   }
   else
   {
      LocalAllocator *tla = GetLocalAlloc();

      if (inIsObject)
      {
         void* result = tla->CallAlloc(inSize,IMMIX_ALLOC_IS_CONTAINER);
         return result;
      }
      else
      {
         #if defined(HXCPP_GC_MOVING) && defined(HXCPP_M64)
         if (inSize<8)
            return tla->CallAlloc(8,0);
         #endif

         void* result = tla->CallAlloc( (inSize+3)&~3,0);
         return result;
      }
   }
}


// Force global collection - should only be called from 1 thread.
int InternalCollect(bool inMajor,bool inCompact)
{
   if (!sgAllocInit)
       return 0;

   GetLocalAlloc()->SetupStackAndCollect(inMajor, inCompact);

   return sGlobalAlloc->MemUsage();
}

inline unsigned int ObjectSize(void *inData)
{
   unsigned int header = ((unsigned int *)(inData))[-1];

   return (header & IMMIX_ALLOC_ROW_COUNT) ?
            ( (header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT) :
             ((unsigned int *)(inData))[-2];
}


unsigned int ObjectSizeSafe(void *inData)
{
   unsigned int header = ((unsigned int *)(inData))[-1];
   if (header & HX_GC_CONST_ALLOC_BIT)
      return 0;

   #ifdef HXCPP_GC_NURSERY
   if (!(header & 0xff000000))
   {
      // Small object
      if (header & 0x00ffffff)
         return header & 0x0000ffff;
      // Large object
   }
   #endif


   return (header & IMMIX_ALLOC_ROW_COUNT) ?
            ( (header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT) :
             ((unsigned int *)(inData))[-2];
}

void GCChangeManagedMemory(int inDelta, const char *inWhy)
{
   sGlobalAlloc->onMemoryChange(inDelta, inWhy);
}

void InternalReleaseMem(void *inMem)
{
   if (inMem)
   {
      unsigned int s = ObjectSizeSafe(inMem);
      if (s>=IMMIX_LARGE_OBJ_SIZE)
      {
         //Can release asap
         sGlobalAlloc->FreeLarge(inMem);
      }
   }
}



void *InternalRealloc(int inFromSize, void *inData,int inSize, bool inExpand)
{
   if (inData==0 || inFromSize==0)
   {
      if (inData)
         InternalReleaseMem(inData);
      return hx::InternalNew(inSize,false);
   }

   HX_STACK_FRAME("GC", "realloc", 0, "GC::relloc", __FILE__ , __LINE__, 0)

   #ifdef HXCPP_DEBUG
   if (sgSpamCollects && sgAllocsSinceLastSpam>=sgSpamCollects)
   {
      //GCLOG("InternalNew spam\n");
      CollectFromThisThread(false,false);
   }
   _hx_atomic_add(&sgAllocsSinceLastSpam, 1);
   #endif

   void *new_data = 0;
   if (inSize==0)
   {
      new_data = hx::emptyAlloc;
   }
   else if (inSize>=IMMIX_LARGE_OBJ_SIZE)
   {
      new_data = sGlobalAlloc->AllocLarge(inSize, false);
      if (inSize>inFromSize)
         ZERO_MEM((char *)new_data + inFromSize,inSize-inFromSize);
   }
   else
   {
      LocalAllocator *tla = GetLocalAlloc();

      #if defined(HXCPP_GC_MOVING) && defined(HXCPP_M64)
      if (inSize<8)
          new_data =  tla->CallAlloc(8,0);
      else
      #endif
      {
         inSize = (inSize+3) & ~3;
         if (inExpand)
            tla->ExpandAlloc(inSize);

         new_data = tla->CallAlloc(inSize,0);
      }
   }


#ifdef HXCPP_TELEMETRY
   //printf(" -- reallocating %018x to %018x, size from %d to %d\n", inData, new_data, s, inSize);
   __hxt_gc_realloc(inData, new_data, inSize);
#endif

   int min_size = inFromSize < inSize ? inFromSize : inSize;

   if (min_size)
      memcpy(new_data, inData, min_size );

   InternalReleaseMem(inData);

   return new_data;
}

#ifdef HXCPP_GC_GENERATIONAL
void NewMarkedObject(hx::Object *inPtr)
{
   HX_OBJ_WB_PESSIMISTIC_GET(inPtr);
}
#endif

void RegisterCurrentThread(void *inTopOfStack)
{
   // Create a local-alloc
   LocalAllocator *local = sGlobalAlloc->GetPooledAllocator();
   if (!local)
   {
      local = new LocalAllocator((int *)inTopOfStack);
   }
   else
   {
      local->AttachThread((int *)inTopOfStack);
   }

   tlsStackContext = local;
   #ifdef HXCPP_SCRIPTABLE
   local->byteMarkId = hx::gByteMarkID;
   #endif
}

void UnregisterCurrentThread()
{
   LocalAllocator *local = (LocalAllocator *)(hx::ImmixAllocator *)tlsStackContext;
   local->Release();
}

void RegisterVTableOffset(int inOffset)
{
   if (inOffset>sgCheckInternalOffset)
   {
      sgCheckInternalOffset = inOffset;
      sgCheckInternalOffsetRows = 1 + (inOffset>>IMMIX_LINE_BITS);
   }
}

void PushTopOfStack(void *inTop)
{
   bool threadAttached = false;
   if (!sgAllocInit)
      InitAlloc();
   else
   {
      if (tlsStackContext==0)
      {
         GCPrepareMultiThreaded();
         RegisterCurrentThread(inTop);
         threadAttached = true;
      }
   }
 
   LocalAllocator *tla = GetLocalAlloc();
   tla->PushTopOfStack(inTop);
   if (threadAttached)
      tla->onThreadAttach();
}

void PopTopOfStack()
{
   LocalAllocator *tla = GetLocalAlloc();
   tla->PopTopOfStack();
}

int GcGetThreadAttachedCount()
{
   LocalAllocator *tla = GetLocalAlloc(true);
   if (!tla)
      return 0;
   return tla->mStackLocks + (tla->mGlobalStackLock ? 1 : 0);
}


#ifdef HXCPP_VISIT_ALLOCS
class GcFreezer : public hx::VisitContext
{
public:
   void visitObject(hx::Object **ioPtr)
   {
      hx::Object *obj = *ioPtr;
      if (!obj || IsConstAlloc(obj))
         return;

      unsigned int s = ObjectSize(obj);
      void *result = InternalCreateConstBuffer(obj,s,false);
      //printf(" Freeze %d\n", s);
      *ioPtr = (hx::Object *)result;
      (*ioPtr)->__Visit(this);
   }

   void visitAlloc(void **ioPtr)
   {
      void *data = *ioPtr;
      if (!data || IsConstAlloc(data))
         return;
      unsigned int s = ObjectSize(data);
      //printf(" Freeze %d\n", s);
      void *result = InternalCreateConstBuffer(data,s,false);
      *ioPtr = result;
   }
};
#endif


} // end namespace hx


Dynamic _hx_gc_freeze(Dynamic inObject)
{
#ifdef HXCPP_VISIT_ALLOCS
   hx::GcFreezer freezer;
   hx::Object *base = inObject.mPtr;
   freezer.visitObject(&base);
   return base;
#else
   return inObject;
#endif
}



void __hxcpp_spam_collects(int inEveryNCalls)
{
   #ifdef HXCPP_DEBUG
   sgSpamCollects = inEveryNCalls;
   #else
   GCLOG("Spam collects only available on debug versions\n");
   #endif
}

int __hxcpp_gc_trace(hx::Class inClass,bool inPrint)
{
    #if  !defined(HXCPP_DEBUG)
       #ifdef ANDROID
       __android_log_print(ANDROID_LOG_ERROR, "hxcpp", "GC trace not enabled in release build.");
       #elif defined(HX_WINRT)
       WINRT_LOG("GC trace not enabled in release build.");
       #else
       printf("WARNING : GC trace not enabled in release build.\n");
       #endif
       return 0;
    #else
       gCollectTrace = inClass.GetPtr();
       gCollectTraceCount = 0;
       gCollectTraceDoPrint = inPrint;
       hx::InternalCollect(false,false);
       gCollectTrace = 0;
       return gCollectTraceCount;
    #endif
}

int   __hxcpp_gc_large_bytes()
{
   return sGlobalAlloc->MemLarge();
}

int   __hxcpp_gc_reserved_bytes()
{
   return sGlobalAlloc->MemReserved();
}

double __hxcpp_gc_mem_info(int inWhich)
{
   switch(inWhich)
   {
      case MEM_INFO_USAGE:
         return (double)sGlobalAlloc->MemUsage();
      case MEM_INFO_RESERVED:
         return (double)sGlobalAlloc->MemReserved();
      case MEM_INFO_CURRENT:
         return (double)sGlobalAlloc->MemCurrent();
      case MEM_INFO_LARGE:
         return (double)sGlobalAlloc->MemLarge();
   }
   return 0;
}

int   __hxcpp_gc_used_bytes()
{
   return sGlobalAlloc->MemUsage();
}

void  __hxcpp_gc_do_not_kill(Dynamic inObj)
{
   hx::GCDoNotKill(inObj.GetPtr());
}

hx::Object *__hxcpp_get_next_zombie()
{
   return hx::GCGetNextZombie();
}


void _hx_set_finalizer(Dynamic inObj, void (*inFunc)(Dynamic) )
{
   GCSetHaxeFinalizer( inObj.mPtr, inFunc );
}

void __hxcpp_set_finalizer(Dynamic inObj, void *inFunc)
{
   GCSetHaxeFinalizer( inObj.mPtr, (hx::HaxeFinalizer) inFunc );
}

void __hxcpp_add_member_finalizer(hx::Object *inObject, _hx_member_finalizer f, bool inPin)
{
   AutoLock lock(*gSpecialObjectLock);
   hx::sFinalizableList.push( hx::Finalizable(inObject, f, inPin) );
}

void __hxcpp_add_alloc_finalizer(void *inAlloc, _hx_alloc_finalizer f, bool inPin)
{
   AutoLock lock(*gSpecialObjectLock);
   hx::sFinalizableList.push( hx::Finalizable(inAlloc, f, inPin) );
}



extern "C"
{
void hxcpp_set_top_of_stack()
{
   int i = 0;
   hx::SetTopOfStack(&i,false);
}
}

void __hxcpp_enter_gc_free_zone()
{
   hx::EnterGCFreeZone();
}


bool __hxcpp_try_gc_free_zone()
{
   return hx::TryGCFreeZone();
}



void __hxcpp_exit_gc_free_zone()
{
   hx::ExitGCFreeZone();
}


void __hxcpp_gc_safe_point()
{
    if (hx::gPauseForCollect)
      hx::PauseForCollect();
}

//#define HXCPP_FORCE_OBJ_MAP

#if defined(HXCPP_M64) || defined(HXCPP_GC_MOVING) || defined(HXCPP_FORCE_OBJ_MAP)
#define HXCPP_USE_OBJECT_MAP
#endif

int __hxcpp_obj_id(Dynamic inObj)
{
   #if (HXCPP_API_LEVEL<331)
   hx::Object *obj = inObj->__GetRealObject();
   #else
   hx::Object *obj = inObj.mPtr;
   #endif
   if (!obj) return -1;
   #ifdef HXCPP_USE_OBJECT_MAP
   return sGlobalAlloc->GetObjectID(obj);
   #else
   return (int)(obj);
   #endif
}

hx::Object *__hxcpp_id_obj(int inId)
{
   #ifdef HXCPP_USE_OBJECT_MAP
   return (hx::Object *)sGlobalAlloc->GetIDObject(inId);
   #else
   return (hx::Object *)(inId);
   #endif
}

#ifdef HXCPP_USE_OBJECT_MAP
unsigned int __hxcpp_obj_hash(Dynamic inObj)
{
   return __hxcpp_obj_id(inObj);
}
#else
unsigned int __hxcpp_obj_hash(Dynamic inObj)
{
   if (!inObj.mPtr) return 0;
   #if (HXCPP_API_LEVEL<331)
   hx::Object *obj = inObj->__GetRealObject();
   #else
   hx::Object *obj = inObj.mPtr;
   #endif
   #if defined(HXCPP_M64)
   size_t h64 = (size_t)obj;
   return (unsigned int)(h64>>2) ^ (unsigned int)(h64>>32);
   #else
   return ((unsigned int)inObj.mPtr) >> 4;
   #endif
}
#endif




void DummyFunction(void *inPtr) { }

