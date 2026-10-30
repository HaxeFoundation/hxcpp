#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, atomic, AtomicObject)

namespace hx
{
	namespace atomic
	{
		struct AtomicObject_obj final : public hx::Object
		{
			AtomicObject_obj(Dynamic val);

			void __Mark(hx::MarkContext* __inCtx) HXCPP_OVERRIDE;
#ifdef HXCPP_VISIT_ALLOCS
			void __Visit(hx::VisitContext* __inCtx) HXCPP_OVERRIDE;
#endif

			Dynamic store(Dynamic val);
			Dynamic load();
			Dynamic exchange(Dynamic val);
			Dynamic compareExchange(Dynamic expected, Dynamic replacement);

		private:
			struct Impl;

			Impl* impl;
		};
	}
}