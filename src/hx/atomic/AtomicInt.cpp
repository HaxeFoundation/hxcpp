#include <hxcpp.h>
#include "hx/atomic/AtomicInt.hpp"
#include <atomic>

// Atomic operations never block, so they don't enter a GC free zone. Each one is still a GC
// safe point, so that a loop which spins on atomics without allocating can't stall a collection.

struct hx::atomic::AtomicInt_obj::Impl
{
	std::atomic_int atomic;
};

hx::atomic::AtomicInt_obj::AtomicInt_obj(int val) : impl(new Impl())
{
	impl->atomic.store(val);
}

int hx::atomic::AtomicInt_obj::fetchAdd(int val)
{
	__hxcpp_gc_safe_point();

	return impl->atomic.fetch_add(val);
}

int hx::atomic::AtomicInt_obj::fetchSub(int val)
{
	__hxcpp_gc_safe_point();

	return impl->atomic.fetch_sub(val);
}

int hx::atomic::AtomicInt_obj::fetchAnd(int val)
{
	__hxcpp_gc_safe_point();

	return impl->atomic.fetch_and(val);
}

int hx::atomic::AtomicInt_obj::fetchXor(int val)
{
	__hxcpp_gc_safe_point();

	return impl->atomic.fetch_xor(val);
}

int hx::atomic::AtomicInt_obj::fetchOr(int val)
{
	__hxcpp_gc_safe_point();

	return impl->atomic.fetch_or(val);
}

int hx::atomic::AtomicInt_obj::store(int val)
{
	__hxcpp_gc_safe_point();

	impl->atomic.store(val);

	return val;
}

int hx::atomic::AtomicInt_obj::load()
{
	__hxcpp_gc_safe_point();

	return impl->atomic.load();
}

int hx::atomic::AtomicInt_obj::exchange(int val)
{
	__hxcpp_gc_safe_point();

	return impl->atomic.exchange(val);
}

int hx::atomic::AtomicInt_obj::compareExchange(int expected, int replacement)
{
	__hxcpp_gc_safe_point();

	// On failure, compare_exchange_strong stores the current value in `original`.
	int original{ expected };
	impl->atomic.compare_exchange_strong(original, replacement);

	return original;
}
