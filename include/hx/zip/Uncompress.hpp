#ifndef HX_ZIP_UNCOMPRESS_HPP
#define HX_ZIP_UNCOMPRESS_HPP

#include "Zip.hpp"

HX_DECLARE_CLASS2(hx, zip, Uncompress)

namespace hx
{
	namespace zip
	{
		struct Uncompress_obj : Zip_obj
		{
			static Uncompress create(int windowSize);
			static Array<uint8_t> run(cpp::marshal::View<uint8_t> src, int bufferSize);
		};
	}
}

#endif
