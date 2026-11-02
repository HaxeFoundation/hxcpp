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
	typedef std::list<Impl*> SlotList;

	// finalise() runs from the GC, which may still collect while the process exits:
	// these must outlive the static destructors (see HXCPP_IMMORTAL_STATICS)
	HX_IMMORTAL_STATIC_DECL(std::mutex, slotLock);
	HX_IMMORTAL_STATIC_DECL(SlotList, slots);
	static int counter;

	const int slot;
	int generation;

	Impl(const int _slot) : slot(_slot), generation(0) {}

	static Impl* get()
	{
		std::lock_guard<std::mutex> lock(slotLock);

		if (slots.empty())
		{
			return new Impl(counter++);
		}

		auto recycled = slots.front();

		slots.pop_front();

		// Threads might still hold values from the previous owner of this slot.
		recycled->generation++;

		return recycled;
	}

	static void finalise(hx::Object* obj)
	{
		std::lock_guard<std::mutex> lock(slotLock);

		slots.emplace_back(reinterpret_cast<hx::thread::ThreadLocal_obj*>(obj)->impl);
	}
};

HX_IMMORTAL_GLOBAL(std::mutex, hx::thread::ThreadLocal_obj::Impl::slotLock);

HX_IMMORTAL_GLOBAL(hx::thread::ThreadLocal_obj::Impl::SlotList, hx::thread::ThreadLocal_obj::Impl::slots);

int hx::thread::ThreadLocal_obj::Impl::counter = 0;

hx::thread::ThreadLocal_obj::ThreadLocal_obj() : impl(Impl::get()), generation(impl->generation)
{
	hx::GCSetFinalizer(this, Impl::finalise);
}

Dynamic hx::thread::ThreadLocal_obj::get()
{
	auto current = reinterpret_cast<hx::thread::ThreadImpl_obj*>(Thread_obj::current().GetPtr());

	return current->getSlot(impl->slot, generation);
}

void hx::thread::ThreadLocal_obj::set(Dynamic obj)
{
	auto current = reinterpret_cast<hx::thread::ThreadImpl_obj*>(Thread_obj::current().GetPtr());

	current->setSlot(impl->slot, obj, generation);
}