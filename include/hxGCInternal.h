#ifndef HX_GC_INTERNAL_H
#define HX_GC_INTERNAL_H


#define INTERNAL_GC
/*
#ifdef IPHONE
 #define INTERNAL_GC
#endif
*/

class hxObject;

typedef void (*finalizer)(hxObject *v);


void *hxInternalNew(int inSize);
void hxInternalAddFinalizer(void *v, finalizer f);
void *hxInternalRealloc(void *inData,int inSize);
void hxInternalEnableGC(bool inEnable);
void hxInternalCollect();

void hxGCMarkNow();


// Some inline implementations ...
// Use macros to allow for mark/move
#define HX_GC_MARKED    0x80000000
#define HX_GC_IS_CONST  0x40000000

// About the same spees as dynamic_cast<void *> ....
   //unsigned int &flags =  ((unsigned int *)dynamic_cast<void *>(ioPtr))[-1];

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

#define HX_MARK_STRING(ioPtr,type) \
if (ioPtr) \
{ \
   int &flags = ((int *)(ioPtr))[-1]; \
   if ( !(flags & (HX_GC_MARKED | HX_GC_IS_CONST) ) ) \
      flags |= HX_GC_MARKED; \
}


#endif

