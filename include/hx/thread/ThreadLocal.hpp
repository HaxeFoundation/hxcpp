#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, thread, ThreadLocal)

namespace hx
{
	namespace thread
	{
		class ThreadLocal_obj : public hx::Object
		{
			struct Impl;

			Impl* impl;

		public:
			ThreadLocal_obj();

			Dynamic get();
			void set(Dynamic obj);
		};
	}
}