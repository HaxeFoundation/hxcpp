#ifndef HX_NATIVE_INCLUDED_H
#define HX_NATIVE_INCLUDED_H
/*
 *
 This file is in the public domain, and can be freely distributed.

*/

#ifndef HXCPP_H
#define HXCPP_H
typedef double Float;
typedef void Void;

//typedef int Int;
//typedef bool Bool;

#ifndef HXCPP_CLASS_ATTRIBUTES
#define HXCPP_CLASS_ATTRIBUTES
#endif

#endif



#define HX_DECLARE_NATIVE0(klass) \
	class klass;
#define HX_DECLARE_NATIVE1(ns1,klass) namespace ns1 { HX_DECLARE_NATIVE0(klass) }
#define HX_DECLARE_NATIVE2(ns2,ns1,klass) namespace ns2 { HX_DECLARE_NATIVE1(ns1,klass) }
#define HX_DECLARE_NATIVE3(ns3,ns2,ns1,klass) namespace ns3 { HX_DECLARE_NATIVE2(ns2,ns1,klass) }
#define HX_DECLARE_NATIVE4(ns4,ns3,ns2,ns1,klass) namespace ns4 { HX_DECLARE_NATIVE3(ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE5(ns5,ns4,ns3,ns2,ns1,klass) namespace ns5 { HX_DECLARE_NATIVE4(ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE6(ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns6 { HX_DECLARE_NATIVE5(ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns7 { HX_DECLARE_NATIVE6(ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns8 { HX_DECLARE_NATIVE7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE9(ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns9 { HX_DECLARE_NATIVE8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }





namespace hx
{
   class Object;

   class HXCPP_CLASS_ATTRIBUTES NativeInterface
   {
      public:
         virtual hx::Object *__GetRealObject() = 0;
         void _hx_addRef();
         void _hx_decRef();
   };

   template<typename T>
   class Native
   {
      public:
         T ptr;

         Native () : ptr(0) { }
         Native (T inPtr) : ptr(inPtr) { }
         Native (const Native<T> &inNative) : ptr(inNative.ptr) { }
         #ifdef CPP_VARIANT_ONCE_H
         Native (const cpp::Variant &inVariant) {
            hx::Object *obj = inVariant.asObject();
            ptr = obj  ? (T)inVariant.valObject->__GetHandle() : 0;
         }
         #endif

         inline Native &operator=(T inPtr) { ptr=inPtr; return *this; }
         inline Native &operator=(const Native<T> &inNative) { ptr=inNative.ptr; return *this; }
         #ifdef HX_NULL_H
         inline Native &operator=(const ::null &) { ptr=0; return *this; }
         #endif
         inline T operator->() const { return ptr; }

         inline operator T() const { return ptr; }

         template<typename O>
         inline bool operator==(const Native<O> &inOther) const
            { return ptr == inOther.ptr; }
         template<typename O>
         inline bool operator!=(const Native<O> &inOther) const
            { return ptr != inOther.ptr; }

   };

   HXCPP_CLASS_ATTRIBUTES const char *Init();
   HXCPP_CLASS_ATTRIBUTES void PushTopOfStack(void *);
   HXCPP_CLASS_ATTRIBUTES void PopTopOfStack();
   HXCPP_CLASS_ATTRIBUTES void GcAddOffsetRoot(void *inRoot, int inOffset);
   HXCPP_CLASS_ATTRIBUTES void GcSetOffsetRoot(void *inRoot, int inOffset);
   HXCPP_CLASS_ATTRIBUTES void GcRemoveOffsetRoot(void *inRoot);
   HXCPP_CLASS_ATTRIBUTES int  GcGetThreadAttachedCount();

   class HXCPP_CLASS_ATTRIBUTES NativeAttach
   {
      bool isAttached;
      public:
         NativeAttach(bool inAttach=true)
         {
            isAttached = false;
            if (inAttach)
               attach();
         }
         ~NativeAttach()
         {
            detach();
         }
         void attach()
         {
            if (!isAttached)
            {
               isAttached = true;
               hx::PushTopOfStack(this);
            }
         }
         void detach()
         {
            if (isAttached)
            {
               isAttached = false;
               hx::PopTopOfStack();
            }
         }
   };

   template<typename T>
   class Ref
   {
   public:
      T ptr;

      Ref() : ptr(0) { }
      Ref(const T &inT) : ptr(0) { setPtr(inT); }
      template<typename O>
      inline Ref(const Native<O> &inNative) : ptr(0) { setPtr(inNative.ptr); }
      template<typename O>
      inline Ref(const Ref<O> &inRef) : ptr(0) { setPtr(inRef.ptr); }

      ~Ref() { setPtr(0); }
      void setPtr(T inPtr)
      {
         hx::Object *old = ptr ? ptr->__GetRealObject() : 0;
         int oldOffset = old ? (int)(size_t)((char *)inPtr - (char *)old) : 0;
         hx::Object *next = inPtr ? inPtr->__GetRealObject() : 0;
         int nextOffset = next ? (int)(size_t)((char *)inPtr - (char *)next) : 0;

         ptr = inPtr;
         if (next)
         {
            if (!old)
               GcAddOffsetRoot(&ptr, nextOffset);
            else if (oldOffset!=nextOffset)
               GcSetOffsetRoot(&ptr, nextOffset);
         }
         else if (old)
            GcRemoveOffsetRoot(&ptr);
      }

      inline Ref &operator=(const T &inPtr) { setPtr(inPtr); return *this; }
      template<typename O>
      inline Ref &operator=(const Native<O> &inNative) { setPtr(inNative.ptr); return *this; }
      template<typename O>
      inline Ref &operator=(const Ref<O> &inRef) { setPtr(inRef.ptr); return *this; }

      template<typename O>
      inline bool operator==(const Ref<O> &inOther) const
            { return ptr == inOther.ptr; }
      template<typename O>
      inline bool operator!=(const Ref<O> &inOther) const
            { return ptr != inOther.ptr; }

      T operator->() { return ptr; }
   };

   #define HX_NATIVE_IMPLEMENTATION hx::Object *__GetRealObject() { return this; }
   #define HX_EXTERN_NATIVE_IMPLEMENTATION hx::Object *__GetRealObject() { return 0; }
}

#endif

