#include <hxcpp.h>
#include "hx/atomic/AtomicObject.hpp"
#include <atomic>

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
	hx::EnterGCFreeZone();
	impl->atomic.store(val);
	hx::ExitGCFreeZone();

	HX_OBJ_WB_GET(this, val.mPtr);

	return val;
}

Dynamic hx::atomic::AtomicObject_obj::load()
{
	hx::AutoGCFreeZone _;

	return impl->atomic.load();
}

Dynamic hx::atomic::AtomicObject_obj::exchange(Dynamic val)
{
	hx::EnterGCFreeZone();
	Dynamic old{ impl->atomic.exchange(val) };
	hx::ExitGCFreeZone();

	HX_OBJ_WB_GET(this, val.mPtr);

	return old;
}

Dynamic hx::atomic::AtomicObject_obj::compareExchange(Dynamic expected, Dynamic replacement)
{
	// Note: using Dynamic instead of hx::Object* is important
	// Dynamic has an overloaded == operator, a raw pointer to hx::Object does not.
	Dynamic original{ impl->atomic.load() };
	while (original == expected)
	{
		hx::EnterGCFreeZone();
		if (impl->atomic.compare_exchange_strong(original.mPtr, replacement.mPtr))
		{
			hx::ExitGCFreeZone();

			HX_OBJ_WB_GET(this, replacement.mPtr);

			return original;
		}
		else
		{
			hx::ExitGCFreeZone();
			continue;
		}
	}

	return original;
}
