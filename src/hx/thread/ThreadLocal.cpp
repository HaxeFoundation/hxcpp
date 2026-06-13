#include <hxcpp.h>
#include <atomic>
#include <mutex>
#include <list>
#include <hx/thread/Thread.hpp>
#include <hx/thread/RecursiveMutex.hpp>
#include <hx/thread/ThreadLocal.hpp>
#include "ThreadImpl.hpp"

struct hx::thread::ThreadLocal_obj::Impl
{
	static std::mutex slotLock;
	static std::list<Impl*> slots;
	static int counter;

	const int slot;

	Impl(const int _slot) : slot(_slot) {}

	static Impl* get()
	{
		std::lock_guard<std::mutex> lock(slotLock);

		if (slots.empty())
		{
			return new Impl(counter++);
		}

		auto recycled = slots.front();

		slots.pop_front();

		return recycled;
	}

	static void finalise(hx::Object* obj)
	{
		std::lock_guard<std::mutex> lock(slotLock);

		slots.emplace_back(reinterpret_cast<hx::thread::ThreadLocal_obj*>(obj)->impl);
	}
};

std::mutex hx::thread::ThreadLocal_obj::Impl::slotLock;

std::list<hx::thread::ThreadLocal_obj::Impl*> hx::thread::ThreadLocal_obj::Impl::slots;

int hx::thread::ThreadLocal_obj::Impl::counter = 0;

hx::thread::ThreadLocal_obj::ThreadLocal_obj() : impl(Impl::get())
{
	hx::GCSetFinalizer(this, Impl::finalise);
}

Dynamic hx::thread::ThreadLocal_obj::get()
{
	auto current = reinterpret_cast<hx::thread::ThreadImpl_obj*>(Thread_obj::current().GetPtr());

	return current->getSlot(impl->slot);
}

void hx::thread::ThreadLocal_obj::set(Dynamic obj)
{
	auto current = reinterpret_cast<hx::thread::ThreadImpl_obj*>(Thread_obj::current().GetPtr());

	current->setSlot(impl->slot, obj);
}