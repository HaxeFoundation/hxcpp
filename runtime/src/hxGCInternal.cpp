#include <hxGCInternal.h>

char **gMovedPtrs = 0;
int gMarkBit = 0;


#ifdef INTERNAL_GC

#include <hxObject.h>

#include <map>
#include <vector>
#include <set>

#define SIZE_MASK    0x07ffffff


static bool sgAllocInit = 0;
static bool sgInternalEnable = true;

int sgAllocedSinceLastCollect = 0;


template<typename T>
struct QuickVec
{
	QuickVec() : mPtr(0), mAlloc(0), mSize(0) { } 
	inline void push(T inT)
	{
		if (mSize+1>=mAlloc)
		{
			mAlloc = 10 + (mSize*3/2);
			mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
		}
		mPtr[mSize++]=inT;
	}
	inline T pop()
	{
		return mPtr[--mSize];
	}
   inline void qerase(int inPos)
   {
      --mSize;
      mPtr[inPos] = mPtr[mSize];
   }
	inline bool empty() const { return !mSize; }
	inline void clear() { mSize = 0; }
	inline int next()
	{
		if (mSize+1>=mAlloc)
		{
			mAlloc = 10 + (mSize*3/2);
			mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
		}
		return mSize++;
	}
	inline int size() const { return mSize; }
	inline T &operator[](int inIndex) { return mPtr[inIndex]; }

	int mAlloc;
	int mSize;
	T *mPtr;
};


// --- hxInternalFinalizer -------------------------------

typedef QuickVec<hxInternalFinalizer *> FinalizerList;

FinalizerList *sgFinalizers = 0;

hxInternalFinalizer::hxInternalFinalizer(hxObject *inObj)
{
	mUsed = false;
	mValid = true;
	mObject = inObj;
	mFinalizer = 0;
	sgFinalizers->push(this);
}

void hxInternalFinalizer::Detach()
{
	mValid = false;
}

void RunFinalizers()
{
	FinalizerList &list = *sgFinalizers;
	int idx = 0;
	while(idx<list.size())
	{
		hxInternalFinalizer *f = list[idx];
		if (!f->mValid)
			list.qerase(idx);
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
}



void hxInternalEnableGC(bool inEnable)
{
   sgInternalEnable = inEnable;
}


void *hxInternalCreateConstBuffer(const void *inData,int inSize)
{
   int *result = (int *)malloc(inSize + sizeof(int));

   *result = 0xffffffff;
   memcpy(result+1,inData,inSize);

   return result+1;
}

#ifdef HX_GC_IMMIX

// ---  Internal GC - IMMIX Implementation ------------------------------



struct BlockInfo
{
	BlockInfo(char *inPtr=0)
	{
		mPtr = inPtr;
		mFreeLines = mPtr ? IMMIX_USEFUL_LINES : 0;
	}
   char *mPtr;
   int  mFreeLines;
};

typedef QuickVec<BlockInfo> BlockList;

typedef QuickVec<int *> LargeList;

class GlobalAllocator
{
public:
	GlobalAllocator()
	{
		mNextRecycled = 0;
		mNextEmpty = 0;
		mRowsInUse = 0;
		mLargeAllocated = 0;
		mLargeSinceLastCollect = 0;
		mTotalLastCollect = 0;
	}
   void *AllocLarge(int inSize)
	{
		int *result = (int *)malloc(inSize + sizeof(int));
		mLargeList.push(result);

		mLargeAllocated += inSize;
		*result = inSize | HX_GC_FIXED | gMarkBit;
		memset(result+1,0,inSize);
		mLargeSinceLastCollect += inSize;
		return result+1;
	}
	BlockInfo GetRecycledBlock()
	{
		if (mNextRecycled < mRecycledBlock.size())
		{
			return mRecycledBlock[mNextRecycled++];
		}
		return GetEmptyBlock();
	}

	BlockInfo GetEmptyBlock()
	{
		if (mNextEmpty >= mEmptyBlocks.size())
		{
			// Allocate some more blocks...
			// Using simple malloc for now, so allocate a big chuck in case we have to
			//  waste space by doing block-aligning
			char *chunk = (char *)malloc( 1<<20 );
			int n = 1<<(20-IMMIX_BLOCK_BITS);
			char *aligned = (char *)( (((size_t)chunk) + IMMIX_BLOCK_SIZE-1) & IMMIX_BLOCK_BASE_MASK);
			if (aligned!=chunk)
				n--;

			for(int i=0;i<n;i++)
			{
				char *ptr = aligned + i*IMMIX_BLOCK_SIZE;
				//memset(ptr,0,IMMIX_HEADER_LINES * IMMIX_LINE_LEN );
				memset(ptr,0,IMMIX_BLOCK_SIZE);
				for(int i=0;i<IMMIX_HEADER_LINES;i++)
					ptr[i] = 1;
				mAllBlocks.push( BlockInfo(ptr) );
				mEmptyBlocks.push( BlockInfo(ptr) );
			}
		}

		return mEmptyBlocks[mNextEmpty++];
	}

	void ClearRowMarks()
	{
		for(int i=0;i<mAllBlocks.size();i++)
			memset(mAllBlocks[i].mPtr,0,IMMIX_LINE_LEN * IMMIX_HEADER_LINES);
	}

	void Reclaim()
	{
		mTotalLastCollect = MemUsage();

		mEmptyBlocks.clear();
		mRecycledBlock.clear();
		mNextEmpty = 0;
		mNextRecycled = 0;
		mRowsInUse = 0;
		mLargeSinceLastCollect = 0;
		for(int i=0;i<mAllBlocks.size();i++)
		{
			BlockInfo &info = mAllBlocks[i];
			char *row_used = info.mPtr;
			int free = 0;
			for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
				if (!row_used[r])
					free++;
			info.mFreeLines = free;
			mRowsInUse += (IMMIX_USEFUL_LINES-free);
			if (free==IMMIX_USEFUL_LINES)
				mEmptyBlocks.push(info);
			else if (free>0)
				mRecycledBlock.push(info);
		}

		int idx = 0;
		while(idx<mLargeList.size())
		{
			int *blob = mLargeList[idx];
			if ( (*blob & HX_GC_MARKED) != gMarkBit )
			{
				mLargeAllocated -= (*blob & SIZE_MASK);
				free(blob);
				mLargeList.qerase(idx);
			}
			else
				idx++;
		}
	}

	int MemUsage()
	{
		return mLargeAllocated + (mRowsInUse<<IMMIX_LINE_BITS);
	}

	int MemSinceLastCollect()
	{
		return mLargeSinceLastCollect;
	}
	int MemLastCollect()
	{
		return mTotalLastCollect;
	}


	int mRowsInUse;
	int mLargeAllocated;
	int mLargeSinceLastCollect;
	int mTotalLastCollect;

	int mNextEmpty;
	int mNextRecycled;

   BlockList mAllBlocks;
   BlockList mEmptyBlocks;
   BlockList mRecycledBlock;
	LargeList mLargeList;
};

GlobalAllocator *sGlobalAlloc = 0;

class LocalAllocator
{
public:
	LocalAllocator()
	{
		Reset();
	}

	void Reset()
	{
		mCurrent.mPtr = 0;
		mOverflow.mPtr = 0;
		mCurrentLine = IMMIX_LINES;
		mLinesSinceLastCollect = 0; 
	}

	void *Alloc(int inSize)
   {
		int s = ((inSize+3) & ~3) +sizeof(int);
		while(1)
		{
			if (!mCurrent.mPtr || mCurrentLine==IMMIX_LINES)
			{
				mCurrent = sGlobalAlloc->GetRecycledBlock();
				// Start on line 2 (there are 256 line-markers at the beginning)
				mCurrentLine = IMMIX_HEADER_LINES;
				// find first empty line ...
				while(mCurrent.mPtr[mCurrentLine])
					mCurrentLine++;
				mCurrentPos = 0;
			}
			// easy case ...
			int extra_lines = (s + mCurrentPos-1) >> IMMIX_LINE_BITS;
			if (extra_lines == 0)
			{
				int *result = (int *)(mCurrent.mPtr + (mCurrentLine<<IMMIX_LINE_BITS) + mCurrentPos);
				*result = inSize | HX_GC_FIXED | HX_GC_SMALL_OBJ | gMarkBit;
				mCurrentPos += s;
				if (mCurrentPos==IMMIX_LINE_LEN)
				{
					mLinesSinceLastCollect++;
					mCurrentPos = 0;
					mCurrentLine++;
					while( (mCurrentLine<IMMIX_LINES) && mCurrent.mPtr[mCurrentLine])
						mCurrentLine++;
				}
				return result+1;
			}
			// Do we have enough blank lines?
			while (mCurrentLine+extra_lines < IMMIX_LINES)
			{
				bool clear = true;
				char *base = mCurrent.mPtr + 1 + mCurrentLine;
				for(int k=0;k<extra_lines;k++)
					if (base[k])
					{
						mCurrentPos = 0;
						clear = false;
						mCurrentLine += k + 2;
						while( (mCurrentLine+extra_lines<IMMIX_LINES) && mCurrent.mPtr[mCurrentLine])
						   mCurrentLine++;
						break;
					}
				if (clear)
				{
				   int *result = (int *)(mCurrent.mPtr + (mCurrentLine<<IMMIX_LINE_BITS) + mCurrentPos);
				   *result = inSize | HX_GC_FIXED | HX_GC_MEDIUM_OBJ | gMarkBit;
					mCurrentLine += extra_lines;
					mCurrentPos = (mCurrentPos+s) & (IMMIX_LINE_LEN-1);
					if (mCurrentPos==0)
					{
						mCurrentLine++;
						while( (mCurrentLine<IMMIX_LINES) && mCurrent.mPtr[mCurrentLine])
							mCurrentLine++;
					}
					mLinesSinceLastCollect+=extra_lines+1;
					return result + 1;
				}
			}
			// Not enough gap - to progress or end-of-block
			// Simply progress to next block for now...
			//   TODO?
			mCurrentLine = IMMIX_LINES;
		}
		return 0;
	}

	int MemSinceLastCollect()
	{
		return mLinesSinceLastCollect<<IMMIX_LINE_BITS;
	}

	int mCurrentPos;
	int mCurrentLine;

	int mOverflowPos;
	int mOverflowLine;

	int mLinesSinceLastCollect;

	BlockInfo mCurrent;
	BlockInfo mOverflow;
};

// Should have 1 per thread...
LocalAllocator *sLocalAlloc = 0;


void *hxInternalNew(int inSize)
{
	if (!sgAllocInit)
	{
		sgAllocInit = true;
		sGlobalAlloc = new GlobalAllocator();
		sLocalAlloc = new LocalAllocator();
      sgFinalizers = new FinalizerList();
	}
	if (inSize>IMMIX_LARGE_OBJ_SIZE)
		return sGlobalAlloc->AllocLarge(inSize);

	return sLocalAlloc->Alloc(inSize);
}

// immix...
void hxInternalCollect()
{
   if (!sgAllocInit || !sgInternalEnable)
		return;

	int since_last = sGlobalAlloc->MemSinceLastCollect() + sLocalAlloc->MemSinceLastCollect();
	int prev_total = sGlobalAlloc->MemLastCollect();
	if (since_last<prev_total)
		return;


	gMarkBit ^= HX_GC_MARKED;

	sGlobalAlloc->ClearRowMarks();

   hxGCMarkNow();

	RunFinalizers();

	sGlobalAlloc->Reclaim();

	int alloced = sGlobalAlloc->MemUsage();

	sLocalAlloc->Reset();
}



void *hxInternalRealloc(void *inData,int inSize)
{
   if (inData==0)
      return hxInternalNew(inSize);

   int *base = ((int *)(inData)) -1;

   int s = (*base) & SIZE_MASK;

   void *new_data = hxInternalNew(inSize);

   int min_size = s < inSize ? s : inSize;

   memcpy(new_data, inData, min_size );

   return new_data;
}



#else // Naive implementation ...

// ---  Internal GC - Simple Implementation ------------------------------


// --- AllocInfo -----------------

typedef QuickVec<struct AllocInfo *> AllocInfoList;

struct AllocInfo
{
   static void Init()
   {
      mActivePointers = new AllocInfoList();
   }

   inline void Unlink()
   {
      //sTotalSize -= (mSize & SIZE_MASK);
      sTotalObjs--;
   }

   inline void Link()
   {
      mActivePointers->push(this);
      //sTotalSize += (mSize & SIZE_MASK);
      sTotalObjs++;
   }

   static AllocInfo *Create(int inSize)
   {
      AllocInfo *result = (AllocInfo *)malloc( inSize + sizeof(AllocInfo) );
      memset(result,0,inSize+sizeof(AllocInfo));
      result->mSize = inSize;
      result->Link();
      return result;
   }

   // We will also use mSize for the mark-bit.
   // If aligment means that there are 4 bytes spare after size, then we
   //  will use those instead.
   // For strings generated by the linker, rather than malloc, this last int
   //  will be 0xffffffff
   int        mSize;

   static unsigned int sTotalSize;
   static unsigned int sTotalObjs;

   static AllocInfoList *mActivePointers;
};

AllocInfoList *AllocInfo::mActivePointers = 0;
unsigned int AllocInfo::sTotalSize = 0;
unsigned int AllocInfo::sTotalObjs = 0;




#define POOL_BIN_SHIFT   3
#define POOL_BIN         (1<<POOL_BIN_SHIFT)
#define POOL_COUNT       32
#define MAX_POOLED       (POOL_COUNT<<POOL_BIN_SHIFT)

static AllocInfoList sgFreePool[POOL_COUNT];
static int sgMaxPool[POOL_COUNT];

void *hxInternalNew( int inSize )
{
   AllocInfo *data = 0;

   inSize = (inSize + POOL_BIN -1 ) & (~(POOL_BIN-1));

   sgAllocedSinceLastCollect+=inSize;

   //printf("hxInternalNew %d\n", inSize);
   if (!sgAllocInit)
   {
      sgAllocInit = true;
      AllocInfo::Init();
      sgFinalizers = new FinalizerList();
      sgMaxPool[0] = 0;
      // Up to 500k per bin ...
      for(int i=1;i<POOL_COUNT;i++)
         sgMaxPool[i] = 500000 / (i<<POOL_BIN_SHIFT);
   }
   // First run, we can't be sure the pool has initialised - but now we can.
   else if (inSize < MAX_POOLED )
   {
      int bin =  inSize >> POOL_BIN_SHIFT;
      AllocInfoList &spares = sgFreePool[bin];
      if (!spares.empty())
      {
          data = spares.pop();
          data->Link();
      }
   }

   if (!data)
      data = AllocInfo::Create(inSize);
   else
      memset(data+1,0,inSize);

   return data + 1;
}



void hxInternalCollect()
{
   if (!sgAllocInit || !sgInternalEnable)
		return;

   //printf("New Bytes %d/%d\n", sgAllocedSinceLastCollect, AllocInfo::sTotalSize );
   //if (sgAllocedSinceLastCollect*4 < AllocInfo::sTotalSize)
      //return;

   sgAllocedSinceLastCollect= 0;


   hxGCMarkNow();

	RunFinalizers();

   // And sweep ...
   int deleted = 0;
   int retained = 0;

   int &size = AllocInfo::mActivePointers->mSize;
   AllocInfo **info = AllocInfo::mActivePointers->mPtr;
   int idx = 0;
   while(idx<size)
   {
      AllocInfo *data = info[idx];
      int &flags = ( (int *)(data+1) )[-1];
      if ( !(flags & HX_GC_MARKED) )
      {
          data->Unlink();
          AllocInfo::mActivePointers->qerase(idx);

          int bin = data->mSize >> POOL_BIN_SHIFT;
          if ( bin < POOL_COUNT )
          {
               AllocInfoList &pool = sgFreePool[bin];
               if (pool.size()<sgMaxPool[bin])
                  pool.push(data);
               else
                  free(data);
          }
          else
             free(data);
          deleted++;
      }
      else
      {
           flags ^= HX_GC_MARKED;
           retained++;
           idx++;
      }
   }

   //printf("Objs freed %d/%d)\n", deleted, retained);
}




void *hxInternalRealloc(void *inData,int inSize)
{
   if (inData==0)
      return hxInternalNew(inSize);

   AllocInfo *data = ((AllocInfo *)(inData) ) -1;
   int s = data->mSize & SIZE_MASK;

   void *new_data = hxInternalNew(inSize);

   int min_size = s < inSize ? s : inSize;

   memcpy(new_data, inData, min_size );

   return new_data;
}

#endif // Naive implementation

#endif // if INTERNAL_GC
