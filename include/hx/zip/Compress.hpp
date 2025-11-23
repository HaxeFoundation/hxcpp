#pragma once

#include <hxcpp.h>

HX_DECLARE_CLASS2(hx, zip, Compress)

namespace hx
{
	namespace zip
	{
		struct Compress_obj : hx::Object
		{
			static Compress create(int level);

			virtual void execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) = 0;
			virtual void setFlushMode(int mode) = 0;
			virtual void close() = 0;
		};
	}
}
