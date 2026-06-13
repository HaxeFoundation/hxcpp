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
	}
}