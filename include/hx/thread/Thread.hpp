#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, thread, Thread)

namespace hx
{
	namespace thread
	{
		class Scratch final
		{
			using ReleaseFunc = void(*)(cpp::marshal::View<uint8_t>);

			int* count;
			ReleaseFunc release;

		public:
			static Scratch alloc(int bytes);

			cpp::marshal::View<uint8_t> view;

			Scratch(int* _count, cpp::marshal::View<uint8_t> _view, ReleaseFunc _release);
			Scratch(const Scratch& other);

			~Scratch();

			Scratch& operator=(const Scratch& other);
		};

		struct Thread_obj : public hx::Object
		{
			static Thread create(Callable<void(void)> job);
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