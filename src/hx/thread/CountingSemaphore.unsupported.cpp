#include <hxcpp.h>
#include <hx/thread/CountingSemaphore.hpp>

struct hx::thread::CountingSemaphore_obj::Impl {};

hx::thread::CountingSemaphore_obj::CountingSemaphore_obj(int value) : impl(nullptr) {}

void hx::thread::CountingSemaphore_obj::acquire()
{
	hx::Throw(HX_CSTRING("Unsupported platform"));
}

void hx::thread::CountingSemaphore_obj::release()
{
	hx::Throw(HX_CSTRING("Unsupported platform"));
}

bool hx::thread::CountingSemaphore_obj::tryAcquire(Null<double> timeout)
{
	hx::Throw(HX_CSTRING("Unsupported platform"));

	return false;
}