#ifndef HX_GC_INTERNAL_H
#define HX_GC_INTERNAL_H


// INTERNAL_GC is defined on the command-line with "-DINTERNAL_GC"

class hxObject;

typedef void (*finalizer)(hxObject *v);


void *hxInternalNew(int inSize);
void *hxInternalRealloc(void *inData,int inSize);
void hxInternalEnableGC(bool inEnable);
void *hxInternalCreateConstBuffer(const void *inData,int inSize);
void hxInternalCollect();

void hxGCMarkNow();

struct hxInternalFinalizer
{
	hxInternalFinalizer(hxObject *inObj);

	void Mark() { mUsed=true; }
	void Detach();

	bool      mUsed;
	bool      mValid;
	finalizer mFinalizer;
	hxObject  *mObject;
};


// Some inline implementations ...
// Use macros to allow for mark/move
#define HX_GC_MARKED     0x80000000
#define HX_GC_IS_CONST   0x40000000
#define HX_GC_FIXED      0x20000000
#define HX_GC_SMALL_OBJ  0x10000000
#define HX_GC_MEDIUM_OBJ 0x08000000
#define HX_MOVED_ID_MASK 0x07ffffff
#define HX_SIZE_MASK     HX_MOVED_ID_MASK

// About the same spees as dynamic_cast<void *> ....
   //unsigned int &flags =  ((unsigned int *)dynamic_cast<void *>(ioPtr))[-1];


#define HX_GC_IMMIX


#ifdef HX_GC_IMMIX

extern char **gMovedPtrs;
extern int  gMarkBit;

#define IMMIX_BLOCK_BITS      15
#define IMMIX_LINE_BITS        7

#define IMMIX_BLOCK_SIZE      (1<<IMMIX_BLOCK_BITS)
#define IMMIX_BLOCK_OFFSET_MASK (IMMIX_BLOCK_SIZE-1)
#define IMMIX_BLOCK_BASE_MASK (~(size_t)(IMMIX_BLOCK_OFFSET_MASK))
#define IMMIX_LINE_LEN        (1<<IMMIX_LINE_BITS)
#define IMMIX_LINES           (1<<(IMMIX_BLOCK_BITS-IMMIX_LINE_BITS))
#define IMMIX_HEADER_LINES    (IMMIX_LINES>>IMMIX_LINE_BITS)
#define IMMIX_USEFUL_LINES    (IMMIX_LINES - IMMIX_HEADER_LINES)
#define IMMIX_LINE_MASK       (~(size_t)((IMMIX_BLOCK_SIZE>>IMMIX_LINE_BITS)-1))

#define IMMIX_LARGE_OBJ_SIZE 8000

#define MARK_ROW(ptr,flags) \
{ \
	register size_t ptr_i = ((size_t)ptr)-sizeof(int); \
   char *block = (char *)(ptr_i & IMMIX_BLOCK_BASE_MASK); \
   char *base = block + ((ptr_i & IMMIX_BLOCK_OFFSET_MASK)>>IMMIX_LINE_BITS); \
	if ( flags & HX_GC_SMALL_OBJ ) \
		*base = 1; \
	else if (flags & HX_GC_MEDIUM_OBJ) \
	{ \
		int rows = (( (flags & HX_SIZE_MASK) + (sizeof(int) + IMMIX_LINE_LEN -1)  + \
				    (ptr_i & (IMMIX_LINE_LEN-1))) >> IMMIX_LINE_BITS); \
		for(int i=0;i<=rows;i++) \
			*base++ = 1; \
	} \
}

#define HX_MARK_OBJECT(ioPtr) \
if (ioPtr) \
{ \
   char *root = (char *)ioPtr->__root(); \
   unsigned int &flags =  ((unsigned int *)root)[-1]; \
   if ( !(flags&HX_GC_FIXED) ) \
	{ \
		*(char **)ioPtr =  gMovedPtrs[flags & HX_MOVED_ID_MASK] + ( (char *)(ioPtr) - root) ; \
	} \
	if ( (flags&HX_GC_MARKED)!=gMarkBit  ) \
   { \
		flags ^= HX_GC_MARKED; \
		MARK_ROW(root,flags); \
		(ioPtr)->__Mark(); \
	} \
}

#define HX_MARK_STRING(ioPtr) \
if (ioPtr) \
{ \
   int &flags = ((int *)(ioPtr))[-1]; \
   if ( !(flags & HX_GC_IS_CONST) ) \
	{ \
		if (!( flags&HX_GC_FIXED ) ) \
		{ \
			*(char **)ioPtr = gMovedPtrs[flags & HX_MOVED_ID_MASK]; \
		} \
		if ( (flags & HX_GC_MARKED) != gMarkBit) \
		{ \
			flags ^= HX_GC_MARKED; \
			MARK_ROW(ioPtr,flags); \
		} \
	} \
}

#else // Naive GC (non-immix)...

#define HX_MARK_OBJECT(ioPtr) \
if (ioPtr) \
{ \
   unsigned int &flags =  ((unsigned int *)ioPtr->__root())[-1]; \
   if (!( flags&HX_GC_MARKED ) ) \
   { \
		flags |= HX_GC_MARKED; \
		(ioPtr)->__Mark(); \
	} \
}

#define HX_MARK_STRING(ioPtr) \
if (ioPtr) \
{ \
   int &flags = ((int *)(ioPtr))[-1]; \
   if ( !(flags & (HX_GC_MARKED | HX_GC_IS_CONST) ) ) \
      flags |= HX_GC_MARKED; \
}

#endif

#endif

