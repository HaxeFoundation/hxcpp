#include <hxcpp.h>

#include <hx/GC.h>
#include <hx/Thread.h>
#include "Hash.h"

int gByteMarkID = 0;
int gMarkID = 0;

enum { gFillWithJunk = 0 } ;

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

#include "QuickVec.h"

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

static bool sgAllocInit = 0;
static bool sgInternalEnable = true;
static void *sgObject_root = 0;
int gInAlloc = false;

#if HX_HAS_ATOMIC
  #if defined(HX_MACOS) || defined(HX_WINDOWS) || defined(HX_LINUX)
  enum { MAX_MARK_THREADS = 4 };
  #else
  enum { MAX_MARK_THREADS = 2 };
  #endif
#else
  enum { MAX_MARK_THREADS = 1 };
#endif

enum { MARK_BYTE_MASK = 0x7f };


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
static bool sMultiThreadMode = false;

DECLARE_TLS_DATA(LocalAllocator, tlsLocalAlloc);

static void MarkLocalAlloc(LocalAllocator *inAlloc,hx::MarkContext *__inCtx);
static void WaitForSafe(LocalAllocator *inAlloc);
static void ReleaseFromSafe(LocalAllocator *inAlloc);
static void CollectFromThisThread();

namespace hx {
int gPauseForCollect = 0;
void ExitGCFreeZoneLocked();
#ifdef HXCPP_SCRIPTABLE
extern void scriptMarkStack(hx::MarkContext *);
#endif
}

//#define DEBUG_ALLOC_PTR ((char *)0xb68354)



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
#define IMMIX_LINE_BITS        7

#define IMMIX_BLOCK_SIZE        (1<<IMMIX_BLOCK_BITS)
#define IMMIX_BLOCK_OFFSET_MASK (IMMIX_BLOCK_SIZE-1)
#define IMMIX_BLOCK_BASE_MASK   (~(size_t)(IMMIX_BLOCK_OFFSET_MASK))
#define IMMIX_LINE_LEN          (1<<IMMIX_LINE_BITS)
#define IMMIX_LINE_COUNT_BITS   (IMMIX_BLOCK_BITS-IMMIX_LINE_BITS)
#define IMMIX_LINES             (1<<IMMIX_LINE_COUNT_BITS)
#define IMMIX_HEADER_LINES      (IMMIX_LINES>>IMMIX_LINE_BITS)
#define IMMIX_USEFUL_LINES      (IMMIX_LINES - IMMIX_HEADER_LINES)
#define IMMIX_LINE_POS_MASK     ((size_t)(IMMIX_LINE_LEN-1))
#define IMMIX_START_OF_ROW_MASK  (~IMMIX_LINE_POS_MASK)

#define IMMIX_MAX_ALLOC_GROUPS_SIZE  (1<<IMMIX_BLOCK_GROUP_BITS)



/*

 IMMIX Alloc Header - 32 bits


 The header is placed in the 4 bytes before the object pointer, and it is placed there using a uint32.
 When addressed as the uint32, you can use the bitmasks below to extract the various data.

 The "mark id" and "obj next" elements can conveniently be interpreted as bytes, however the offsets well
  be different depending on whether the system is little endian or big endian.


  Little endian - lsb first

 7   0 15 8 23 16 31 24
 -----------------------
 |    |    | OBJ | MID  | obj start here .....
 -----------------------



  Big endian - msb first

 31 24 23 16 15 8 7   0
 -----------------------
 |MID |OBJ |     |      | obj start here .....
 -----------------------

MID = ENDIAN_MARK_ID_BYTE = is measured from the object pointer
      ENDIAN_MARK_ID_BYTE_HEADER = is measured from the header pointer (4 bytes before object)

OBJ = ENDIAN_OBJ_NEXT_BYTE = start is measured from the header pointer


*/

#ifndef HXCPP_BIG_ENDIAN
#define ENDIAN_MARK_ID_BYTE        -1
#define ENDIAN_OBJ_NEXT_BYTE       2
#else
#define ENDIAN_MARK_ID_BYTE        -4
#define ENDIAN_OBJ_NEXT_BYTE       1
#endif
#define ENDIAN_MARK_ID_BYTE_HEADER (ENDIAN_MARK_ID_BYTE + 4)


#define IMMIX_ALLOC_MARK_ID     0xff000000
#define IMMIX_ALLOC_OBJ_NEXT    0x00ff0000
#define IMMIX_ALLOC_IS_OBJECT   0x00008000
#define IMMIX_ALLOC_IS_PINNED   0x00004000
#define IMMIX_ALLOC_SIZE_MASK   0x00003ffc
#define IMMIX_ALLOC_MEDIUM_OBJ  0x00000002
#define IMMIX_ALLOC_SMALL_OBJ   0x00000001






#define IMMIX_OBJECT_HAS_MOVED (~IMMIX_ALLOC_MEDIUM_OBJ)

/*

 IMMIX Row Header - 8 bits

*/
#define IMMIX_ROW_CLEAR           0x80
#define IMMIX_ROW_LINK_MASK       0x7C
#define IMMIX_ROW_HAS_OBJ_LINK    0x02
#define IMMIX_ROW_MARKED          0x01
#define IMMIX_NOT_MARKED_MASK     (~IMMIX_ROW_MARKED)


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

// For threaded marking
static int sActiveThreads = 0;
static int sRunningThreads = 0;
static bool sMarkThreadsInit = false;
static MySemaphore *sThreadWake[MAX_MARK_THREADS];
static MySemaphore *sMarkDone[MAX_MARK_THREADS];


struct AtomicLock
{
   AtomicLock() : mCount(0) { }

   void Lock()
   {
      if (sActiveThreads)
      {
         while(true)
         {
             if (HxAtomicInc(&mCount)==0)
                break;
             // nanosleep?
             HxAtomicDec(&mCount);
         }
      }
      else
         mCount++;
   }
   inline bool locked() { return mCount>0; }
   bool TryLock()
   {
      if (sActiveThreads)
      {
         if (HxAtomicInc(&mCount)==0)
            return true;
         HxAtomicDec(&mCount);
         return false;
      }
      else
      {
         if (mCount>0)
            return false;
         mCount++;
         return true;
      }
   }
   void Unlock()
   {
      if (sActiveThreads)
        HxAtomicDec(&mCount);
      else
        mCount--;
   }

   volatile int mCount;
};
typedef TAutoLock<AtomicLock> AutoAtomic;

AtomicLock gMarkMutex;
namespace hx { void MarkerReleaseWorkerLocked(); }


struct BlockDataInfo
{
   enum { MAX_STACK = 32 };

   int mId;
   int mGroupId;
   int mUsedRows;
   int mFreeInARow;
   int mHoles;
   union BlockData *mPtr;
   bool mPinned;
};

hx::QuickVec<BlockDataInfo> *gBlockInfo = 0;

union BlockData
{
   void Init(int inGID)
   {
      if (gFillWithJunk)
         memset(this,0x55,sizeof(*this));
      mId = nextBlockId();
      gBlockInfo->push( BlockDataInfo() );
      BlockDataInfo &info = getInfo();
      info.mId = mId;
      info.mGroupId = inGID;
      info.mUsedRows = 0;
      info.mFreeInARow = 0;
      info.mHoles = 0;
      info.mPinned = 0;
      info.mPtr = this;
   }
   inline int GetFreeRows() const { return (IMMIX_USEFUL_LINES - getUsedRows()); }
   inline int GetFreeData() const { return (IMMIX_USEFUL_LINES - getUsedRows())<<IMMIX_LINE_BITS; }
   void ClearEmpty()
   {
      memset((char *)this + 2,0,IMMIX_HEADER_LINES * IMMIX_LINE_LEN - 2);
      memset(mRow[IMMIX_HEADER_LINES],0,IMMIX_USEFUL_LINES * IMMIX_LINE_LEN);
      getInfo().mUsedRows = 0;
   }

   void destroy()
   {
      (*gBlockInfo)[mId].mPtr = 0;
      #ifdef USE_POSIX_MEMALIGN
      free(this);
      #endif
   }

   int nextBlockId()
   {
      if (gBlockInfo==0)
         gBlockInfo = new hx::QuickVec<BlockDataInfo>;
      for(int i=0;i<gBlockInfo->size();i++)
         if ( !(*gBlockInfo)[i].mPtr )
           return i;
      return  gBlockInfo->next();
   }

   inline BlockDataInfo &getInfo() const { return (*gBlockInfo)[mId]; }

   void ClearRecycled()
   {
      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         unsigned char &flags = mRowFlags[r];
         if (!(flags & IMMIX_ROW_MARKED) /*&& !(flags & IMMIX_ROW_CLEAR) */)
         {
            //__int64 *row = (__int64 *)mRow[r];
            double *row = (double *)mRow[r];
            row[0] = 0;
            row[1] = 0;
            row[2] = 0;
            row[3] = 0;
            row[4] = 0;
            row[5] = 0;
            row[6] = 0;
            row[7] = 0;
            row[8] = 0;
            row[9] = 0;
            row[10] = 0;
            row[11] = 0;
            row[12] = 0;
            row[13] = 0;
            row[14] = 0;
            row[15] = 0;
            //flags = IMMIX_ROW_CLEAR;
            flags = 0;
         }
      }
   }
   void DirtyLines(int inFirst,int inN)
   {
      unsigned char *ptr = mRowFlags + inFirst;
      for(int i=0;i<inN;i++)
         ptr[i] &= ~(IMMIX_ROW_CLEAR);
   }

   void FillTo(int inRow, int inPos)
   {
      BlockDataInfo &info = getInfo();
      info.mUsedRows = inRow-IMMIX_HEADER_LINES+(inPos>0);
      info.mFreeInARow = IMMIX_USEFUL_LINES - info.mUsedRows;
      info.mHoles = 1;
      int offset = (inRow<<IMMIX_LINE_BITS)+inPos;
      if (offset< IMMIX_BLOCK_SIZE)
         memset(&mRow[0][0] + offset, 0, IMMIX_BLOCK_SIZE-offset);
      for(int i=0;i<info.mUsedRows;i++)
         mRowFlags[i+IMMIX_HEADER_LINES] |= IMMIX_ROW_MARKED;
   }


   bool IsEmpty() const { return getUsedRows() == 0; }
   bool IsFull() const { return getUsedRows() == IMMIX_USEFUL_LINES; }
   int getUsedRows() const { return getInfo().mUsedRows; }
   int getFreeInARow() const { return getInfo().mFreeInARow; }
   int isPinned() const { return getInfo().mPinned; }
   inline bool IsRowUsed(int inRow) const { return mRowFlags[inRow] & IMMIX_ROW_MARKED; }

   void Verify()
   {
      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         unsigned char &row_flag = mRowFlags[r];
         if ( !(row_flag & IMMIX_ROW_MARKED) )
         {
            if (row_flag!=0)
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
   }
 
   void FillWithJunk()
   {
       memset(&mRow[0][0] + 2, 0x55, IMMIX_BLOCK_SIZE-2 );
   }

   #define CHECK_TABLE_LIVE \
      if (*table && ((row[*table]) !=  gByteMarkID)) *table = 0;

   void Reclaim(bool inFull)
   {
      int free = 0;
      int max_free_in_a_row = 0;
      int free_in_a_row = 0;
      int holes = 0;
      bool update_table = inFull || gFillWithJunk;

      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         unsigned char &row_flag = mRowFlags[r];
         if (row_flag & IMMIX_ROW_MARKED)
         {
            if (update_table)
            {
               // Must update from the object mark flag ...
               if (row_flag & IMMIX_ROW_HAS_OBJ_LINK)
               {
                  unsigned char *row = mRow[r];
                  unsigned char *last_link = &row_flag;
                  int pos = (row_flag & IMMIX_ROW_LINK_MASK);

                  while(1)
                  {
                     // Still current ....
                     if (row[pos+ENDIAN_MARK_ID_BYTE_HEADER] == gByteMarkID)
                     {
                        *last_link = pos | IMMIX_ROW_HAS_OBJ_LINK;
                        last_link = row+pos+ENDIAN_OBJ_NEXT_BYTE;
                     }
                     else
                     {
                       if (gFillWithJunk)
                       {
                           unsigned int header = *(unsigned int *)(row + pos);
                           int size = header & IMMIX_ALLOC_SIZE_MASK;
                           //GCLOG("Fill %d (%p+%d=%p) mark=%d/%d\n",size, row,pos, row+pos+4,row[pos+ENDIAN_MARK_ID_BYTE_HEADER], gByteMarkID);
                           memset(row+pos+4,0x55,size);
                       }
                     }
                     if (row[pos+ENDIAN_OBJ_NEXT_BYTE] & IMMIX_ROW_HAS_OBJ_LINK)
                        pos = row[pos+ENDIAN_OBJ_NEXT_BYTE] & IMMIX_ROW_LINK_MASK;
                     else
                        break;
                  }
                  *last_link = 0;
                  row_flag |= IMMIX_ROW_MARKED;
               }
            }
            free_in_a_row = 0;
         }
         else
         {
            if (free_in_a_row==0)
               holes++;
            row_flag = 0;
            free_in_a_row++;
            if (gFillWithJunk)
              memset(mRow[r],0x55,IMMIX_LINE_LEN);

            if (free_in_a_row>max_free_in_a_row)
               max_free_in_a_row = free_in_a_row;
            free++;
         }
      }

      BlockDataInfo &info = getInfo();
      info.mUsedRows = IMMIX_USEFUL_LINES - free;
      info.mFreeInARow = max_free_in_a_row;
      info.mHoles = holes;
      // GCLOG("Used %f, biggest=%f, holes=%d\n", (float)mUsedRows/IMMIX_USEFUL_LINES, (float)mFreeInARow/IMMIX_USEFUL_LINES, holes );

      //Verify();
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
      unsigned char time = mRow[0][inOffset+ENDIAN_MARK_ID_BYTE_HEADER];
      if ( ((time+1) & MARK_BYTE_MASK) != gByteMarkID )
      {
         // Object is either out-of-date, or already marked....
         if (inReport)
            GCLOG(time==gByteMarkID ? " M " : " O ");
         return time==gByteMarkID ? allocMarked : allocNone;
      }

      int flags = mRowFlags[r];
      if (!(flags & (IMMIX_ROW_HAS_OBJ_LINK)))
      {
         if (inReport)
            GCLOG("  row has no new objs :[%d] = %d\n", r, flags );
         return allocNone;
      }


      int sought = (inOffset & IMMIX_LINE_POS_MASK);
      unsigned char *row = mRow[r];
      int pos = (flags & IMMIX_ROW_LINK_MASK);

      while( pos!=sought && (row[pos+ENDIAN_OBJ_NEXT_BYTE] & IMMIX_ROW_HAS_OBJ_LINK) )
         pos = row[pos+ENDIAN_OBJ_NEXT_BYTE] & IMMIX_ROW_LINK_MASK;

      if (pos==sought)
      {
         if (*(unsigned int *)(mRow[0] + inOffset) & IMMIX_ALLOC_IS_OBJECT)
         {
            // See if object::new has been called, but not constructor yet ...
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

      if (inReport)
      {
         GCLOG("  not found in table (r=%p,sought =%d): ", row, sought);
         int pos = (flags & IMMIX_ROW_LINK_MASK);
         GCLOG(" %d ", pos );
         while( pos!=sought && (row[pos+ENDIAN_OBJ_NEXT_BYTE] & IMMIX_ROW_HAS_OBJ_LINK) )
         {
            pos = row[pos+ENDIAN_OBJ_NEXT_BYTE] & IMMIX_ROW_LINK_MASK;
            GCLOG(" %d ", pos );
         }

         GCLOG("\n");
      }

      return allocNone;
   }

   void pin() { getInfo().mPinned = true; }

   void ClearRowMarks()
   {
      getInfo().mPinned = false;
      unsigned char *header = mRowFlags + IMMIX_HEADER_LINES;
      unsigned char *header_end = header + IMMIX_USEFUL_LINES;
      while(header !=  header_end)
         *header++ &= IMMIX_NOT_MARKED_MASK;
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void VisitBlock(hx::VisitContext *inCtx)
   {
      if (IsEmpty())
         return;

      for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
      {
         if ((mRowFlags[r] & (IMMIX_ROW_HAS_OBJ_LINK|IMMIX_ROW_MARKED)) ==
                            (IMMIX_ROW_HAS_OBJ_LINK|IMMIX_ROW_MARKED) )
         {
            int pos = (mRowFlags[r] & IMMIX_ROW_LINK_MASK);
            unsigned char *row = mRow[r];

            while(true)
            {
               if (row[pos+ENDIAN_MARK_ID_BYTE_HEADER] == gByteMarkID)
               {
                  if ( (*(unsigned int *)(row+pos)) & IMMIX_ALLOC_IS_OBJECT )
                  {
                     hx::Object *obj = (hx::Object *)(row+pos+4);
                     obj->__Visit(inCtx);
                  }
               }

               int next = row[pos+ENDIAN_OBJ_NEXT_BYTE];
               if (! (next & IMMIX_ROW_HAS_OBJ_LINK) )
                  break;
               pos = next & IMMIX_ROW_LINK_MASK;
            }
         }
      }
   }
   #endif



   // First 2 bytes are not needed for row markers (first 2 rows are for flags)
   unsigned short mId;

   // First 2 rows contain a byte-flag-per-row 
   unsigned char  mRowFlags[IMMIX_LINES];
   // Row data as union - don't use first 2 rows
   unsigned char  mRow[IMMIX_LINES][IMMIX_LINE_LEN];
};


#define MARK_ROWS \
   unsigned char &mark = ((unsigned char *)inPtr)[ENDIAN_MARK_ID_BYTE]; \
   if ( mark==gByteMarkID || mark & HX_GC_CONST_ALLOC_MARK_BIT) \
      return; \
   mark = gByteMarkID; \
 \
   register size_t ptr_i = ((size_t)inPtr)-sizeof(int); \
   unsigned int flags =  *((unsigned int *)ptr_i); \
 \
   char *block = (char *)(ptr_i & IMMIX_BLOCK_BASE_MASK); \
   if ( flags & (IMMIX_ALLOC_SMALL_OBJ | IMMIX_ALLOC_MEDIUM_OBJ) ) \
   { \
      char *base = block + ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS); \
      *base |= IMMIX_ROW_MARKED; \
 \
      if (flags & IMMIX_ALLOC_MEDIUM_OBJ) \
      { \
         int rows = (( (flags & IMMIX_ALLOC_SIZE_MASK) + sizeof(int) + \
                (ptr_i & (IMMIX_LINE_LEN-1)) -1 ) >> IMMIX_LINE_BITS); \
         for(int i=1;i<=rows;i++) \
            base[i] |= IMMIX_ROW_MARKED; \
      } \
   }




namespace hx
{

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
   AtomicLock lock;
   hx::QuickVec< MarkChunk * >   chunks;
   hx::QuickVec< MarkChunk * > spare;

   MarkChunk *pushJob(MarkChunk *inChunk)
   {
      AutoAtomic l(lock);
      chunks.push(inChunk);

      if (sActiveThreads)
      {
         // Wake someone up...
         for(int i=0;i<sActiveThreads;i++)
            if (!(sRunningThreads & (1<<i)))
            {
               sRunningThreads |= (1<<i);
               sThreadWake[i]->Set();
            }
      }

      if (spare.size()==0)
         return new MarkChunk;
      return spare.pop();
   }

   MarkChunk *popJob(MarkChunk *inChunk,int inThreadId)
   {
      while(true)
      {
         lock.Lock();
         if (inChunk)
         {
            spare.push(inChunk);
            inChunk = 0;
         }

         if (chunks.size())
         {
            MarkChunk *result = chunks.pop();
            lock.Unlock();
            return result;
         }
         if (inThreadId<0)
         {
            lock.Unlock();
            return 0;
         }

         sRunningThreads &= ~(1<<inThreadId);
         // Last one out, turn off the lights
         if (sRunningThreads==0)
         {
            lock.Unlock();
            for(int i=0;i<sActiveThreads;i++)
               if (i!=inThreadId)
                  sThreadWake[i]->Set();
            return 0;
         }
         else // wait to be woken....
         {
            lock.Unlock();
            sThreadWake[inThreadId]->Wait();
            if (sRunningThreads==0)
               return 0;
         }
      }
   }

   void free(MarkChunk *inChunk)
   {
      AutoAtomic l(lock);
      spare.push(inChunk);
   }

   MarkChunk *alloc()
   {
      AutoAtomic l(lock);
      if (spare.size()==0)
         return new MarkChunk;
      return spare.pop();
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

    void releaseJobs()
    {
       if (marking && marking->count)
       {
          sGlobalChunks.chunks.push(marking);
          marking = 0;
       }
       if (spare->count)
          spare = sGlobalChunks.pushJob(spare);
    }

    void Process()
    {
       if (!marking)
          marking = sGlobalChunks.popJob(marking,mThreadId);

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
          {
             marking = sGlobalChunks.popJob(marking,mThreadId);
          }
       }
    }
};


/*
void MarkerReleaseWorkerLocked( )
{
   //printf("Release...\n");
   for(int i=0;i<sActiveThreads;i++)
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



void MarkAlloc(void *inPtr,hx::MarkContext *__inCtx)
{
   MARK_ROWS
}

void MarkObjectAlloc(hx::Object *inPtr,hx::MarkContext *__inCtx)
{
   MARK_ROWS
   if (flags & IMMIX_ALLOC_IS_OBJECT)
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


InternalFinalizer::InternalFinalizer(hx::Object *inObj)
{
   mUsed = false;
   mValid = true;
   mObject = inObj;
   mFinalizer = 0;

   AutoLock lock(*gSpecialObjectLock);
   sgFinalizers->push(this);
}

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
      mNextRecycled = 0;
      mNextEmpty = 0;
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

   void *AllocLarge(int inSize)
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

      unsigned int *result = (unsigned int *)malloc(inSize + sizeof(int)*2);
      if (!result)
      {
         #ifdef SHOW_MEM_EVENTS
         GCLOG("Large alloc failed - forcing collect\n");
         #endif

         CollectFromThisThread();
         result = (unsigned int *)malloc(inSize + sizeof(int)*2);
      }
      result[0] = inSize;
      result[1] = gMarkID;

      bool do_lock = sMultiThreadMode;
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

      bool do_lock = sMultiThreadMode;
      if (do_lock)
         mLargeListLock.Lock();

      mLargeAllocated += inDelta;

      if (do_lock)
         mLargeListLock.Unlock();
   }


   // Making this function "virtual" is actually a (big) performance enhancement!
   // On the iphone, sjlj (set-jump-long-jump) exceptions are used, which incur a
   //  performance overhead.  It seems that the overhead in only in routines that call
   //  malloc/new.  This is not called very often, so the overhead should be minimal.
   //  However, gcc inlines this function!  requiring every alloc the have sjlj overhead.
   //  Making it virtual prevents the overhead.
   virtual BlockData * GetRecycledBlock(int inRequiredRows)
   {
      if (sMultiThreadMode)
      {
         hx::EnterGCFreeZone();
         gThreadStateChangeLock->Lock();
         hx::ExitGCFreeZoneLocked();
      }

      BlockData *result = 0;

      for(int pass= 0 ;pass<2 && result==0;pass++)
      {
         // Try for recycled first
         //   pass0 - any lying around before a collect
         //   pass1 - any lying around after a collect
         if (mNextRecycled < mRecycledBlock.size())
         {
            if (mRecycledBlock[mNextRecycled]->getFreeInARow()>=inRequiredRows)
            {
               result = mRecycledBlock[mNextRecycled++];
            }
            else
            {
               for(int block = mNextRecycled+1; block<mRecycledBlock.size(); block++)
               {
                  if (mRecycledBlock[block]->getFreeInARow()>=inRequiredRows)
                  {
                     result = mRecycledBlock[block];
                     mRecycledBlock.erase(block);
                  }
               }
            }

            if (result)
            {
               result->ClearRecycled();
               mCurrentRowsInUse += result->GetFreeRows();
               break;
            }
         }

         // No recycled blocks
         //   pass0 - Allow a collect if no free block, allocat more if tight
         //   pass1 - We have already tried the collect, so force allocate
         //  without sgInternalEnable, force allocation of there are no blocks
         result = GetEmptyBlock(pass==0 && sgInternalEnable);
      }

      if (sMultiThreadMode)
         gThreadStateChangeLock->Unlock();

      return result;
   }

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
   
            #ifdef USE_POSIX_MEMALIGN
            int gid = 0;
            char *chunk = 0;
            int n = 1;
            int result = posix_memalign( (void **)&chunk, IMMIX_BLOCK_SIZE, IMMIX_BLOCK_SIZE );
            if (chunk==0)
            {
               // We really really have to try collect!
               if (inTryCollect==false)
               {
                  #ifdef SHOW_MEM_EVENTS
                  GCLOG("Alloc failed - forcing collect\n");
                  #endif
                  return GetEmptyBlock(true);
               }

               GCLOG("Error in posix_memalign %d!!!\n", result);
            }
            char *aligned = chunk;
            want_more--;

            #else
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
            // We really really have to try collect!
            if (chunk==0 && !inTryCollect)
            {
               #ifdef SHOW_MEM_EVENTS
               GCLOG("Alloc failed - forcing collect\n");
               #endif
               return GetEmptyBlock(true);
            }


            int n = 1<<IMMIX_BLOCK_GROUP_BITS;

            char *aligned = (char *)( (((size_t)chunk) + IMMIX_BLOCK_SIZE-1) & IMMIX_BLOCK_BASE_MASK);
            if (aligned!=chunk)
               n--;
            gAllocGroups[gid].alloc = chunk;
            // Only do one group allocation
            want_more = 0;
            #endif


            for(int i=0;i<n;i++)
            {
               BlockData *block = (BlockData *)(aligned + i*IMMIX_BLOCK_SIZE);
               block->Init(gid);
               mAllBlocks.push(block);
               mEmptyBlocks.push(block);
            }
            mAllBlocksCount = mAllBlocks.size();
            #ifdef SHOW_MEM_EVENTS
            GCLOG("Blocks %d = %d k\n", mAllBlocks.size(), (mAllBlocks.size() << IMMIX_BLOCK_BITS)>>10);
            #endif
         }
      }

      BlockData *block = mEmptyBlocks[mNextEmpty++];
      block->ClearEmpty();
      mActiveBlocks.insert(block);
      mCurrentRowsInUse += IMMIX_USEFUL_LINES;
      return block;
   }

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



   #ifdef HXCPP_VISIT_ALLOCS // {
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
            if ((from.mRowFlags[r] & (IMMIX_ROW_HAS_OBJ_LINK|IMMIX_ROW_MARKED)) ==
                               (IMMIX_ROW_HAS_OBJ_LINK|IMMIX_ROW_MARKED) )
            {
               int pos = (from.mRowFlags[r] & IMMIX_ROW_LINK_MASK);
               unsigned char *row = from.mRow[r];
   
               while(true)
               {
                  int next = row[pos+ENDIAN_OBJ_NEXT_BYTE];

                  if (row[pos+ENDIAN_MARK_ID_BYTE_HEADER] == gByteMarkID)
                  {
                     unsigned int &flags = *(unsigned int *)(row+pos);
                     bool isObject = flags & IMMIX_ALLOC_IS_OBJECT;
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
                     unsigned char &row_flag = dest->mRowFlags[destRow];

                     int *result = (int *)(row + destPos);
                     *result = size | gMarkID | (row_flag<<16) |
                           (extra_lines==0 ? IMMIX_ALLOC_SMALL_OBJ : IMMIX_ALLOC_MEDIUM_OBJ );

                     if (isObject)
                        *result |= IMMIX_ALLOC_IS_OBJECT;

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
      for(PointerSet::iterator i=mActiveBlocks.begin(); i!=mActiveBlocks.end();++i)
         (*i)->ClearRowMarks();
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

   void MarkAll(bool inDoClear)
   {
      // Bit 0x80 is reserved for "const allocation"
      gByteMarkID = (gByteMarkID+1) & MARK_BYTE_MASK;
      gMarkID = gByteMarkID << 24;
      gBlockStack = 0;

      if (inDoClear)
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
         if (!sMarkThreadsInit)
         {
            sMarkThreadsInit = true;
            for(int i=0;i<MAX_MARK_THREADS;i++)
               CreateMarker(i);
         }

         mMarker.releaseJobs();

         // Unleash the workers...
         sActiveThreads = MAX_MARK_THREADS;
         sRunningThreads = (1<<sActiveThreads) - 1;
         for(int i=0;i<sActiveThreads;i++)
            sThreadWake[i]->Set();

         // Join the workers...
         for(int i=0;i<sActiveThreads;i++)
            sMarkDone[i]->Wait();
         sActiveThreads = 0;
      }
      else
      {
         mMarker.Process();
      }

      hx::FindZombies(mMarker);

      hx::RunFinalizers();
   }

   void MarkerLoop(int inId)
   {
      hx::MarkContext context(inId);
      while(true)
      {
         sThreadWake[inId]->Wait();
         context.Process();
         sMarkDone[inId]->Set();
      }
   }

   static THREAD_FUNC_TYPE SMarkerFunc( void *inInfo )
   {
      sGlobalAlloc->MarkerLoop((int)(size_t)inInfo);
      THREAD_FUNC_RET;
   }

   void CreateMarker(int inId)
   {
      sThreadWake[inId] = new MySemaphore();
      sMarkDone[inId] = new MySemaphore();

      void *info = (void *)(size_t)inId;
    #ifdef HX_WINRT
      // TODO
    #elif defined(EMSCRIPTEN)
    // Only one thread
    #elif defined(HX_WINDOWS)
      bool ok = _beginthreadex(0,0,SMarkerFunc,info,0,0) != 0;
    #else
      pthread_t result = 0;
      int created = pthread_create(&result,0,SMarkerFunc,info);
      bool ok = created==0;
    #endif
   }

   int Collect(bool inMajor, bool inForceCompact)
   {
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
      int largeAlloced = mLargeAllocated;
      LocalAllocator *this_local = 0;
      if (sMultiThreadMode)
      {
         hx::EnterGCFreeZone();
         gThreadStateChangeLock->Lock();
         hx::ExitGCFreeZoneLocked();
         // Someone else beat us to it ...
         if (hx::gPauseForCollect)
         {
            gThreadStateChangeLock->Unlock();
#ifdef HXCPP_TELEMETRY
            __hxt_gc_end();
#endif
            return false;
         }

         hx::gPauseForCollect = true;

         this_local = tlsLocalAlloc;
         for(int i=0;i<mLocalAllocs.size();i++)
            if (mLocalAllocs[i]!=this_local)
               WaitForSafe(mLocalAllocs[i]);
      }

      // Now all threads have mTopOfStack & mBottomOfStack set.

      STAMP(t1)

      MarkAll(true);

      STAMP(t2)

      // Reclaim ...

      sgTimeToNextTableUpdate--;
      if (sgTimeToNextTableUpdate<0)
         sgTimeToNextTableUpdate = 20;
      bool full = inMajor || (sgTimeToNextTableUpdate==0) || inForceCompact;


      // Clear lists, start fresh...
      mEmptyBlocks.clear();
      mRecycledBlock.clear();
      for(PointerSet::iterator i=mActiveBlocks.begin(); i!=mActiveBlocks.end();++i)
         (*i)->Reclaim(full);
      mActiveBlocks.clear();
      mNextEmpty = 0;
      mNextRecycled = 0;

      mRowsInUse = 0;
      for(int i=0;i<mAllBlocks.size();i++)
         mRowsInUse += mAllBlocks[i]->getUsedRows();

      int idx = 0;
      while(idx<mLargeList.size())
      {
         unsigned int *blob = mLargeList[idx];
         if ( (blob[1] & IMMIX_ALLOC_MARK_ID) != gMarkID )
         {
            mLargeAllocated -= *blob;
            free(mLargeList[idx]);
            mLargeList.qerase(idx);
         }
         else
            idx++;
      }

      STAMP(t3)

      mTotalAfterLastCollect = MemUsage();
      int blockSize =  mAllBlocks.size()<<IMMIX_BLOCK_BITS;
      if (blockSize > mLargeAllocSpace)
         mLargeAllocSpace = blockSize;
      mLargeAllocForceRefresh = mLargeAllocated + mLargeAllocSpace;

      //GCLOG("Using %d, blocks %d (%d)\n", mTotalAfterLastCollect, mAllBlocks.size(), mAllBlocks.size()*IMMIX_BLOCK_SIZE);

      int free_rows = mAllBlocks.size()*IMMIX_USEFUL_LINES - mRowsInUse;
      // Aim for 50% free space...
      int  delta = mRowsInUse - free_rows;
      int  want_more = delta>0 ? (delta >> IMMIX_LINE_COUNT_BITS ) : 0;
      int  want_less = (delta < -(IMMIX_USEFUL_LINES<<IMMIX_BLOCK_GROUP_BITS)) ?
                            ((-delta) >> IMMIX_LINE_COUNT_BITS ) : 0;
      if (!sgInternalEnable)
         want_less = 0;
      if (inForceCompact)
      {
         want_less = mAllBlocks.size();
         want_more = false;
      }

      bool released = want_less && ReleaseBlocks(want_less);


      #if defined(HXCPP_GC_MOVING) && defined(HXCPP_VISIT_ALLOCS)
      if (!released && full)
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


      // IMMIX suggest filling up in creation order ....
      mRowsInUse = 0;
      for(int i=0;i<mAllBlocks.size();i++)
      {
         BlockData *block = mAllBlocks[i];

         if (block->IsEmpty())
            mEmptyBlocks.push(block);
         else
         {
            mActiveBlocks.insert(block);
            mRowsInUse += block->getUsedRows();
            if (!block->IsFull())
               mRecycledBlock.push(block);
         }
      }
      mAllBlocksCount   = mAllBlocks.size();
      mCurrentRowsInUse = mRowsInUse;


      if (sMultiThreadMode)
      {
         for(int i=0;i<mLocalAllocs.size();i++)
         if (mLocalAllocs[i]!=this_local)
            ReleaseFromSafe(mLocalAllocs[i]);

         hx::gPauseForCollect = false;
         gThreadStateChangeLock->Unlock();
      }

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

      return want_more;
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

   MemType GetMemType(void *inPtr)
   {
      BlockData *block = (BlockData *)( ((size_t)inPtr) & IMMIX_BLOCK_BASE_MASK);
      if ( mActiveBlocks.find(block) != mActiveBlocks.end() )
      {
         return memBlock;
      }

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

   int mNextEmpty;
   int mNextRecycled;

   BlockList mAllBlocks;
   BlockList mEmptyBlocks;
   BlockList mRecycledBlock;
   LargeList mLargeList;
   PointerSet mActiveBlocks;
   MyMutex    mLargeListLock;
   hx::QuickVec<LocalAllocator *> mLocalAllocs;
   LocalAllocator *mLocalPool[LOCAL_POOL_SIZE];
};



namespace hx
{

void MarkConservative(int *inBottom, int *inTop,hx::MarkContext *__inCtx)
{
   #ifdef SHOW_MEM_EVENTS
   GCLOG("Mark conservative %p...%p (%d)\n", inBottom, inTop, (inTop-inBottom) );
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
            AllocType t = block->GetAllocType(pos-sizeof(int),true);
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
               block->pin();
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
class LocalAllocator
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
      tlsLocalAlloc = 0;

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
      tlsLocalAlloc = 0;
   }

   void Reset()
   {
      mCurrent = 0;
      mOverflow = 0;
      mCurrentLine = IMMIX_LINES;
      mCurrentPos = 0;
      mLinesSinceLastCollect = 0; 
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
      hx::RegisterCapture::Instance()->Capture(mTopOfStack,mRegisterBuf,mRegisterBufSize,20,mBottomOfStack);
   }


   void PauseForCollect()
   {
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      hx::RegisterCapture::Instance()->Capture(mTopOfStack,mRegisterBuf,mRegisterBufSize,20,mBottomOfStack);
 
      mReadyForCollect.Set();
      mCollectDone.Wait();
   }

   void EnterGCFreeZone()
   {
      volatile int dummy = 1;
      mBottomOfStack = (int *)&dummy;
      mGCFreeZone = true;
      if (mTopOfStack)
         hx::RegisterCapture::Instance()->Capture(mTopOfStack,
                mRegisterBuf,mRegisterBufSize,20,mBottomOfStack);
      mReadyForCollect.Set();
   }

   void ExitGCFreeZone()
   {
      #ifdef HXCPP_DEBUG
      if (!mGCFreeZone)
         CriticalGCError("GCFree Zone mismatch");
      #endif

      if (sMultiThreadMode)
      {
         AutoLock lock(*gThreadStateChangeLock);
         mReadyForCollect.Reset();
         mGCFreeZone = false;
      }
   }
        // For when we already hold the lock
   void ExitGCFreeZoneLocked()
   {
      if (sMultiThreadMode)
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



   void *Alloc(int inSize,bool inIsObject)
   {
      #ifdef HXCPP_ALIGN_FLOAT
         #define HXCPP_ALIGN_ALLOC
      #endif



      #ifdef HXCPP_DEBUG
      if (mGCFreeZone)
         CriticalGCError("Allocating from a GC-free thread");
      #endif
      if (hx::gPauseForCollect)
         PauseForCollect();

      inSize = (inSize + 3 ) & ~3;
      #ifdef HXCPP_VISIT_ALLOCS
      // Make sure we can fit a relocation pointer
      if (sizeof(void *)>4 && inSize<sizeof(void *))
         inSize = sizeof(void *);
      #endif

      int s = inSize +sizeof(int);

      #ifdef HXCPP_ALIGN_ALLOC
      if (mCurrentPos >= IMMIX_LINE_LEN-4)
      {
         mCurrentPos = 0;
         mCurrentLine++;
      }
      #endif
      // Try to squeeze it on this line ...
      if (mCurrentPos > 0
          )
      {
         #ifdef HXCPP_ALIGN_ALLOC
         // Since we add sizeof(int), must be unaligned initially
         if ( !(mCurrentPos & 0x4))
            mCurrentPos += 4;
         #endif
         int skip = 1;
         int extra_lines = (s + mCurrentPos-1) >> IMMIX_LINE_BITS;

         //GCLOG("check for %d extra lines ...\n", extra_lines);
         if (mCurrentLine + extra_lines < IMMIX_LINES)
         {
            int test = 0;
            if (extra_lines)
            {
               const unsigned char *used = mCurrent->mRowFlags + mCurrentLine+test+1;
               for(test=0;test<extra_lines;test++)
                  if ( used[test] & IMMIX_ROW_MARKED)
                     break;
            }
            //GCLOG(" found %d extra lines\n", test);
            if (test==extra_lines)
            {
               // Ok, fits on the line! - setup object table
               unsigned char *row = mCurrent->mRow[mCurrentLine];
               unsigned char &row_flag = mCurrent->mRowFlags[mCurrentLine];

               int *result = (int *)(row + mCurrentPos);
               *result = inSize | gMarkID |
                  (row_flag<<16) |
                  (extra_lines==0 ? IMMIX_ALLOC_SMALL_OBJ : IMMIX_ALLOC_MEDIUM_OBJ );

               if (inIsObject)
                  *result |= IMMIX_ALLOC_IS_OBJECT;

               row_flag =  mCurrentPos | IMMIX_ROW_HAS_OBJ_LINK;

               mCurrentLine += extra_lines;
               mCurrentPos = (mCurrentPos + s) & (IMMIX_LINE_LEN-1);
               if (mCurrentPos==0)
                  mCurrentLine++;

               //GCLOG("Alloced %d - %d/%d now\n", s, mCurrentPos, mCurrentLine);

               return result + 1;
            }
            //GCLOG("not enought extra lines - skip %d\n",skip);
            skip = test + 1;
         }
         else
            skip = extra_lines;

         // Does not fit on this line - we may also know how many to skip, so
         //  jump down those lines...
         mCurrentPos = 0;
         mCurrentLine += skip;
      }

      int required_rows = (s + IMMIX_LINE_LEN-1) >> IMMIX_LINE_BITS;
      int last_start = IMMIX_LINES - required_rows;

      #ifdef HXCPP_ALIGN_ALLOC
      s+=4;
      #endif
      while(1)
      {
         // Alloc new block, if required ...
         if (!mCurrent || mCurrentLine>last_start)
         {
            volatile int dummy = 1;
            mBottomOfStack = (int *)&dummy;
            hx::RegisterCapture::Instance()->Capture(mTopOfStack,mRegisterBuf,mRegisterBufSize,20,mBottomOfStack);
            mCurrent = sGlobalAlloc->GetRecycledBlock(required_rows);
            //mCurrent->Verify();
            // Start on line 2 (there are 256 line-markers at the beginning)
            mCurrentLine = IMMIX_HEADER_LINES;
         }

         // Look for N in a row ....
         while(mCurrentLine <= last_start)
         {
            int test = 0;
            const unsigned char *used = mCurrent->mRowFlags + mCurrentLine+test;
            for(;test<required_rows;test++)
               if ( used[test] & IMMIX_ROW_MARKED)
                  break;

            // Not enough room...
            if (test<required_rows)
            {
               mCurrentLine += test+1;
               //GCLOG("  Only found %d good - skip to %d\n",test,mCurrentLine);
               continue;
            }

            // Ok, found a gap
            unsigned char *row = mCurrent->mRow[mCurrentLine];

            int *result = (int *)(row /*+ mCurrentPos*/);
            #ifdef HXCPP_ALIGN_ALLOC
            result++;
            #endif
            *result = inSize | gMarkID |
               (required_rows==1 ? IMMIX_ALLOC_SMALL_OBJ : IMMIX_ALLOC_MEDIUM_OBJ );

            if (inIsObject)
               *result |= IMMIX_ALLOC_IS_OBJECT;

            #ifdef HXCPP_ALIGN_ALLOC
            mCurrent->mRowFlags[mCurrentLine] = 4 | IMMIX_ROW_HAS_OBJ_LINK;
            #else
            mCurrent->mRowFlags[mCurrentLine] = /*mCurrentPos |*/ IMMIX_ROW_HAS_OBJ_LINK;
            #endif

            //mCurrent->DirtyLines(mCurrentLine,required_rows);
            mCurrentLine += required_rows - 1;
            mCurrentPos = (/*mCurrentPos +*/ s) & (IMMIX_LINE_LEN-1);
            if (mCurrentPos==0)
               mCurrentLine++;

            return result + 1;
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
      hx::MarkConservative((int *)mRegisterBuf, (int *)(mRegisterBuf+mRegisterBufSize) , __inCtx);
      MarkPopClass(__inCtx);
      #else
      hx::MarkConservative(mBottomOfStack, mTopOfStack , __inCtx);
      hx::MarkConservative((int *)mRegisterBuf, (int *)(mRegisterBuf+mRegisterBufSize) , __inCtx);
      #endif

      Reset();
   }

   int mCurrentPos;
   int mCurrentLine;

   int mOverflowPos;
   int mOverflowLine;

   int mLinesSinceLastCollect;

   BlockData * mCurrent;
   BlockData * mOverflow;

   int *mTopOfStack;
   int *mBottomOfStack;

   int  *mRegisterBuf[20];
   int  mRegisterBufSize;

   bool            mGCFreeZone;
   int             mStackLocks;
   int             mID;
   MySemaphore     mReadyForCollect;
   MySemaphore     mCollectDone;
};

LocalAllocator *sMainThreadAlloc = 0;


LocalAllocator *GetLocalAllocMT()
{
   LocalAllocator *result =  tlsLocalAlloc;
   if (!result)
   {
      #ifdef ANDROID
      __android_log_print(ANDROID_LOG_ERROR, "hxcpp",
      #else
      fprintf(stderr,
      #endif

      "GetLocalAllocMT - requesting memory from unregistered thread!"

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
   return result;
}


inline LocalAllocator *GetLocalAlloc()
{
   if (sMultiThreadMode)
      return GetLocalAllocMT();
   return sMainThreadAlloc;
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
   if (sMultiThreadMode)
   {
      LocalAllocator *tla = GetLocalAlloc();
      tla->EnterGCFreeZone();
   }
}

void ExitGCFreeZone()
{
   if (sMultiThreadMode)
   {
      LocalAllocator *tla = GetLocalAlloc();
      tla->ExitGCFreeZone();
   }
}

void ExitGCFreeZoneLocked()
{
   if (sMultiThreadMode)
   {
      LocalAllocator *tla = GetLocalAlloc();
      tla->ExitGCFreeZoneLocked();
   }
}

void InitAlloc()
{
   sgAllocInit = true;
   sGlobalAlloc = new GlobalAllocator();
   sgFinalizers = new FinalizerList();
   sFinalizerLock = new MyMutex();
   sGCRootLock = new MyMutex();
   hx::Object tmp;
   void **stack = *(void ***)(&tmp);
   sgObject_root = stack[0];
   //GCLOG("__root pointer %p\n", sgObject_root);
   sMainThreadAlloc =  new LocalAllocator();
   tlsLocalAlloc = sMainThreadAlloc;
}


void GCPrepareMultiThreaded()
{
   if (!sMultiThreadMode)
      sMultiThreadMode = true;
}


void SetTopOfStack(int *inTop,bool inForce)
{
   if (inTop)
   {
      if (!sgAllocInit)
         InitAlloc();
      else
      {
         if (tlsLocalAlloc==0)
         {
            GCPrepareMultiThreaded();
            RegisterCurrentThread(inTop);
         }
      }
   }

   LocalAllocator *tla = GetLocalAlloc();

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
      void *result = sGlobalAlloc->AllocLarge(inSize);
      memset(result,0,inSize);
      return result;
   }
   else
   {
      LocalAllocator *tla = GetLocalAlloc();
      void* result = tla->Alloc(inSize,inIsObject);
       return result;
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

   return (header & ( IMMIX_ALLOC_SMALL_OBJ | IMMIX_ALLOC_MEDIUM_OBJ)) ?
         (header & IMMIX_ALLOC_SIZE_MASK) :  ((unsigned int *)(inData))[-2];
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
      new_data = sGlobalAlloc->AllocLarge(inSize);
      if (inSize>s)
         memset((char *)new_data + s,0,inSize-s);
   }
   else
   {
      LocalAllocator *tla = GetLocalAlloc();

      new_data = tla->Alloc(inSize,false);
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
   tlsLocalAlloc = local;
}

void UnregisterCurrentThread()
{
   LocalAllocator *local = tlsLocalAlloc;
   delete local;
}



} // end namespace hx



namespace hx
{

void *Object::operator new( size_t inSize, NewObjectType inType, const char *inName )
{
   #if defined(HXCPP_DEBUG)
   if (inSize>=IMMIX_LARGE_OBJ_SIZE)
      throw Dynamic(HX_CSTRING("Object size violation"));
   #endif

   if (inType==NewObjConst)
      return InternalCreateConstBuffer(0,inSize);

   LocalAllocator *tla = GetLocalAlloc();

   return tla->Alloc(inSize,inType==NewObjContainer);
}

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

