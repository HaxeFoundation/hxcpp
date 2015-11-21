#ifndef HX_THREAD_H
#define HX_THREAD_H

#ifdef HX_WINRT

#include <windows.h>
#include <process.h>
#include <mutex>

#elif defined(_WIN32)

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <process.h>
#else
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#endif

#ifdef RegisterClass
#undef RegisterClass
#endif

#if defined(ANDROID)

#if (HXCPP_ANDROID_PLATFORM>=21)
// Nice one, google, no one was using that.
#define __ATOMIC_INLINE__ static __inline__ __attribute__((always_inline))
// returns 0=exchange took place, 1=not
__ATOMIC_INLINE__ int __atomic_cmpxchg(int old, int _new, volatile int *ptr)
   { return __sync_val_compare_and_swap(ptr, old, _new) != old; }
__ATOMIC_INLINE__ int __atomic_dec(volatile int *ptr) { return __sync_fetch_and_sub (ptr, 1); }
__ATOMIC_INLINE__ int __atomic_inc(volatile int *ptr) { return __sync_fetch_and_add (ptr, 1); }
#else
#include <sys/atomics.h>
#endif

// returns 1 if exchange took place
inline bool HxAtomicExchangeIf(int inTest, int inNewVal,volatile int *ioWhere)
   { return !__atomic_cmpxchg(inTest, inNewVal, ioWhere); }
// Returns old value naturally
inline int HxAtomicInc(volatile int *ioWhere)
   { return __atomic_inc(ioWhere); }
inline int HxAtomicDec(volatile int *ioWhere)
   { return __atomic_dec(ioWhere); }

#elif defined(HX_WINDOWS) && !defined(HX_WINRT) //winrt test

inline bool HxAtomicExchangeIf(int inTest, int inNewVal,volatile int *ioWhere)
   { return InterlockedCompareExchange((volatile LONG *)ioWhere, inNewVal, inTest)==inTest; }
// Make it return old value
inline int HxAtomicInc(volatile int *ioWhere)
   { return InterlockedIncrement((volatile LONG *)ioWhere)-1; }
inline int HxAtomicDec(volatile int *ioWhere)
   { return InterlockedDecrement((volatile LONG *)ioWhere)+1; }

#define HX_HAS_ATOMIC 1

#elif defined(HX_MACOS) || defined(IPHONE) || defined(APPLETV)
#include <libkern/OSAtomic.h>

#define HX_HAS_ATOMIC 1

inline bool HxAtomicExchangeIf(int inTest, int inNewVal,volatile int *ioWhere)
   { return OSAtomicCompareAndSwap32Barrier(inTest, inNewVal, ioWhere); }
inline int HxAtomicInc(volatile int *ioWhere)
   { return OSAtomicIncrement32Barrier(ioWhere)-1; }
inline int HxAtomicDec(volatile int *ioWhere)
   { return OSAtomicDecrement32Barrier(ioWhere)-1; }


#elif defined(HX_LINUX)

#define HX_HAS_ATOMIC 1

inline bool HxAtomicExchangeIf(int inTest, int inNewVal,volatile int *ioWhere)
   { return __sync_bool_compare_and_swap(ioWhere, inTest, inNewVal); }
// Returns old value naturally
inline int HxAtomicInc(volatile int *ioWhere)
   { return __sync_fetch_and_add(ioWhere,1); }
inline int HxAtomicDec(volatile int *ioWhere)
   { return __sync_fetch_and_sub(ioWhere,1); }

#else

#define HX_HAS_ATOMIC 0

inline int HxAtomicExchangeIf(int inTest, int inNewVal,volatile int *ioWhere)
{
   if (*ioWhere == inTest)
   {
      *ioWhere = inNewVal;
      return true;
   }
   return false;
}
inline int HxAtomicInc(volatile int *ioWhere)
   { return (*ioWhere)++; }
inline int HxAtomicDec(volatile int *ioWhere)
   { return (*ioWhere)--; }


#endif



#if defined(HX_WINDOWS)


struct MyMutex
{
   MyMutex()
   {
      mValid = true;
      #ifdef HX_WINRT
      InitializeCriticalSectionEx(&mCritSec,4000,0);
      #else
      InitializeCriticalSection(&mCritSec);
      #endif
   }
   ~MyMutex() { if (mValid) DeleteCriticalSection(&mCritSec); }
   void Lock() { EnterCriticalSection(&mCritSec); }
   void Unlock() { LeaveCriticalSection(&mCritSec); }
   bool TryLock() { return TryEnterCriticalSection(&mCritSec); }
   void Clean()
   {
      if (mValid)
      {
         DeleteCriticalSection(&mCritSec);
         mValid = false;
      }
   }

   bool             mValid;
   CRITICAL_SECTION mCritSec;
};


#define THREAD_FUNC_TYPE unsigned int WINAPI
#define THREAD_FUNC_RET return 0;

#else

struct MyMutex
{
   MyMutex()
   {
      pthread_mutexattr_t mta;
      pthread_mutexattr_init(&mta);
      pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(&mMutex,&mta);
      mValid = true;
   }
   ~MyMutex() { if (mValid) pthread_mutex_destroy(&mMutex); }
   void Lock() { pthread_mutex_lock(&mMutex); }
   void Unlock() { pthread_mutex_unlock(&mMutex); }
   bool TryLock() { return !pthread_mutex_trylock(&mMutex); }
   void Clean()
   {
      pthread_mutex_destroy(&mMutex);
      mValid = 0;
   }

   bool mValid;
   pthread_mutex_t mMutex;
};

#define THREAD_FUNC_TYPE void *
#define THREAD_FUNC_RET return 0;

#endif




template<typename LOCKABLE>
struct TAutoLock
{
   TAutoLock(LOCKABLE &inMutex) : mMutex(inMutex) { mMutex.Lock(); }
   ~TAutoLock() { mMutex.Unlock(); }
   void Lock() { mMutex.Lock(); }
   void Unlock() { mMutex.Unlock(); }

   LOCKABLE &mMutex;
};

typedef TAutoLock<MyMutex> AutoLock;


#if defined(HX_WINDOWS)

struct MySemaphore
{
   MySemaphore()
   {
      #ifdef HX_WINRT
      mSemaphore = CreateEventEx(nullptr,nullptr,0,EVENT_ALL_ACCESS);
      #else
      mSemaphore = CreateEvent(0,0,0,0);
      #endif
   }
   ~MySemaphore() { if (mSemaphore) CloseHandle(mSemaphore); }
   void Set() { SetEvent(mSemaphore); }
   void Wait()
   {
      #ifdef HX_WINRT
      WaitForSingleObjectEx(mSemaphore,INFINITE,false);
      #else
      WaitForSingleObject(mSemaphore,INFINITE);
      #endif
   }
    // Returns true on success, false on timeout
   bool WaitSeconds(double inSeconds)
   {
      #ifdef HX_WINRT
      return WaitForSingleObjectEx(mSemaphore,inSeconds*1000.0,false) != WAIT_TIMEOUT;
      #else
      return WaitForSingleObject(mSemaphore,inSeconds*1000.0) != WAIT_TIMEOUT;
      #endif
   }
   void Reset() { ResetEvent(mSemaphore); }
   void Clean() { if (mSemaphore) CloseHandle(mSemaphore); mSemaphore = 0; }

   HANDLE mSemaphore;
};

#else


struct MySemaphore
{
   MySemaphore()
   {
      mSet = false;
      mValid = true;
      pthread_cond_init(&mCondition,0);
   }
   ~MySemaphore()
   {
      if (mValid)
      {
         pthread_cond_destroy(&mCondition);
      }
   }
   // For autolock
   inline operator MyMutex &() { return mMutex; }
   void Set()
   {
      AutoLock lock(mMutex);
      if (!mSet)
      {
         mSet = true;
         pthread_cond_signal( &mCondition );
      }
   }
   void QSet()
   {
      mSet = true;
      pthread_cond_signal( &mCondition );
   }
   void Reset()
   {
      AutoLock lock(mMutex);
      mSet = false;
   }
   void QReset() { mSet = false; }
   void Wait()
   {
      AutoLock lock(mMutex);
      while( !mSet )
         pthread_cond_wait( &mCondition, &mMutex.mMutex );
      mSet = false;
   }
   // when we already hold the mMutex lock ...
   void QWait()
   {
      while( !mSet )
         pthread_cond_wait( &mCondition, &mMutex.mMutex );
      mSet = false;
   }
   // Returns true if the wait was success, false on timeout.
   bool WaitSeconds(double inSeconds)
   {
      struct timeval tv;
      gettimeofday(&tv, 0);

      int isec = (int)inSeconds;
      int usec = (int)((inSeconds-isec)*1000000.0);
      timespec spec;
      spec.tv_nsec = (tv.tv_usec + usec) * 1000;
      if (spec.tv_nsec>1000000000)
      {
         spec.tv_nsec-=1000000000;
         isec++;
      }
      spec.tv_sec = tv.tv_sec + isec;

      AutoLock lock(mMutex);

      int result = 0;
      // Wait for set to be true...
      while( !mSet &&  (result=pthread_cond_timedwait( &mCondition, &mMutex.mMutex, &spec )) != ETIMEDOUT)
      {
         if (result!=0)
         {
            // Error - something's gone wrong...
            /*
            if (result==EINVAL) 
               printf("ERROR: Condition EINVAL\n");
            else if (result==EPERM)
               printf("ERROR: Condition EPERM\n");
            else
               printf("ERROR: Condition unknown error\n");
            */
            break;
         }
         // Condition signalled - but try mSet again ...
      }

      bool wasSet = mSet;
      mSet = false;
      return wasSet;
   }
   void Clean()
   {
      mMutex.Clean();
      if (mValid)
      {
         mValid = false;
         pthread_cond_destroy(&mCondition);
      }
   }


   MyMutex         mMutex;
   pthread_cond_t  mCondition;
   bool            mSet;
   bool            mValid;
};


#endif



#endif
