#ifndef HX_GC_H
#define HX_GC_H


#define HX_MARK_ARG __inCtx
//#define HX_MARK_ADD_ARG ,__inCtx
#define HX_MARK_PARAMS hx::MarkContext *__inCtx
//#define HX_MARK_ADD_PARAMS ,hx::MarkContext *__inCtx

#ifdef HXCPP_VISIT_ALLOCS
#define HX_VISIT_ARG __inCtx
#define HX_VISIT_PARAMS hx::VisitContext *__inCtx
#else
#define HX_VISIT_ARG
#define HX_VISIT_PARAMS
#endif

// Tell compiler the extra functions are supported
#define HXCPP_GC_FUNCTIONS_1

// Helpers for debugging code
void  __hxcpp_reachable(hx::Object *inKeep);
void  __hxcpp_enable(bool inEnable);
void  __hxcpp_collect(bool inMajor=true);
void   __hxcpp_gc_compact();
int   __hxcpp_gc_trace(hx::Class inClass, bool inPrint);
int   __hxcpp_gc_used_bytes();
int   __hxcpp_gc_mem_info(int inWhat);
void  __hxcpp_enter_gc_free_zone();
void  __hxcpp_exit_gc_free_zone();
void  __hxcpp_gc_safe_point();
void  __hxcpp_spam_collects(int inEveryNCalls);

// Finalizers from haxe code...
void  __hxcpp_gc_do_not_kill(Dynamic inObj);
void  __hxcpp_set_finalizer(Dynamic inObj, void *inFunction);
hx::Object *__hxcpp_get_next_zombie();

hx::Object *__hxcpp_weak_ref_create(Dynamic inObject);
hx::Object *__hxcpp_weak_ref_get(Dynamic inRef);


unsigned int __hxcpp_obj_hash(Dynamic inObj);
int __hxcpp_obj_id(Dynamic inObj);
hx::Object *__hxcpp_id_obj(int);

namespace hx
{

extern int gPauseForCollect;
void PauseForCollect();
bool IsWeakRefValid(hx::Object *inPtr);

void MarkConservative(int *inBottom, int *inTop,hx::MarkContext *__inCtx);

class RegisterCapture
{
public:
	virtual int Capture(int *inTopOfStack,int **inBuf,int &outSize,int inMaxSize,int *inDummy);
   static RegisterCapture *Instance();
};

#ifdef HXCPP_MULTI_THREADED
#define __SAFE_POINT if (hx::gPauseForCollect) hx::PauseForCollect();
#else
#define __SAFE_POINT
#endif


// Create a new root.
// All statics are explicitly registered - this saves adding the whole data segment
// to the collection list.
void GCAddRoot(hx::Object **inRoot);
void GCRemoveRoot(hx::Object **inRoot);




// This is used internally in hxcpp
HX_CHAR *NewString(int inLen);

// Internal for arrays
void *GCRealloc(void *inData,int inSize);
void GCInit();
void MarkClassStatics(hx::MarkContext *__inCtx);
void LibMark();
void GCMark(Object *inPtr);
#ifdef HXCPP_VISIT_ALLOCS
void VisitClassStatics(hx::VisitContext *__inCtx);
#endif

// This will be GC'ed
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewGCBytes(void *inData,int inSize);
// This wont be GC'ed
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewGCPrivate(void *inData,int inSize);


typedef void (*finalizer)(hx::Object *v);

void  GCSetFinalizer( hx::Object *, hx::finalizer f );


void GCCheckPointer(void *);
void *InternalNew(int inSize,bool inIsObject);
void *InternalRealloc(void *inData,int inSize);
void InternalEnableGC(bool inEnable);
void *InternalCreateConstBuffer(const void *inData,int inSize,bool inAddStringHash=false);
void RegisterNewThread(void *inTopOfStack);
void SetTopOfStack(void *inTopOfStack,bool inForce=false);
int InternalCollect(bool inMajor,bool inCompact);
void GCChangeManagedMemory(int inDelta, const char *inWhy=0);

void EnterGCFreeZone();
void ExitGCFreeZone();

// Threading ...
void RegisterCurrentThread(void *inTopOfStack);
void UnregisterCurrentThread();
void EnterSafePoint();
void GCPrepareMultiThreaded();




void PrologDone();


HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkAlloc(void *inPtr ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkObjectAlloc(hx::Object *inPtr ,hx::MarkContext *__inCtx);

#ifdef HXCPP_DEBUG
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkSetMember(const char *inName ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkPushClass(const char *inName ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkPopClass(hx::MarkContext *__inCtx);
#endif

// Make sure we can do a conversion to hx::Object **
inline void EnsureObjPtr(hx::Object *) { }

} // end namespace hx

#ifdef HXCPP_DEBUG

#define HX_MARK_MEMBER_NAME(x,name) { hx::MarkSetMember(name, __inCtx); hx::MarkMember(x, __inCtx ); }
#define HX_MARK_BEGIN_CLASS(x) hx::MarkPushClass(#x, __inCtx );
#define HX_MARK_END_CLASS() hx::MarkPopClass(__inCtx );
#define HX_MARK_MEMBER(x) { hx::MarkSetMember(0, __inCtx); hx::MarkMember(x, __inCtx ); }

#else

#define HX_MARK_MEMBER_NAME(x,name) hx::MarkMember(x, __inCtx )
#define HX_MARK_BEGIN_CLASS(x)
#define HX_MARK_END_CLASS()
#define HX_MARK_MEMBER(x) hx::MarkMember(x, __inCtx )

#endif



#define HX_MARK_OBJECT(ioPtr) if (ioPtr) hx::MarkObjectAlloc(ioPtr, __inCtx );


#define HX_GC_CONST_ALLOC_BIT  0x80000000
#define HX_GC_CONST_ALLOC_MARK_BIT  0x80
#define HX_GC_NO_STRING_HASH   0x40000000
#define HX_GC_NO_HASH_MASK     (HX_GC_CONST_ALLOC_BIT | HX_GC_NO_STRING_HASH)

#define HX_MARK_STRING(ioPtr) \
   if (ioPtr && !(((unsigned int *)ioPtr)[-1] & HX_GC_CONST_ALLOC_BIT) ) hx::MarkAlloc((void *)ioPtr, __inCtx );

#define HX_MARK_ARRAY(ioPtr) { if (ioPtr) hx::MarkAlloc((void *)ioPtr, __inCtx ); }




#define HX_VISIT_MEMBER_NAME(x,name) hx::VisitMember(x, __inCtx )
#define HX_VISIT_MEMBER(x) hx::VisitMember(x, __inCtx )

#define HX_VISIT_OBJECT(ioPtr) \
  { hx::EnsureObjPtr(ioPtr); if (ioPtr) __inCtx->visitObject( (hx::Object **)&ioPtr); }

#define HX_VISIT_STRING(ioPtr) \
   if (ioPtr && !(((unsigned int *)ioPtr)[-1] & HX_GC_CONST_ALLOC_BIT) ) __inCtx->visitAlloc((void **)&ioPtr);

#define HX_VISIT_ARRAY(ioPtr) { if (ioPtr) __inCtx->visitAlloc((void **)&ioPtr); }



namespace hx
{

struct InternalFinalizer
{
	InternalFinalizer(hx::Object *inObj);

	void Mark() { mUsed=true; }
   #ifdef HXCPP_VISIT_ALLOCS
	void Visit(VisitContext *__inCtx) { HX_VISIT_OBJECT(mObject); }
   #endif
	void Detach();

	bool      mUsed;
	bool      mValid;
	finalizer mFinalizer;
	hx::Object  *mObject;
};


} // end namespace hx


#endif

