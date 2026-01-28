#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, thread, ConditionVariable)

namespace hx
{
	namespace thread
	{
		struct ConditionVariable_obj : public hx::Object
		{
			ConditionVariable_obj();

			void acquire();
			bool tryAcquire();
			void release();
			void wait();
			void signal();
			void broadcast();

		private:
			struct Impl;

			Impl* impl;
		};
	}
}