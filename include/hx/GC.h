#ifndef HX_GC_H
#define HX_GC_H


//#define HX_MARK_RECURSIVE

#ifdef HX_MARK_RECURSIVE

#define HX_MARK_ARG
#define HX_MARK_ADD_ARG
#define HX_MARK_PARAMS
#define HX_MARK_ADD_PARAMS

#else

#define HX_MARK_ARG __inCtx
#define HX_MARK_ADD_ARG ,__inCtx
#define HX_MARK_PARAMS hx::MarkContext *__inCtx
#define HX_MARK_ADD_PARAMS ,hx::MarkContext *__inCtx

#endif

#define HX_MARK_BEGIN_CLASS(x)
#define HX_MARK_END_CLASS()


// Helpers for debugging code
void  __hxcpp_reachable(hx::Object *inKeep);
void  __hxcpp_enable(bool inEnable);
void  __hxcpp_collect();
int   __hxcpp_gc_used_bytes();

namespace hx
{

extern int gPauseForCollect;
void PauseForCollect();

class RegisterCapture
{
public:
	virtual int Capture(int *inTopOfStack,int **inBuf,int &outSize,int inMaxSize);
   static RegisterCapture *Instance();
};

#ifdef HX_INTERNAL_GC
  #ifdef HXCPP_MULTI_THREADED
  #define __SAFE_POINT if (hx::gPauseForCollect) hx::PauseForCollect();
  #else
  #define __SAFE_POINT
  #endif
#else
#define __SAFE_POINT
#endif


// Create a new root.
// All statics are explicitly registered - this saves adding the whole data segment
// to the collection list.
void RegisterObject(hx::Object **inObj);
void RegisterString(const HX_CHAR **inString);

void GCAddRoot(hx::Object **inRoot);
void GCRemoveRoot(hx::Object **inRoot);




// This is used internally in hxcpp
HX_CHAR *NewString(int inLen);

// Internal for arrays
void *GCRealloc(void *inData,int inSize);
void GCInit();
void MarkClassStatics(HX_MARK_PARAMS);
void LibMark();
void GCMark(Object *inPtr);

// This will be GC'ed
void *NewGCBytes(void *inData,int inSize);
// This wont be GC'ed
void *NewGCPrivate(void *inData,int inSize);


typedef void (*finalizer)(hx::Object *v);

void  GCAddFinalizer( hx::Object *, hx::finalizer f );


void *InternalNew(int inSize,bool inIsObject);
void *InternalRealloc(void *inData,int inSize);
void InternalEnableGC(bool inEnable);
void *InternalCreateConstBuffer(const void *inData,int inSize);
void RegisterNewThread(void *inTopOfStack);
void SetTopOfStack(void *inTopOfStack,bool inForce=false);
void InternalCollect();

void EnterGCFreeZone();
void ExitGCFreeZone();

// Threading ...
void RegisterCurrentThread(void *inTopOfStack);
void UnregisterCurrentThread();
void EnterSafePoint();
void GCPrepareMultiThreaded();




void PrologDone();

struct InternalFinalizer
{
	InternalFinalizer(hx::Object *inObj);

	void Mark() { mUsed=true; }
	void Detach();

	bool      mUsed;
	bool      mValid;
	finalizer mFinalizer;
	hx::Object  *mObject;
};


void MarkAlloc(void *inPtr HX_MARK_ADD_PARAMS);
void MarkObjectAlloc(hx::Object *inPtr HX_MARK_ADD_PARAMS);


} // end namespace hx


#define HX_MARK_MEMBER(x) hx::MarkMember(x HX_MARK_ADD_ARG )

#define HX_MARK_MEMBER_NAME(x,name) hx::MarkMember(x HX_MARK_ADD_ARG )

#define HX_MARK_OBJECT(ioPtr) if (ioPtr) hx::MarkObjectAlloc(ioPtr HX_MARK_ADD_ARG );

#define HX_GC_CONST_STRING  0xffffffff

#define HX_MARK_STRING(ioPtr) \
   if (ioPtr && (((int *)ioPtr)[-1] != HX_GC_CONST_STRING) ) hx::MarkAlloc((void *)ioPtr HX_MARK_ADD_ARG );

#define HX_MARK_ARRAY(ioPtr) { if (ioPtr) hx::MarkAlloc((void *)ioPtr HX_MARK_ADD_ARG ); }


#endif

