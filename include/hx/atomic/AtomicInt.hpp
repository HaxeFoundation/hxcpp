#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, atomic, AtomicInt)

namespace hx
{
	namespace atomic
	{
		struct AtomicInt_obj final : public hx::Object
		{
			AtomicInt_obj(int val);

			int fetchAdd(int val);
			int fetchSub(int val);
			int fetchAnd(int val);
			int fetchXor(int val);
			int fetchOr(int val);
			int store(int val);
			int load();
			int exchange(int val);
			int compareExchange(int expected, int replacement);

		private:
			struct Impl;

			Impl* impl;
		};
	}
}