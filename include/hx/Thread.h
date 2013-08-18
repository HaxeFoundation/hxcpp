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
#endif

#ifdef RegisterClass
#undef RegisterClass
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

#ifndef HX_WINRT
template<typename DATA>
struct TLSData
{
   TLSData()
   {
      mSlot = TlsAlloc();
      TlsSetValue(mSlot,0);
   }
   inline DATA *operator=(DATA *inData)
   {
      TlsSetValue(mSlot,inData);
      return inData;
   }
   inline operator DATA *() { return (DATA *)TlsGetValue(mSlot); }

   int mSlot;
};

#endif


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



#ifdef HX_WINRT

#define DECLARE_TLS_DATA(TYPE,NAME) \
   __declspec(thread) TYPE * NAME = nullptr;

#else

#define DECLARE_TLS_DATA(TYPE,NAME) \
   TLSData<TYPE> NAME;

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
      return WaitForSingleObjectEx(mSemaphore,inSeconds*0.001,false) != WAIT_TIMEOUT;
      #else
      return WaitForSingleObject(mSemaphore,inSeconds*0.001) != WAIT_TIMEOUT;
      #endif
   }
   void Reset() { ResetEvent(mSemaphore); }
   void Clean() { if (mSemaphore) CloseHandle(mSemaphore); mSemaphore = 0; }

   HANDLE mSemaphore;
};

#else


template<typename DATA>
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
      if (mSet) {
          mSet = false;
          return true;
      }

      while (pthread_cond_timedwait
           ( &mCondition, &mMutex.mMutex, &spec ) != ETIMEDOUT) {
          if (mSet) {
              mSet = false;
              return true;
          }
      }

      if (mSet) {
          mSet = false;
          return true;
      }
      else {
          return false;
      }
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
