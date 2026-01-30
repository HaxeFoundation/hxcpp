#include <hxcpp.h>
#include "hx/thread/RecursiveMutex.hpp"
#include <mutex>

struct hx::thread::RecursiveMutex_obj::Impl
{
	std::recursive_mutex mutex;
};

hx::thread::RecursiveMutex_obj::RecursiveMutex_obj() : impl(new Impl())
{
	GCSetFinalizer(this, [](hx::Object* obj)
	{
		auto mutex = reinterpret_cast<RecursiveMutex_obj*>(obj);

		delete mutex->impl;
	});
}

void hx::thread::RecursiveMutex_obj::acquire()
{
	hx::AutoGCFreeZone zone;

	impl->mutex.lock();
}

void hx::thread::RecursiveMutex_obj::release()
{
	impl->mutex.unlock();
}

bool hx::thread::RecursiveMutex_obj::tryAcquire()
{
	return impl->mutex.try_lock();
}
