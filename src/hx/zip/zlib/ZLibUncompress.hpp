#pragma once

#include <hxcpp.h>
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

				ZLibUncompress(z_stream* inHandle);

				Result execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) override;
				void setFlushMode(Flush mode) override;
				void close() override;
			};
		}
	}
}
