#include <hxcpp.h>
#include <hx/thread/Thread.hpp>
#include "ThreadImpl.hpp"

String hx::thread::ThreadImpl_obj::getName()
{
	return null();
}

void hx::thread::ThreadImpl_obj::setName(const String& name)
{
	//
}

hx::thread::ThreadImpl_obj::Native::Native() : thread(), handle() {}