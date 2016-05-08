#ifndef HX_OBJECT_H
#define HX_OBJECT_H

#ifndef HXCPP_H
#error "Please include hxcpp.h, not hx/Object.h"
#endif

#ifdef HXCPP_TELEMETRY
extern void __hxt_gc_new(void* obj, int inSize, const char *inName);
#endif


// --- Constants -------------------------------------------------------

// These values are returned from the "__GetType" function
enum hxObjectType
{
   vtUnknown = -1,
   vtInt = 0xff,
   vtNull = 0,
   vtFloat = 1,
   vtBool = 2,
   vtString = 3,
   vtObject = 4,
   vtArray = 5,
   vtFunction = 6,
   vtEnum,
   vtClass,
   vtInt64,
   vtAbstractBase = 0x100,
};


namespace hx
{



class FieldRef;
class IndexRef;
typedef Array<Dynamic> DynamicArray;
HXCPP_EXTERN_CLASS_ATTRIBUTES null BadCast();

#ifdef HXCPP_SCRIPTABLE
typedef void (*StackExecute)(struct CppiaCtx *ctx);
struct ScriptFunction
{
   ScriptFunction(StackExecute inExe=0,const char *inSig=0)
      : execute(inExe), signature(inSig) { }
   StackExecute execute;
   const char   *signature;
};
struct ScriptCallable;

#endif

enum NewObjectType
{
   NewObjAlloc,
   NewObjContainer,
   NewObjConst,
};


// --- hx::Object ------------------------------------------------------------
//
// Base for all hxcpp objects.
// This contains the virtual functions required by the core to provide
//  a generic interface to the specific classes.
//
// Hxcpp classes inherit from this.
//
class HXCPP_EXTERN_CLASS_ATTRIBUTES Object
{
public:
   // These allocate the function using the garbage-colleced malloc
   inline void *operator new( size_t inSize, bool inContainer=true, const char *inName=0 )
   {
      #ifdef HX_USE_INLINE_IMMIX_OPERATOR_NEW
         ImmixAllocator *alloc =  hx::gMultiThreadMode ? tlsImmixAllocator : gMainThreadAlloc;

         #ifdef HXCPP_DEBUG
         if (!alloc)
            BadImmixAlloc();
         #endif

         #ifndef HXCPP_ALIGN_ALLOC
            // Inline the fast-path if we can
            // We know the object can hold a pointer (vtable) and that the size is int-aligned

            int start = alloc->spaceStart;
            int end = start + sizeof(int) + inSize;

            if ( end <= (alloc->spaceEnd WITH_PAUSE_FOR_COLLECT_FLAG ) )
            {
               alloc->spaceStart = end;

               int startRow = start>>IMMIX_LINE_BITS;

               alloc->allocStartFlags[ startRow ] |= gImmixStartFlag[start&127];
               //alloc->allocBase[ startRow ] |= (1<<( (start>>2) & 31) );

               unsigned int *buffer = (unsigned int *)(alloc->allocBase + start);

               if (inContainer)
                  *buffer++ =  (( (end+(IMMIX_LINE_LEN-1))>>IMMIX_LINE_BITS) -startRow) |
                               (inSize<<IMMIX_ALLOC_SIZE_SHIFT) |
                               gMarkIDWithContainer;
               else
                  *buffer++ =  (( (end+(IMMIX_LINE_LEN-1))>>IMMIX_LINE_BITS) -startRow) |
                               (inSize<<IMMIX_ALLOC_SIZE_SHIFT) |
                               gMarkID;

               #ifdef HXCPP_TELEMETRY
               __hxt_gc_new(buffer, inSize, inName);
               #endif
               return buffer;
            }
         #endif // HXCPP_ALIGN_ALLOC

         // Fall back to external method
         void *result = alloc->CallAlloc(inSize, inContainer ? IMMIX_ALLOC_IS_CONTAINER : 0);

      #else // Not HX_USE_INLINE_IMMIX_OPERATOR_NEW ...

         void *result = hx::InternalNew(inSize,inContainer);

      #endif


      #ifdef HXCPP_TELEMETRY
         __hxt_gc_new(result, inSize, inName);
      #endif
      return result;
   }

   inline void *operator new( size_t inSize, hx::NewObjectType inType,  const char *inName=0 )
   {
      if (inType==NewObjConst)
         return InternalCreateConstBuffer(0,(int)inSize);
      return operator new(inSize, inType==NewObjContainer, inName);
   }

   void operator delete( void *, bool) { }
   void operator delete( void *, bool, const char * ) { }
   void operator delete( void *, int ) { }
   void operator delete( void *, hx::NewObjectType) { }
   void operator delete( void *, hx::NewObjectType, const char * ) { }

   //virtual void *__root();
   virtual void __Mark(hx::MarkContext *__inCtx) { }
   #ifdef HXCPP_VISIT_ALLOCS
   virtual void __Visit(hx::VisitContext *__inCtx) { }
   #endif
   virtual bool __Is(hx::Object *inClass) const { return true; }
   virtual hx::Object *__GetRealObject() { return this; }

   // helpers...
   bool __Is(Dynamic inClass ) const;
   inline bool __IsArray() const { return __GetType()==vtArray; }

   virtual int __GetType() const { return vtClass; }
   virtual void *__GetHandle() const { return 0; }


   virtual hx::FieldRef __FieldRef(const String &inString);

   virtual String __ToString() const;

   virtual int __ToInt() const { return 0; }
   virtual double __ToDouble() const { return __ToInt(); }
   virtual cpp::Int64 __ToInt64() const { return (cpp::Int64)(__ToDouble()); }
   virtual const char * __CStr() const;
   virtual String toString();
   virtual bool __HasField(const String &inString);
   virtual hx::Val __Field(const String &inString, hx::PropertyAccess inCallProp);

   #if (HXCPP_API_LEVEL >= 330)
   // Non-virtual
   Dynamic __IField(int inFieldID);
   double __INumField(int inFieldID);

   virtual void *_hx_getInterface(int inId);
   #else
   virtual hx::Object *__ToInterface(const hx::type_info &inInterface) { return 0; }
   virtual Dynamic __IField(int inFieldID);
   virtual double __INumField(int inFieldID);

   // These have been moved to EnumBase
   virtual DynamicArray __EnumParams();
   virtual String __Tag() const;
   virtual int __Index() const;
   virtual void __SetSize(int inLen) { }

   #endif
   virtual hx::Val __SetField(const String &inField,const hx::Val &inValue, hx::PropertyAccess inCallProp);

   virtual void  __SetThis(Dynamic inThis);
   virtual Dynamic __Run(const Array<Dynamic> &inArgs);
   virtual Dynamic *__GetFieldMap();
   virtual void __GetFields(Array<String> &outFields);
   virtual hx::Class __GetClass() const;

   virtual int __Compare(const hx::Object *inRHS) const;

   virtual int __length() const { return 0; }
   virtual Dynamic __GetItem(int inIndex) const;
   virtual Dynamic __SetItem(int inIndex,Dynamic inValue);


   typedef const Dynamic &D;
   virtual Dynamic __run();
   virtual Dynamic __run(D a);
   virtual Dynamic __run(D a,D b);
   virtual Dynamic __run(D a,D b,D c);
   virtual Dynamic __run(D a,D b,D c,D d);
   virtual Dynamic __run(D a,D b,D c,D d,D e);

   virtual int __ArgCount() const { return -1; }

   #ifdef HXCPP_SCRIPTABLE
   virtual void **__GetScriptVTable() { return 0; }
   virtual hx::ScriptCallable *__GetScriptCallable() { return 0; }
   static hx::ScriptFunction __script_construct;
   #endif

   inline bool __compare( hx::Object *inRHS )
      { return __GetRealObject()!=inRHS->__GetRealObject(); }

   static hx::Class &__SGetClass();
   static void __boot();
};

// --- hx::ObjectPtr ---------------------------------------------------------------
//
// This class simply provides syntax so that pointers can be written as objects,
//  and overloaded operators can be used

template<typename OBJ_>
class ObjectPtr
{
protected:
   inline bool SetPtr(OBJ_ *inPtr)
   {
      mPtr = inPtr;
      return true;
   }
   inline bool SetPtr(...) { return false; }

   inline void CastPtr(hx::Object *inPtr,bool inThrowOnInvalid)
   {
      if (inPtr)
      {
         mPtr = dynamic_cast<OBJ_ *>(inPtr->__GetRealObject());
         #if (HXCPP_API_LEVEL < 330)
         if (!mPtr)
            mPtr = (Ptr)inPtr->__ToInterface(typeid(Obj));
         #endif
         if (inThrowOnInvalid && !mPtr)
            ::hx::BadCast();
      }
      else
         mPtr = 0;
   }

public:
   typedef OBJ_ Obj;
   typedef OBJ_ *Ptr;

   inline ObjectPtr() : mPtr(0) { }
   inline ObjectPtr(OBJ_ *inObj) : mPtr(inObj) { }
   inline ObjectPtr(const null &inNull) : mPtr(0) { }
   inline ObjectPtr(const ObjectPtr<OBJ_> &inOther) : mPtr( inOther.mPtr ) {  }
   template<typename T>
   inline ObjectPtr(const hx::Native<T> &inNative) : mPtr( dynamic_cast<T>(inNative.ptr) ) {  }

   template<typename SOURCE_>
   inline ObjectPtr(const ObjectPtr<SOURCE_> &inObjectPtr)
   {
      if (!SetPtr(inObjectPtr.mPtr))
         CastPtr(inObjectPtr.mPtr,false);
   }


   inline ObjectPtr(const ::cpp::Variant &inVariant)
   {
      hx::Object *object = inVariant.asObject();
      if (!SetPtr(object))
         CastPtr(object,false);
   }

   template<typename SOURCE_>
   inline ObjectPtr(const SOURCE_ *inPtr,bool inCheckCast=true)
   {
      if (!SetPtr(const_cast<SOURCE_ *>(inPtr)))
         CastPtr(const_cast<SOURCE_ *>(inPtr),inCheckCast);
   }

   inline ObjectPtr &operator=(const null &inNull) { mPtr = 0; return *this; }
   inline ObjectPtr &operator=(Ptr inRHS) { mPtr = inRHS; return *this; }
   inline ObjectPtr &operator=(const ObjectPtr &inRHS) { mPtr = inRHS.mPtr; return *this; }
   template<typename InterfaceImpl>
   inline ObjectPtr &operator=(InterfaceImpl *inRHS)
   {
      mPtr = inRHS->operator Ptr();
      return *this;
   }

   inline OBJ_ *GetPtr() const { return mPtr; }
   inline OBJ_ *operator->()
   {
      #ifdef HXCPP_CHECK_POINTER
      if (!mPtr) NullReference("Object", true);
      // The handler might have fixed up the null value
      if (!mPtr) NullReference("Object", false);
      #ifdef HXCPP_GC_CHECK_POINTER
         GCCheckPointer(mPtr);
      #endif
      #endif
      return mPtr;
   }
   inline const OBJ_ *operator->() const
   {
      #ifdef HXCPP_CHECK_POINTER
      if (!mPtr) NullReference("Object", true);
      // The handler might have fixed up the null value
      if (!mPtr) NullReference("Object", false);
      #ifdef HXCPP_GC_CHECK_POINTER
         GCCheckPointer(mPtr);
      #endif
      #endif
      return mPtr;
   }

   template<typename T>
   inline bool operator==(const T &inTRHS) const
   {
      ObjectPtr inRHS(inTRHS.mPtr,false);
      if (mPtr==inRHS.mPtr) return true;
      if (!mPtr || !inRHS.mPtr) return false;
      return !mPtr->__compare(inRHS.mPtr);
   }
   inline bool operator==(const cpp::Variant &inRHS) const;
   inline bool operator!=(const cpp::Variant &inRHS) const;

   template<typename T>
   inline bool operator!=(const T &inTRHS) const
   {
      ObjectPtr inRHS(inTRHS.mPtr,false);
      if (mPtr==inRHS.mPtr) return false;
      if (!mPtr || !inRHS.mPtr) return true;
      return mPtr->__compare(inRHS.mPtr);
   }

   template<typename T>
   operator hx::Native<T> () { return hx::Native<T>( mPtr ); }

   inline bool operator==(const null &inRHS) const { return mPtr==0; }
   inline bool operator!=(const null &inRHS) const { return mPtr!=0; }

   //inline bool operator==(const Dynamic &inRHS) const { return inRHS==*this; }
   //inline bool operator!=(const Dynamic &inRHS) const { return inRHS!=*this; }


   // This is defined in the "FieldRef" class...
   inline class hx::FieldRef FieldRef(const String &inString);
   inline class hx::IndexRef IndexRef(int inString);
   inline static hx::Class &__SGetClass() { return OBJ_::__SGetClass(); }

   OBJ_ *mPtr;
};


} // end namespace hx



#endif
