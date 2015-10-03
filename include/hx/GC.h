#ifndef HX_GC_H
#define HX_GC_H

#include <hx/Tls.h>

// Under the current scheme (as defined by HX_HCSTRING/HX_CSTRING in hxcpp.h)
//  each constant string data is prepended with a 4-byte header that says the string
//  is constant (ie, not part of GC) and whether there is(not) a pre-computed hash at
//  the end of the data.

#define HX_GC_CONST_ALLOC_BIT  0x80000000
#define HX_GC_CONST_ALLOC_MARK_BIT  0x80
#define HX_GC_NO_STRING_HASH   0x40000000
#define HX_GC_NO_HASH_MASK     (HX_GC_CONST_ALLOC_BIT | HX_GC_NO_STRING_HASH)

// Must allign allocs to 8 bytes to match floating point requirement?
#ifdef HXCPP_ALIGN_FLOAT
   #define HXCPP_ALIGN_ALLOC
#endif



// Tell compiler the extra functions are supported
#define HXCPP_GC_FUNCTIONS_1

// Function called by the haxe code...


// Helpers for debugging code
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_reachable(hx::Object *inKeep);
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_enable(bool inEnable);
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_collect(bool inMajor=true);
HXCPP_EXTERN_CLASS_ATTRIBUTES void   __hxcpp_gc_compact();
HXCPP_EXTERN_CLASS_ATTRIBUTES int   __hxcpp_gc_trace(hx::Class inClass, bool inPrint);
HXCPP_EXTERN_CLASS_ATTRIBUTES int   __hxcpp_gc_used_bytes();
HXCPP_EXTERN_CLASS_ATTRIBUTES int   __hxcpp_gc_mem_info(int inWhat);
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_enter_gc_free_zone();
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_exit_gc_free_zone();
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_gc_safe_point();
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_spam_collects(int inEveryNCalls);
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_set_minimum_working_memory(int inBytes);
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_set_minimum_free_space(int inBytes);
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_set_target_free_space_percentage(int inPercentage);

// Finalizers from haxe code...
void  __hxcpp_gc_do_not_kill(Dynamic inObj);
void  __hxcpp_set_finalizer(Dynamic inObj, void *inFunction);
hx::Object *__hxcpp_get_next_zombie();

#ifdef HXCPP_TELEMETRY
void __hxcpp_set_hxt_finalizer(void* inObj, void *inFunc);
#endif

hx::Object *__hxcpp_weak_ref_create(Dynamic inObject);
hx::Object *__hxcpp_weak_ref_get(Dynamic inRef);


unsigned int __hxcpp_obj_hash(Dynamic inObj);
int __hxcpp_obj_id(Dynamic inObj);
hx::Object *__hxcpp_id_obj(int);






namespace hx
{
// Generic allocation routine.
// If inSize is small (<4k) it will be allocated from the immix pool.
// Larger, and it will be allocated from a separate memory pool
// inIsObject specifies whether "__Mark"  should be called on the resulting object
void *InternalNew(int inSize,bool inIsObject);

// Used internall - realloc array data
void *InternalRealloc(void *inData,int inSize);

// Const buffers are allocated outside the GC system, and do not require marking
// String buffers can optionally have a pre-computed hash appended with this method
void *InternalCreateConstBuffer(const void *inData,int inSize,bool inAddStringHash=false);

// Called after collection by an unspecified thread
typedef void (*finalizer)(hx::Object *v);

// Used internally by the runtime.
// The constructor will add this object to the internal list of finalizers.
// You must call "Mark" on this object within every collection loop to keep it alive,
//  otherwise the Gc system will call the finalizer and delete this object.
// This would typically be done in response to a "__Mark" call on another object
struct InternalFinalizer
{
   InternalFinalizer(hx::Object *inObj, finalizer inFinalizer=0);

   void Mark() { mUsed=true; }
   #ifdef HXCPP_VISIT_ALLOCS
   void Visit(VisitContext *__inCtx);
   #endif
   void Detach();

   bool      mUsed;
   bool      mValid;
   finalizer mFinalizer;
   hx::Object  *mObject;
};

// Attach a finalizer to any object allocation.  This can be called from haxe code, but be aware that
// you can't make any GC calls from the finalizer.
void  GCSetFinalizer( hx::Object *, hx::finalizer f );

// If another thread wants to do a collect, it will signal this variable.
// This automatically gets checked when you call "new", but if you are in long-running
//  loop with no new call, you might starve another thread if you to not check this.
//  0xffffffff = pause requested
extern int gPauseForCollect;


// Minimum total memory - used + buffer for new objects
extern int sgMinimumWorkingMemory;

// Minimum free memory - not counting used memory
extern int sgMinimumFreeSpace;

// Also ensure that the free memory is larger than this amount of used memory
extern int sgTargetFreeSpacePercentage;


// Call in response to a gPauseForCollect. Normally, this is done for you in "new"
void PauseForCollect();


// Used by WeakHash to work out if it needs to dispose its keys
bool IsWeakRefValid(hx::Object *inPtr);

// Used by CFFI to scan a block of memory for GC Pointers. May picks up random crap
//  that points to real, active objects.
void MarkConservative(int *inBottom, int *inTop,hx::MarkContext *__inCtx);


// Create/Remove a root.
// All statics are explicitly registered - this saves adding the whole data segment
//  to the collection list.
// It takes a pointer-pointer so it can move the contents, and the caller can change the contents
void GCAddRoot(hx::Object **inRoot);
void GCRemoveRoot(hx::Object **inRoot);


// This is used internally in hxcpp
// It calls InternalNew, and takes care of null-terminating the result
HX_CHAR *NewString(int inLen);

// The concept of 'private' is from the old conservative Gc method.
// Now with explicit marking, these functions do the same thing, which is
//  to allocate some GC memory and optionally copy the 'inData' into those bytes
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewGCBytes(void *inData,int inSize);
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewGCPrivate(void *inData,int inSize);

// Force a collect from the calling thread
// Only one thread should call this at a time
int InternalCollect(bool inMajor,bool inCompact);


// Disable the garbage collector.  It will try to increase its internal buffers to honour extra requests.
//  If it runs out of memory, it will actually try to do a collect.
void InternalEnableGC(bool inEnable);

// Record that fact that external memory has been allocated and associated with a haxe object
//  eg. BitmapData.  This will help the collector know when to collect
void GCChangeManagedMemory(int inDelta, const char *inWhy=0);

// Haxe threads can center GC free zones, where they can't make GC allocation calls, and should not mess with GC memory.
// This means that they do not need to pause while the GC collections happen, and other threads will not
//  wait for them to "check in" before collecting.  The standard runtime makes these calls around OS calls, such as "Sleep"
void EnterGCFreeZone();
void ExitGCFreeZone();

// Defined in Class.cpp, these function is called from the Gc to start the marking/visiting
void MarkClassStatics(hx::MarkContext *__inCtx);
#ifdef HXCPP_VISIT_ALLOCS
void VisitClassStatics(hx::VisitContext *__inCtx);
#endif


// Called by haxe/application code to mark allocations.
//  "Object" allocs will recursively call __Mark
inline void MarkAlloc(void *inPtr ,hx::MarkContext *__inCtx);
inline void MarkObjectAlloc(hx::Object *inPtr ,hx::MarkContext *__inCtx);

// Implemented differently for efficiency
void MarkObjectArray(hx::Object **inPtr, int inLength, hx::MarkContext *__inCtx);
void MarkStringArray(String *inPtr, int inLength, hx::MarkContext *__inCtx);

// Provide extra debug info to the marking routines
#ifdef HXCPP_DEBUG
HXCPP_EXTERN_CLASS_ATTRIBUTES void MarkSetMember(const char *inName ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES void MarkPushClass(const char *inName ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES void MarkPopClass(hx::MarkContext *__inCtx);
#endif


// Used by runtime if it is being paranoid about pointers.  It checks that the pointer is real and alive at last collect.
void GCCheckPointer(void *);


void SetTopOfStack(void *inTopOfStack,bool inForce=false);

// Called internally before and GC operations
void CommonInitAlloc();


// Threading ...
void RegisterNewThread(void *inTopOfStack);
void RegisterCurrentThread(void *inTopOfStack);
void UnregisterCurrentThread();
void GCPrepareMultiThreaded();




} // end namespace hx



// Inline code tied to the immix implementation

namespace hx
{

#define HX_USE_INLINE_IMMIX_OPERATOR_NEW

// Each line ast 128 bytes (2^7)
#define IMMIX_LINE_BITS    7
#define IMMIX_LINE_LEN     (1<<IMMIX_LINE_BITS)

// The size info is stored in the header 8 bits to the right
#define IMMIX_ALLOC_SIZE_SHIFT  8

// Indicates that __Mark must be called recursively
#define IMMIX_ALLOC_IS_CONTAINER   0x00800000


extern bool gMultiThreadMode;


class ImmixAllocator
{
public:
   virtual ~ImmixAllocator() {}
   virtual void *CallAlloc(int inSize,unsigned int inObjectFlags) = 0;

   int            spaceStart;
   int            spaceEnd;
   unsigned int   *allocStartFlags;
   unsigned char  *allocBase;
};

EXTERN_FAST_TLS_DATA(ImmixAllocator, tlsImmixAllocator);

extern ImmixAllocator *gMainThreadAlloc;
extern unsigned int gImmixStartFlag[128];
extern int gMarkID;
extern int gMarkIDWithContainer;
extern void BadImmixAlloc();


// The gPauseForCollect bits will turn spaceEnd negative, and so force the slow path
#ifndef HXCPP_SINGLE_THREADED_APP
   #define WITH_PAUSE_FOR_COLLECT_FLAG | hx::gPauseForCollect
#else
   #define WITH_PAUSE_FOR_COLLECT_FLAG
#endif



HXCPP_EXTERN_CLASS_ATTRIBUTES extern unsigned int gPrevMarkIdMask;

// Called only once it is determined that a new mark is required
HXCPP_EXTERN_CLASS_ATTRIBUTES void MarkAllocUnchecked(void *inPtr ,hx::MarkContext *__inCtx); 
HXCPP_EXTERN_CLASS_ATTRIBUTES void MarkObjectAllocUnchecked(hx::Object *inPtr ,hx::MarkContext *__inCtx);

inline void MarkAlloc(void *inPtr ,hx::MarkContext *__inCtx)
{
   // This will also skip const regions
   if ( ((unsigned int *)inPtr)[-1] & hx::gPrevMarkIdMask )
      MarkAllocUnchecked(inPtr,__inCtx);
}
inline void MarkObjectAlloc(hx::Object *inPtr ,hx::MarkContext *__inCtx)
{
   // This will also skip const regions
   if ( ((unsigned int *)inPtr)[-1] & hx::gPrevMarkIdMask )
      MarkObjectAllocUnchecked(inPtr,__inCtx);
}


} // end namespace hx













// It was theoretically possible to redefine the MarkContext arg type (or skip it)
//  incase the particular GC scheme did not need it.  This may take a bit of extra
//  work to get going again

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





// These macros add debug to the mark/visit calls if required
// They also perform some inline checking to avoid function calls if possible


#ifdef HXCPP_DEBUG

#define HX_MARK_MEMBER_NAME(x,name) { hx::MarkSetMember(name, __inCtx); hx::MarkMember(x, __inCtx ); }
#define HX_MARK_BEGIN_CLASS(x) hx::MarkPushClass(#x, __inCtx );
#define HX_MARK_END_CLASS() hx::MarkPopClass(__inCtx );
#define HX_MARK_MEMBER(x) { hx::MarkSetMember(0, __inCtx); hx::MarkMember(x, __inCtx ); }
#define HX_MARK_MEMBER_ARRAY(x,len) { hx::MarkSetMember(0, __inCtx); hx::MarkMemberArray(x, len, __inCtx ); }

#else

#define HX_MARK_MEMBER_NAME(x,name) hx::MarkMember(x, __inCtx )
#define HX_MARK_BEGIN_CLASS(x)
#define HX_MARK_END_CLASS()
#define HX_MARK_MEMBER(x) hx::MarkMember(x, __inCtx )
#define HX_MARK_MEMBER_ARRAY(x,len) hx::MarkMemberArray(x, len, __inCtx )

#endif

#define HX_MARK_OBJECT(ioPtr) if (ioPtr) hx::MarkObjectAlloc(ioPtr, __inCtx );




#define HX_MARK_STRING(ioPtr) \
   if (ioPtr) hx::MarkAlloc((void *)ioPtr, __inCtx );

#define HX_MARK_ARRAY(ioPtr) { if (ioPtr) hx::MarkAlloc((void *)ioPtr, __inCtx ); }




#define HX_VISIT_MEMBER_NAME(x,name) hx::VisitMember(x, __inCtx )
#define HX_VISIT_MEMBER(x) hx::VisitMember(x, __inCtx )

#define HX_VISIT_OBJECT(ioPtr) \
  { if (ioPtr) __inCtx->visitObject( (hx::Object **)&ioPtr); }

#define HX_VISIT_STRING(ioPtr) \
   if (ioPtr && !(((unsigned int *)ioPtr)[-1] & HX_GC_CONST_ALLOC_BIT) ) __inCtx->visitAlloc((void **)&ioPtr);

#define HX_VISIT_ARRAY(ioPtr) { if (ioPtr) __inCtx->visitAlloc((void **)&ioPtr); }







#endif

