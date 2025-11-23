#pragma once

#include <hx/zip/Compress.hpp>
#include <zlib.h>

namespace hx
{
	namespace zip
	{
		namespace zlib
		{
			struct ZLibCompress final : Compress_obj
			{
				z_stream* handle;
				int flush;

				ZLibCompress(z_stream* inHandle, int inFlush);

				void execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) override;
				void setFlushMode(int mode) override;
				void close() override;
			};
		}
	}
}