#ifdef HX_THREAD_H_OVERRIDE
// Users can define their own header to use here, but there is no API
// compatibility gaurantee for future changes.
#include HX_THREAD_H_OVERRIDE
#else

#ifndef HX_THREAD_H
#define HX_THREAD_H

#ifndef HXCPP_HEADER_VERSION
#include "hx/HeaderVersion.h"
#endif

#ifdef HX_WINRT

#include <windows.h>
#include <process.h>
#include <mutex>

#elif defined(_WIN32)

#ifdef HXCPP_WINXP_COMPAT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#else
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <process.h>
#else
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#define HXCPP_PTHREADS
#endif

#ifdef RegisterClass
#undef RegisterClass
#endif

#if defined(HX_WINDOWS)


struct HxMutex
{
   HxMutex()
   {
      mValid = true;
      #ifdef HX_WINRT
      InitializeCriticalSectionEx(&mCritSec,4000,0);
      #else
      InitializeCriticalSection(&mCritSec);
      #endif
   }
   ~HxMutex() { if (mValid) DeleteCriticalSection(&mCritSec); }
   void Lock() { EnterCriticalSection(&mCritSec); }
   void Unlock() { LeaveCriticalSection(&mCritSec); }
   bool TryLock() { return TryEnterCriticalSection(&mCritSec); }
   bool IsValid() { return mValid; }
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


#define THREAD_FUNC_TYPE DWORD WINAPI
#define THREAD_FUNC_RET return 0;

inline bool HxCreateDetachedThread(DWORD (WINAPI *func)(void *), void *param)
{
	return (CreateThread(NULL, 0, func, param, 0, 0) != 0);
}

#else

struct HxMutex
{
   bool mValid;
   pthread_mutex_t *mMutex;

   HxMutex()
   {
      pthread_mutexattr_t mta;
      pthread_mutexattr_init(&mta);
      pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
      mMutex = new pthread_mutex_t();
      mValid = pthread_mutex_init(mMutex,&mta) ==0;
   }
   ~HxMutex()
   {
      Clean();
   }
   void Lock() { pthread_mutex_lock(mMutex); }
   void Unlock() { pthread_mutex_unlock(mMutex); }
   bool TryLock() { return !pthread_mutex_trylock(mMutex); }
   bool IsValid() { return mValid; }
   void Clean()
   {
      if (mValid)
      {
         pthread_mutex_destroy(mMutex);
         mValid = false;
      }
      if (mMutex)
      {
         delete mMutex;
         mMutex = nullptr;
      }
   }
};

#define THREAD_FUNC_TYPE void *
#define THREAD_FUNC_RET return 0;

inline bool HxCreateDetachedThread(void *(*func)(void *), void *param)
{
	pthread_t t;
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0)
		return false;
#ifdef PTHREAD_CREATE_DETACHED
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
		return false;
#endif
	if (pthread_create(&t, &attr, func, param) != 0 )
		return false;
	if (pthread_attr_destroy(&attr) != 0)
		return false;
	return true;
}

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

typedef TAutoLock<HxMutex> AutoLock;


#if defined(HX_WINDOWS)

struct HxSemaphore
{
   HxSemaphore()
   {
      #ifdef HX_WINRT
      mSemaphore = CreateEventEx(nullptr,nullptr,0,EVENT_ALL_ACCESS);
      #else
      mSemaphore = CreateEvent(0,0,0,0);
      #endif
   }
   ~HxSemaphore() { if (mSemaphore) CloseHandle(mSemaphore); }
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


#define HX_THREAD_SEMAPHORE_LOCKABLE

struct HxSemaphore
{
   HxMutex         mMutex;
   pthread_cond_t  *mCondition;
   bool            mSet;


   HxSemaphore()
   {
      mSet = false;
      mCondition = new pthread_cond_t();
      pthread_cond_init(mCondition,0);
   }
   ~HxSemaphore()
   {
      Clean();
   }
   // For autolock
   inline operator HxMutex &() { return mMutex; }
   void Set()
   {
      AutoLock lock(mMutex);
      if (!mSet)
      {
         mSet = true;
         pthread_cond_signal( mCondition );
      }
   }
   void QSet()
   {
      mSet = true;
      pthread_cond_signal( mCondition );
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
         pthread_cond_wait( mCondition, mMutex.mMutex );
      mSet = false;
   }
   // when we already hold the mMutex lock ...
   void QWait()
   {
      while( !mSet )
         pthread_cond_wait( mCondition, mMutex.mMutex );
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
      while( !mSet &&  (result=pthread_cond_timedwait( mCondition, mMutex.mMutex, &spec )) != ETIMEDOUT)
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
      if (mCondition)
      {
         pthread_cond_destroy(mCondition);
         delete mCondition;
         mCondition = nullptr;
      }
   }

};


#endif


#if defined HX_WINRT

inline void HxSleep(unsigned int ms)
{
	::Sleep(ms);
}

#elif defined HX_WINDOWS

inline void HxSleep(unsigned int ms)
{
	::Sleep(ms);
}

#else

inline void HxSleep(unsigned int ms)
{
   struct timespec t;
   struct timespec tmp;
   t.tv_sec = 0;
   t.tv_nsec = ms * 1000000;
   nanosleep(&t, &tmp);
}

#endif


#endif
#endif
