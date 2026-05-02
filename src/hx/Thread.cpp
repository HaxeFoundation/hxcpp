#include <hxcpp.h>

#include <hx/Thread.h>
#include <time.h>
#include <hx/thread/ConditionVariable.hpp>
#include <hx/thread/RecursiveMutex.hpp>
#include <hx/thread/CountingSemaphore.hpp>
#include "thread/ThreadImpl.hpp"
#include <atomic>

// --- Deque ----------------------------------------------------------

struct Deque : public Array_obj<Dynamic>
{
	Deque() : Array_obj<Dynamic>(0,0) { }

	static Deque *Create()
	{
		Deque *result = new Deque();
		result->mFinalizer = new hx::InternalFinalizer(result,clean);
		return result;
	}
	static void clean(hx::Object *inObj)
	{
		Deque *d = dynamic_cast<Deque *>(inObj);
		if (d) d->Clean();
	}
	void Clean()
	{
		#ifdef HX_WINDOWS
		mMutex.Clean();
		#endif
		mSemaphore.Clean();
	}

   #ifdef HXCPP_VISIT_ALLOCS
  	void __Visit(hx::VisitContext *__inCtx)
	{
		Array_obj<Dynamic>::__Visit(__inCtx);
		mFinalizer->Visit(__inCtx);
	}
   #endif


	#ifndef HX_THREAD_SEMAPHORE_LOCKABLE
	HxMutex     mMutex;
	void PushBack(Dynamic inValue)
	{
		hx::EnterGCFreeZone();
		AutoLock lock(mMutex);
		hx::ExitGCFreeZone();

		push(inValue);
		mSemaphore.Set();
	}
	void PushFront(Dynamic inValue)
	{
		hx::EnterGCFreeZone();
		AutoLock lock(mMutex);
		hx::ExitGCFreeZone();

		unshift(inValue);
		mSemaphore.Set();
	}

	
	Dynamic PopFront(bool inBlock)
	{
		hx::EnterGCFreeZone();
		AutoLock lock(mMutex);
		if (!inBlock)
		{
			hx::ExitGCFreeZone();
			return shift();
		}
		// Ok - wait for something on stack...
		while(!length)
		{
			mSemaphore.Reset();
			lock.Unlock();
			mSemaphore.Wait();
			lock.Lock();
		}
		hx::ExitGCFreeZone();
		if (length==1)
			mSemaphore.Reset();
		return shift();
	}
	#else
	void PushBack(Dynamic inValue)
	{
		hx::EnterGCFreeZone();
		AutoLock lock(mSemaphore);
		hx::ExitGCFreeZone();
		push(inValue);
		mSemaphore.QSet();
	}
	void PushFront(Dynamic inValue)
	{
		hx::EnterGCFreeZone();
		AutoLock lock(mSemaphore);
		hx::ExitGCFreeZone();
		unshift(inValue);
		mSemaphore.QSet();
	}

	
	Dynamic PopFront(bool inBlock)
	{
		hx::EnterGCFreeZone();
		AutoLock lock(mSemaphore);
		while(inBlock && !length)
			mSemaphore.QWait();
		hx::ExitGCFreeZone();
		Dynamic result =  shift();
		if (length)
			mSemaphore.QSet();
		return result;
	}
	#endif

	hx::InternalFinalizer *mFinalizer;
	HxSemaphore mSemaphore;
};

Dynamic __hxcpp_deque_create()
{
	return Deque::Create();
}

void __hxcpp_deque_add(Dynamic q,Dynamic inVal)
{
	Deque *d = dynamic_cast<Deque *>(q.mPtr);
	if (!d)
		throw HX_INVALID_OBJECT;
	d->PushBack(inVal);
}

void __hxcpp_deque_push(Dynamic q,Dynamic inVal)
{
	Deque *d = dynamic_cast<Deque *>(q.mPtr);
	if (!d)
		throw HX_INVALID_OBJECT;
	d->PushFront(inVal);
}

Dynamic __hxcpp_deque_pop(Dynamic q,bool block)
{
	Deque *d = dynamic_cast<Deque *>(q.mPtr);
	if (!d)
		throw HX_INVALID_OBJECT;
	return d->PopFront(block);
}

// --- Thread ----------------------------------------------------------

#if (HXCPP_API_LEVEL>=500)
Dynamic __hxcpp_thread_create(hx::Callable<void()> inStart)
#else
Dynamic __hxcpp_thread_create(Dynamic inStart)
#endif
{
	return hx::thread::Thread_obj::create(inStart);
}

Dynamic __hxcpp_thread_current()
{
	return hx::thread::Thread_obj::current();
}

void __hxcpp_thread_send(Dynamic inThread, Dynamic inMessage)
{
	hx::Throw(HX_CSTRING("Not Implemented"));
}

Dynamic __hxcpp_thread_read_message(bool inBlocked)
{
	return hx::Throw(HX_CSTRING("Not Implemented"));
}

bool __hxcpp_is_current_thread(hx::Object *inThread)
{
   return inThread == hx::thread::Thread_obj::current();
}

// --- TLS ------------------------------------------------------------

Dynamic __hxcpp_tls_get(int inID)
{
	return reinterpret_cast<hx::thread::ThreadImpl_obj*>(hx::thread::Thread_obj::current().GetPtr())->getSlot(inID);
}

void __hxcpp_tls_set(int inID,Dynamic inVal)
{
	reinterpret_cast<hx::thread::ThreadImpl_obj*>(hx::thread::Thread_obj::current().GetPtr())->setSlot(inID, inVal);
}

// --- Mutex ------------------------------------------------------------

Dynamic __hxcpp_mutex_create()
{
	return new hx::thread::RecursiveMutex_obj();
}
void __hxcpp_mutex_acquire(Dynamic inMutex)
{
	auto mutex = inMutex.Cast<hx::thread::RecursiveMutex>();

	mutex->acquire();
}
bool __hxcpp_mutex_try(Dynamic inMutex)
{
	auto mutex = inMutex.Cast<hx::thread::RecursiveMutex>();

	return mutex->tryAcquire();
}
void __hxcpp_mutex_release(Dynamic inMutex)
{
	auto mutex = inMutex.Cast<hx::thread::RecursiveMutex>();

	mutex->release();
}

// --- Semaphore ------------------------------------------------------------

Dynamic __hxcpp_semaphore_create(int value) {
	return new hx::thread::CountingSemaphore_obj(value);
}
void __hxcpp_semaphore_acquire(Dynamic inSemaphore) {
	auto semaphore = inSemaphore.Cast<hx::thread::CountingSemaphore>();

	semaphore->acquire();
}
bool __hxcpp_semaphore_try_acquire(Dynamic inSemaphore, double timeout) {
	auto semaphore = inSemaphore.Cast<hx::thread::CountingSemaphore>();

	return semaphore->tryAcquire(timeout);
}
void __hxcpp_semaphore_release(Dynamic inSemaphore) {
	auto semaphore = inSemaphore.Cast<hx::thread::CountingSemaphore>();

	semaphore->release();
}

// --- Condition ------------------------------------------------------------

Dynamic __hxcpp_condition_create(void)
{
	return new hx::thread::ConditionVariable_obj();
}
void __hxcpp_condition_acquire(Dynamic inCond)
{
	auto condition = inCond.Cast<hx::thread::ConditionVariable>();
  
	condition->acquire();
}
bool __hxcpp_condition_try_acquire(Dynamic inCond)
{
	auto condition = inCond.Cast<hx::thread::ConditionVariable>();

	return condition->tryAcquire();
}
void __hxcpp_condition_release(Dynamic inCond)
{
	auto condition = inCond.Cast<hx::thread::ConditionVariable>();

	condition->release();
}
void __hxcpp_condition_wait(Dynamic inCond)
{
	auto condition = inCond.Cast<hx::thread::ConditionVariable>();

	condition->wait();
}
bool __hxcpp_condition_timed_wait(Dynamic inCond, double timeout)
{
	return hx::Throw(HX_CSTRING("Not Implemented"));
}
void __hxcpp_condition_signal(Dynamic inCond)
{
	auto condition = inCond.Cast<hx::thread::ConditionVariable>();

	condition->signal();
}
void __hxcpp_condition_broadcast(Dynamic inCond)
{
	auto condition = inCond.Cast<hx::thread::ConditionVariable>();

	condition->broadcast();
}

// --- Lock ------------------------------------------------------------

class hxLock : public hx::Object
{
public:

	hxLock()
	{
		mFinalizer = new hx::InternalFinalizer(this);
		mFinalizer->mFinalizer = clean;
	}

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdLock };

   #ifdef HXCPP_VISIT_ALLOCS
	void __Visit(hx::VisitContext *__inCtx) { mFinalizer->Visit(__inCtx); }
   #endif

	hx::InternalFinalizer *mFinalizer;

	#if defined(HX_WINDOWS) || defined(__SNC__)
	double Now()
	{
		return (double)clock()/CLOCKS_PER_SEC;
	}
	#else
	double Now()
	{
		struct timeval tv;
		gettimeofday(&tv,0);
		return tv.tv_sec + tv.tv_usec*0.000001;
	}
	#endif

	static void clean(hx::Object *inObj)
	{
		hxLock *l = dynamic_cast<hxLock *>(inObj);
		if (l)
		{
			l->mNotEmpty.Clean();
			l->mAvailableLock.Clean();
		}
	}
	bool Wait(double inTimeout)
	{
		double stop = 0;
		if (inTimeout>=0)
			stop = Now() + inTimeout;
		while(1)
		{
			mAvailableLock.Lock();
			if (mAvailable)
			{
				--mAvailable;
		      if (mAvailable>0)
               mNotEmpty.Set();
				mAvailableLock.Unlock();
				return true;
			}
			mAvailableLock.Unlock();
			double wait = 0;
			if (inTimeout>=0)
			{
				wait = stop-Now();
				if (wait<=0)
					return false;
			}

			hx::EnterGCFreeZone();
			if (inTimeout<0)
				mNotEmpty.Wait( );
			else
				mNotEmpty.WaitSeconds(wait);
			hx::ExitGCFreeZone();
		}
	}
	void Release()
	{
		AutoLock lock(mAvailableLock);
		mAvailable++;
		mNotEmpty.Set();
	}


	HxSemaphore mNotEmpty;
   HxMutex     mAvailableLock;
	int         mAvailable;
};



Dynamic __hxcpp_lock_create()
{
	return new hxLock;
}
bool __hxcpp_lock_wait(Dynamic inlock,double inTime)
{
	hxLock *lock = dynamic_cast<hxLock *>(inlock.mPtr);
	if (!lock)
		throw HX_INVALID_OBJECT;
	return lock->Wait(inTime);
}
void __hxcpp_lock_release(Dynamic inlock)
{
	hxLock *lock = dynamic_cast<hxLock *>(inlock.mPtr);
	if (!lock)
		throw HX_INVALID_OBJECT;
	lock->Release();
}


int __hxcpp_GetCurrentThreadNumber()
{
	return hx::thread::Thread_obj::id();
}

// --- Atomic ---

bool _hx_atomic_exchange_if(::cpp::Pointer<cpp::AtomicInt> inPtr, int test, int  newVal )
{
   return _hx_atomic_compare_exchange(inPtr, test, newVal) == test;
}

int _hx_atomic_inc(::cpp::Pointer<cpp::AtomicInt> inPtr )
{
   return _hx_atomic_add(inPtr, 1);
}

int _hx_atomic_dec(::cpp::Pointer<cpp::AtomicInt> inPtr )
{
   return _hx_atomic_sub(inPtr, 1);
}


