#include <hxObject.h>
#include <hxThread.h>
#include <time.h>

#ifndef INTERNAL_GC
extern "C" {
#include <gc.h>
}
#endif

static TLSData<class hxThreadInfo> tlsCurrentThread;

// --- Deque ----------------------------------------------------------

struct Deque : public Array_obj<Dynamic>
{
	Deque() : Array_obj<Dynamic>(0,0) { }

	static Deque *Create()
	{
		Deque *result = new Deque();
		#ifdef INTERNAL_GC
		result->mFinalizer = new hxInternalFinalizer(result);
		result->mFinalizer->mFinalizer = clean;
		#endif
		return result;
	}
	static void clean(hxObject *inObj)
	{
		Deque *d = dynamic_cast<Deque *>(inObj);
		if (d) d->Clean();
	}
	void Clean()
	{
		#ifdef _MSC_VER
		mMutex.Clean();
		#endif
		mSemaphore.Clean();
	}

	#ifdef INTERNAL_GC
	void __Mark()
	{
		Array_obj<Dynamic>::__Mark();
		mFinalizer->Mark();
	}
	#endif

	#ifdef _MSC_VER
	MyMutex     mMutex;
	void PushBack(Dynamic inValue)
	{
		hxEnterGCFreeZone();
		AutoLock lock(mMutex);
		hxExitGCFreeZone();

		push(inValue);
		mSemaphore.Set();
	}
	void PushFront(Dynamic inValue)
	{
		hxEnterGCFreeZone();
		AutoLock lock(mMutex);
		hxExitGCFreeZone();

		unshift(inValue);
		mSemaphore.Set();
	}

	
	Dynamic PopFront(bool inBlock)
	{
		hxEnterGCFreeZone();
		AutoLock lock(mMutex);
		if (!inBlock)
		{
			hxExitGCFreeZone();
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
		hxExitGCFreeZone();
		if (length==1)
			mSemaphore.Reset();
		return shift();
	}
	#else
	void PushBack(Dynamic inValue)
	{
		hxEnterGCFreeZone();
		AutoLock lock(mSemaphore);
		hxExitGCFreeZone();
		push(inValue);
		mSemaphore.QSet();
	}
	void PushFront(Dynamic inValue)
	{
		hxEnterGCFreeZone();
		AutoLock lock(mSemaphore);
		hxExitGCFreeZone();
		unshift(inValue);
		mSemaphore.QSet();
	}

	
	Dynamic PopFront(bool inBlock)
	{
		hxEnterGCFreeZone();
		AutoLock lock(mSemaphore);
		while(inBlock && !length)
			mSemaphore.QWait();
		hxExitGCFreeZone();
		return shift();
	}
	#endif

	hxInternalFinalizer *mFinalizer;
	MySemaphore mSemaphore;
};

Dynamic __hxcpp_deque_create()
{
	return Deque::Create();
}

void __hxcpp_deque_add(Dynamic q,Dynamic inVal)
{
	Deque *d = dynamic_cast<Deque *>(q.mPtr);
	if (!d)
		throw INVALID_OBJECT;
	d->PushBack(inVal);
}

void __hxcpp_deque_push(Dynamic q,Dynamic inVal)
{
	Deque *d = dynamic_cast<Deque *>(q.mPtr);
	if (!d)
		throw INVALID_OBJECT;
	d->PushFront(inVal);
}

Dynamic __hxcpp_deque_pop(Dynamic q,bool block)
{
	Deque *d = dynamic_cast<Deque *>(q.mPtr);
	if (!d)
		throw INVALID_OBJECT;
	return d->PopFront(block);
}



// --- Thread ----------------------------------------------------------

class hxThreadInfo : public hxObject
{
public:
	hxThreadInfo(Dynamic inFunction) : mFunction(inFunction), mTLS(0,0)
	{
		mSemaphore = new MySemaphore;
		mDeque = Deque::Create();
	}
	hxThreadInfo()
	{
		mSemaphore = 0;
		mDeque = Deque::Create();
	}
	void Clean()
	{
		mDeque->Clean();
	}
	void CleanSemaphore()
	{
		delete mSemaphore;
		mSemaphore = 0;
	}
	void Send(Dynamic inMessage)
	{
		mDeque->PushBack(inMessage);
	}
	Dynamic ReadMessage(bool inBlocked)
	{
		return mDeque->PopFront(inBlocked);
	}
	void SetTLS(int inID,Dynamic inVal) { mTLS[inID] = inVal; }
	Dynamic GetTLS(int inID) { return mTLS[inID]; }

	void __Mark()
	{
		MarkMember(mFunction);
		MarkMember(mTLS);
		if (mDeque)
			HX_MARK_OBJECT(mDeque);
	}

	Array<Dynamic> mTLS;
	MySemaphore *mSemaphore;
	Dynamic mFunction;
	Deque   *mDeque;
};


THREAD_FUNC_TYPE hxThreadFunc( void *inInfo )
{
	int dummy = 0;
#ifdef INTERNAL_GC
	hxRegisterCurrentThread(&dummy);
#endif

	hxThreadInfo *info = (hxThreadInfo *)inInfo;
	tlsCurrentThread.Set(info);

	// Release the creation function
	info->mSemaphore->Set();

	if ( info->mFunction.GetPtr() )
	{
		// Try ... catch
		info->mFunction->__run();
	}

#ifdef INTERNAL_GC
	hxUnregisterCurrentThread();
#endif

	tlsCurrentThread.Set(0);

	THREAD_FUNC_RET
}



Dynamic __hxcpp_thread_create(Dynamic inStart)
{
	hxThreadInfo *info = new hxThreadInfo(inStart);

#ifdef INTERNAL_GC

   #if defined(_MSC_VER)
      _beginthreadex(0,0,hxThreadFunc,info,0,0);
   #else
      pthread_t result;
      pthread_create(&result,0,hxThreadFunc,info);
	#endif

#else

   #if defined(_MSC_VER)
      GC_beginthreadex(0,0,hxThreadFunc,info,0,0);
   #else
      pthread_t result;
      GC_pthread_create(&result,0,hxThreadFunc,info);
   #endif

#endif

#ifdef INTERNAL_GC
	hxEnterGCFreeZone();
	info->mSemaphore->Wait();
	hxExitGCFreeZone();
#else
	info->mSemaphore->Wait();
#endif
	info->CleanSemaphore();
	return info;
}

static hxObject *sMainThreadInfo = 0;

static hxThreadInfo *GetCurrentInfo()
{
	hxThreadInfo *info = tlsCurrentThread.Get();
	if (!info)
	{
		// Hmm, must be the "main" thead...
		info = new hxThreadInfo(null());
		sMainThreadInfo = info;
		hxGCAddRoot(&sMainThreadInfo);
		tlsCurrentThread.Set(info);
	}
	return info;
}

Dynamic __hxcpp_thread_current()
{
	return GetCurrentInfo();
}

void __hxcpp_thread_send(Dynamic inThread, Dynamic inMessage)
{
	hxThreadInfo *info = dynamic_cast<hxThreadInfo *>(inThread.mPtr);
	if (!info)
		throw INVALID_OBJECT;
	info->Send(inMessage);
}

Dynamic __hxcpp_thread_read_message(bool inBlocked)
{
	hxThreadInfo *info = GetCurrentInfo();
	return info->ReadMessage(inBlocked);
}

// --- TLS ------------------------------------------------------------

Dynamic __hxcpp_tls_get(int inID)
{
	return GetCurrentInfo()->GetTLS(inID);
}

void __hxcpp_tls_set(int inID,Dynamic inVal)
{
	GetCurrentInfo()->SetTLS(inID,inVal);
}



// --- Mutex ------------------------------------------------------------

class hxMutex : public hxObject
{
public:

	hxMutex()
	{
	#ifdef INTERNAL_GC
		mFinalizer = new hxInternalFinalizer(this);
		mFinalizer->mFinalizer = clean;
	#else
		hxGCAddFinalizer(this,clean);
	#endif
	}

	#ifdef INTERNAL_GC
	void __Mark() { mFinalizer->Mark(); }
	hxInternalFinalizer *mFinalizer;
	#endif

	static void clean(hxObject *inObj)
	{
		hxMutex *m = dynamic_cast<hxMutex *>(inObj);
		if (m) m->mMutex.Clean();
	}
	bool Try()
	{
		return mMutex.TryLock();
	}
	void Acquire()
	{
		hxEnterGCFreeZone();
		mMutex.Lock();
		hxExitGCFreeZone();
	}
	void Release()
	{
		mMutex.Unlock();
	}


   MyMutex mMutex;
};



Dynamic __hxcpp_mutex_create()
{
	return new hxMutex;
}
void __hxcpp_mutex_acquire(Dynamic inMutex)
{
	hxMutex *mutex = dynamic_cast<hxMutex *>(inMutex.mPtr);
	if (!mutex)
		throw INVALID_OBJECT;
	mutex->Acquire();
}
bool __hxcpp_mutex_try(Dynamic inMutex)
{
	hxMutex *mutex = dynamic_cast<hxMutex *>(inMutex.mPtr);
	if (!mutex)
		throw INVALID_OBJECT;
	return mutex->Try();
}
void __hxcpp_mutex_release(Dynamic inMutex)
{
	hxMutex *mutex = dynamic_cast<hxMutex *>(inMutex.mPtr);
	if (!mutex)
		throw INVALID_OBJECT;
	return mutex->Release();
}






// --- Lock ------------------------------------------------------------

class hxLock : public hxObject
{
public:

	hxLock()
	{
	#ifdef INTERNAL_GC
		mFinalizer = new hxInternalFinalizer(this);
		mFinalizer->mFinalizer = clean;
	#else
		hxGCAddFinalizer(this,clean);
	#endif
	}

	#ifdef INTERNAL_GC
	void __Mark() { mFinalizer->Mark(); }
	hxInternalFinalizer *mFinalizer;
	#endif

	#ifdef _MSC_VER
	double Now()
	{
		return (double)clock()/CLOCKS_PER_SEC;
	}
	#else
	double Now()
	{
		struct timeval tv;
		gettimeofday(&tv,NULL);
		return tv.tv_sec + tv.tv_usec*0.000001;
	}
	#endif

	static void clean(hxObject *inObj)
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

			hxEnterGCFreeZone();
			if (inTimeout<0)
				mNotEmpty.Wait( );
			else
				mNotEmpty.WaitFor(wait);
			hxExitGCFreeZone();
		}
	}
	void Release()
	{
		AutoLock lock(mAvailableLock);
		mAvailable++;
		mNotEmpty.Set();
	}


	MySemaphore mNotEmpty;
   MyMutex     mAvailableLock;
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
		throw INVALID_OBJECT;
	return lock->Wait(inTime);
}
void __hxcpp_lock_release(Dynamic inlock)
{
	hxLock *lock = dynamic_cast<hxLock *>(inlock.mPtr);
	if (!lock)
		throw INVALID_OBJECT;
	lock->Release();
}


