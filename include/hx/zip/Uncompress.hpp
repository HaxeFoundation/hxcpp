#pragma once

#include <hxcpp.h>
#include "Zip.hpp"

HX_DECLARE_CLASS2(hx, zip, Uncompress)

namespace hx
{
	namespace zip
	{
		struct Uncompress_obj : hx::Object
		{
			static Uncompress create(int windowSize);

			virtual Result execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) = 0;
			virtual void setFlushMode(Flush mode) = 0;
			virtual void close() = 0;
		};
	}
}
