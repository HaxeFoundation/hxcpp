#include <hxcpp.h>
#include "hx/atomic/AtomicInt.hpp"
#include <atomic>

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
	return impl->atomic.fetch_add(val);
}

int hx::atomic::AtomicInt_obj::fetchSub(int val)
{
	return impl->atomic.fetch_sub(val);
}

int hx::atomic::AtomicInt_obj::fetchAnd(int val)
{
	return impl->atomic.fetch_and(val);
}

int hx::atomic::AtomicInt_obj::fetchXor(int val)
{
	return impl->atomic.fetch_xor(val);
}

int hx::atomic::AtomicInt_obj::fetchOr(int val)
{
	return impl->atomic.fetch_or(val);
}

int hx::atomic::AtomicInt_obj::store(int val)
{
	impl->atomic.store(val);

	return val;
}

int hx::atomic::AtomicInt_obj::load()
{
	return impl->atomic.load();
}

int hx::atomic::AtomicInt_obj::exchange(int val)
{
	return impl->atomic.exchange(val);
}

int hx::atomic::AtomicInt_obj::compareExchange(int expected, int replacement)
{
	int original{ impl->atomic.load() };
	while (original == expected)
	{
		if (impl->atomic.compare_exchange_strong(expected, replacement))
		{
			return original;
		}
		else
		{
			continue;
		}
	}

	return original;
}
