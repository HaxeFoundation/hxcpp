#include <hxGCInternal.h>
	// TODO: what is ThreadLocalAlloc is NULL (foreign thread) ?

char **gMovedPtrs = 0;
int gMarkBit = 0;


#ifdef INTERNAL_GC

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <hxObject.h>

#include <map>
#include <vector>
#include <set>

#define SIZE_MASK    0x07ffffff

#define STRING_MAGIC 0xD82BC9B1

void hxMarkCurrentThread(void *inBottomOfStack);

typedef std::map<void *,StaticMarkFunc> StaticMarkFuncMap;
StaticMarkFuncMap sStaticMarkFuncMap;

void hxGCSetVTables(_VtableMarks inVtableMark[])
{
	if (inVtableMark)
	{
	   for(_VtableMarks *v=inVtableMark; v->mVtable; v++)
	      sStaticMarkFuncMap[v->mVtable] = v->mMark;
	}
}



static bool sgAllocInit = 0;
static bool sgInternalEnable = false;
static void *sgObject_root = 0;

//#define DEBUG_ALLOC_PTR

#ifdef DEBUG_ALLOC_PTR
static char *sgDebugPointer = (char *)0xb5a584;
#endif

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
		mDistributedSinceLastCollect = 0;
		// Start at 1 Meg...
		mTotalAfterLastCollect = 1<<20;
	}
	// TODO: make thread safe
   void *AllocLarge(int inSize,bool inIsString)
	{
		int *result = (int *)malloc(inSize + sizeof(int)*2);
		mLargeList.push(result);

		mLargeAllocated += inSize;
		result[0] = inIsString ? STRING_MAGIC : 0;
		result[1] = inSize | HX_GC_FIXED | gMarkBit;
		memset(result+2,0,inSize);
		mDistributedSinceLastCollect += inSize;
		return result+2;
	}
	BlockInfo GetRecycledBlock(void *inBottomOfStack)
	{
		CheckCollect(inBottomOfStack);
		if (mNextRecycled < mRecycledBlock.size())
		{
			BlockInfo &block = mRecycledBlock[mNextRecycled++];
			mDistributedSinceLastCollect +=  block.mFreeLines << IMMIX_LINE_BITS;
			return block;
		}
		return GetEmptyBlock(false,inBottomOfStack);
	}

	BlockInfo GetEmptyBlock(bool inCheckCollect, void *inBottomOfStack)
	{
		if (inCheckCollect)
			CheckCollect(inBottomOfStack);
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

		BlockInfo &block = mEmptyBlocks[mNextEmpty++];
		mActiveBlocks.insert(block.mPtr);
		mDistributedSinceLastCollect +=  block.mFreeLines << IMMIX_LINE_BITS;
		return block;
	}

	void ClearRowMarks()
	{
		for(int i=0;i<mAllBlocks.size();i++)
			memset(mAllBlocks[i].mPtr,0,IMMIX_LINE_LEN * IMMIX_HEADER_LINES);
	}

	void Collect(void *inBottomOfStack)
	{
		static int collect = 0;
		//printf("Collect %d\n",collect++);
		gMarkBit ^= HX_GC_MARKED;

		ClearRowMarks();

   	hxGCMarkNow();

		hxMarkCurrentThread(inBottomOfStack);

		RunFinalizers();

		// Reclaim ...
		mEmptyBlocks.clear();
		mRecycledBlock.clear();
		mNextEmpty = 0;
		mNextRecycled = 0;
		mRowsInUse = 0;
		for(int i=0;i<mAllBlocks.size();i++)
		{
			BlockInfo &info = mAllBlocks[i];
			char *row_used = info.mPtr;
			int free = 0;
			int free_in_a_row = 0;
			for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
				if (!row_used[r])
				{
					free++;
					free_in_a_row++;
				}
				else if (free_in_a_row)
				{
					char *base = info.mPtr + ((r-free_in_a_row)<<IMMIX_LINE_BITS);
					memset(base, 0, free_in_a_row<<IMMIX_LINE_BITS);
					#ifdef DEBUG_ALLOC_PTR
	            if (base<=sgDebugPointer && sgDebugPointer<( base + (free_in_a_row<<IMMIX_LINE_BITS)) )
		            printf("Pointer collected !\n");
					#endif
					free_in_a_row = 0;
				}
			if (free_in_a_row)
			{
				char *base = info.mPtr + ((IMMIX_LINES-free_in_a_row)<<IMMIX_LINE_BITS);
				#ifdef DEBUG_ALLOC_PTR
	         if (base<=sgDebugPointer && sgDebugPointer<( base + (free_in_a_row<<IMMIX_LINE_BITS)) )
		         printf("Pointer collected !\n");
				#endif
				memset(base, 0, free_in_a_row<<IMMIX_LINE_BITS);
			}
			info.mFreeLines = free;
			mRowsInUse += (IMMIX_USEFUL_LINES-free);
			if (free==IMMIX_USEFUL_LINES)
			{
				mEmptyBlocks.push(info);
				mActiveBlocks.erase(info.mPtr);
			}
			else if (free>0)
				mRecycledBlock.push(info);
		}

		int idx = 0;
		while(idx<mLargeList.size())
		{
			int *blob = mLargeList[idx] + 1;
			if ( (*blob & HX_GC_MARKED) != gMarkBit )
			{
				mLargeAllocated -= (*blob & SIZE_MASK);
				free(mLargeList[idx]);
				mLargeList.qerase(idx);
			}
			else
				idx++;
		}

		mTotalAfterLastCollect = MemUsage();
		mDistributedSinceLastCollect = 0;
	}

	void CheckCollect(void *inBottomOfStack)
	{
		if (sgAllocInit && sgInternalEnable && mDistributedSinceLastCollect>(1<<20) &&
		    mDistributedSinceLastCollect>mTotalAfterLastCollect)
		{
			Collect(inBottomOfStack);
		}
	}

	size_t MemUsage()
	{
		return mLargeAllocated + (mRowsInUse<<IMMIX_LINE_BITS);
	}

	bool IsValidObjectAddress(void *inPtr)
	{
		for(int i=0;i<mLargeList.size();i++)
		{
			int *blob = mLargeList[i] + 2;
			if (blob==inPtr)
				return true;
		}
		void *aligned = (void *)( ((size_t)inPtr) & IMMIX_BLOCK_BASE_MASK);
		if ( ((char *)inPtr - (char *)aligned) < ( IMMIX_HEADER_LINES << IMMIX_LINE_BITS) )
			return false;
		if ( mActiveBlocks.find(aligned) != mActiveBlocks.end() )
			return true;
		return false;
	}


	size_t mDistributedSinceLastCollect;

	size_t mRowsInUse;
	size_t mLargeAllocated;
	size_t mTotalAfterLastCollect;


	int mNextEmpty;
	int mNextRecycled;

   BlockList mAllBlocks;
   BlockList mEmptyBlocks;
   BlockList mRecycledBlock;
	LargeList mLargeList;
	std::set<void *> mActiveBlocks;
};

GlobalAllocator *sGlobalAlloc = 0;


class LocalAllocator
{
public:
	LocalAllocator(int *inTopOfStack=0)
	{
		mTopOfStack = inTopOfStack;
		Reset();
	}

	void Reset()
	{
		mCurrent.mPtr = 0;
		mOverflow.mPtr = 0;
		mCurrentLine = IMMIX_LINES;
		mLinesSinceLastCollect = 0; 
	}

	void SetTopOfStack(int *inTop)
	{
		// stop early to allow for ptr[1] ....
		mTopOfStack = inTop;
	}

	void *Alloc(int inSize,bool inIsString)
   {
		int s = ((inSize+3) & ~3) +sizeof(int);
		if (inIsString) s += sizeof(int);
		while(1)
		{
			if (!mCurrent.mPtr || mCurrentLine==IMMIX_LINES)
			{
				mCurrent = sGlobalAlloc->GetRecycledBlock(&inSize);
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
		      if (inIsString)
					*result++ = STRING_MAGIC;
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
		         if (inIsString)
						*result++ = STRING_MAGIC;
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

	bool TryVTable0(void **inVTable)
	{
		if (IsBadReadPtr(inVTable, sizeof(void *)))
			return false;
		return inVTable[0]==sgObject_root;
	}


	void Mark(void *inBottomOfStack)
	{
		int here = 0;
		void *prev = 0;
		for(int *ptr = inBottomOfStack ? (int *)inBottomOfStack : &here; ptr<mTopOfStack; ptr++)
		{
			void *vptr = *(void **)ptr;
			if (vptr && !((size_t)vptr & 0x03) && vptr!=prev && sGlobalAlloc->IsValidObjectAddress(vptr))
			{
				prev = vptr;
					void **vtable = *(void ***)vptr;
					if ( (size_t)vtable > 0x10000 )
					{
						//printf("Found poiner %p -> %p.\n",vptr,vtable);
						if (sStaticMarkFuncMap.find(vtable)!=sStaticMarkFuncMap.end())
						{
							//printf("  found func too\n");
							sStaticMarkFuncMap[vtable](vptr);
						}
						// See if it is an object derived (non-virtually) from hxObject
						else if ( TryVTable0(vtable) )
						{
							//printf("  found hxObject\n");
							// ( (hxObject *)vptr )->__Mark();
							HX_MARK_OBJECT( ((hxObject *)vptr) );
						}
						else
						{
							// Check for string...
							unsigned int *header = ((unsigned int *)vptr);
							if ( header[-2]==STRING_MAGIC && (header[-1] & gMarkBit) )
							{
							   //printf("String! %p (%08x)\n",vptr,header);
								HX_MARK_STRING(vptr);
							}
							else
							{
							   //printf("????????????????????\n",vptr,header);
							}
						}
					}
			}
		}
	   Reset();
	}

	int mCurrentPos;
	int mCurrentLine;

	int mOverflowPos;
	int mOverflowLine;

	int mLinesSinceLastCollect;

	int *mTopOfStack;

	BlockInfo mCurrent;
	BlockInfo mOverflow;
};

#ifdef _MSC_VER
static int sTLSSlot = 0;
#else
static __thread LocalAllocator *ThreadLocalAlloc = 0;
#endif


void hxSetTopOfStack(int *inTop)
{
	#ifdef _MSC_VER
	LocalAllocator *ThreadLocalAlloc = (LocalAllocator *)TlsGetValue(sTLSSlot);
	#endif

	sgInternalEnable = true;

	return ThreadLocalAlloc->SetTopOfStack(inTop);

}

void hxMarkCurrentThread(void *inBottomOfStack)
{
	#ifdef _MSC_VER
	LocalAllocator *ThreadLocalAlloc = (LocalAllocator *)TlsGetValue(sTLSSlot);
	#endif
	ThreadLocalAlloc->Mark(inBottomOfStack);
}


void *hxInternalNew(int inSize,bool inIsString)
{
	if (!sgAllocInit)
	{
		sgAllocInit = true;
		sGlobalAlloc = new GlobalAllocator();
      sgFinalizers = new FinalizerList();
		hxObject tmp;
		void **vtable = *(void ***)(&tmp);
		sgObject_root = vtable[0];
		//printf("__root pointer %p\n", sgObject_root);
		#ifdef _MSC_VER
		sTLSSlot = TlsAlloc();
		// Store object for main thread ...
		TlsSetValue(sTLSSlot, new LocalAllocator() );
		#endif
	}
	void *result;

	if (inSize>IMMIX_LARGE_OBJ_SIZE)
		result = sGlobalAlloc->AllocLarge(inSize,inIsString);
	else
	{
		#ifdef _MSC_VER
		LocalAllocator *ThreadLocalAlloc = (LocalAllocator *)TlsGetValue(sTLSSlot);
		// TODO: what is ThreadLocalAlloc is NULL (foreign thread) ?
		#endif

		result = ThreadLocalAlloc->Alloc(inSize,inIsString);
	}

	#ifdef DEBUG_ALLOC_PTR
	if (result<=sgDebugPointer && sgDebugPointer -inSize < result)
		printf("Pointer alloced !\n");
	/*
	for(int i=0;i<inSize;i++)
		if ( ((char *)result)[i]!=0)
			*(int *)0=0;
	*/
	#endif
	return result;
}

// Force global collection - should only be called from 1 thread.
void hxInternalCollect()
{
	int dummy;
   if (!sgAllocInit || !sgInternalEnable)
		return;

	sGlobalAlloc->Collect(&dummy);
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
	memset(inData,0,s);

	#ifdef DEBUG_ALLOC_PTR
	if (inData<=sgDebugPointer && sgDebugPointer -s < inData)
		printf("Pointer re-alloced !\n");
	/*
	for(int i=0;i<inSize;i++)
		if ( ((char *)result)[i]!=0)
			*(int *)0=0;
	*/
	#endif


   return new_data;
}


#endif // if INTERNAL_GC
