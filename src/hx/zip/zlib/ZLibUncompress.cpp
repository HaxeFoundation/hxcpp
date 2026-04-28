#include <hxcpp.h>
#include "ZLibUncompress.hpp"

#include <memory>
#include <vector>

#define ZLIB_OBJ_CLOSED ::hx::Throw(HX_CSTRING("Compress closed"))

hx::zip::Uncompress hx::zip::Uncompress_obj::create(int windowSize)
{
	auto handle = std::unique_ptr<z_stream>(new z_stream());
	auto error  = inflateInit2(handle.get(), windowSize);

	if (error != Z_OK)
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	return new hx::zip::zlib::ZLibUncompress(handle.release());
}

Array<uint8_t> hx::zip::Uncompress_obj::run(cpp::marshal::View<uint8_t> src, int bufferSize)
{
	auto handle = std::unique_ptr<z_stream>(new z_stream());
	auto error  = inflateInit2(handle.get(), 15);

	if (error != Z_OK)
	{
		hx::Throw(HX_CSTRING("ZLib Error"));
	}

	auto buffer    = std::vector<uint8_t>(bufferSize);
	auto output    = Array<uint8_t>(0, 0);
	auto srcCursor = 0;

	while (Z_STREAM_END != error)
	{
		auto srcView = src.slice(srcCursor);

		handle->next_in   = srcView.ptr;
		handle->next_out  = buffer.data();
		handle->avail_in  = srcView.length;
		handle->avail_out = buffer.size();

		EnterGCFreeZone();
		error = inflate(handle.get(), Z_SYNC_FLUSH);
		ExitGCFreeZone();

		if (error < 0)
		{
			hx::Throw(HX_CSTRING("ZLib Error"));
		}

		output->memcpy(output->length, buffer.data(), buffer.size() - handle->avail_out);

		srcCursor += srcView.length - handle->avail_in;
	}

	return output;
}

hx::zip::zlib::ZLibUncompress::ZLibUncompress(z_stream* inHandle) : handle(inHandle), flush(0)
{
	_hx_set_finalizer(this, [](Dynamic obj) { reinterpret_cast<ZLibUncompress*>(obj.mPtr)->close(); });
}

hx::zip::Result hx::zip::zlib::ZLibUncompress::execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst)
{
	if (handle == nullptr)
	{
		ZLIB_OBJ_CLOSED;
	}

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

	return
		Result(
			error == Z_STREAM_END,
			static_cast<int>(handle->total_in),
			static_cast<int>(handle->total_out));
}

void hx::zip::zlib::ZLibUncompress::setFlushMode(Flush mode)
{
	if (handle == nullptr)
	{
		ZLIB_OBJ_CLOSED;
	}

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

	_hx_set_finalizer(this, nullptr);
}
