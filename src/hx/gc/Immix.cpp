#include <hxcpp.h>

#include <hx/GC.h>
#include <hx/Thread.h>
#include "../Hash.h"
#include "GcRegCapture.h"



static int gByteMarkID = 0x10;

namespace hx
{
   unsigned int gPrevMarkIdMask = 0;
}


enum { gFillWithJunk = 0 } ;


// #define HXCPP_SINGLE_THREADED_APP

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef HX_WINDOWS
#include <windows.h>
#endif

#include <map>
#include <vector>
#include <set>
#include <stdio.h>

#include "../QuickVec.h"

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

static bool sgAllocInit = 0;
static bool sgInternalEnable = true;
static void *sgObject_root = 0;
int gInAlloc = false;

// This is recalculated from the other parameters
static int sWorkingMemorySize          = 10*1024*1024;



#if HX_HAS_ATOMIC
  #if defined(HX_MACOS) || defined(HX_WINDOWS) || defined(HX_LINUX)
  enum { MAX_MARK_THREADS = 4 };
  #else
  enum { MAX_MARK_THREADS = 2 };
  #endif
#else
  enum { MAX_MARK_THREADS = 1 };
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


#ifndef HXCPP_GC_MOVING
// Enable moving collector...
//#define HXCPP_GC_MOVING
#endif

//#define SHOW_MEM_EVENTS
//#define COLLECTOR_STATS
// Allocate this many blocks at a time - this will increase memory usage % when rounding to block size must be done.
// However, a bigger number makes it harder to release blocks due to pinning
#define IMMIX_BLOCK_GROUP_BITS  5

#ifdef __GNUC__
// Not sure if this is worth it ....
//#define USE_POSIX_MEMALIGN
#endif


#ifdef HXCPP_DEBUG
static hx::Object *gCollectTrace = 0;
static bool gCollectTraceDoPrint = false;
static int gCollectTraceCount = 0;
static int sgSpamCollects = 0;
static int sgAllocsSinceLastSpam = 0;
#endif

#ifdef ANDROID
#define GCLOG(...) __android_log_print(ANDROID_LOG_INFO, "gclog", __VA_ARGS__)
#else
#define GCLOG printf
#endif


//#define PROFILE_COLLECT


#ifdef PROFILE_COLLECT
   #define STAMP(t) double t = __hxcpp_time_stamp();
#else
   #define STAMP(t)
#endif

// TODO: Telemetry.h ?
#ifdef HXCPP_TELEMETRY
extern void __hxt_gc_new(void* obj, int inSize);
extern void __hxt_gc_realloc(void* old_obj, void* new_obj, int new_size);
extern void __hxt_gc_start();
extern void __hxt_gc_end();
#endif

static int sgTimeToNextTableUpdate = 0;




MyMutex  *gThreadStateChangeLock=0;
MyMutex  *gSpecialObjectLock=0;

class LocalAllocator;
enum LocalAllocState { lasNew, lasRunning, lasStopped, lasWaiting, lasTerminal };


static void MarkLocalAlloc(LocalAllocator *inAlloc,hx::MarkContext *__inCtx);
static void WaitForSafe(LocalAllocator *inAlloc);
static void ReleaseFromSafe(LocalAllocator *inAlloc);
static void CollectFromThisThread();

namespace hx
{
int gPauseForCollect = 0x00000000;

bool gMultiThreadMode = false;

ImmixAllocator *gMainThreadAlloc = 0;

unsigned int gImmixStartFlag[128];

int gMarkID = 0x10 << 24;

int gMarkIDWithContainer = (0x10 << 24) | IMMIX_ALLOC_IS_CONTAINER;

void ExitGCFreeZoneLocked();

DECLARE_FAST_TLS_DATA(ImmixAllocator, tlsImmixAllocator);

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


#ifndef USE_POSIX_MEMALIGN

struct GroupInfo
{
   void clear()
   {
      pinned = false;
      free = 0;
      used = 0;
   }

   char *alloc;

   bool pinned;
   int  free;
   int  used;
};
 
hx::QuickVec<GroupInfo> gAllocGroups;

#endif


// ---  Internal GC - IMMIX Implementation ------------------------------



// Some inline implementations ...
// Use macros to allow for mark/move



/*
  IMMIX block size, and various masks for converting addresses

*/

#define IMMIX_BLOCK_BITS      15

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

MID = ENDIAN_MARK_ID_BYTE = is measured from the object pointer
      ENDIAN_MARK_ID_BYTE_HEADER = is measured from the header pointer (4 bytes before object)


*/

#ifndef HXCPP_BIG_ENDIAN
#define ENDIAN_MARK_ID_BYTE        -1
#else
#define ENDIAN_MARK_ID_BYTE        -4
#endif
#define ENDIAN_MARK_ID_BYTE_HEADER (ENDIAN_MARK_ID_BYTE + 4)


// Used by strings
// HX_GC_CONST_ALLOC_BIT  0x80000000

#define IMMIX_ALLOC_MARK_ID     0x3f000000
#define IMMIX_ALLOC_IS_PINNED   0x00400000
#define IMMIX_ALLOC_SIZE_MASK   0x003fff00
#define IMMIX_ALLOC_ROW_COUNT   0x000000ff


#define IMMIX_OBJECT_HAS_MOVED 0x000000fe;


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
   #else
   printf("Critical Error: %s\n", inMessage);
   #endif


   #if __has_builtin(__builtin_trap)
   __builtin_trap();
   #else
   (* (volatile int *) 0) = 0;
   #endif
}





enum AllocType { allocNone, allocString, allocObject, allocMarked };

struct BlockDataInfo *gBlockStack = 0;
typedef hx::QuickVec<hx::Object *> ObjectStack;


#if defined(EMSCRIPTEN)
// Dummy lock
typedef HxAtomicLock ThreadPoolLock;
#else
typedef MyMutex ThreadPoolLock;
#endif

static ThreadPoolLock sThreadPoolLock;

#if !defined(HX_WINDOWS) && !defined(EMSCRIPTEN) && !defined(HX_WINRT)
#define HX_GC_PTHREADS
typedef pthread_cond_t ThreadPoolSignal;
inline void WaitThreadLocked(ThreadPoolSignal &ioSignal)
{
   pthread_cond_wait(&ioSignal, &sThreadPoolLock.mMutex);
}
#else
typedef MySemaphore ThreadPoolSignal;
#endif

typedef TAutoLock<ThreadPoolLock> ThreadPoolAutoLock;

// For threaded marking/block reclaiming
static unsigned int sRunningThreads = 0;
static unsigned int sAllThreads = 0;
static bool sThreadPoolInit = false;

enum ThreadPoolJob
{
   tpjUnknown,
   tpjMark,
   tpjReclaim,
   tpjReclaimFull,
};

int sgThreadCount = 0;
static ThreadPoolJob sgThreadPoolJob = tpjUnknown;
static bool sgThreadPoolAbort = false;

// Pthreads enters the sleep state while holding a mutex, so it no cost to update
//  the sleeping state and thereby avoid over-signalling the condition
bool             sThreadSleeping[MAX_MARK_THREADS];
ThreadPoolSignal sThreadWake[MAX_MARK_THREADS];
bool             sThreadJobDoneSleeping = false;
ThreadPoolSignal sThreadJobDone;


inline void SignalThreadPool(ThreadPoolSignal &ioSignal, bool sThreadSleeping)
{
   #ifdef HX_GC_PTHREADS
   if (sThreadSleeping)
      pthread_cond_signal(&ioSignal);
   #else
   ioSignal.Set();
   #endif
}

namespace hx { void MarkerReleaseWorkerLocked(); }

struct HoleRange
{
   unsigned short start;
   unsigned short length;
};

struct BlockDataInfo
{
   int             mId;
   int             mGroupId;
   union BlockData *mPtr;

   unsigned int allocStart[IMMIX_LINES];
   int          mUsedRows;
   HoleRange    mRanges[MAX_HOLES];
   int          mHoles;
   int          mMaxHoleSize;
   bool         mPinned;
   bool         mZeroed;
   volatile bool mReady;


   void clear()
   {
      mUsedRows = 0;
      mPinned = false;
      ZERO_MEM(allocStart,sizeof(int)*IMMIX_LINES);
      mRanges[0].start = IMMIX_HEADER_LINES << IMMIX_LINE_BITS;
      mRanges[0].length = IMMIX_USEFUL_LINES << IMMIX_LINE_BITS;
      mMaxHoleSize = mRanges[0].length;
      mHoles = 1;
      mZeroed = false;
      mReady = true;
   }
};

hx::QuickVec<BlockDataInfo *> *gBlockInfo = 0;
static int gBlockInfoEmptySlots = 0;




union BlockData
{
   void Init(int inGID)
   {
      mId = nextBlockId();
      BlockDataInfo &info = getInfo();
      info.mId = mId;
      info.mGroupId = inGID;
      info.mPtr = this;
      info.clear();
   }
   inline int GetFreeRows() const { return (IMMIX_USEFUL_LINES - getUsedRows()); }
   inline int GetFreeData() const { return (IMMIX_USEFUL_LINES - getUsedRows())<<IMMIX_LINE_BITS; }
   void zero()
   {
      BlockDataInfo &info = getInfo();
      if (!info.mZeroed)
      {
         info.mZeroed = true;
         for(int i=0;i<info.mHoles;i++)
            ZERO_MEM( mRow[0]+info.mRanges[i].start, info.mRanges[i].length );
      }
   }
   
   void destroy()
   {
      delete (*gBlockInfo)[mId];
      (*gBlockInfo)[mId] = 0;
      gBlockInfoEmptySlots++;
      #ifdef USE_POSIX_MEMALIGN
      free(this);
      #endif
   }

   int nextBlockId()
   {
      if (gBlockInfo==0)
         gBlockInfo = new hx::QuickVec<BlockDataInfo *>;
      if (gBlockInfoEmptySlots)
      {
         for(int i=0;i<gBlockInfo->size();i++)
            if ( !(*gBlockInfo)[i] )
            {
               gBlockInfoEmptySlots--;
               (*gBlockInfo)[i] = new BlockDataInfo;
               return i;
            }
      }
      int result = gBlockInfo->size();
      gBlockInfo->push( new BlockDataInfo );
      return result;
   }

   inline BlockDataInfo &getInfo() const { return *(*gBlockInfo)[mId]; }


   bool IsEmpty() const { return getUsedRows() == 0; }
   bool IsFull() const { return getUsedRows() == IMMIX_USEFUL_LINES; }
   int getUsedRows() const { return getInfo().mUsedRows; }
   int getMaxFree() const { return getInfo().mMaxHoleSize; }
   int isPinned() const { return getInfo().mPinned; }

   void Verify()
   {
      unsigned int *allocStart = getInfo().allocStart;
      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         unsigned char &row_flag = mRowMarked[r];
         if (mRowMarked[r] && allocStart[r])
         {
            GCLOG("Block verification failed on row %d\n",r);
            #if __has_builtin(__builtin_trap)
            __builtin_trap();
            #else
            *(int *)0=0;
            #endif
         }
      }
   }
 
   void FillWithJunk()
   {
       memset(&mRow[0][0] + 2, 0x55, IMMIX_BLOCK_SIZE-2 );
   }

   int CountUsedRows()
   {
      int used = 0;
      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
         used += mRowMarked[r];
      getInfo().mUsedRows = used;
      return used;
   }
   void SetUnreclaimed()
   {
      BlockDataInfo &info = getInfo();
      info.mZeroed = false;
      info.mReady = false;
   }

   int Reclaim(bool inFull, bool inZero)
   {
      int free = 0;
      int hole = 0;
      int firstFreeRow = 0;
      bool update_table = inFull || gFillWithJunk;
      BlockDataInfo &info = getInfo();
      unsigned int *allocStart = info.allocStart;

      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         if (mRowMarked[r])
         {
            if (firstFreeRow)
            {
               info.mRanges[hole].start  = firstFreeRow<<IMMIX_LINE_BITS;
               info.mRanges[hole].length = (r-firstFreeRow)<<IMMIX_LINE_BITS;
               if (info.mRanges[hole].length > info.mMaxHoleSize)
                  info.mMaxHoleSize = info.mRanges[hole].length;
               hole++;
               firstFreeRow = 0;
            }
            if (update_table)
            {
               unsigned int &starts = allocStart[r];
               if (starts)
               {
                  for(int i=0;i<32;i++)
                     if ( starts & (1<<i) )
                     {
                        unsigned int header = ((int *)mRow[r])[i];
                        // Old object start?
                        if ( (header & IMMIX_ALLOC_MARK_ID) != hx::gMarkID )
                        {
                           starts &= ~(1<<i);
                           if (!starts)
                              break;
                        }
                     }
               }
            }
         }
         else
         {
            allocStart[r] = 0;
            if (!firstFreeRow)
               firstFreeRow = r;
            if (gFillWithJunk)
               memset(mRow[r],0x55,IMMIX_LINE_LEN);
            free++;
         }
      }

      if (firstFreeRow)
      {
         info.mRanges[hole].start  = firstFreeRow<<IMMIX_LINE_BITS;
         info.mRanges[hole].length = (IMMIX_LINES-firstFreeRow)<<IMMIX_LINE_BITS;
         if (info.mRanges[hole].length > info.mMaxHoleSize)
            info.mMaxHoleSize = info.mRanges[hole].length;
         hole++;
      }
      info.mUsedRows = IMMIX_USEFUL_LINES - free;
      info.mHoles = hole;
      if (inZero)
      {
         info.mZeroed = true;
         for(int i=0;i<info.mHoles;i++)
            ZERO_MEM( mRow[0]+info.mRanges[i].start, info.mRanges[i].length );
      }
      else
         info.mZeroed = false;

      info.mReady = true;
      // GCLOG("Used %f, biggest=%f, holes=%d\n", (float)mUsedRows/IMMIX_USEFUL_LINES, (float)mFreeInARow/IMMIX_USEFUL_LINES, holes );

      //Verify();
      return info.mUsedRows;
   }


   AllocType GetAllocType(int inOffset,bool inReport = false)
   {
      inReport = false;
      int r = inOffset >> IMMIX_LINE_BITS;
      if (r < IMMIX_HEADER_LINES || r >= IMMIX_LINES)
      {
         if (inReport)
            GCLOG("  bad row %d (off=%d)\n", r, inOffset);
         return allocNone;
      }

      if ( !( getInfo().allocStart[r] & (1<<((inOffset>>2) & 31)) ) )
      {
         // Not a actual start...
         return allocNone;
      }



      char time = mRow[0][inOffset+ENDIAN_MARK_ID_BYTE_HEADER];
      if ( ((time+1) & MARK_BYTE_MASK) != (gByteMarkID & MARK_BYTE_MASK)  )
      {
         // Object is either out-of-date, or already marked....
         if (inReport)
            GCLOG(time==gByteMarkID ? " M " : " O ");
         return time==gByteMarkID ? allocMarked : allocNone;
      }


      if (*(unsigned int *)(mRow[0] + inOffset) & IMMIX_ALLOC_IS_CONTAINER)
      {
         // See if object::new has been called, but not constructed yet ...
         void **vtable = (void **)(mRow[0] + inOffset + sizeof(int));
         if (vtable[0]==0)
         {
            // GCLOG("Partially constructed object.");
            return allocString;
         }
         return allocObject;
      }

      return allocString;
   }

   void pin() { getInfo().mPinned = true; }

   void ClearRowMarks()
   {
      getInfo().mPinned = false;
      ZERO_MEM(mRowMarked+IMMIX_HEADER_LINES, IMMIX_USEFUL_LINES);
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void VisitBlock(hx::VisitContext *inCtx)
   {
      if (IsEmpty())
         return;

      unsigned int *allocStart = getInfo().allocStart;
      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         if (mRowMarked[r])
         {
            unsigned int starts = allocStart[r];
            unsigned char *row = mRow[r];
            for(int i=0;i<32;i++)
            {
               int pos = i<<2;
               if ( (starts & (1<<i)) &&
                   (row[pos+ENDIAN_MARK_ID_BYTE_HEADER]&FULL_MARK_BYTE_MASK) == gByteMarkID)
                  {
                     if ( (*(unsigned int *)(row+pos)) & IMMIX_ALLOC_IS_CONTAINER )
                     {
                        hx::Object *obj = (hx::Object *)(row+pos+4);
                        obj->__Visit(inCtx);
                     }
                  }
            }
         }
      }
   }
   #endif



   // First 2 bytes are not needed for row markers (first 2 rows are for flags)
   unsigned short mId;

   // First 2 rows contain a byte-flag-per-row 
   unsigned char  mRowMarked[IMMIX_LINES];
   // Row data as union - don't use first 2 rows
   unsigned char  mRow[IMMIX_LINES][IMMIX_LINE_LEN];
};


bool MostUsedFirst(BlockData *inA, BlockData *inB)
{
   return inA->getUsedRows() > inB->getUsedRows();
}

#define MARK_ROWS_UNCHECKED_BEGIN \
   ((unsigned char *)inPtr)[ENDIAN_MARK_ID_BYTE] = gByteMarkID; \
 \
   size_t ptr_i = ((size_t)inPtr)-sizeof(int); \
   unsigned int flags =  *((unsigned int *)ptr_i); \
 \
   int rows = flags & IMMIX_ALLOC_ROW_COUNT; \
   if (rows) \
   { \
      char *block = (char *)(ptr_i & IMMIX_BLOCK_BASE_MASK); \
      char *rowMark = block + ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS); \
      *rowMark = 1;\
      if (rows>1) \
      { \
         rowMark[1] = 1; \
         if (rows>2) \
         { \
            rowMark[2] = 1; \
            if (rows>3) \
            { \
               rowMark[3] = 1; \
               for(int r=4; r<rows; r++) \
                  rowMark[r]=1; \
            } \
         } \
      } \

#define MARK_ROWS_UNCHECKED_END \
    }

namespace hx
{



void BadImmixAlloc()
{
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

   #if __has_builtin(__builtin_trap)
   __builtin_trap();
   #else
   *(int *)0=0;
   #endif
}



void GCCheckPointer(void *inPtr)
{
   unsigned char&mark = ((unsigned char *)inPtr)[ENDIAN_MARK_ID_BYTE];
   if ( !(mark & HX_GC_CONST_ALLOC_MARK_BIT) && mark!=gByteMarkID  )
   {
      GCLOG("Old object %s\n", ((hx::Object *)inPtr)->toString().__s );
      NullReference("Object", false);
   }
}

// --- Marking ------------------------------------

struct MarkInfo
{
   const char *mClass;
   const char *mMember;
};

struct MarkChunk
{
   enum { SIZE = 31 };

   MarkChunk() : count(0) { }
   Object *stack[SIZE];
   int    count;

   inline void push(Object *inObj)
   {
      stack[count++] = inObj;
   }
   inline Object *pop()
   {
      if (count)
         return stack[--count];
      return 0;
   }

};


struct GlobalChunks
{
   hx::QuickVec< MarkChunk * > chunks;
   hx::QuickVec< MarkChunk * > spare;

   ::String   *stringArrayJob;
   hx::Object **objectArrayJob;
   int         arrayJobLen;
   int         arrayDistribute;

   GlobalChunks()
   {
      arrayJobLen = 0;
      stringArrayJob = 0;
      objectArrayJob = 0;
      arrayDistribute = 0;
   }

   MarkChunk *pushJob(MarkChunk *inChunk)
   {
      if (MAX_MARK_THREADS>1)
      {
         ThreadPoolAutoLock l(sThreadPoolLock);
         chunks.push(inChunk);
         MarkChunk *reult = allocLocked();

         if (sAllThreads ^ sRunningThreads)
         {
            #define CHECK_THREAD_WAKE(tid) \
            if (MAX_MARK_THREADS >tid && sgThreadCount>tid && (!(sAllThreads & (1<<tid)))) { \
               sRunningThreads |= (1<<tid); \
               SignalThreadPool(sThreadWake[tid],sThreadSleeping[tid]); \
            }

            CHECK_THREAD_WAKE(0)
            else CHECK_THREAD_WAKE(1)
            else CHECK_THREAD_WAKE(2)
            else CHECK_THREAD_WAKE(3)
            else CHECK_THREAD_WAKE(4)
            else CHECK_THREAD_WAKE(5)
            else CHECK_THREAD_WAKE(6)
            else CHECK_THREAD_WAKE(7)
         }

         return allocLocked();
      }
      else
      {
         chunks.push(inChunk);
         return allocLocked();
      }
   }

   int takeArrayJob(hx::Object **inPtr, int inLen)
   {
      if (arrayJobLen)
         return 0;

      ThreadPoolAutoLock l(sThreadPoolLock);
      if (sRunningThreads == sAllThreads)
         return 0;

      int count = 0;
      for(int i=0;i<sgThreadCount ;i++)
         if ( !(sRunningThreads & (1<<i)) )
            count++;

      // Items per thread, rounded up to multiple of 16 ...
      arrayDistribute = ((inLen/(count + 1)) + 15) & ~15;
      int itemsForCallingThread = arrayDistribute;


      arrayJobLen = inLen - itemsForCallingThread;
      stringArrayJob = 0;
      objectArrayJob = inPtr;

      // Wake othter threads...
      for(int i=0;i<sgThreadCount ;i++)
         if ( !(sRunningThreads & (1<<i)) )
         {
            sRunningThreads |= (1<<i);
            SignalThreadPool(sThreadWake[i],sThreadSleeping[i]);
         }

      // Return how many to skip
      return arrayJobLen;
   }



   MarkChunk *popJobLocked(MarkChunk *inChunk)
   {
      if (inChunk)
         spare.push(inChunk);

      if (chunks.size())
         return chunks.pop();
      return 0;
   }


   void completeThreadLocked(int inThreadId)
   {
      sRunningThreads &= ~(1<<inThreadId);
      if (!sRunningThreads)
         SignalThreadPool(sThreadJobDone,sThreadJobDoneSleeping);
   }


   // Optionally returns inChunk to empty pool (while we have the lock),
   //  and returns a new job if there is one
   MarkChunk *popJob(MarkChunk *inChunk,int inThreadId, hx::Object ***outArrayJob, int *outJobLen)
   {
      if (MAX_MARK_THREADS > 1 && sAllThreads)
      {
         ThreadPoolAutoLock l(sThreadPoolLock);
         if (arrayJobLen)
         {
            if (inChunk)
               spare.push(inChunk);

            int items = std::min( arrayDistribute, arrayJobLen );
            *outJobLen = items;
            *outArrayJob = objectArrayJob;
            objectArrayJob += items;
            arrayJobLen -= items;
            // Ensure that the array marker has something to recurse into if required
            return allocLocked();
         }

         MarkChunk *result =  popJobLocked(inChunk);
         if (!result)
            completeThreadLocked(inThreadId);
         return result;
      }
      return popJobLocked(inChunk);
   }

   inline void freeLocked(MarkChunk *inChunk)
   {
      spare.push(inChunk);
   }

   void free(MarkChunk *inChunk)
   {
      if (MAX_MARK_THREADS>1 && sAllThreads)
      {
         ThreadPoolAutoLock l(sThreadPoolLock);
         freeLocked(inChunk);
      }
      else
         freeLocked(inChunk);
   }

   inline MarkChunk *allocLocked()
   {
      if (spare.size()==0)
         return new MarkChunk;
      return spare.pop();
   }

   MarkChunk *alloc()
   {
      if (MAX_MARK_THREADS>1 && sAllThreads)
      {
         ThreadPoolAutoLock l(sThreadPoolLock);
         return allocLocked();
      }
      return allocLocked();
   }
};

GlobalChunks sGlobalChunks;

class MarkContext
{
    int       mPos;
    MarkInfo  *mInfo;
    int       mThreadId;
    MarkChunk *marking;
    MarkChunk *spare;

public:
    enum { StackSize = 8192 };

    char      *block;

    MarkContext(int inThreadId = -1)
    {
       mInfo = new MarkInfo[StackSize];
       mThreadId = inThreadId;
       mPos = 0;
       marking = sGlobalChunks.alloc();
       spare = sGlobalChunks.alloc();
       block = 0;
    }
    ~MarkContext()
    {
       if (marking) sGlobalChunks.free(marking);
       if (spare) sGlobalChunks.free(spare);
       delete [] mInfo;
       // TODO: Free slabs
    }
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

    void pushObj(hx::Object *inObject)
    {
       if (marking->count < MarkChunk::SIZE)
       {
          //printf("push %d\n", marking->count);
          marking->push(inObject);
          // consider pushing spare if any thread is waiting?
       }
       else if (spare->count==0)
       {
          // Swap ...
          MarkChunk *tmp = spare;
          spare = marking;
          marking = tmp;
          //printf("swap %d\n", marking->count);
          marking->push(inObject);
       }
       else
       {
          //printf("push job %d\n", marking->count);
          marking = sGlobalChunks.pushJob(marking);
          marking->push(inObject);
       }
    }

    void init()
    {
       block = 0;
       if (!marking)
          marking = sGlobalChunks.alloc();
    }

    void releaseJobsLocked()
    {
       if (marking && marking->count)
       {
          sGlobalChunks.chunks.push(marking);
          marking = 0;
       }
       if (spare->count)
          spare = sGlobalChunks.pushJob(spare);
    }

    void InitChunkLocked()
    {
       if (!marking)
          marking = sGlobalChunks.popJobLocked(marking);
    }

    void Process()
    {
       while(true)
       {
          if (!marking || !marking->count)
          {
             if (MAX_MARK_THREADS>1 && sgThreadPoolAbort)
             {
                ThreadPoolAutoLock l(sThreadPoolLock);
                releaseJobsLocked();
                return;
             }

             hx::Object **objectArray;
             int        arrayLength = 0;

             marking = sGlobalChunks.popJob(marking,mThreadId, &objectArray, &arrayLength);

             if (arrayLength)
                MarkObjectArray(objectArray, arrayLength, this);
             else if (!marking)
                break;
          }

          while(marking)
          {
             hx::Object *obj = marking->pop();
             if (obj)
             {
                block = (char *)(((size_t)obj) & IMMIX_BLOCK_BASE_MASK);
                obj->__Mark(this);
             }
             else if (spare->count)
             {
                // Swap ...
                MarkChunk *tmp = spare;
                spare = marking;
                marking = tmp;
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



void MarkAllocUnchecked(void *inPtr,hx::MarkContext *__inCtx)
{
   MARK_ROWS_UNCHECKED_BEGIN
   MARK_ROWS_UNCHECKED_END
}

void MarkObjectAllocUnchecked(hx::Object *inPtr,hx::MarkContext *__inCtx)
{
   MARK_ROWS_UNCHECKED_BEGIN

   if (flags & IMMIX_ALLOC_IS_CONTAINER)
   {
      #ifdef HXCPP_DEBUG
      if (gCollectTrace && gCollectTrace==inPtr->__GetClass().GetPtr())
      {
         gCollectTraceCount++;
         if (gCollectTraceDoPrint)
             __inCtx->Trace();
      }
      #endif
      
      #ifdef HXCPP_DEBUG
         // Recursive mark so stack stays intact..
         if (gCollectTrace)
            inPtr->__Mark(__inCtx);
         else
      #endif

      // There is a slight performance gain by calling recursively, but you
      //   run the risk of stack overflow.  Also, a parallel mark algorithm could be
      //   done when the marking is stack based.
      //inPtr->__Mark(__inCtx);
      if (block==__inCtx->block)
         inPtr->__Mark(__inCtx);
      else
         __inCtx->pushObj(inPtr);
   }

   MARK_ROWS_UNCHECKED_END
}


void MarkObjectArray(hx::Object **inPtr, int inLength, hx::MarkContext *__inCtx)
{
   if (MAX_MARK_THREADS>1 && sAllThreads && inLength>4096)
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
   {
      for(int i=0; i<inLength; i++)
         if (inPtr[i])
            MarkObjectAlloc(inPtr[i],__inCtx);
   }
}

void MarkStringArray(String *inPtr, int inLength, hx::MarkContext *__inCtx)
{
   #if 0
   if (MAX_MARK_THREADS>1 && sAllThreads && inLength>4096)
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
         const char *str = inPtr[i].__s;
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

FILE_SCOPE MyMutex *sGCRootLock = 0;
typedef std::set<hx::Object **> RootSet;
static RootSet sgRootSet;

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




// --- Finalizers -------------------------------


class WeakRef;
typedef hx::QuickVec<WeakRef *> WeakRefs;

FILE_SCOPE MyMutex *sFinalizerLock = 0;
FILE_SCOPE WeakRefs sWeakRefs;

class WeakRef : public hx::Object
{
public:
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

typedef std::map<hx::Object *,hx::finalizer> FinalizerMap;
FILE_SCOPE FinalizerMap sFinalizerMap;
#ifdef HXCPP_TELEMETRY
FILE_SCOPE FinalizerMap hxtFinalizerMap;
#endif

typedef void (*HaxeFinalizer)(Dynamic);
typedef std::map<hx::Object *,HaxeFinalizer> HaxeFinalizerMap;
FILE_SCOPE HaxeFinalizerMap sHaxeFinalizerMap;

hx::QuickVec<int> sFreeObjectIds;
typedef std::map<hx::Object *,int> ObjectIdMap;
typedef hx::QuickVec<hx::Object *> IdObjectMap;
FILE_SCOPE ObjectIdMap sObjectIdMap;
FILE_SCOPE IdObjectMap sIdObjectMap;

typedef std::set<hx::Object *> MakeZombieSet;
FILE_SCOPE MakeZombieSet sMakeZombieSet;

typedef hx::QuickVec<hx::Object *> ZombieList;
FILE_SCOPE ZombieList sZombieList;

typedef hx::QuickVec<hx::HashBase<Dynamic> *> WeakHashList;
FILE_SCOPE WeakHashList sWeakHashList;


InternalFinalizer::InternalFinalizer(hx::Object *inObj, finalizer inFinalizer)
{
   mUsed = false;
   mValid = true;
   mObject = inObj;
   mFinalizer = inFinalizer;

   AutoLock lock(*gSpecialObjectLock);
   sgFinalizers->push(this);
}

#ifdef HXCPP_VISIT_ALLOCS
void InternalFinalizer::Visit(VisitContext *__inCtx) { HX_VISIT_OBJECT(mObject); }
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

      unsigned char mark = ((unsigned char *)obj)[ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         sZombieList.push(obj);
         sMakeZombieSet.erase(i);

         // Mark now to prevent secondary zombies...
         inContext.init();
         hx::MarkObjectAlloc(obj , &inContext );
         inContext.Process();
      }

      i = next;
   }
}

bool IsWeakRefValid(hx::Object *inPtr)
{
   unsigned char mark = ((unsigned char *)inPtr)[ENDIAN_MARK_ID_BYTE];

    // Special case of member closure - check if the 'this' pointer is still alive
    bool isCurrent = mark==gByteMarkID;
    if ( !isCurrent && inPtr->__GetType()==vtFunction)
    {
        hx::Object *thiz = (hx::Object *)inPtr->__GetHandle();
        if (thiz)
        {
            mark = ((unsigned char *)thiz)[ENDIAN_MARK_ID_BYTE];
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


void RunFinalizers()
{
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
      else if (!f->mUsed)
      {
         if (f->mFinalizer)
            f->mFinalizer(f->mObject);
         list.qerase(idx);
         delete f;
      }
      else
      {
         f->mUsed = false;
         idx++;
      }
   }

   for(FinalizerMap::iterator i=sFinalizerMap.begin(); i!=sFinalizerMap.end(); )
   {
      hx::Object *obj = i->first;
      FinalizerMap::iterator next = i;
      ++next;

      unsigned char mark = ((unsigned char *)obj)[ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
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

      unsigned char mark = ((unsigned char *)obj)[ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         (*i->second)(obj);
         sHaxeFinalizerMap.erase(i);
      }

      i = next;
   }

   #ifdef HXCPP_TELEMETRY
   for(FinalizerMap::iterator i=hxtFinalizerMap.begin(); i!=hxtFinalizerMap.end(); )
   {
      hx::Object *obj = i->first;
      FinalizerMap::iterator next = i;
      ++next;

      unsigned char mark = ((unsigned char *)obj)[ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         (*i->second)(obj);
         hxtFinalizerMap.erase(i);
      }

      i = next;
   }
   #endif

   for(ObjectIdMap::iterator i=sObjectIdMap.begin(); i!=sObjectIdMap.end(); )
   {
      ObjectIdMap::iterator next = i;
      next++;

      unsigned char mark = ((unsigned char *)i->first)[ENDIAN_MARK_ID_BYTE];
      if ( mark!=gByteMarkID )
      {
         sFreeObjectIds.push(i->second);
         sObjectIdMap.erase(i);
         sIdObjectMap[i->second] = 0;
      }

      i = next;
   }

   for(int i=0;i<sWeakHashList.size();    )
   {
      HashBase<Dynamic> *ref = sWeakHashList[i];
      unsigned char mark = ((unsigned char *)ref)[ENDIAN_MARK_ID_BYTE];
      // Object itself is gone - no need to worrk about that again
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
      unsigned char mark = ((unsigned char *)ref)[ENDIAN_MARK_ID_BYTE];
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
         unsigned char mark = ((unsigned char *)r)[ENDIAN_MARK_ID_BYTE];

         // Special case of member closure - check if the 'this' pointer is still alive
         if ( mark!=gByteMarkID && r->__GetType()==vtFunction)
         {
            hx::Object *thiz = (hx::Object *)r->__GetHandle();
            if (thiz)
            {
               mark = ((unsigned char *)thiz)[ENDIAN_MARK_ID_BYTE];
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

#ifdef HXCPP_TELEMETRY
// Callback finalizer on non-abstract type;
void  GCSetHXTFinalizer( void*obj, hx::finalizer f )
{
   if (!obj)
      throw Dynamic(HX_CSTRING("set_hxt_finalizer - invalid null object"));
   //if (((unsigned int *)obj)[-1] & HX_GC_CONST_ALLOC_BIT) return;

   //printf("Setting hxt finalizer on %018x\n", obj);

   AutoLock lock(*gSpecialObjectLock);
   if (f==0)
   {
     FinalizerMap::iterator i = hxtFinalizerMap.find((hx::Object*)obj);
      if (i!=hxtFinalizerMap.end())
         hxtFinalizerMap.erase(i);
   }
   else
     hxtFinalizerMap[(hx::Object*)obj] = f;
}
#endif

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


void *InternalCreateConstBuffer(const void *inData,int inSize,bool inAddStringHash)
{
   bool addHash = inAddStringHash && inData && inSize>0;

   int *result = (int *)malloc(inSize + sizeof(int) + (addHash ? sizeof(int):0) );
   if (addHash)
   {
      unsigned int hash = 0;
      for(int i=0;i<inSize-1;i++)
         hash = hash*223 + ((unsigned char *)inData)[i];

      result[0] = hash;
      result++;
      *result++ = HX_GC_CONST_ALLOC_BIT;
   }
   else
   {
      *result++ = HX_GC_CONST_ALLOC_BIT | HX_GC_NO_STRING_HASH;
   }

   if (inData)
      memcpy(result,inData,inSize);
   else
      memset(result,0,inSize);

   return result;
}


} // namespace hx


hx::Object *__hxcpp_weak_ref_create(Dynamic inObject)
{
   return new hx::WeakRef(inObject);
}

hx::Object *__hxcpp_weak_ref_get(Dynamic inRef)
{
   hx::WeakRef *ref = dynamic_cast<hx::WeakRef *>(inRef.mPtr);
   return ref->mRef.mPtr;
}


// --- GlobalAllocator -------------------------------------------------------

typedef std::set<BlockData *> PointerSet;
typedef hx::QuickVec<BlockData *> BlockList;

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


class GlobalAllocator
{
   enum { LOCAL_POOL_SIZE = 2 };

public:
   GlobalAllocator()
   {
      mNextFreeBlock = 0;

      mRowsInUse = 0;
      mLargeAllocated = 0;
      mLargeAllocSpace = 40 << 20;
      mLargeAllocForceRefresh = mLargeAllocSpace;
      // Start at 1 Meg...
      mTotalAfterLastCollect = 1<<20;
      mCurrentRowsInUse = 0;
      mAllBlocksCount = 0;
      for(int p=0;p<LOCAL_POOL_SIZE;p++)
         mLocalPool[p] = 0;
   }
   void AddLocal(LocalAllocator *inAlloc)
   {
      if (!gThreadStateChangeLock)
      {
         gThreadStateChangeLock = new MyMutex();
         gSpecialObjectLock = new MyMutex();
      }
      // Until we add ourselves, the colled will not wait
      //  on us - ie, we are assumed ot be in a GC free zone.
      AutoLock lock(*gThreadStateChangeLock);
      mLocalAllocs.push(inAlloc);
   }

   bool ReturnToPool(LocalAllocator *inAlloc)
   {
      // Until we add ourselves, the colled will not wait
      //  on us - ie, we are assumed ot be in a GC free zone.
      AutoLock lock(*gThreadStateChangeLock);
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

   void RemoveLocal(LocalAllocator *inAlloc)
   {
      // You should be in the GC free zone before you call this...
      AutoLock lock(*gThreadStateChangeLock);
      mLocalAllocs.qerase_val(inAlloc);
   }

   void *AllocLarge(int inSize, bool inClear)
   {
      if (hx::gPauseForCollect)
         __hxcpp_gc_safe_point();

      //Should we force a collect ? - the 'large' data are not considered when allocating objects
      // from the blocks, and can 'pile up' between smalll object allocations
      if ((inSize+mLargeAllocated > mLargeAllocForceRefresh) && sgInternalEnable)
      {
         //GCLOG("Large alloc causing collection");
         CollectFromThisThread();
      }

      inSize = (inSize +3) & ~3;

      if (inSize<<1 > mLargeAllocSpace)
         mLargeAllocSpace = inSize<<1;

      unsigned int *result = inClear ? 
                             (unsigned int *)calloc(1,inSize + sizeof(int)*2) :
                             (unsigned int *)malloc(inSize + sizeof(int)*2);
      if (!result)
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Large alloc failed - forcing collect\n");
         #endif

         CollectFromThisThread();
         result = (unsigned int *)malloc(inSize + sizeof(int)*2);
      }
      result[0] = inSize;
      result[1] = hx::gMarkID;

      bool do_lock = hx::gMultiThreadMode;
      if (do_lock)
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
            CollectFromThisThread();
         }

         int rounded = (inDelta +3) & ~3;
   
         if (rounded<<1 > mLargeAllocSpace)
            mLargeAllocSpace = rounded<<1;
      }

      bool do_lock = hx::gMultiThreadMode;
      if (do_lock)
         mLargeListLock.Lock();

      mLargeAllocated += inDelta;

      if (do_lock)
         mLargeListLock.Unlock();
   }

   BlockData *GetNextFree(int inRequiredBytes)
   {
      for(int i=mNextFreeBlock; i<mFreeBlocks.size(); i++)
      {
          if (mFreeBlocks[i]->getMaxFree()>=inRequiredBytes)
          {
              BlockData *result = mFreeBlocks[i];
              if (i==mNextFreeBlock)
                 mNextFreeBlock++;
              else
              {
                 mFreeBlocks.erase(i);
              }
              return result;
          }
      }
      return 0;
   }

   inline int GetWorkingMemory()
   {
       return mAllBlocks.size() << IMMIX_BLOCK_BITS;
   }

   // Making this function "virtual" is actually a (big) performance enhancement!
   // On the iphone, sjlj (set-jump-long-jump) exceptions are used, which incur a
   //  performance overhead.  It seems that the overhead in only in routines that call
   //  malloc/new.  This is not called very often, so the overhead should be minimal.
   //  However, gcc inlines this function!  requiring every alloc the have sjlj overhead.
   //  Making it virtual prevents the overhead.
   virtual void AllocMoreBlocks()
   {
      #ifdef USE_POSIX_MEMALIGN
      enum { newBlockCount = 1 };
      #else
      enum { newBlockCount = 1<<(IMMIX_BLOCK_GROUP_BITS) };
      #endif

      // Currently, we only have 2 bytes for a block header
      if (mAllBlocks.size()+newBlockCount >= 0xfffe )
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Block id count used - collect");
         #endif
         return;
      }


      #ifdef USE_POSIX_MEMALIGN
      // One aligned block that can be freed on its on
      int gid = 0;
      char *chunk = 0;
      int n = 1;
      #ifdef ANDROID
      chunk = (char *)memalign( IMMIX_BLOCK_SIZE, IMMIX_BLOCK_SIZE );
      #else
      posix_memalign( (void **)&chunk, IMMIX_BLOCK_SIZE, IMMIX_BLOCK_SIZE );
      #endif
      if (chunk==0)
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Memalign failed - try collect\n");
         #endif
         return;
      }
      char *aligned = chunk;

      #else // Not memalign

      // Find spare group...
      int gid = -1;
      for(int i=0;i<gAllocGroups.size();i++)
         if (!gAllocGroups[i].alloc)
         {
            gid = i;
            break;
         }
      if (gid<0)
        gid = gAllocGroups.next();

      char *chunk = (char *)malloc( 1<<(IMMIX_BLOCK_GROUP_BITS + IMMIX_BLOCK_BITS) );
      if (!chunk)
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Alloc failed - try collect\n");
         #endif
         return;
      }


      int n = 1<<IMMIX_BLOCK_GROUP_BITS;

      char *aligned = (char *)( (((size_t)chunk) + IMMIX_BLOCK_SIZE-1) & IMMIX_BLOCK_BASE_MASK);
      if (aligned!=chunk)
         n--;
      gAllocGroups[gid].alloc = chunk;
      // Only do one group allocation
      #endif


      for(int i=0;i<n;i++)
      {
         BlockData *block = (BlockData *)(aligned + i*IMMIX_BLOCK_SIZE);
         block->Init(gid);

         mAllBlocks.push(block);
         mFreeBlocks.push(block);
      }
      std::stable_sort(&mAllBlocks[0], &mAllBlocks[0] + mAllBlocks.size() );
      mAllBlocksCount = mAllBlocks.size();
      #ifdef SHOW_MEM_EVENTS
      GCLOG("Blocks %d = %d k\n", mAllBlocks.size(), (mAllBlocks.size() << IMMIX_BLOCK_BITS)>>10);
      #endif
   }


   BlockData * GetFreeBlock(int inRequiredBytes, int **inCallersStack)
   {
      volatile int dummy = 1;
      if (hx::gMultiThreadMode)
      {
         hx::EnterGCFreeZone();
         gThreadStateChangeLock->Lock();
         hx::ExitGCFreeZoneLocked();
      }

      BlockData *result = GetNextFree(inRequiredBytes);
      if (!result && (!sgInternalEnable || GetWorkingMemory()<sWorkingMemorySize))
      {
         AllocMoreBlocks();
         result = GetNextFree(inRequiredBytes);
      }

      if (!result)
      {
         volatile int dummy = 1;
         *inCallersStack = (int *)&dummy;
         Collect(false,false,true);
         result = GetNextFree(inRequiredBytes);
      }

      if (!result)
      {
         AllocMoreBlocks();
         result = GetNextFree(inRequiredBytes);
      }

      // Assume all wil be used
      mCurrentRowsInUse += result->GetFreeRows();

      BlockDataInfo &info = result->getInfo();

      if (hx::gMultiThreadMode)
         gThreadStateChangeLock->Unlock();

      while(!info.mReady)
      {
         // Spin
      }
      result->zero();

      return result;
  }


#if 0
   BlockData *GetEmptyBlock(bool inTryCollect)
   {
      if (mNextEmpty >= mEmptyBlocks.size())
      {
         int want_more = 0;
         if (inTryCollect)
         {
            want_more = Collect(false,false);
            if (!want_more)
               return 0;
         }
         else
         {
            want_more = 1<<IMMIX_BLOCK_GROUP_BITS;
         }

         // Allocate some more blocks...
         // Using simple malloc for now, so allocate a big chuck in case we have to
         //  waste space by doing block-aligning

         #ifdef SHOW_MEM_EVENTS
         GCLOG("Request %d blocks\n", want_more);
         #endif
         while(want_more>0)
         {
   
         }
      }

      BlockData *block = mEmptyBlocks[mNextEmpty++];
      block->ClearEmpty();
      mCurrentRowsInUse += IMMIX_USEFUL_LINES;
      return block;
   }
#endif

   void MoveSpecial(hx::Object *inTo, hx::Object *inFrom)
   {
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

       #ifdef HXCPP_TELEMETRY
       i = hx::hxtFinalizerMap.find(inFrom);
       if (i!=hx::hxtFinalizerMap.end())
       {
          hx::finalizer f = i->second;
          hx::hxtFinalizerMap.erase(i);
          hx::hxtFinalizerMap[inTo] = f;
          printf("HXT TODO: potential lost collection / ID collision, maintain alloc_map across move?");
       }
       #endif

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
   }



   #if 0 && defined(HXCPP_VISIT_ALLOCS) // {
   void MoveBlocks(BlockData **inFrom, BlockData **inTo, int inCount)
   {
      BlockData *dest = *inTo++;
      int destRow = IMMIX_HEADER_LINES;
      int destPos = 0;
      int moved = 0;
      
      for(int f=0;f<inCount;f++)
      {
         BlockData &from = *inFrom[f];
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Move from %p -> %p\n", &from, dest );
         #endif

         for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
         {
            if ((from.mRowMarked[r] & (IMMIX_ROW_HAS_OBJ_LINK|IMMIX_ROW_MARKED)) ==
                               (IMMIX_ROW_HAS_OBJ_LINK|IMMIX_ROW_MARKED) )
            {
               int pos = (from.mRowMarked[r] & IMMIX_ROW_LINK_MASK);
               unsigned char *row = from.mRow[r];
   
               while(true)
               {
                  int next = row[pos+ENDIAN_OBJ_NEXT_BYTE];

                  if (row[pos+ENDIAN_MARK_ID_BYTE_HEADER] == gByteMarkID)
                  {
                     unsigned int &flags = *(unsigned int *)(row+pos);
                     bool isObject = flags & IMMIX_ALLOC_IS_CONTAINER;
                     int size = flags & IMMIX_ALLOC_SIZE_MASK;
                     void **ptr = (void **)(&flags + 1);


                     // Find some room
                     int s = size +sizeof(int);
                     int extra_lines = (s + destPos - 1) >> IMMIX_LINE_BITS;
                     // New block ...
                     if (destRow+extra_lines >= IMMIX_LINES)
                     {
                        // Fill in stats of old block...
                        dest->FillTo(destRow,destPos);
 
                        dest = *inTo++;
                        destRow = IMMIX_HEADER_LINES;
                        destPos = 0;
                        extra_lines = (s + destPos -1) >> IMMIX_LINE_BITS;

                        #ifdef SHOW_MEM_EVENTS
                        GCLOG("          %p -> %p\n", &from, dest );
                        #endif
                     }
                     
                     // Append to current position...
                     unsigned char *row = dest->mRow[destRow];
                     unsigned char &row_flag = dest->mRowMarked[destRow];

                     int *result = (int *)(row + destPos);
                     *result = size | hx::gMarkID | (row_flag<<16) |
                           (extra_lines==0 ? IMMIX_ALLOC_SMALL_OBJ : IMMIX_ALLOC_MEDIUM_OBJ );

                     if (isObject)
                        *result |= IMMIX_ALLOC_IS_CONTAINER;

                     row_flag =  destPos | IMMIX_ROW_HAS_OBJ_LINK | IMMIX_ROW_MARKED;

                     destRow += extra_lines;
                     destPos = (destPos + s) & (IMMIX_LINE_LEN-1);
                     if (destPos==0)
                        destRow++;

                     if (isObject)
                        MoveSpecial((hx::Object *)result+1,(hx::Object *)ptr);
                     // Result has moved - store movement in original position...
                     memcpy(result+1, ptr, size);
                     *ptr = result + 1;
                     flags = IMMIX_OBJECT_HAS_MOVED;

                     moved++;
                  }
   
                  // Next allocation...
                  if (! (next & IMMIX_ROW_HAS_OBJ_LINK) )
                     break;
                  pos = next & IMMIX_ROW_LINK_MASK;
               }
            }
         }

         // Mark as free
         BlockDataInfo &info = from.getInfo();
         info.mUsedRows = 0;
      }

      #ifdef SHOW_MEM_EVENTS
      GCLOG("Moved %d\n", moved);
      #endif

      // Fill in stats of last block...
      dest->FillTo(destRow,destPos);

      AdjustPointers();
   }

   void AdjustPointers()
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
               //GCLOG("Found object to  %p -> %p\n", *ioPtr,  (*(hx::Object ***)ioPtr)[0]);
               *ioPtr = (*(hx::Object ***)ioPtr)[0];
            }
         }

         void visitAlloc(void **ioPtr)
         {
            if ( ((*(unsigned int **)ioPtr)[-1]) == IMMIX_OBJECT_HAS_MOVED )
            {
               //GCLOG("Found reference to  %p -> %p\n", *ioPtr,  (*(void ***)ioPtr)[0]);
               *ioPtr = (*(void ***)ioPtr)[0];
            }
         }
      };


      AdjustPointer *adjust = new AdjustPointer(this);
      VisitAll(adjust);

      delete adjust;
   }

   #ifndef USE_POSIX_MEMALIGN // {
   bool EvacuateGroup(int inGid)
   {
      BlockData *from[IMMIX_MAX_ALLOC_GROUPS_SIZE];
      BlockData *to[IMMIX_MAX_ALLOC_GROUPS_SIZE];
      int fromCount = 0;
      int toCount = 0;

      int fromRows = 0;

      for(int i=0;i<mAllBlocks.size();i++)
      {
         BlockDataInfo &info = mAllBlocks[i]->getInfo();
         if (info.mGroupId == inGid)
         {
            if ( info.mUsedRows>0 )
            {
               fromRows += info.mUsedRows;
               from[fromCount++] = mAllBlocks[i];
            }
         }
         else if (info.mUsedRows==0 && toCount<IMMIX_MAX_ALLOC_GROUPS_SIZE)
         {
            to[toCount++] = mAllBlocks[i];
         }
      }

      if (toCount*IMMIX_USEFUL_LINES > fromRows)
      {
         while(toCount<fromCount)
            to[toCount++] = 0;
         #ifdef SHOW_MEM_EVENTS
         GCLOG("EvacuateGroup %d\n", inGid );
         #endif
         MoveBlocks(from,to,fromCount);
         return true;
      }
   
      #ifdef SHOW_MEM_EVENTS
      GCLOG("EvacuateGroup %d - not enough room (%d/%d)\n", inGid, toCount*IMMIX_USEFUL_LINES, fromRows);
      #endif
      return false;
   }
   #endif // !USE_POSIX_MEMALIGN }
   #endif // HXCPP_VISIT_ALLOCS }

   #ifndef USE_POSIX_MEMALIGN // {
   void ReleaseGroup(int inGid)
   {
      int remaining=0;
      for(int i=0; i<mAllBlocks.size();  )
      {
         if (mAllBlocks[i]->getInfo().mGroupId==inGid)
         {
            mAllBlocks[i]->destroy();
            mAllBlocks.erase(i);
         }
         else
         {
            remaining++;
            i++;
         }
      }

      free(gAllocGroups[inGid].alloc);
      gAllocGroups[inGid].alloc = 0;

      #ifdef SHOW_MEM_EVENTS
      GCLOG("Release group %d-> %d blocks left = %dk\n", inGid, remaining, remaining<<(IMMIX_BLOCK_BITS-10) );
      #endif
   }
   #endif // !USE_POSIX_MEMALIGN }


   bool ReleaseBlocks(int inBlocks)
   {
      #ifdef USE_POSIX_MEMALIGN
      for(int i=0; i<mAllBlocks.size();  )
      {
         BlockDataInfo &block = mAllBlocks[i]->getInfo();
         if (!block.mPinned && !block.mUsedRows)
         { 
            #ifdef SHOW_MEM_EVENTS
            GCLOG("Release block %d (%p) = %dk\n",block.mPtr->mId, block.mPtr,  (mAllBlocks.size()-1)<<(IMMIX_BLOCK_BITS-10) );
            #endif
            mAllBlocks[i]->destroy();
            mAllBlocks.erase(i);
            inBlocks--;
            if (inBlocks==0)
               return true;
         }
         else
            i++;
      }

      #ifdef HXCPP_VISIT_ALLOCS
      // TODO: do some joining to free some blocks
      #endif

      return true;

      #else

      for(int i=0;i<gAllocGroups.size();i++)
         gAllocGroups[i].clear();

      for(int i=0; i<mAllBlocks.size(); i++ )
      {
         BlockDataInfo &block = mAllBlocks[i]->getInfo();
         GroupInfo &g = gAllocGroups[block.mGroupId];
         if (block.mPinned)
            g.pinned = true;
         g.used += block.mUsedRows;
         g.free += IMMIX_USEFUL_LINES - block.mUsedRows;
      }

      int bestGroup = -1;
      int bestFree = 0;
      int pinned = 0;
      int active = 0;
      for(int i=0;i<gAllocGroups.size();i++)
      {
         GroupInfo &g = gAllocGroups[i];
         if (g.alloc)
         {
            active++;
            if (g.pinned)
               pinned++;
            else
            {
               //GCLOG("Group %d/%d, free = %d\n", i, gAllocGroups.size(), g.free );
   
               // Release this group - it's all free
               if (!g.used)
               {
                  ReleaseGroup(i);
                  return true;
               }
         
               if ( g.free>=bestFree )
               {
                  bestFree = g.free;
                  bestGroup = i;
               }
            }
         }
      }

      #if defined(HXCPP_VISIT_ALLOCS) && defined(HXCPP_GC_MOVING)
      if (bestGroup>=0)
      {
         if (EvacuateGroup(bestGroup))
            ReleaseGroup(bestGroup);
         return true;
      }
      #endif

      // No unpinned groups ...
      #ifdef SHOW_MEM_EVENTS
      GCLOG("Could not release, %d/%d pinned.\n",pinned,active);
      #endif

      #endif // !USE_POSIX_MEMALIGN
      return false;
   }
 
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
         id = hx::sFreeObjectIds.pop();
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
         mAllBlocks[i]->ClearRowMarks();
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void VisitAll(hx::VisitContext *inCtx)
   {
      for(int i=0;i<mAllBlocks.size();i++)
         mAllBlocks[i]->VisitBlock(inCtx);

      hx::VisitClassStatics(inCtx);

      for(hx::RootSet::iterator i = hx::sgRootSet.begin(); i!=hx::sgRootSet.end(); ++i)
      {
         hx::Object **obj = &**i;
         if (*obj)
         {
            inCtx->visitObject(obj);
            (*obj)->__Visit(inCtx);
         }
      }
      for(int i=0;i<hx::sZombieList.size();i++)
      {
         inCtx->visitObject( &hx::sZombieList[i] );

         hx::sZombieList[i]->__Visit(inCtx);
      }

   }
   #endif

   #if defined(HXCPP_GC_MOVING) && defined(HXCPP_VISIT_ALLOCS)
   void TryDefrag()
   {
      // Try compacting ...
      enum { MAX_DEFRAG = 64 };
      enum { MIN_DEFRAG = 2 };
      enum { DEFRAG_HOLES = 5 };
      BlockData *from[MAX_DEFRAG]; 
      BlockData *to[MAX_DEFRAG]; 
      int fromCount = 0;
      int toCount = 0;

      for(int i=0;i<mAllBlocks.size();i++)
      {
         BlockData *block = mAllBlocks[i];
         BlockDataInfo &info = block->getInfo();

         if (info.mUsedRows==0)
         {
            if (toCount<MAX_DEFRAG)
               to[toCount++] = block;
         }
         else if (!info.mPinned && info.mHoles>=DEFRAG_HOLES && fromCount<MAX_DEFRAG )
         {
            from[fromCount++] = block;
         }
      }

      if (fromCount>MIN_DEFRAG)
      {
         int n = std::min(fromCount,toCount);
         // Not enough blocks, but perhaps enough memory?
         if (toCount<fromCount)
         {
            int rows = toCount * IMMIX_USEFUL_LINES;
            n = 0;
            for(n=0; n<fromCount; n++)
            {
               rows -= from[n]->getInfo().mUsedRows;
               if (rows<0)
                  break;
            }
         }
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Defrag %d/%d (%d)\n", fromCount, n, toCount);
         #endif
         if (n)
            MoveBlocks(from,to,n);
      }
   }
   #endif

   void ReclaimAsync(int inThreadId)
   {
      while(!sgThreadPoolAbort || sgThreadPoolJob==tpjReclaimFull)
      {
         int blockId = HxAtomicInc( &mNextReclaim );
         if (blockId<mReclaimList.size())
         {
            mReclaimList[blockId]->Reclaim( sgThreadPoolJob==tpjReclaimFull, true);
         }
         else
            break;
      }

      ThreadPoolAutoLock l(sThreadPoolLock);
      sRunningThreads &= ~(1<<inThreadId);
      if (!sRunningThreads)
         SignalThreadPool(sThreadJobDone,sThreadJobDoneSleeping);
   }


   void ThreadLoop(int inId)
   {
      hx::MarkContext context(inId);

      while(true)
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

            if (sgThreadPoolJob==tpjMark)
            {
               // Pthread Api requires that we have the lock - so may as well use it
               // See if there is a chunk waiting...
               context.InitChunkLocked();
               // Otherwise fall though - there might be an array job
            }
         }
         #else
         sThreadWake[inId].Wait();
         #endif


         if (sgThreadPoolJob==tpjMark)
            context.Process();
         else
            ReclaimAsync(inId);
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
      #else
         #ifdef HX_WINRT
         // TODO
         #elif defined(EMSCRIPTEN)
         // Only one thread
         #elif defined(HX_WINDOWS)
         bool ok = _beginthreadex(0,0,SThreadLoop,info,0,0) != 0;
         #endif
      #endif
   }

   void StopThreadJobs(bool inKill)
   {
      if (sAllThreads)
      {
         if (inKill)
            sgThreadPoolAbort = true;

         #ifdef HX_GC_PTHREADS
         ThreadPoolAutoLock lock(sThreadPoolLock);
         sThreadJobDoneSleeping = true;
         while(sRunningThreads)
             WaitThreadLocked(sThreadJobDone);
         sThreadJobDoneSleeping = false;
         #else
         sThreadJobDone.Wait();
         #endif
         sgThreadPoolAbort = false;
         sAllThreads = 0;
      }
   }


   void StartThreadJobs(ThreadPoolJob inJob, int inWorkers, bool inWait, int inThreadLimit = -1)
   {
      if (!inWorkers)
         return;

      if (!sThreadPoolInit)
      {
         sThreadPoolInit = true;
         for(int i=0;i<MAX_MARK_THREADS;i++)
            CreateWorker(i);
      }

      #ifdef HX_GC_PTHREADS
      ThreadPoolAutoLock lock(sThreadPoolLock);
      #endif

      sgThreadPoolJob = inJob;

      sgThreadCount = inThreadLimit<0 ? MAX_MARK_THREADS : std::min((int)MAX_MARK_THREADS, inThreadLimit) ;

      int start = std::min(inWorkers, sgThreadCount );

      sAllThreads = (1<<sgThreadCount) - 1;

      sRunningThreads = (1<<start) - 1;

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
         sThreadJobDone.Wait();
         #endif

         sAllThreads = 0;
      }
   }


   void MarkAll()
   {
      // The most-significant header byte looks like:
      // C nH Odd Even  c c c c
      //  C = "is const alloc bit" - in this case Odd and Even will be false
      // nH = non-hashed const string bit
      // Odd = true if cycle is odd
      // Even = true if cycle is even
      // c c c c = 4 bit cycle code
      //
      hx::gPrevMarkIdMask = hx::gMarkID & 0x30000000;

      // 4 bits of cycle
      gByteMarkID = (gByteMarkID + 1) & 0x0f;
      if (gByteMarkID & 0x1)
         gByteMarkID |= 0x20;
      else
         gByteMarkID |= 0x10;

      hx::gMarkID = gByteMarkID << 24;
      hx::gMarkIDWithContainer = hx::gMarkID | IMMIX_ALLOC_IS_CONTAINER;

      gBlockStack = 0;

      ClearRowMarks();

      mMarker.init();

      hx::MarkClassStatics(&mMarker);

      for(hx::RootSet::iterator i = hx::sgRootSet.begin(); i!=hx::sgRootSet.end(); ++i)
      {
         hx::Object *&obj = **i;
         if (obj)
            hx::MarkObjectAlloc(obj , &mMarker );
      }

      // Mark zombies too....
      for(int i=0;i<hx::sZombieList.size();i++)
         hx::MarkObjectAlloc(hx::sZombieList[i] , &mMarker );

      // Mark local stacks
      for(int i=0;i<mLocalAllocs.size();i++)
         MarkLocalAlloc(mLocalAllocs[i] , &mMarker);

      #ifdef HXCPP_SCRIPTABLE
      scriptMarkStack(&mMarker);
      #endif


      if (MAX_MARK_THREADS>1)
      {
         mMarker.releaseJobsLocked();

         // Unleash the workers...
         StartThreadJobs(tpjMark, hx::sGlobalChunks.chunks.size(), true);
      }
      else
      {
         mMarker.Process();
      }

      hx::FindZombies(mMarker);

      hx::RunFinalizers();
   }





   void Collect(bool inMajor, bool inForceCompact, bool inLocked=false)
   {
      // If we set the flag from 0 -> 0xffffffff then we are the collector
      //  otherwise, someone else is collecting at the moment - so wait...
      if (!HxAtomicExchangeIf(0, 0xffffffff,(volatile int *)&hx::gPauseForCollect))
      {
         hx::PauseForCollect();
         return;
      }

      // We are the collector - all must wait for us
      LocalAllocator *this_local = 0;
      if (hx::gMultiThreadMode)
      {
         this_local = (LocalAllocator *)(hx::ImmixAllocator *)hx::tlsImmixAllocator;

         if (!inLocked)
            gThreadStateChangeLock->Lock();

         for(int i=0;i<mLocalAllocs.size();i++)
            if (mLocalAllocs[i]!=this_local)
               WaitForSafe(mLocalAllocs[i]);
      }

      StopThreadJobs(true);
      #ifdef DEBUG
      sgAllocsSinceLastSpam = 0;
      #endif

      HX_STACK_FRAME("GC", "collect", 0, "GC::collect", __FILE__, __LINE__,0)
      #ifdef SHOW_MEM_EVENTS
      int here = 0;
      GCLOG("=== Collect === %p\n",&here);
      #endif
      STAMP(t0)

      #ifdef HXCPP_TELEMETRY
      __hxt_gc_start();
      #endif


      // Now all threads have mTopOfStack & mBottomOfStack set.

      STAMP(t1)

      MarkAll();

      STAMP(t2)

      // Sweep large
      int idx = 0;
      while(idx<mLargeList.size())
      {
         unsigned int *blob = mLargeList[idx];
         if ( (blob[1] & IMMIX_ALLOC_MARK_ID) != hx::gMarkID )
         {
            mLargeAllocated -= *blob;
            free(mLargeList[idx]);
            mLargeList.qerase(idx);
         }
         else
            idx++;
      }

      STAMP(t3)


      // Setup memory target ...
      // Count free rows, and prep blocks for sorting
      mRowsInUse = 0;
      for(int i=0;i<mAllBlocks.size();i++)
         mRowsInUse += mAllBlocks[i]->CountUsedRows();

      int mem = (mRowsInUse<<IMMIX_LINE_BITS);
      int targetFree = std::max(hx::sgMinimumFreeSpace, mem*hx::sgTargetFreeSpacePercentage/100 );
      sWorkingMemorySize = std::max( mem + targetFree, hx::sgMinimumWorkingMemory);
 
      // Large alloc target
      int blockSize =  mAllBlocks.size()<<IMMIX_BLOCK_BITS;
      if (blockSize > mLargeAllocSpace)
         mLargeAllocSpace = blockSize;
      mLargeAllocForceRefresh = mLargeAllocated + mLargeAllocSpace;

      mTotalAfterLastCollect = MemUsage();



      // Compact/Defrag?
      #if defined(HXCPP_GC_MOVING) && defined(HXCPP_VISIT_ALLOCS)
      // TODO - 'to' blocks are not reclaimed at this stage
      bool released = false;
      if (inForceCompact)
         released = ReleaseBlocks( mAllBlocks.size() );
      else if (sgInternalEnable)
      {
         int releaseBytes = blockSize - sWorkingMemorySize;
         if (releaseBytes>0)
         {
            int releaseGroups = releaseBytes >> (IMMIX_BLOCK_SIZE<<IMMIX_BLOCK_GROUP_BITS);
            if (releaseGroups)
                released = ReleaseBlocks(releaseGroups<<IMMIX_BLOCK_GROUP_BITS);
         }
      }
      if (!released && full)
         TryDefrag();
      #endif



      // Reclaim ...

      // Update table entries?  This needs to be done before the gMarkId count clocks
      //  back to the same number
      sgTimeToNextTableUpdate--;
      if (sgTimeToNextTableUpdate<0)
         sgTimeToNextTableUpdate = 15;
      bool full = inMajor || (sgTimeToNextTableUpdate==0) || inForceCompact;


      mFreeBlocks.clear();
      mNextFreeBlock = 0;
      mNextReclaim = 0;

      bool asyncReclaim =  MAX_MARK_THREADS > 1;

      for(int i=0;i<mAllBlocks.size();i++)
      {
         BlockData *block = mAllBlocks[i];
         if (block->getUsedRows() < IMMIX_USEFUL_LINES-4)
         {
            mFreeBlocks.push(block);
            if (asyncReclaim)
               block->SetUnreclaimed();
            else
               block->Reclaim(full,false);
         }
      }
      std::sort(&mFreeBlocks[0], &mFreeBlocks[0] + mFreeBlocks.size(), MostUsedFirst );

      mReclaimList.clear();
      if (asyncReclaim)
      {
         if (mFreeBlocks.size()<=4)
         {
            for(int i=0;i<mFreeBlocks.size();i++)
               mFreeBlocks[i]->Reclaim(full,false);
         }
         else
         {
            mReclaimList.setSize(mFreeBlocks.size());
            memcpy( &mReclaimList[0], &mFreeBlocks[0], mFreeBlocks.size()*sizeof(void *));

            // Clear the first few synchnonously, to avoid spin lock..
            for(int i=0;i<4;i++)
               mReclaimList[mNextReclaim++]->Reclaim(full,false);

            // Only use one thread for parallel zeroing.  Try to get though the work wihout
            //  slowing down the main thread
            StartThreadJobs(full ? tpjReclaimFull : tpjReclaim, mReclaimList.size(), false, 1);
         }
      }

      mAllBlocksCount   = mAllBlocks.size();
      mCurrentRowsInUse = mRowsInUse;

      #ifdef SHOW_MEM_EVENTS
      GCLOG("Collect Done\n");
      #endif

      #ifdef PROFILE_COLLECT
      STAMP(t4)
      GCLOG("Collect time %.2f  %.2f/%.2f/%.2f/%.2f\n", (t4-t0)*1000,
              (t1-t0)*1000, (t2-t1)*1000, (t3-t2)*1000, (t4-t3)*1000 );
      #endif

      #ifdef HXCPP_TELEMETRY
      __hxt_gc_end();
      #endif


      hx::gPauseForCollect = 0x00000000;
      if (hx::gMultiThreadMode)
      {
         for(int i=0;i<mLocalAllocs.size();i++)
         if (mLocalAllocs[i]!=this_local)
            ReleaseFromSafe(mLocalAllocs[i]);

         if (!inLocked)
            gThreadStateChangeLock->Unlock();
      }
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
         if (block==mAllBlocks[0])
            return true;
         if (block==mAllBlocks[max])
            return true;
         if (block>mAllBlocks[0] && block<mAllBlocks[max])
         {
            while(min<max-1)
            {
               int mid = (max+min)>>1;
               if (mAllBlocks[mid]==block)
                  return true;

               if (mAllBlocks[mid]<block)
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

   hx::MarkContext mMarker;

   int mNextFreeBlock;
   volatile int mNextReclaim;

   BlockList mAllBlocks;
   BlockList mFreeBlocks;
   BlockList mReclaimList;
   LargeList mLargeList;
   MyMutex    mLargeListLock;
   hx::QuickVec<LocalAllocator *> mLocalAllocs;
   LocalAllocator *mLocalPool[LOCAL_POOL_SIZE];
};



namespace hx
{

void MarkConservative(int *inBottom, int *inTop,hx::MarkContext *__inCtx)
{
   #ifdef SHOW_MEM_EVENTS
   GCLOG("Mark conservative %p...%p (%d)\n", inBottom, inTop, (int)(inTop-inBottom) );
   #endif

   #ifdef EMSCRIPTEN
   if (inTop<inBottom)
      std::swap(inBottom,inTop);
   #endif

   if (sizeof(int)==4 && sizeof(void *)==8)
   {
      // Can't start pointer on last integer boundary...
      inTop--;
   }


   void *prev = 0;
   for(int *ptr = inBottom ; ptr<inTop; ptr++)
   {
      void *vptr = *(void **)ptr;
      MemType mem;
      if (vptr && !((size_t)vptr & 0x03) && vptr!=prev &&
              (mem = sGlobalAlloc->GetMemType(vptr)) != memUnmanaged )
      {
         if (mem==memLarge)
         {
            unsigned char &mark = ((unsigned char *)(vptr))[ENDIAN_MARK_ID_BYTE];
            if (mark!=gByteMarkID)
               mark = gByteMarkID;
         }
         else
         {
            BlockData *block = (BlockData *)( ((size_t)vptr) & IMMIX_BLOCK_BASE_MASK);
            int pos = (int)(((size_t)vptr) & IMMIX_BLOCK_OFFSET_MASK);
            AllocType t = block->GetAllocType(pos-sizeof(int),false);
            if ( t==allocObject )
            {
               //GCLOG(" Mark object %p (%p)\n", vptr,ptr);
               HX_MARK_OBJECT( ((hx::Object *)vptr) );
               block->pin();
            }
            else if (t==allocString)
            {
               // GCLOG(" Mark string %p (%p)\n", vptr,ptr);
               HX_MARK_STRING(vptr);
               block->pin();
            }
            else if (t==allocMarked)
            {
               block->pin();
            }
         }
      }
      // GCLOG(" rejected %p %p %d %p %d=%d\n", ptr, vptr, !((size_t)vptr & 0x03), prev,
      //    sGlobalAlloc->GetMemType(vptr) , memUnmanaged );
   }
}

} // namespace hx


// --- LocalAllocator -------------------------------------------------------
//
// One per thread ...

static int sLocalAllocatorCount = 0;
class LocalAllocator : public hx::ImmixAllocator
{
public:
   LocalAllocator(int *inTopOfStack=0)
   {
      mTopOfStack = mBottomOfStack = inTopOfStack;
      mRegisterBufSize = 0;
      mGCFreeZone = false;
      mStackLocks = 0;
      Reset();
      sGlobalAlloc->AddLocal(this);
      sLocalAllocatorCount++;
      #ifdef HX_WINDOWS
      mID = GetCurrentThreadId();
      #endif
   }

   ~LocalAllocator()
   {
      EnterGCFreeZone();
      sGlobalAlloc->RemoveLocal(this);
      hx::tlsImmixAllocator = 0;

      sLocalAllocatorCount--;
   }

   void AttachThread(int *inTopOfStack)
   {
      mTopOfStack = mBottomOfStack = inTopOfStack;
      mRegisterBufSize = 0;
      mStackLocks = 0;
      #ifdef HX_WINDOWS
      mID = GetCurrentThreadId();
      #endif

      // It is in the free zone - wait for 'SetTopOfStack' to activate
      mGCFreeZone = true;
   }

   void ReturnToPool()
   {
      #ifdef HX_WINDOWS
      mID = 0;
      #endif
      if (!sGlobalAlloc->ReturnToPool(this))
         delete this;
      hx::tlsImmixAllocator = 0;
   }

   void Reset()
   {
      allocBase = 0;
      mCurrentHole = 0;
      mCurrentHoles = 0;
      spaceEnd = 0;
      spaceStart = 0;
      mMoreHoles = false;
   }

   void SetTopOfStack(int *inTop,bool inForce)
   {
      if (inTop)
      {
         // stop early to allow for ptr[1] ....
         if (inTop>mTopOfStack || (inForce && mStackLocks==0) )
            mTopOfStack = inTop;

         if (inForce)
            mStackLocks++;

         if (mGCFreeZone)
            ExitGCFreeZone();
      }
      else
      {
         if (inForce)
            mTopOfStack = 0;

         if (!mGCFreeZone)
            EnterGCFreeZone();

         if (inForce)
         {
            mStackLocks--;
            if (mStackLocks<=0)
            {
               mStackLocks = 0;
               ReturnToPool();
            }
         }
      }
   }

   void SetBottomOfStack(int *inBottom)
   {
      mBottomOfStack = inBottom;
   }

   void SetupStack()
   {
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      SetTopOfStack(mBottomOfStack,false);
      CAPTURE_REGS;
   }


   void PauseForCollect()
   {
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      CAPTURE_REGS;
      mReadyForCollect.Set();
      mCollectDone.Wait();
   }

   void EnterGCFreeZone()
   {
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      mGCFreeZone = true;
      if (mTopOfStack)
      {
         CAPTURE_REGS;
      }
      mReadyForCollect.Set();
   }

   void ExitGCFreeZone()
   {
      #ifdef HXCPP_DEBUG
      if (!mGCFreeZone)
         CriticalGCError("GCFree Zone mismatch");
      #endif

      if (hx::gMultiThreadMode)
      {
         AutoLock lock(*gThreadStateChangeLock);
         mReadyForCollect.Reset();
         mGCFreeZone = false;
      }
   }
        // For when we already hold the lock
   void ExitGCFreeZoneLocked()
   {
      if (hx::gMultiThreadMode)
      {
         mReadyForCollect.Reset();
         mGCFreeZone = false;
      }
   }



   // Called by the collecting thread to make sure this allocator is paused.
   // The collecting thread has the lock, and will not be releasing it until
   //  it has finished the collect.
   void WaitForSafe()
   {
      if (!mGCFreeZone)
         mReadyForCollect.Wait();
   }

   void ReleaseFromSafe()
   {
      if (!mGCFreeZone)
         mCollectDone.Set();
   }



   void *CallAlloc(int inSize,unsigned int inObjectFlags)
   {
      #ifdef HXCPP_DEBUG
      if (mGCFreeZone)
         CriticalGCError("Allocating from a GC-free thread");
      #endif
      if (hx::gPauseForCollect)
         PauseForCollect();

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

      while(1)
      {
         if (spaceStart + allocSize + skip4 <= spaceEnd)
         {
            #ifdef HXCPP_ALIGN_ALLOC
            spaceStart += skip4;
            #endif

            int startRow = spaceStart>>IMMIX_LINE_BITS;
            allocStartFlags[ startRow ] |= 1<<( (spaceStart>>2) & 31);

            int endRow = 1 + ((spaceStart+allocSize-1)>>IMMIX_LINE_BITS);

            unsigned char *buffer = allocBase + spaceStart;
            *(unsigned int *)(buffer) = inObjectFlags |
                  hx::gMarkID | (inSize<<IMMIX_ALLOC_SIZE_SHIFT) | (endRow-startRow);

            spaceStart += allocSize;
            return buffer + sizeof(int);
         }
         else if (mMoreHoles)
         {
            spaceStart = mCurrentRange[mCurrentHole].start;
            spaceEnd = spaceStart + mCurrentRange[mCurrentHole].length;
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
            volatile int dummy = 1;
            mBottomOfStack = (int *)&dummy;
            CAPTURE_REGS;

            BlockData *block = sGlobalAlloc->GetFreeBlock(allocSize,&mBottomOfStack);
            allocBase = (unsigned char *)block;
            BlockDataInfo &info = block->getInfo();
            mCurrentRange = info.mRanges;
            allocStartFlags = info.allocStart;
            mCurrentHoles = info.mHoles;
            spaceStart = mCurrentRange->start;
            spaceEnd = spaceStart + mCurrentRange->length;
            mCurrentHole = 1;
            mMoreHoles = mCurrentHole<mCurrentHoles;
         }
      }
      return 0;
   }


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
      hx::MarkConservative(mBottomOfStack, mTopOfStack , __inCtx);
      MarkSetMember("Registers",__inCtx);
      hx::MarkConservative(CAPTURE_REG_START, CAPTURE_REG_END, __inCtx);
      MarkPopClass(__inCtx);
      #else
      hx::MarkConservative(mBottomOfStack, mTopOfStack , __inCtx);
      hx::MarkConservative(CAPTURE_REG_START, CAPTURE_REG_END, __inCtx);
      #endif

      Reset();
   }

   int            mCurrentHole;
   int            mCurrentHoles;
   HoleRange     *mCurrentRange;

     bool           mMoreHoles;

   int *mTopOfStack;
   int *mBottomOfStack;

   hx::RegisterCaptureBuffer mRegisterBuf;
   int                   mRegisterBufSize;

   bool            mGCFreeZone;
   int             mStackLocks;
   int             mID;
   MySemaphore     mReadyForCollect;
   MySemaphore     mCollectDone;
};




inline LocalAllocator *GetLocalAlloc()
{
   #ifndef HXCPP_SINGLE_THREADED_APP
   if (hx::gMultiThreadMode)
   {
      #ifdef HXCPP_DEBUG
      LocalAllocator *result = (LocalAllocator *)(hx::ImmixAllocator *)hx::tlsImmixAllocator;
      if (!result)
      {
         hx::BadImmixAlloc();
      }
      return result;
      #else
      return (LocalAllocator *)(hx::ImmixAllocator *)hx::tlsImmixAllocator;
      #endif
   }
   #endif

   return (LocalAllocator *)hx::gMainThreadAlloc;
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


void CollectFromThisThread()
{
   LocalAllocator *la = GetLocalAlloc();
   la->SetupStack();
   sGlobalAlloc->Collect(true,false);
}

namespace hx
{

void PauseForCollect()
{
   GetLocalAlloc()->PauseForCollect();
}



void EnterGCFreeZone()
{
   if (hx::gMultiThreadMode)
   {
      LocalAllocator *tla = GetLocalAlloc();
      tla->EnterGCFreeZone();
   }
}

void ExitGCFreeZone()
{
   if (hx::gMultiThreadMode)
   {
      LocalAllocator *tla = GetLocalAlloc();
      tla->ExitGCFreeZone();
   }
}

void ExitGCFreeZoneLocked()
{
   if (hx::gMultiThreadMode)
   {
      LocalAllocator *tla = GetLocalAlloc();
      tla->ExitGCFreeZoneLocked();
   }
}

void InitAlloc()
{
   hx::CommonInitAlloc();
   sgAllocInit = true;
   sGlobalAlloc = new GlobalAllocator();
   sgFinalizers = new FinalizerList();
   sFinalizerLock = new MyMutex();
   sGCRootLock = new MyMutex();
   hx::Object tmp;
   void **stack = *(void ***)(&tmp);
   sgObject_root = stack[0];
   //GCLOG("__root pointer %p\n", sgObject_root);
   gMainThreadAlloc =  new LocalAllocator();
   tlsImmixAllocator = gMainThreadAlloc;
   for(int i=0;i<IMMIX_LINE_LEN;i++)
      gImmixStartFlag[i] = 1<<( i>>2 ) ;
}


void GCPrepareMultiThreaded()
{
   if (!hx::gMultiThreadMode)
      hx::gMultiThreadMode = true;
}


void SetTopOfStack(int *inTop,bool inForce)
{
   if (inTop)
   {
      if (!sgAllocInit)
         InitAlloc();
      else
      {
         if (tlsImmixAllocator==0)
         {
            GCPrepareMultiThreaded();
            RegisterCurrentThread(inTop);
         }
      }
   }

   LocalAllocator *tla = (LocalAllocator *)(hx::ImmixAllocator *)tlsImmixAllocator;

   if (tla)
      tla->SetTopOfStack(inTop,inForce);
}



void *InternalNew(int inSize,bool inIsObject)
{
   //HX_STACK_FRAME("GC", "new", 0, "GC::new", "src/hx/GCInternal.cpp", __LINE__, 0)
   HX_STACK_FRAME("GC", "new", 0, "GC::new", "src/hx/GCInternal.cpp", inSize, 0)

   #ifdef HXCPP_DEBUG
   if (sgSpamCollects && sgAllocsSinceLastSpam>=sgSpamCollects)
   {
      //GCLOG("InternalNew spam\n");
      CollectFromThisThread();
   }
   sgAllocsSinceLastSpam++;
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

#ifndef ANDROID
   GetLocalAlloc()->SetupStack();
#endif
   sGlobalAlloc->Collect(inMajor,inCompact);

   return sGlobalAlloc->MemUsage();
}

inline unsigned int ObjectSize(void *inData)
{
   unsigned int header = ((unsigned int *)(inData))[-1];

   return (header & IMMIX_ALLOC_ROW_COUNT) ?
            ( (header & IMMIX_ALLOC_SIZE_MASK) >> IMMIX_ALLOC_SIZE_SHIFT) :
             ((unsigned int *)(inData))[-2];
}

void GCChangeManagedMemory(int inDelta, const char *inWhy)
{
   sGlobalAlloc->onMemoryChange(inDelta, inWhy);
}


void *InternalRealloc(void *inData,int inSize)
{
   if (inData==0)
      return hx::InternalNew(inSize,false);

   HX_STACK_FRAME("GC", "realloc", 0, "GC::relloc", __FILE__ , __LINE__, 0)

   #ifdef HXCPP_DEBUG
   if (sgSpamCollects && sgAllocsSinceLastSpam>=sgSpamCollects)
   {
      //GCLOG("InternalNew spam\n");
      CollectFromThisThread();
   }
   sgAllocsSinceLastSpam++;
   #endif

   unsigned int s = ObjectSize(inData);

   void *new_data = 0;

   if (inSize>=IMMIX_LARGE_OBJ_SIZE)
   {
      new_data = sGlobalAlloc->AllocLarge(inSize, false);
      if (inSize>s)
         ZERO_MEM((char *)new_data + s,inSize-s);
   }
   else
   {
      LocalAllocator *tla = GetLocalAlloc();

      new_data = tla->CallAlloc((inSize+3)&~3,0);
   }

#ifdef HXCPP_TELEMETRY
   //printf(" -- reallocating %018x to %018x, size from %d to %d\n", inData, new_data, s, inSize);
   __hxt_gc_realloc(inData, new_data, inSize);
#endif

   int min_size = s < inSize ? s : inSize;

   memcpy(new_data, inData, min_size );

   return new_data;
}

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
   tlsImmixAllocator = local;
}

void UnregisterCurrentThread()
{
   LocalAllocator *local = (LocalAllocator *)(hx::ImmixAllocator *)tlsImmixAllocator;
   delete local;
}



} // end namespace hx




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

int __hxcpp_gc_mem_info(int inWhich)
{
   switch(inWhich)
   {
      case MEM_INFO_USAGE:
         return (int)sGlobalAlloc->MemUsage();
      case MEM_INFO_RESERVED:
         return (int)sGlobalAlloc->MemReserved();
      case MEM_INFO_CURRENT:
         return (int)sGlobalAlloc->MemCurrent();
      case MEM_INFO_LARGE:
         return (int)sGlobalAlloc->MemLarge();
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

void __hxcpp_set_finalizer(Dynamic inObj, void *inFunc)
{
   GCSetHaxeFinalizer( inObj.mPtr, (hx::HaxeFinalizer) inFunc );
}

#ifdef HXCPP_TELEMETRY
void __hxcpp_set_hxt_finalizer(void* inObj, void *inFunc)
{
   GCSetHXTFinalizer( inObj, (hx::finalizer) inFunc );
}
#endif

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
   hx::Object *obj = inObj->__GetRealObject();
   if (!obj) return 0;
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

#if defined(HXCPP_GC_MOVING) || defined(HXCPP_FORCE_OBJ_MAP)
unsigned int __hxcpp_obj_hash(Dynamic inObj)
{
   return __hxcpp_obj_id(inObj);
}
#else
unsigned int __hxcpp_obj_hash(Dynamic inObj)
{
   if (!inObj.mPtr) return 0;
   hx::Object *obj = inObj->__GetRealObject();
   #if defined(HXCPP_M64)
   size_t h64 = (size_t)obj;
   return (unsigned int)(h64>>2) ^ (unsigned int)(h64>>32);
   #else
   return ((unsigned int)inObj.mPtr) >> 4;
   #endif
}
#endif




void DummyFunction(void *inPtr) { }

