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

hx::asys::libuv::stream::StreamReader_obj::StreamReader_obj(uv_stream_t* stream)
    : ctx(new Ctx(stream))
{
    stream->data = &ctx;

    hx::GCSetFinalizer(this, [](hx::Object* obj) {
        delete reinterpret_cast<StreamReader_obj*>(obj)->ctx;
    });
}

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

        auto result = uv_read_start(ctx->stream, onAlloc, onRead);
        if (result < 0 && result != UV_EALREADY)
        {
            cbFailure(uv_err_to_enum(result));
        }
    }

    ctx->queue.emplace_back(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::stream::StreamReader_obj::close(Dynamic cbSuccess, Dynamic cbFailure)
{
    uv_close(reinterpret_cast<uv_handle_t*>(ctx->stream), hx::asys::libuv::clean_handle);
}

void hx::asys::libuv::stream::StreamReader_obj::onAlloc(uv_handle_t* handle, size_t suggested, uv_buf_t* buffer)
{
    auto ctx     = static_cast<Ctx*>(handle->data);
    auto staging = std::vector<char>(suggested);

    buffer->base = staging.data();
    buffer->len  = staging.size();

    ctx->staging.push_back(staging);
}

void hx::asys::libuv::stream::StreamReader_obj::onRead(uv_stream_t* stream, ssize_t len, const uv_buf_t* read)
{
    auto gc  = hx::AutoGCZone();
    auto ctx = static_cast<Ctx*>(stream->data);

    if (len <= 0)
    {
        ctx->reject(len);

        return;
    }

    ctx->buffer.insert(ctx->buffer.end(), read->base, read->base + len);
    ctx->consume();
}
