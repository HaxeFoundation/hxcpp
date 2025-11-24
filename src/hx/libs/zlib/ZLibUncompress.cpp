#include "ZlibUncompress.hpp"

#include <memory>

hx::zip::Uncompress hx::zip::Uncompress_obj::create(int windowSize)
{
	auto handle = std::make_unique<z_stream>();
	auto error  = inflateInit2(handle.get(), windowSize);

	if (error != Z_OK)
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	return new hx::zip::zlib::ZLibUncompress(handle.release());
}

hx::zip::zlib::ZLibUncompress::ZLibUncompress(z_stream* inHandle) : handle(inHandle), flush(0)
{
	_hx_set_finalizer(this, [](Dynamic obj) { reinterpret_cast<ZLibUncompress*>(obj.mPtr)->close(); });
}

hx::zip::Result hx::zip::zlib::ZLibUncompress::execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst)
{
	handle->next_in   = src.ptr;
	handle->next_out  = dst.ptr;
	handle->avail_in  = src.length;
	handle->avail_out = dst.length;

	EnterGCFreeZone();
	auto error = inflate(handle, flush);
	ExitGCFreeZone();

	if (error < 0)
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	auto result = Result();
	result.done  = error == Z_STREAM_END;
	result.write = src.length - handle->avail_in;
	result.read  = dst.length - handle->avail_out;

	return result;
}

void hx::zip::zlib::ZLibUncompress::setFlushMode(Flush mode)
{
	switch (mode)
	{
	case Flush::None:
		flush = Z_NO_FLUSH;
		break;

	case Flush::Sync:
		flush = Z_SYNC_FLUSH;
		break;

	case Flush::Full:
		flush = Z_FULL_FLUSH;
		break;

	case Flush::Finish:
		flush = Z_FINISH;
		break;

	case Flush::Block:
		flush = Z_BLOCK;
		break;
	}
}

void hx::zip::zlib::ZLibUncompress::close()
{
	if (nullptr == handle)
	{
		return;
	}

	inflateEnd(handle);

	handle = nullptr;
}
