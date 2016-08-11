#include <hxcpp.h>
#include <math.h>
#include <hxMath.h>
#include <stdio.h>

using namespace hx;

namespace hx {
extern hx::Class __ObjcClass;

class ObjcData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc,const char *inName="objc.BoxedType")
      { return hx::Object::operator new(inSize,inAlloc,inName); }

   ObjcData(const id inValue) : mValue(inValue) 
   {
      #ifndef OBJC_ARC
      [ inValue retain ];
      #endif
		mFinalizer = new hx::InternalFinalizer(this,clean);
   };

	static void clean(hx::Object *inObj)
	{
		ObjcData *m = dynamic_cast<ObjcData *>(inObj);
      if (m)
      {
         #ifndef OBJC_ARC
         [m->mValue :qrelease];
         #else
         m->mValue = nil;
         #endif
      }
	}

	void __Mark(hx::MarkContext *__inCtx) { mFinalizer->Mark(); }

   #ifdef HXCPP_VISIT_ALLOCS
	void __Visit(hx::VisitContext *__inCtx) { mFinalizer->Visit(__inCtx); }
   #endif

   hx::Class __GetClass() const { return __ObjcClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< ObjcData *>(inClass); }

   // k_cpp_objc
   int __GetType() const { return vtAbstractBase + 4; }
   #ifdef OBJC_ARC
   void * __GetHandle() const { return (__bridge void *) mValue; }
   #else
   void * __GetHandle() const { return (void *) mValue; }
   #endif
   String toString()
   {
      return String(!mValue ? "null" : [[mValue description] UTF8String]);
   }
   String __ToString() const { return String(!mValue ? "null" : [[mValue description] UTF8String]); }

   int __Compare(const hx::Object *inRHS) const
   {
      if (!inRHS)
        return mValue == 0 ? 0 : 1;
      const ObjcData *data = dynamic_cast< const ObjcData *>(inRHS);
      if (data) 
      {
         return [data->mValue isEqual:mValue] ? 0 : mValue < data->mValue ? -1 : 1;
      }
      else
      {
        void *r = inRHS->__GetHandle();

        #ifdef OBJC_ARC
        void * ptr = (__bridge void *) mValue;
        #else
        void * ptr = (void *) mValue;
        #endif
 
        return ptr < r ? -1 : ptr==r ? 0 : 1;
      }
   }

   #ifdef OBJC_ARC
   id mValue;
   #else
   const id mValue;
   #endif
   hx::InternalFinalizer *mFinalizer;
};
}

Dynamic::Dynamic(const id inVal)
{
   mPtr = inVal ? (hx::Object *)new ObjcData(inVal) : 0;
}
