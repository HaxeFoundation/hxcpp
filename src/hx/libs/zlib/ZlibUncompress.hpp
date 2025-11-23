#pragma once

#include <hx/zip/Uncompress.hpp>
#include <zlib.h>

namespace hx
{
	namespace zip
	{
		namespace zlib
		{
			struct ZLibUncompress final : Uncompress_obj
			{
				z_stream* handle;
				int flush;

				ZLibUncompress(z_stream* inHandle, int inFlush);

				UncompressResult execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) override;
				void setFlushMode(int mode) override;
				void close() override;
			};
		}
	}
}
