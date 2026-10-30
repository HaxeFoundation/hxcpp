#include <hxcpp.h>
#include "hx/atomic/AtomicObject.hpp"
#include <atomic>

// Atomic operations never block, so they don't enter a GC free zone. Each one is still a GC
// safe point, so that a loop which spins on atomics without allocating can't stall a collection.

struct hx::atomic::AtomicObject_obj::Impl
{
	std::atomic<hx::Object*> atomic;
};

hx::atomic::AtomicObject_obj::AtomicObject_obj(Dynamic val) : impl(new Impl())
{
	impl->atomic.store(val.mPtr);

	HX_OBJ_WB_NEW_MARKED_OBJECT(this);
}

void hx::atomic::AtomicObject_obj::__Mark(HX_MARK_PARAMS)
{
	Dynamic ptr{ impl->atomic.load() };

	HX_MARK_MEMBER(ptr);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::atomic::AtomicObject_obj::__Visit(HX_VISIT_PARAMS)
{
	Dynamic ptr{ impl->atomic.load() };

	HX_VISIT_MEMBER(ptr);

	impl->atomic.store(ptr);
}
#endif

Dynamic hx::atomic::AtomicObject_obj::store(Dynamic val)
{
	__hxcpp_gc_safe_point();

	impl->atomic.store(val);

	HX_OBJ_WB_GET(this, val.mPtr);

	return val;
}

Dynamic hx::atomic::AtomicObject_obj::load()
{
	__hxcpp_gc_safe_point();

	return impl->atomic.load();
}

Dynamic hx::atomic::AtomicObject_obj::exchange(Dynamic val)
{
	__hxcpp_gc_safe_point();

	Dynamic old{ impl->atomic.exchange(val) };

	HX_OBJ_WB_GET(this, val.mPtr);

	return old;
}

Dynamic hx::atomic::AtomicObject_obj::compareExchange(Dynamic expected, Dynamic replacement)
{
	__hxcpp_gc_safe_point();

	// Note: using Dynamic instead of hx::Object* is important
	// Dynamic has an overloaded == operator, a raw pointer to hx::Object does not.
	Dynamic original{ impl->atomic.load() };
	while (original == expected)
	{
		if (impl->atomic.compare_exchange_strong(original.mPtr, replacement.mPtr))
		{
			HX_OBJ_WB_GET(this, replacement.mPtr);

			return original;
		}
	}

	return original;
}
