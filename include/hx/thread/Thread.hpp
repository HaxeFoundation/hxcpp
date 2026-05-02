#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, thread, Thread)

namespace hx
{
	namespace thread
	{
		struct Thread_obj : public hx::Object
		{
			using CreateFunction =
#if (HXCPP_API_LEVEL >= 500)
				Callable<void(void)>;
#else
				Dynamic;
#endif

			static Thread create(CreateFunction);
			static Thread current();
			static int id();

			virtual String getName() = 0;
			virtual void setName(const String& name) = 0;

			virtual String toString() override = 0;

			virtual void __Mark(HX_MARK_PARAMS) override = 0;
#ifdef HXCPP_VISIT_ALLOCS
			virtual void __Visit(HX_VISIT_PARAMS) override = 0;
#endif
		};
	}
}