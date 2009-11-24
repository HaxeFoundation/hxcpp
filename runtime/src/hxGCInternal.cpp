#include <hxGCInternal.h>
	// TODO: what is ThreadLocalAlloc is NULL (foreign thread) ?

char **gMovedPtrs = 0;
int gMarkBit = 0;


#ifdef INTERNAL_GC

#define PRECISE_STACK_MARK
#define CREATE_OBJ_TABLE
#define MARK_OBJ_TABLE
#define UPDATE_OBJ_TABLE

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <hxObject.h>

#include <map>
#include <vector>
#include <set>

void hxMarkCurrentThread(void *inBottomOfStack);


static bool sgAllocInit = 0;
static bool sgInternalEnable = false;
static void *sgObject_root = 0;

//#define DEBUG_ALLOC_PTR ((char *)0xb68354)
//#define DEBUG_ALLOC_SIZE 0xd24

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
#define IMMIX_LINES             (1<<(IMMIX_BLOCK_BITS-IMMIX_LINE_BITS))
#define IMMIX_HEADER_LINES      (IMMIX_LINES>>IMMIX_LINE_BITS)
#define IMMIX_USEFUL_LINES      (IMMIX_LINES - IMMIX_HEADER_LINES)
#define IMMIX_LINE_POS_MASK     ((size_t)(IMMIX_LINE_LEN-1))
#define IMMIX_START_OF_ROW_MASK  (~IMMIX_LINE_POS_MASK)



/*

 IMMIX Alloc Header - 32 bits

*/

#define IMMIX_ALLOC_MARKED      0x80000000
#define IMMIX_ALLOC_IS_CONST    0x40000000
#define IMMIX_ALLOC_SMALL_OBJ   0x20000000
#define IMMIX_ALLOC_MEDIUM_OBJ  0x10000000
#define IMMIX_ALLOC_TO_OBJ_TBL  0x00ff0000
#define IMMIX_ALLOC_IS_OBJECT   0x00008000
#define IMMIX_ALLOC_SIZE_MASK   0x00007fff



/*

 IMMIX Row Header - 8 bits

*/
#define IMMIX_ROW_NEEDS_CLEAR     0x80
#define IMMIX_ROW_BIG_OBJ_TABLE   0x80
#define IMMIX_ROW_OBJ_TABLE_POS   0x7C
#define IMMIX_ROW_HAS_OBJ_TABLE   0x02
#define IMMIX_ROW_MARKED          0x01
#define IMMIX_NOT_MARKED_MASK     (~IMMIX_ROW_MARKED)



/*
 OBJ_TBL - 8 bits
*/
#define IMMIX_OBJ_TBL_ALLOC_POS_MASK  0x7C
#define IMMIX_OBJ_TBL_ALL_ALLOC_POS_MASK  0x7C7C7C7C
#define IMMIX_OBJ_TBL_OBJECT_LIVE     0x02
#define IMMIX_OBJ_TBL_MARKED          0x01

// Bigger than this, and they go in the large object pool
#define IMMIX_LARGE_OBJ_SIZE 4000
#define IMMIX_LARGE_OBJ_SIZE_MASK   0x0fffffff

#ifdef allocString
#undef allocString
#endif



enum AllocType { allocNone, allocString, allocObject };

union BlockData
{
	void Init() { mUsedRows = 0; }
	inline int GetFreeData() const { return (IMMIX_USEFUL_LINES - mUsedRows)<<IMMIX_LINE_BITS; }
	void ClearEmpty()
	{
		memset(this,0,IMMIX_BLOCK_SIZE);
	}
	void ClearRecycled()
	{
		for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
		{
			if (!(mRowFlags[r] & IMMIX_ROW_MARKED))
			{
				mRowFlags[r] = 0;
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
			}
		}
	}
	bool IsEmpty() const { return mUsedRows == 0; }
	bool IsFull() const { return mUsedRows == IMMIX_USEFUL_LINES; }
	int GetRowsInUse() const { return mUsedRows; }
	inline bool IsRowUsed(int inRow) const { return mRowFlags[inRow] & IMMIX_ROW_MARKED; }

	void Verify()
	{
		for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
		{
			unsigned char &row_flag = mRowFlags[r];
			if ( !(row_flag * IMMIX_ROW_MARKED) )
			{
				if (row_flag!=0)
				{
					printf("Block verification failed on row %d\n",r);
					*(int *)0=0;
				}
			}
		}
	}

	#define CHECK_TABLE_LIVE \
					if (*table &&  \
			            ((row[*table & IMMIX_OBJ_TBL_ALLOC_POS_MASK] & IMMIX_ALLOC_MARKED) != \
							   gMarkBit)) *table = 0;

	void Reclaim()
	{
		int free = 0;
		for(int r=IMMIX_HEADER_LINES;r<IMMIX_LINES;r++)
		{
			unsigned char &row_flag = mRowFlags[r];
			if (row_flag & IMMIX_ROW_MARKED)
			{
				#ifdef UPDATE_OBJ_TABLE
				// Find object table
				if (row_flag & IMMIX_ROW_HAS_OBJ_TABLE)
				{
					int table_pos = row_flag & IMMIX_ROW_OBJ_TABLE_POS;

					#ifndef MARK_OBJ_TABLE
					// Must update from the object mark flag ...
					unsigned char *row = mRow[r];
					unsigned char *table = row + table_pos;

					CHECK_TABLE_LIVE
					table++;
					CHECK_TABLE_LIVE
					table++;
					CHECK_TABLE_LIVE
					table++;
					CHECK_TABLE_LIVE
					if (row_flag & IMMIX_ROW_BIG_OBJ_TABLE)
					{
						table++;
						CHECK_TABLE_LIVE
						table++;
						CHECK_TABLE_LIVE
						table++;
						CHECK_TABLE_LIVE
						table++;
						CHECK_TABLE_LIVE
					}

					#else
					unsigned int *table = (unsigned int *)(mRow[r] + table_pos);

					// Shift mark bit into "live" bit
					*table = (*table & IMMIX_OBJ_TBL_ALL_ALLOC_POS_MASK) | ((*table & 0x01010101) << 1);

					if (row_flag & IMMIX_ROW_BIG_OBJ_TABLE)
					{
						table++;
						// Shift mark bit into "live" bit
						*table = (*table & IMMIX_OBJ_TBL_ALL_ALLOC_POS_MASK) | ((*table & 0x01010101) << 1);
					}
					#endif
				}
				#endif
			}
			else
			{
				row_flag = 0;
				free++;
			}
		}

		mUsedRows = IMMIX_USEFUL_LINES - free;

		//Verify();
	}


	#ifdef PRECISE_STACK_MARK
	AllocType GetAllocType(int inOffset,bool inReport = false)
	{
		inReport = false;
		int row = inOffset >> IMMIX_LINE_BITS;
		if (row < IMMIX_HEADER_LINES || row >= IMMIX_LINES)
		{
			if (inReport)
			   printf("  bad row %d (off=%d)\n", row);
			return allocNone;
		}
		int flags = mRowFlags[row];
		if (!(flags & (IMMIX_ROW_HAS_OBJ_TABLE)))
		{
			if (inReport)
				printf("  row has no obj table :[%d] = %d\n", row, flags );
			return allocNone;
		}
		int sought = inOffset & IMMIX_LINE_POS_MASK;
		if (sought==0)
		{
			if (inReport)
				printf("  obj on start of row :[%d] (&%d)\n", row, IMMIX_LINE_POS_MASK );
			return allocNone;
		}

		int table_pos = flags & IMMIX_ROW_OBJ_TABLE_POS;
		unsigned char *table = (unsigned char *)(mRow[row] + table_pos);
		for(int n=(flags & IMMIX_ROW_BIG_OBJ_TABLE) ? 8 : 4; n; n--)
		{
			// Ok, it is a valid offset
			if ( (*table & IMMIX_OBJ_TBL_ALLOC_POS_MASK) == sought)
			{
				// Object dead ?
				if ( ! (*table & (IMMIX_OBJ_TBL_OBJECT_LIVE | IMMIX_OBJ_TBL_MARKED))  )
				{
					if (inReport)
						printf(" Object appears dead.\n");
					break; 
				}
				return ( (*(int *)(mRow[0] + inOffset)) & IMMIX_ALLOC_IS_OBJECT) ? allocObject : allocString;
			}
			table++;
		}

	   /*
		if (inReport)
		{
			printf("  not found in table (r=%d,tp=%d,sought =%d): ", row, table_pos, sought);
			table = (unsigned char *)(mRow[row] + table_pos);
			for(int n=(flags & IMMIX_ROW_BIG_OBJ_TABLE) ? 8 : 4; n; n--)
				printf ("%d ", *table++ & IMMIX_OBJ_TBL_ALLOC_POS_MASK);
			printf("\n");
		}
		*/

		return allocNone;
	}
	#endif

	void ClearRowMarks()
	{
		unsigned char *header = mRowFlags + IMMIX_HEADER_LINES;
		unsigned char *header_end = header + IMMIX_USEFUL_LINES;
		while(header !=  header_end)
			*header++ &= IMMIX_NOT_MARKED_MASK;
	}


	// First 2 bytes are not needed for row markers (first 2 rows are for flags)
	unsigned short mUsedRows;
	// First 2 rows contain a byte-flag-per-row 
	unsigned char  mRowFlags[IMMIX_LINES];
	// Row data as union - don't use first 2 rows
	unsigned char  mRow[IMMIX_LINES][IMMIX_LINE_LEN];
};


bool hxMarkAlloc(void *inPtr)
{
	register size_t ptr_i = ((size_t)inPtr)-sizeof(int);
   unsigned int &flags =  *((unsigned int *)ptr_i);
	if ( (flags&IMMIX_ALLOC_MARKED)==gMarkBit  )
		return false;

	if ( flags & IMMIX_ALLOC_IS_CONST )
		return false;

	flags ^= IMMIX_ALLOC_MARKED;

   char *block = (char *)(ptr_i & IMMIX_BLOCK_BASE_MASK);
   char *base = block + ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS);
	if ( flags & IMMIX_ALLOC_SMALL_OBJ )
	{
		*base |= IMMIX_ROW_MARKED;
		#ifdef MARK_OBJ_TABLE
		((unsigned char *)(ptr_i & IMMIX_START_OF_ROW_MASK))
			  [ (flags & IMMIX_ALLOC_TO_OBJ_TBL) >> 16 ] |= IMMIX_OBJ_TBL_MARKED;
		#endif
	}
	else if (flags & IMMIX_ALLOC_MEDIUM_OBJ)
	{
		int rows = (( (flags & IMMIX_ALLOC_SIZE_MASK) + sizeof(int) +
				    (ptr_i & (IMMIX_LINE_LEN-1)) -1 ) >> IMMIX_LINE_BITS);
		#ifdef MARK_OBJ_TABLE
		((unsigned char *)(ptr_i & IMMIX_START_OF_ROW_MASK))
			  [ (flags & IMMIX_ALLOC_TO_OBJ_TBL) >> 16 ] |= IMMIX_OBJ_TBL_MARKED;
		#endif
		for(int i=0;i<=rows;i++)
			*base++ |= IMMIX_ROW_MARKED;
	}

	return true;
}



typedef std::set<BlockData *> PointerSet;
typedef QuickVec<BlockData *> BlockList;

typedef QuickVec<int *> LargeList;

enum MemType { memUnmanaged, memBlock, memLarge };

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
   void *AllocLarge(int inSize)
	{
		inSize = (inSize +3) & ~3;
		int *result = (int *)malloc(inSize + sizeof(int));
		mLargeList.push(result);

		mLargeAllocated += inSize;
		result[0] = inSize | gMarkBit;
		mDistributedSinceLastCollect += inSize;
		return result+1;
	}
	BlockData * GetRecycledBlock(void *inBottomOfStack)
	{
		#ifdef PRECISE_STACK_MARK
		CheckCollect(inBottomOfStack);
		#endif
		if (mNextRecycled < mRecycledBlock.size())
		{
			BlockData *block = mRecycledBlock[mNextRecycled++];
			mDistributedSinceLastCollect +=  block->GetFreeData();
			block->ClearRecycled();
			return block;
		}
		return GetEmptyBlock(false,inBottomOfStack);
	}

	BlockData *GetEmptyBlock(bool inCheckCollect, void *inBottomOfStack)
	{
		#ifdef PRECISE_STACK_MARK
		if (inCheckCollect)
			CheckCollect(inBottomOfStack);
		#endif
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
				BlockData *block = (BlockData *)(aligned + i*IMMIX_BLOCK_SIZE);
				block->Init();
				mAllBlocks.push(block);
				mEmptyBlocks.push(block);
			}
		}

		BlockData *block = mEmptyBlocks[mNextEmpty++];
		block->ClearEmpty();
		mActiveBlocks.insert(block);
		mDistributedSinceLastCollect +=  block->GetFreeData();
		return block;
	}

	void ClearRowMarks()
	{
		for(PointerSet::iterator i=mActiveBlocks.begin(); i!=mActiveBlocks.end();++i)
			(*i)->ClearRowMarks();
	}

	void Collect(void *inBottomOfStack)
	{
		static int collect = 0;
		//printf("Collect %d\n",collect++);
		gMarkBit ^= IMMIX_ALLOC_MARKED;

		ClearRowMarks();

		hxGCMarkNow();

		hxMarkCurrentThread(inBottomOfStack);

		RunFinalizers();

		// Reclaim ...
		//
		// Clear lists, start fresh...
		mEmptyBlocks.clear();
		mRecycledBlock.clear();
		for(PointerSet::iterator i=mActiveBlocks.begin(); i!=mActiveBlocks.end();++i)
			(*i)->Reclaim();
		mActiveBlocks.clear();
		mNextEmpty = 0;
		mNextRecycled = 0;
		mRowsInUse = 0;


		// IMMIX suggest filling up in creation order ....
		for(int i=0;i<mAllBlocks.size();i++)
		{
			BlockData *block = mAllBlocks[i];

			if (block->IsEmpty())
				mEmptyBlocks.push(block);
			else
			{
				mActiveBlocks.insert(block);
				mRowsInUse += block->GetRowsInUse();
				if (!block->IsFull())
					mRecycledBlock.push(block);
			}
		}

		int idx = 0;
		while(idx<mLargeList.size())
		{
			int *blob = mLargeList[idx];
			if ( (*blob & IMMIX_ALLOC_MARKED) != gMarkBit )
			{
				mLargeAllocated -= (*blob & IMMIX_LARGE_OBJ_SIZE_MASK);
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
			//printf("Collect %d/%d\n", mDistributedSinceLastCollect, mTotalAfterLastCollect);
			Collect(inBottomOfStack);
		}
	}

	size_t MemUsage()
	{
		return mLargeAllocated + (mRowsInUse<<IMMIX_LINE_BITS);
	}

	MemType GetMemType(void *inPtr)
	{
		BlockData *block = (BlockData *)( ((size_t)inPtr) & IMMIX_BLOCK_BASE_MASK);
		if ( mActiveBlocks.find(block) != mActiveBlocks.end() )
			return memBlock;

		for(int i=0;i<mLargeList.size();i++)
		{
			int *blob = mLargeList[i] + 1;
			if (blob==inPtr)
				return memLarge;
		}

		return memUnmanaged;
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
	PointerSet mActiveBlocks;
};

GlobalAllocator *sGlobalAlloc = 0;


// --- LocalAllocator -------------------------------------------------------
//
// One per thread ...

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
		mCurrent = 0;
		mOverflow = 0;
		mCurrentLine = IMMIX_LINES;
		#ifdef CREATE_OBJ_TABLE
		mObjTable = 0;
		mObjsLeftInTable = 0;
		#endif
		mCurrentPos = 0;
		mLinesSinceLastCollect = 0; 
	}

	void SetTopOfStack(int *inTop)
	{
		// stop early to allow for ptr[1] ....
		mTopOfStack = inTop;
	}

	#ifdef CREATE_OBJ_TABLE
	inline void AddObjectTable(unsigned char *inRow,int inSize)
	{
		if (mCurrent->mRowFlags[mCurrentLine] & IMMIX_ROW_HAS_OBJ_TABLE)
		{
			printf("TABLE ALREADY EXISTS ON ROW %d = %d - bailing\n", mCurrentLine,
			    mCurrent->mRowFlags[mCurrentLine] );
			*(int *)0=0;
		}

		//printf("New table on row %d (%d entries)\n",mCurrentLine,inSize);
		mCurrent->mRowFlags[mCurrentLine] = mCurrentPos | IMMIX_ROW_HAS_OBJ_TABLE |
								(inSize==8 ? IMMIX_ROW_BIG_OBJ_TABLE : 0);
		mObjTable = inRow + mCurrentPos;
		if (inSize==4)
			*(int *)mObjTable = 0;
		else
			*(double *)mObjTable = 0;
		mObjsLeftInTable = inSize;
		mCurrentPos += inSize;
		if (mCurrentPos>=IMMIX_LINE_LEN)
		{
			mCurrentPos = 0;
			mCurrentLine++;
		}
	}
	#endif

	void *Alloc(int inSize,bool inIsObject)
   {
		int s = ((inSize+3) & ~3) +sizeof(int);
		/*
		printf("Request %d, cline=%d, cpos=%d, objtbl=%p, left=%d: ", s,
			mCurrentLine, mCurrentPos, mObjTable, mObjsLeftInTable);
		if (mCurrent)
		for(int i=mCurrentLine; i<mCurrentLine + 4 && i<IMMIX_LINES; i++)
			printf("%02x ", mCurrent->mRowFlags[i]);
		printf("\n");
		*/
		while(1)
		{
			#ifdef CREATE_OBJ_TABLE
		   if (mObjTable && !mObjsLeftInTable)
			{
				//printf("--- not enough table entries---\n");
				mCurrentLine++;
				mObjTable = 0;
				mCurrentPos = 0;
			}
			#endif

			// Try to squeeze it on this line ...
			if (mCurrentPos > 0)
			{
				int skip = 1;
				#ifdef CREATE_OBJ_TABLE
				int table_size = mObjTable ? 0 : (mCurrentPos + inSize<64) ? 8 : 4;
				// enough room to start the object on this line?
				if (mCurrentPos+table_size+sizeof(int) < IMMIX_LINE_LEN)
				#else
				#define table_size 0
				#endif
				{
					int extra_lines = (s + table_size + mCurrentPos-1) >> IMMIX_LINE_BITS;
					//printf("check for %d extra lines ...\n", extra_lines);
					if (mCurrentLine + extra_lines < IMMIX_LINES)
					{
						int test = 0;
						if (extra_lines)
							for(test=0;test<extra_lines;test++)
							{
								if (mCurrent->IsRowUsed(mCurrentLine+test+1))
									break;
							}
						//printf(" found %d extra lines\n", test);
						if (test==extra_lines)
						{
							// Ok, fits on the line! - setup object table
							unsigned char *row = mCurrent->mRow[mCurrentLine];
							#ifdef CREATE_OBJ_TABLE
							//printf("Setting obj table entry %d\n", mCurrentPos);
							if (table_size)
								AddObjectTable(row,table_size);
							#endif

							int *result = (int *)(row + mCurrentPos);
							*result = inSize | gMarkBit |
					  			#ifdef CREATE_OBJ_TABLE
					  			( (mObjTable-row)<<16) |
					  			#endif
								(extra_lines==0 ? IMMIX_ALLOC_SMALL_OBJ : IMMIX_ALLOC_MEDIUM_OBJ );

							if (inIsObject)
								*result |= IMMIX_ALLOC_IS_OBJECT;

							#ifdef CREATE_OBJ_TABLE
							*mObjTable++ = mCurrentPos | IMMIX_OBJ_TBL_OBJECT_LIVE;
							if (extra_lines)
								mObjTable = 0;
							else
								mObjsLeftInTable--;
							#endif

							mCurrentLine += extra_lines;
							mCurrentPos = (mCurrentPos + s) & (IMMIX_LINE_LEN-1);
							if (mCurrentPos==0)
							{
								mCurrentLine++;
								#ifdef CREATE_OBJ_TABLE
								mObjTable = 0;
								#endif
							}

							//printf("Alloced %d - %d/%d now\n", s, mCurrentPos, mCurrentLine);

							return result + 1;
						}
						//printf("not enought extra lines - skip %d\n",skip);
						skip = test + 1;
					}
					else
						skip = extra_lines;
				}

				// Does not fit on this line - we may also know how many to skip, so
				//  jump down those lines...
				mCurrentPos = 0;
				mCurrentLine += skip;
				#ifdef CREATE_OBJ_TABLE
				mObjTable = 0;
				#endif
			}

			#ifdef CREATE_OBJ_TABLE
			int table_size = inSize<40 ? 8 : 4;
			#endif
			int required_rows = (s + table_size + IMMIX_LINE_LEN-1) >> IMMIX_LINE_BITS;
			int last_start = IMMIX_LINES - required_rows;

			// Alloc new block, if required ...
			if (!mCurrent || mCurrentLine>last_start)
			{
				mCurrent = sGlobalAlloc->GetRecycledBlock(&inSize);
				//mCurrent->Verify();
				// Start on line 2 (there are 256 line-markers at the beginning)
				mCurrentLine = IMMIX_HEADER_LINES;
			}

			// Look for N in a row ....
			while(mCurrentLine <= last_start)
			{
				int test = 0;
				for(;test<required_rows;test++)
					if (mCurrent->IsRowUsed(mCurrentLine+test))
						break;

				// Not enough room...
				if (test<required_rows)
				{
					mCurrentLine += test+1;
					//printf("  Only found %d good - skip to %d\n",test,mCurrentLine);
					continue;
				}

				// Ok, found a gap
				unsigned char *row = mCurrent->mRow[mCurrentLine];

				#ifdef CREATE_OBJ_TABLE
				AddObjectTable(row,table_size);
				#endif
				int *result = (int *)(row + mCurrentPos);
				*result = inSize | gMarkBit |
					#ifdef CREATE_OBJ_TABLE
					( (mObjTable-row)<<16) |
					#endif
				   (required_rows==1 ? IMMIX_ALLOC_SMALL_OBJ : IMMIX_ALLOC_MEDIUM_OBJ );

				if (inIsObject)
					*result |= IMMIX_ALLOC_IS_OBJECT;

				#ifdef CREATE_OBJ_TABLE
				*mObjTable++ = mCurrentPos | IMMIX_OBJ_TBL_OBJECT_LIVE;
				if (required_rows==1)
				   mObjsLeftInTable--;
				else
					mObjTable = 0;
				#endif
				mCurrentLine += required_rows - 1;
				mCurrentPos = (mCurrentPos + s) & (IMMIX_LINE_LEN-1);
				if (mCurrentPos==0)
				{
					#ifdef CREATE_OBJ_TABLE
					mObjTable = 0;
					#endif
					mCurrentLine++;
				}
				//printf("Alloced multiple/start %d(%d rows) - %d/%d now\n", s, required_rows, mCurrentPos, mCurrentLine);
				return result + 1;
			}
		}
		return 0;
	}


	void Mark(void *inBottomOfStack)
	{
		#ifdef PRECISE_STACK_MARK
		int here = 0;
		void *prev = 0;
		// printf("=========== Mark Stack ==================== %p/%d\n",inBottomOfStack,&here);
		for(int *ptr = inBottomOfStack ? (int *)inBottomOfStack : &here; ptr<mTopOfStack; ptr++)
		{
			void *vptr = *(void **)ptr;
			MemType mem;
			if (vptr && !((size_t)vptr & 0x03) && vptr!=prev &&
		           (mem = sGlobalAlloc->GetMemType(vptr)) != memUnmanaged )
			{
				if (mem==memLarge)
				{
					int &flags = ((int *)(vptr))[-1];
					if ((flags&IMMIX_ALLOC_MARKED)!=gMarkBit)
						flags ^= IMMIX_ALLOC_MARKED;
				}
				else
				{
					BlockData *block = (BlockData *)( ((size_t)vptr) & IMMIX_BLOCK_BASE_MASK);
					int pos = (int)(((size_t)vptr) & IMMIX_BLOCK_OFFSET_MASK);
					AllocType t = block->GetAllocType(pos-sizeof(int),true);
					if ( t==allocObject )
					{
						// printf(" Mark object %p (%p)\n", vptr,ptr);
						HX_MARK_OBJECT( ((hxObject *)vptr) );
					}
					else if (t==allocString)
					{
						// printf(" Mark string %p (%p)\n", vptr,ptr);
						HX_MARK_STRING(vptr);
					}
				}
			}
		}
		#endif
		//printf("marked\n");
	   Reset();
	}

	int mCurrentPos;
	#ifdef CREATE_OBJ_TABLE
	unsigned char *mObjTable;
	int mObjsLeftInTable;
	#endif
	int mCurrentLine;

	int mOverflowPos;
	int mOverflowLine;

	int mLinesSinceLastCollect;

	int *mTopOfStack;

	BlockData * mCurrent;
	BlockData * mOverflow;
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


void *hxInternalNew(int inSize,bool inIsObject)
{
	if (!sgAllocInit)
	{
		sgAllocInit = true;
		sGlobalAlloc = new GlobalAllocator();
      sgFinalizers = new FinalizerList();
		hxObject tmp;
		void **stack = *(void ***)(&tmp);
		sgObject_root = stack[0];
		//printf("__root pointer %p\n", sgObject_root);
		#ifdef _MSC_VER
		sTLSSlot = TlsAlloc();
		// Store object for main thread ...
		TlsSetValue(sTLSSlot, new LocalAllocator() );
		#endif
	}
	void *result;

	if (inSize>IMMIX_LARGE_OBJ_SIZE)
		result = sGlobalAlloc->AllocLarge(inSize);
	else
	{
		#ifdef _MSC_VER
		LocalAllocator *ThreadLocalAlloc = (LocalAllocator *)TlsGetValue(sTLSSlot);
		// TODO: what is ThreadLocalAlloc is NULL (foreign thread) ?
		#endif

		result = ThreadLocalAlloc->Alloc(inSize,inIsObject);
	}

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
      return hxInternalNew(inSize,false);

   int header = ((int *)(inData))[-1];

	int s = (header & ( IMMIX_ALLOC_SMALL_OBJ | IMMIX_ALLOC_MEDIUM_OBJ)) ?
			(header) & IMMIX_ALLOC_SIZE_MASK : (header) & IMMIX_LARGE_OBJ_SIZE_MASK;

   void *new_data = 0;

	if (inSize>IMMIX_LARGE_OBJ_SIZE)
	{
		new_data = sGlobalAlloc->AllocLarge(inSize);
		if (inSize>s)
			memset((char *)new_data + s,0,inSize-s);
	}
	else
	{
		#ifdef _MSC_VER
		LocalAllocator *ThreadLocalAlloc = (LocalAllocator *)TlsGetValue(sTLSSlot);
		// TODO: what is ThreadLocalAlloc is NULL (foreign thread) ?
		#endif

		new_data = ThreadLocalAlloc->Alloc(inSize,false);
	}

   int min_size = s < inSize ? s : inSize;

   memcpy(new_data, inData, min_size );

   return new_data;
}


#endif // if INTERNAL_GC
