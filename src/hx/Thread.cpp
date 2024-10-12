#include <hxcpp.h>

#include <hx/Thread.h>
#include <time.h>

DECLARE_TLS_DATA(class hxThreadInfo, tlsCurrentThread);

// g_threadInfoMutex allows atomic access to g_nextThreadNumber
static HxMutex g_threadInfoMutex;
// Thread number 0 is reserved for the main thread
static int g_nextThreadNumber = 1;


// How to manage hxThreadInfo references for non haxe threads (main, extenal)?
// HXCPP_THREAD_INFO_PTHREAD - use pthread api
// HXCPP_THREAD_INFO_LOCAL - use thread_local storage
// HXCPP_THREAD_INFO_SINGLETON - use one structure for all threads. Not ideal.

#if __cplusplus > 199711L && !defined(__BORLANDC__)
   #define HXCPP_THREAD_INFO_LOCAL
#elif defined (HXCPP_PTHREADS)
   #define HXCPP_THREAD_INFO_PTHREAD
#else
   #define HXCPP_THREAD_INFO_SINGLETON
#endif


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

#ifdef HXCPP_THREAD_INFO_PTHREAD
static pthread_key_t externThreadInfoKey;;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static void destroyThreadInfo(void *i)
{
   hx::Object **threadRoot = (hx::Object **)i;
   hx::GCRemoveRoot(threadRoot);
   delete threadRoot;
}
static void make_key()
{
   pthread_key_create(&externThreadInfoKey, destroyThreadInfo);
}
#elif defined(HXCPP_THREAD_INFO_LOCAL)
struct ThreadInfoHolder
{
   hx::Object **threadRoot;
   ThreadInfoHolder() : threadRoot(0) { }
   ~ThreadInfoHolder()
   {
      if (threadRoot)
      {
         hx::GCRemoveRoot(threadRoot);
         delete threadRoot;
      }
   }
   void set(hx::Object **info) { threadRoot = info; }
   hxThreadInfo *get() { return threadRoot ? (hxThreadInfo *)*threadRoot : nullptr; }
   
};
static thread_local ThreadInfoHolder threadHolder;
#else
static hx::Object **sMainThreadInfoRoot = 0;
#endif

static hxThreadInfo *GetCurrentInfo(bool createNew = true)
{
	hxThreadInfo *info = tlsCurrentThread;
	if (!info)
   {
      #ifdef HXCPP_THREAD_INFO_PTHREAD
      pthread_once(&key_once, make_key);
      hxThreadInfo **pp = (hxThreadInfo **)pthread_getspecific(externThreadInfoKey);
      if (pp)
         info = *pp;
      #elif defined(HXCPP_THREAD_INFO_LOCAL)
      info = threadHolder.get();
      #else
      if (sMainThreadInfoRoot)
      info = (hxThreadInfo *)*sMainThreadInfoRoot;
      #endif
   }

	if (!info && createNew)
	{
      // New, non-haxe thread - might be the first thread, or might be a new
      //  foreign thread.
		info = new hxThreadInfo(null(), 0);
      hx::Object **threadRoot = new hx::Object *;
      *threadRoot = info; 
		hx::GCAddRoot(threadRoot);
      #ifdef HXCPP_THREAD_INFO_PTHREAD
      pthread_setspecific(externThreadInfoKey, threadRoot);
      #elif defined(HXCPP_THREAD_INFO_LOCAL)
      threadHolder.set(threadRoot);
      #else
      sMainThreadInfoRoot = threadRoot;
      #endif
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

#if defined(HX_LINUX) || defined(HX_ANDROID)
#define POSIX_SEMAPHORE
#include <semaphore.h>
#endif

#if defined(HX_MACOS) || defined(IPHONE) || defined(APPLETV)
#define APPLE_SEMAPHORE
#include <dispatch/dispatch.h>
#endif

class hxSemaphore : public hx::Object {
public:
  hx::InternalFinalizer *mFinalizer;
#ifdef HX_WINDOWS
  HANDLE sem;
#elif defined (POSIX_SEMAPHORE)
  sem_t sem;
#elif defined(APPLE_SEMAPHORE)
	dispatch_semaphore_t sem;
#endif
  bool valid;

  hxSemaphore(int value) {
    mFinalizer = new hx::InternalFinalizer(this);
    mFinalizer->mFinalizer = clean;
#ifdef HX_WINDOWS
    sem = CreateSemaphoreW(NULL, value, 0x7FFFFFFF, NULL);
#elif defined(POSIX_SEMAPHORE)
    sem_init(&sem, 0, value);
#elif defined(APPLE_SEMAPHORE)
    sem = dispatch_semaphore_create(value);
#endif
    valid = true;
  }

  HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSemaphore };

#ifdef HXCPP_VISIT_ALLOCS
  void __Visit(hx::VisitContext *__inCtx) { mFinalizer->Visit(__inCtx); }
#endif

  void Acquire() {
#if HX_WINDOWS
	WaitForSingleObject(sem, INFINITE);
#elif defined(POSIX_SEMAPHORE)
    sem_wait(&sem);
#elif defined(APPLE_SEMAPHORE)
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
#endif
  }

  bool TryAcquire(double timeout) {
#ifdef HX_WINDOWS
    return WaitForSingleObject(sem, (DWORD)((FLOAT)timeout * 1000.0)) == 0;
#elif defined(POSIX_SEMAPHORE)
    if (timeout == 0) {
      return sem_trywait(&sem) == 0;
    } else {
      struct timeval tv;
      struct timespec t;
      double delta = timeout;
      int idelta = (int)delta, idelta2;
      delta -= idelta;
      delta *= 1.0e9;
      gettimeofday(&tv, NULL);
      delta += tv.tv_usec * 1000.0;
      idelta2 = (int)(delta / 1e9);
      delta -= idelta2 * 1e9;
      t.tv_sec = tv.tv_sec + idelta + idelta2;
      t.tv_nsec = (long)delta;
      return sem_timedwait(&sem, &t) == 0;
    }
#elif defined(APPLE_SEMAPHORE)
    return dispatch_semaphore_wait(
               sem,
               dispatch_time(DISPATCH_TIME_NOW,
                             (int64_t)(timeout * 1000 * 1000 * 1000))) == 0;
#else
	return false;
#endif
  }

  void Release() {
#if HX_WINDOWS
	ReleaseSemaphore(sem, 1, NULL);
#elif defined(POSIX_SEMAPHORE)
    sem_post(&sem);
#elif defined(APPLE_SEMAPHORE)
    dispatch_semaphore_signal(sem);
#endif
  }

  static void clean(hx::Object *inObj) {
    hxSemaphore *l = dynamic_cast<hxSemaphore *>(inObj);
    if (l) {
      if(l->valid) {
#ifdef HX_WINDOWS
		CloseHandle(l->sem);
#elif defined(POSIX_SEMAPHORE)
		  sem_destroy(&l->sem);
#endif
		  l->valid = false;
	  }
    }
  }
};

Dynamic __hxcpp_semaphore_create(int value) {
  return new hxSemaphore(value);
}
void __hxcpp_semaphore_acquire(Dynamic inSemaphore) {
  hxSemaphore *semaphore = dynamic_cast<hxSemaphore *>(inSemaphore.mPtr);
  if (!semaphore)
    throw HX_INVALID_OBJECT;
  semaphore->Acquire();
}
bool __hxcpp_semaphore_try_acquire(Dynamic inSemaphore, double timeout) {
  hxSemaphore *semaphore = dynamic_cast<hxSemaphore *>(inSemaphore.mPtr);
  if (!semaphore)
    throw HX_INVALID_OBJECT;
  return semaphore->TryAcquire(timeout);
}
void __hxcpp_semaphore_release(Dynamic inSemaphore) {
  hxSemaphore *semaphore = dynamic_cast<hxSemaphore *>(inSemaphore.mPtr);
  if (!semaphore)
    throw HX_INVALID_OBJECT;
  semaphore->Release();
}

class hxCondition : public hx::Object {
public:
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cond;
#endif
#else
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
#endif
  hx::InternalFinalizer *mFinalizer;
  hxCondition() {
    mFinalizer = new hx::InternalFinalizer(this);
    mFinalizer->mFinalizer = clean;
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
    InitializeCriticalSection(&cs);
	InitializeConditionVariable(&cond);
#else
	throw Dynamic(HX_CSTRING("Condition variables are not supported on Windows XP"));
#endif
#else
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    cond = new pthread_cond_t();
    pthread_cond_init(cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
#endif
  }

  HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCondition };

#ifdef HXCPP_VISIT_ALLOCS
  void __Visit(hx::VisitContext *__inCtx) { mFinalizer->Visit(__inCtx); }
#endif
  static void clean(hx::Object *inObj) {
    hxCondition *cond = dynamic_cast<hxCondition *>(inObj);
    if (cond) {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
      DeleteCriticalSection(&cond->cs);
#endif
#else
      pthread_cond_destroy(cond->cond);
      delete cond->cond;
      pthread_mutex_destroy(cond->mutex);
      delete cond->mutex;
#endif
    }
  }

  void Acquire() {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	  EnterCriticalSection(&cs);
#endif
#else
	  pthread_mutex_lock(mutex);
#endif
  }

  bool TryAcquire() {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
    return (bool)TryEnterCriticalSection(&cs);
#else
	return false;
#endif
#else
    return pthread_mutex_trylock(mutex);
#endif
  }

  void Release() {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	  LeaveCriticalSection(&cs);
#endif
#else
	  pthread_mutex_unlock(mutex);
#endif
  }

  void Wait() {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	  SleepConditionVariableCS(&cond,&cs,INFINITE);
#endif
#else
	  pthread_cond_wait(cond, mutex);
#endif
  }

  bool TimedWait(double timeout) {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	  return (bool)SleepConditionVariableCS(&cond, &cs, (DWORD)((FLOAT)timeout * 1000.0));
#else
	  return false;
#endif
#else
    struct timeval tv;
    struct timespec t;
    double delta = timeout;
    int idelta = (int)delta, idelta2;
    delta -= idelta;
    delta *= 1.0e9;
    gettimeofday(&tv, NULL);
    delta += tv.tv_usec * 1000.0;
    idelta2 = (int)(delta / 1e9);
    delta -= idelta2 * 1e9;
    t.tv_sec = tv.tv_sec + idelta + idelta2;
    t.tv_nsec = (long)delta;
    return pthread_cond_timedwait(cond, mutex, &t);
#endif
  }
  void Signal() {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	  WakeConditionVariable(&cond);
#endif
#else
	  pthread_cond_signal(cond);
#endif
  }
  void Broadcast() {
#ifdef HX_WINDOWS
#ifndef HXCPP_WINXP_COMPAT
	  WakeAllConditionVariable(&cond);
#endif
#else
	  pthread_cond_broadcast(cond);
#endif
  }
};

Dynamic __hxcpp_condition_create(void) {
  return new hxCondition;
}
void __hxcpp_condition_acquire(Dynamic inCond) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  cond->Acquire();
}
bool __hxcpp_condition_try_acquire(Dynamic inCond) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  return cond->TryAcquire();
}
void __hxcpp_condition_release(Dynamic inCond) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  cond->Release();
}
void __hxcpp_condition_wait(Dynamic inCond) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  cond->Wait();
}
bool __hxcpp_condition_timed_wait(Dynamic inCond, double timeout) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  return cond->TimedWait(timeout);
}
void __hxcpp_condition_signal(Dynamic inCond) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  cond->Signal();
}
void __hxcpp_condition_broadcast(Dynamic inCond) {
  hxCondition *cond = dynamic_cast<hxCondition *>(inCond.mPtr);
  if (!cond)
    throw HX_INVALID_OBJECT;
  cond->Broadcast();
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


