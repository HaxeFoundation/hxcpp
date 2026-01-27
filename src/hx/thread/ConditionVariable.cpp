#include <hxcpp.h>
#include "hx/thread/ConditionVariable.hpp"
#include <mutex>
#include <condition_variable>
#include <chrono>

struct hx::thread::ConditionVariable_obj::Impl
{
	std::mutex mutex;
	std::condition_variable_any condition;
};

hx::thread::ConditionVariable_obj::ConditionVariable_obj() : impl(new Impl())
{
	GCSetFinalizer(this, [](hx::Object* obj)
	{
		auto condition = reinterpret_cast<ConditionVariable_obj*>(obj);

		delete condition->impl;
	});
}

void hx::thread::ConditionVariable_obj::acquire()
{
	hx::AutoGCFreeZone zone;

	impl->mutex.lock();
}

bool hx::thread::ConditionVariable_obj::tryAcquire()
{
	return impl->mutex.try_lock();
}

void hx::thread::ConditionVariable_obj::release()
{
	impl->mutex.unlock();
}

void hx::thread::ConditionVariable_obj::wait()
{
	hx::AutoGCFreeZone zone;

	impl->condition.wait(impl->mutex);
}

void hx::thread::ConditionVariable_obj::signal()
{
	impl->condition.notify_one();
}

void hx::thread::ConditionVariable_obj::broadcast()
{
	impl->condition.notify_all();
}
