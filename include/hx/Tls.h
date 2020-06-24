#ifdef HX_TLS_H_OVERRIDE
// Users can define their own header to use here, but there is no API
// compatibility gaurantee for future changes.
#include HX_TLS_H_OVERRIDE
#else

#ifndef HX_TLS_INCLUDED
#define HX_TLS_INCLUDED

#ifdef HX_WINDOWS

  #ifdef HX_WINRT
    // Nothing
  #else

   #ifndef __GNUC__
   #include <intrin.h>
   #endif

   extern "C"
   {
      __declspec(dllimport)
     int __stdcall TlsSetValue(unsigned long  dwTlsIndex, void *lpTlsValue);

     __declspec(dllimport)
     void * __stdcall TlsGetValue(unsigned long  dwTlsIndex);

     __declspec(dllimport)
     unsigned long __stdcall TlsAlloc(void);
   }


   namespace hx {

   template<typename DATA,bool FAST=false>
   struct TLSData
   {
      static const size_t kMaxInlineSlots = 64;

      TLSData()
      {
         mSlot = TlsAlloc();
         TlsSetValue(mSlot,0);
         #ifdef HXCPP_M64
         mFastOffset = mSlot*sizeof(void *) + 0x1480;
         #else
         if (FAST || mSlot < kMaxInlineSlots)
            mFastOffset = mSlot*sizeof(void *) + 0xE10;
         else
            mFastOffset = mSlot - kMaxInlineSlots;
         #endif
      }
      inline DATA *operator=(DATA *inData)
      {
         TlsSetValue(mSlot,inData);
         return inData;
      }

      inline operator DATA *()
      {
         #if !defined(HXCPP_M64) && (_MSC_VER >= 1400)
         const size_t kTibExtraTlsOffset = 0xF94;

         if (FAST || mSlot < kMaxInlineSlots)
           return (DATA *)__readfsdword(mFastOffset);

         DATA **extra = (DATA **)(__readfsdword(kTibExtraTlsOffset));
         return extra[mFastOffset];
         #elif (_MSC_VER >= 1400) & !defined(HXCPP_DEBUG) && !defined(HXCPP_ARM64)// 64 bit version...
         if (mSlot < 64)
           return (DATA *)__readgsqword(mFastOffset);
         else
           return (DATA *)TlsGetValue(mSlot);
         #else
         return (DATA *)TlsGetValue(mSlot);
         #endif
      }

      int mSlot;
      int mFastOffset;
   };

   } // end namespace hx


   #define DECLARE_TLS_DATA(TYPE,NAME) \
      hx::TLSData<TYPE> NAME;
   #define DECLARE_FAST_TLS_DATA(TYPE,NAME) \
      hx::TLSData<TYPE,true> NAME;
   #define EXTERN_TLS_DATA(TYPE,NAME) \
      extern hx::TLSData<TYPE> NAME;
   #define EXTERN_FAST_TLS_DATA(TYPE,NAME) \
      extern hx::TLSData<TYPE,true> NAME;


  #endif
#else // not HX_WINDOWS

#include <pthread.h>

namespace hx
{

template<typename DATA,bool FAST=false>
struct TLSData
{
   TLSData()
   {
      pthread_key_create(&mSlot, 0);
   }
   DATA *Get()
   {
      return (DATA *)pthread_getspecific(mSlot);
   }
   void Set(DATA *inData)
   {
      pthread_setspecific(mSlot,inData);
   }
   inline DATA *operator=(DATA *inData)
   {
      pthread_setspecific(mSlot,inData);
      return inData;
   }
   inline operator DATA *() { return (DATA *)pthread_getspecific(mSlot); }

   pthread_key_t mSlot;
};

} // end namespace hx


#endif



#ifdef HX_WINRT

#define DECLARE_TLS_DATA(TYPE,NAME) \
   __declspec(thread) TYPE * NAME = nullptr;
#define DECLARE_FAST_TLS_DATA(TYPE,NAME) \
   __declspec(thread) TYPE * NAME = nullptr;
#define EXTERN_TLS_DATA(TYPE,NAME) \
   __declspec(thread) extern TYPE * NAME;
#define EXTERN_FAST_TLS_DATA(TYPE,NAME) \
   __declspec(thread) extern TYPE * NAME;

#else

#define DECLARE_TLS_DATA(TYPE,NAME) \
   hx::TLSData<TYPE> NAME;
#define DECLARE_FAST_TLS_DATA(TYPE,NAME) \
   hx::TLSData<TYPE,true> NAME;
#define EXTERN_TLS_DATA(TYPE,NAME) \
   extern hx::TLSData<TYPE> NAME;
#define EXTERN_FAST_TLS_DATA(TYPE,NAME) \
   extern hx::TLSData<TYPE,true> NAME;

#endif




#endif

#endif
