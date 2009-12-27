#ifndef HX_GC_H
#define HX_GC_H




extern int gPauseForCollect;
void hxPauseForCollect();


#ifdef INTERNAL_GC
  #ifdef HXCPP_MULTI_THREADED
  #define __SAFE_POINT if (gPauseForCollect) hxPauseForCollect();
  #else
  #define __SAFE_POINT
  #endif
#else
#define __SAFE_POINT
#endif

class hxObject;

typedef void (*finalizer)(hxObject *v);


void *hxInternalNew(int inSize,bool inIsObject);
void *hxInternalRealloc(void *inData,int inSize);
void hxInternalEnableGC(bool inEnable);
void *hxInternalCreateConstBuffer(const void *inData,int inSize);
void hxRegisterNewThread(void *inTopOfStack);
void hxInternalCollect();

void hxEnterGCFreeZone();
void hxExitGCFreeZone();

// Threading ...
void hxRegisterCurrentThread(void *inTopOfStack);
void hxUnregisterCurrentThread();
void hxEnterSafePoint();
void hxGCPrepareMultiThreaded();



void hxGCMarkNow();

void hxPrologDone();

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


void hxMarkAlloc(void *inPtr);
void hxMarkObjectAlloc(hxObject *inPtr);




#define HX_MARK_OBJECT(ioPtr) if (ioPtr) hxMarkObjectAlloc(ioPtr);

#define GC_CONST_STRING  0xffffffff

#define HX_MARK_STRING(ioPtr) \
   if (ioPtr && (((int *)ioPtr)[-1] != GC_CONST_STRING) ) hxMarkAlloc((void *)ioPtr);

#define HX_MARK_ARRAY(ioPtr) { if (ioPtr) hxMarkAlloc((void *)ioPtr); }




#endif

