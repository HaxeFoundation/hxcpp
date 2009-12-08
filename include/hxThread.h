#ifndef HX_THREAD_H
#define HX_THREAD_H

#ifdef _MSC_VER

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#if defined(_MSC_VER)


struct MyMutex
{
	MyMutex() { mCreated = true; InitializeCriticalSection(&mCritSec); }
	~MyMutex() { if (mCreated) DeleteCriticalSection(&mCritSec); }
	void Lock() { EnterCriticalSection(&mCritSec); }
	void Unlock() { LeaveCriticalSection(&mCritSec); }
	bool TryLock() { return TryEnterCriticalSection(&mCritSec); }
	void Clean()
	{
		if (mCreated)
		{
		   DeleteCriticalSection(&mCritSec);
		   mCreated = false;
		}
	}

	bool             mCreated;
	CRITICAL_SECTION mCritSec;
};

template<typename DATA>
struct TLSData
{
	TLSData()
	{
		mSlot = TlsAlloc();
	}
	DATA *Get()
	{
		return (DATA *)TlsGetValue(mSlot);
	}
	void Set(DATA *inData)
	{
		TlsSetValue(mSlot,inData);
	}
	inline DATA *operator=(DATA *inData)
	{
		TlsSetValue(mSlot,inData);
      return inData;
	}
	inline operator DATA *() { return (DATA *)TlsGetValue(mSlot); }

	int mSlot;
};

#define THREAD_FUNC_TYPE unsigned int WINAPI
#define THREAD_FUNC_RET return 0;

#else

struct MyMutex
{
	MyMutex() { pthread_mutex_init(&mMutex,0); }
	~MyMutex() { if (mMutex) pthread_mutex_destroy(&mMutex); }
	void Lock() { pthread_mutex_lock(&mMutex); }
	void Unlock() { pthread_mutex_unlock(&mMutex); }
	bool TryLock() { return !pthread_mutex_try_lock(&mMutex); }
	void Clean()
	{
		pthread_mutex_destroy(mMutex);
		mMutex = 0;
	}

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


#if defined(_MSC_VER)

struct MySemaphore
{
	MySemaphore() { mSemaphore = CreateEvent(0,0,0,0); }
	~MySemaphore() { if (mSemaphore) CloseHandle(mSemaphore); }
	void Set() { SetEvent(mSemaphore); }
	void Wait() { WaitForSingleObject(mSemaphore,INFINITE); }
	void WaitFor(double inSeconds)
	{
		WaitForSingleObject(mSemaphore,inSeconds*0.001);
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
		pthread_cond_init(&mCondition,NULL);
	}
	~MySemaphore()
	{
		pthread_cond_destroy(&mCondition,NULL);
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
	void Wait()
	{
		AutoLock lock(mMutex);
      while( !mSet )
         pthread_cond_wait( &mCondition, &mMutex.mMutex );
		mSet = false;
	}
	void WaitFor(double inSeconds)
	{
		timespec spec;
		clock_gettime(CLOCK_REALTIME, &spec);
		int isec = (int)inSeconds;
		int usec = (int)((inSeconds-isec)*1000000.0);
      spec.tv_usec += usec;
		if (spec.tv_usec>1000000)
		{
		   spec.tv_usec-=1000000;
		   spec.tv_sec++;
		}
      spec.tv_sec += isec;
      pthread_cond_waittimed( &mCondition, &mMutex.mMutex, &spec );
	}


	MyMutex         mMutex;
   pthread_cond_t  mCondition;
   bool            mSet;
};


#endif



#endif
