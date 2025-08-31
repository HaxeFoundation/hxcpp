#include <hxcpp.h>
#include "StreamReader.h"

hx::asys::libuv::stream::StreamReader_obj::Ctx::Ctx(uv_stream_t* stream)
    : stream(stream)
    , queue()
    , staging()
    , buffer()
{
}

void hx::asys::libuv::stream::StreamReader_obj::Ctx::consume()
{
    while (!buffer.empty() && !queue.empty())
    {
        auto& request = queue.front();
        auto size     = std::min(int(buffer.size()), request.length);

        request.array.rooted->memcpy(request.offset, reinterpret_cast<uint8_t*>(buffer.data()), size);

        buffer.erase(buffer.begin(), buffer.begin() + size);

        queue.pop_front();

        Dynamic(request.cbSuccess.rooted)(size);
    }

    if (queue.empty())
    {
        uv_read_stop(stream);
    }
}

void hx::asys::libuv::stream::StreamReader_obj::Ctx::reject(int code)
{
    buffer.clear();

    while (!queue.empty())
    {
        auto& request = queue.front();

        Dynamic(request.cbFailure.rooted)(asys::libuv::uv_err_to_enum(code));

        queue.pop_front();
    }
}

hx::asys::libuv::stream::StreamReader_obj::QueuedRead::QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure)
    : BaseRequest(_cbSuccess, _cbFailure)
    , array(_array.mPtr)
    , offset(_offset)
    , length(_length)
{
}

hx::asys::libuv::stream::StreamReader_obj::StreamReader_obj(Ctx* ctx, uv_alloc_cb cbAlloc, uv_read_cb cbRead)
    : ctx(ctx)
    , cbAlloc(cbAlloc)
    , cbRead(cbRead) {}

void hx::asys::libuv::stream::StreamReader_obj::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    if (ctx->queue.empty())
    {
        if (!ctx->buffer.empty())
        {
            ctx->queue.emplace_back(output, offset, length, cbSuccess, cbFailure);
            ctx->consume();

            return;
        }

        auto result = uv_read_start(ctx->stream, cbAlloc, cbRead);
        if (result < 0 && result != UV_EALREADY)
        {
            cbFailure(uv_err_to_enum(result));

            return;
        }
    }

    ctx->queue.emplace_back(output, offset, length, cbSuccess, cbFailure);
}
