#include "ZLibCompress.hpp"

#include <memory>
#include <limits>

hx::zip::Compress hx::zip::Compress_obj::create(int level)
{
	auto handle = std::make_unique<z_stream>();
	auto error  = deflateInit(handle.get(), level);

	if (error != Z_OK)
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	return new hx::zip::zlib::ZLibCompress(handle.release());
}

Array<uint8_t> hx::zip::Compress_obj::run(cpp::marshal::View<uint8_t> src, int level)
{
	auto error  = 0;
	auto handle = std::make_unique<z_stream>();

	if (Z_OK != (error = deflateInit(handle.get(), level)))
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	auto bounds = deflateBound(handle.get(), src.length);
	if (bounds > std::numeric_limits<int32_t>::max()) {
		hx::Throw(HX_CSTRING("Size Error"));
	}

	auto output = Array<uint8_t>(bounds, bounds);
	auto dst    = cpp::marshal::View<uint8_t>(output->getBase(), bounds);

	handle->next_in = src.ptr;
	handle->next_out = dst.ptr;
	handle->avail_in = src.length;
	handle->avail_out = dst.length;

	EnterGCFreeZone();
	error = deflate(handle.get(), Z_FINISH);
	ExitGCFreeZone();

	if (Z_STREAM_END != error) {
		hx::Throw(HX_CSTRING("Compression failed"));
	}

	deflateEnd(handle.get());

	return output;
}

hx::zip::zlib::ZLibCompress::ZLibCompress(z_stream* inHandle) : handle(inHandle), flush(0)
{
	_hx_set_finalizer(this, [](Dynamic obj) { reinterpret_cast<ZLibCompress*>(obj.mPtr)->close(); });
}

hx::zip::Result hx::zip::zlib::ZLibCompress::execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst)
{
	handle->next_in   = src.ptr;
	handle->next_out  = dst.ptr;
	handle->avail_in  = src.length;
	handle->avail_out = dst.length;

	EnterGCFreeZone();
	auto error = deflate(handle, flush);
	ExitGCFreeZone();

	if (error < 0)
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	return
		Result(
			error == Z_STREAM_END,
			dst.length - handle->avail_out,
			src.length - handle->avail_in);
}

void hx::zip::zlib::ZLibCompress::setFlushMode(Flush mode)
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

int hx::zip::zlib::ZLibCompress::getBounds(const int length)
{
	return static_cast<int>(deflateBound(handle, length));
}

void hx::zip::zlib::ZLibCompress::close()
{
	if (nullptr == handle)
	{
		return;
	}

	deflateEnd(handle);

	handle = nullptr;
}
