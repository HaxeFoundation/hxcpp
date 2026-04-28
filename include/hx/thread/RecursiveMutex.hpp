#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, thread, RecursiveMutex)

namespace hx
{
	namespace thread
	{
		struct RecursiveMutex_obj : public hx::Object
		{
			RecursiveMutex_obj();

			void acquire();
			void release();
			bool tryAcquire();

		private:
			struct Impl;

			Impl* impl;
		};
	}
}