#pragma once

#include <hxcpp.h>

HX_DECLARE_CLASS2(hx, zip, Uncompress)

namespace hx
{
	namespace zip
	{
		struct UncompressResult final {
			bool done;
			int read;
			int write;

			UncompressResult() = default;
		};

		struct Uncompress_obj : hx::Object
		{
			static int flushNone;
			static int flushSync;
			static int flushFull;
			static int flushFinish;
			static int flushBlock;

			static Uncompress create(int windowSize);

			virtual UncompressResult execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) = 0;
			virtual void setFlushMode(int mode) = 0;
			virtual void close() = 0;
		};
	}
}
