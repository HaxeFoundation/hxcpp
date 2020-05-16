#include <hxcpp.h>

#include <hx/Thread.h>
#include <time.h>

DECLARE_TLS_DATA(class hxThreadInfo, tlsCurrentThread);

// g_threadInfoMutex allows atomic access to g_nextThreadNumber
static HxMutex g_threadInfoMutex;
// Thread number 0 is reserved for the main thread
static int g_nextThreadNumber = 1;


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

class hxThreadInfo : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdThreadInfo };

	hxThreadInfo(Dynamic inFunction, int inThreadNumber)
        : mFunction(inFunction), mThreadNumber(inThreadNumber), mTLS(0,0)
	{
		mSemaphore = new HxSemaphore;
		mDeque = Deque::Create();
      HX_OBJ_WB_NEW_MARKED_OBJECT(this);
	}
	hxThreadInfo()
	{
		mSemaphore = 0;
		mDeque = Deque::Create();
      HX_OBJ_WB_NEW_MARKED_OBJECT(this);
	}
    int GetThreadNumber() const
    {
        return mThreadNumber;
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
	String toString()
	{
		return String(GetThreadNumber());
	}
	void SetTLS(int inID,Dynamic inVal) {
      mTLS->__SetItem(inID,inVal);
   }
	Dynamic GetTLS(int inID) { return mTLS[inID]; }

	void __Mark(hx::MarkContext *__inCtx)
	{
		HX_MARK_MEMBER(mFunction);
		HX_MARK_MEMBER(mTLS);
		if (mDeque)
			HX_MARK_OBJECT(mDeque);
	}
   #ifdef HXCPP_VISIT_ALLOCS
  	void __Visit(hx::VisitContext *__inCtx)
	{
		HX_VISIT_MEMBER(mFunction);
		HX_VISIT_MEMBER(mTLS);
		if (mDeque)
			HX_VISIT_OBJECT(mDeque);
	}
   #endif


	Array<Dynamic> mTLS;
	HxSemaphore *mSemaphore;
	Dynamic mFunction;
    int mThreadNumber;
	Deque   *mDeque;
};


THREAD_FUNC_TYPE hxThreadFunc( void *inInfo )
{
   // info[1] will the the "top of stack" - values under this
   //  (ie info[0] and other stack values) will be in the GC conservative range
	hxThreadInfo *info[2];
   info[0] = (hxThreadInfo *)inInfo;
   info[1] = 0;

	tlsCurrentThread = info[0];

	hx::SetTopOfStack((int *)&info[1], true);

	// Release the creation function
	info[0]->mSemaphore->Set();

    // Call the debugger function to annouce that a thread has been created
    //__hxcpp_dbg_threadCreatedOrTerminated(info[0]->GetThreadNumber(), true);

	if ( info[0]->mFunction.GetPtr() )
	{
		// Try ... catch
		info[0]->mFunction->__run();
	}

    // Call the debugger function to annouce that a thread has terminated
    //__hxcpp_dbg_threadCreatedOrTerminated(info[0]->GetThreadNumber(), false);

	hx::UnregisterCurrentThread();

	tlsCurrentThread = 0;

	THREAD_FUNC_RET
}



Dynamic __hxcpp_thread_create(Dynamic inStart)
{
    #ifdef EMSCRIPTEN
    return hx::Throw( HX_CSTRING("Threads are not supported on Emscripten") );
    #else
    g_threadInfoMutex.Lock();
    int threadNumber = g_nextThreadNumber++;
    g_threadInfoMutex.Unlock();

	hxThreadInfo *info = new hxThreadInfo(inStart, threadNumber);

	hx::GCPrepareMultiThreaded();
	hx::EnterGCFreeZone();

    bool ok = HxCreateDetachedThread(hxThreadFunc, info);
    if (ok)
    {
       info->mSemaphore->Wait();
    }

    hx::ExitGCFreeZone();
    info->CleanSemaphore();

    if (!ok)
       throw Dynamic( HX_CSTRING("Could not create thread") );
    return info;
    #endif
}

static hx::Object *sMainThreadInfo = 0;

static hxThreadInfo *GetCurrentInfo(bool createNew = true)
{
	hxThreadInfo *info = tlsCurrentThread;
	if (!info && createNew)
	{
		// Hmm, must be the "main" thead...
		info = new hxThreadInfo(null(), 0);
		sMainThreadInfo = info;
		hx::GCAddRoot(&sMainThreadInfo);
		tlsCurrentThread = info;
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
		throw HX_INVALID_OBJECT;
	info->Send(inMessage);
}

Dynamic __hxcpp_thread_read_message(bool inBlocked)
{
	hxThreadInfo *info = GetCurrentInfo();
	return info->ReadMessage(inBlocked);
}

bool __hxcpp_is_current_thread(hx::Object *inThread)
{
   hxThreadInfo *info = tlsCurrentThread;
   return info==inThread;
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

class hxMutex : public hx::Object
{
public:

	hxMutex()
	{
		mFinalizer = new hx::InternalFinalizer(this);
		mFinalizer->mFinalizer = clean;
	}

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdMutex };

   #ifdef HXCPP_VISIT_ALLOCS
	void __Visit(hx::VisitContext *__inCtx) { mFinalizer->Visit(__inCtx); }
   #endif

	hx::InternalFinalizer *mFinalizer;

	static void clean(hx::Object *inObj)
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
		hx::EnterGCFreeZone();
		mMutex.Lock();
		hx::ExitGCFreeZone();
	}
	void Release()
	{
		mMutex.Unlock();
	}


   HxMutex mMutex;
};



Dynamic __hxcpp_mutex_create()
{
	return new hxMutex;
}
void __hxcpp_mutex_acquire(Dynamic inMutex)
{
	hxMutex *mutex = dynamic_cast<hxMutex *>(inMutex.mPtr);
	if (!mutex)
		throw HX_INVALID_OBJECT;
	mutex->Acquire();
}
bool __hxcpp_mutex_try(Dynamic inMutex)
{
	hxMutex *mutex = dynamic_cast<hxMutex *>(inMutex.mPtr);
	if (!mutex)
		throw HX_INVALID_OBJECT;
	return mutex->Try();
}
void __hxcpp_mutex_release(Dynamic inMutex)
{
	hxMutex *mutex = dynamic_cast<hxMutex *>(inMutex.mPtr);
	if (!mutex)
		throw HX_INVALID_OBJECT;
	return mutex->Release();
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
    // Can't allow GetCurrentInfo() to create the main thread's info
    // because that can cause a call loop.
    hxThreadInfo *threadInfo = GetCurrentInfo(false);
    if (!threadInfo) {
        return 0;
    }
    return threadInfo->GetThreadNumber();
}

// --- Atomic ---

bool _hx_atomic_exchange_if(::cpp::Pointer<cpp::AtomicInt> inPtr, int test, int  newVal )
{
   return HxAtomicExchangeIf(test, newVal, inPtr);
}

int _hx_atomic_inc(::cpp::Pointer<cpp::AtomicInt> inPtr )
{
   return HxAtomicInc(inPtr);
}

int _hx_atomic_dec(::cpp::Pointer<cpp::AtomicInt> inPtr )
{
   return HxAtomicDec(inPtr);
}


