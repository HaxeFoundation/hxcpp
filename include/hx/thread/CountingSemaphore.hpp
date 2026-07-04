#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, thread, CountingSemaphore)

namespace hx
{
	namespace thread
	{
		struct CountingSemaphore_obj : public hx::Object
		{
			CountingSemaphore_obj(int value);

			void acquire();
			void release();
			bool tryAcquire(Null<double> timeout);

		private:
			struct Impl;

			Impl* impl;
		};
	}
}