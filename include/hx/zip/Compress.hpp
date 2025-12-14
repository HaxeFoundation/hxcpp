#pragma once

#include "Zip.hpp"

HX_DECLARE_CLASS2(hx, zip, Compress)

namespace hx
{
	namespace zip
	{
		struct Compress_obj : Zip_obj
		{
			static Compress create(int level);
			static Array<uint8_t> run(cpp::marshal::View<uint8_t> src, int level);

			virtual int getBounds(int length) = 0;
		};
	}
}
