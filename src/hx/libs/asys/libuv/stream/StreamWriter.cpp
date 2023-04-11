#include <hxcpp.h>
#include "StreamWriter.h"

namespace
{
    void onWriteCallback(uv_write_t* request, int result)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_write_t>(request);
        auto writer    = static_cast<hx::asys::libuv::stream::StreamWriter*>(spRequest->data);

        if (result < 0)
        {
            auto front     = writer->popFront();
            auto cbFailure = Dynamic(front->cbFailure.rooted);

            cbFailure(hx::asys::libuv::uv_err_to_enum(result));

            return;
        }

        auto& front = writer->queue.front();

        front->progress += front->staging.size();

        if (front->progress < front->length)
        {
            writer->consume();
        }
        else
        {
            auto f = writer->popFront();

            writer->consume();

            Dynamic(f->cbSuccess.rooted)(f->length);
        }
    }
}

hx::asys::libuv::stream::QueuedWrite::QueuedWrite(Array<uint8_t> _array, int _offset, int _length, Dynamic _cbSuccess, Dynamic _cbFailure)
    : BaseRequest(_cbSuccess, _cbFailure)
    , array(_array.mPtr)
    , offset(_offset)
    , length(_length)
    , progress(0)
    , staging(std::vector<char>()) {}

hx::asys::libuv::stream::StreamWriter::StreamWriter(uv_stream_t* _stream)
    : stream(_stream)
    , queue(std::deque<std::unique_ptr<QueuedWrite>>()) {}

void hx::asys::libuv::stream::StreamWriter::write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto startConsuming = queue.empty();

    queue.push_back(std::make_unique<QueuedWrite>(input, offset, length, cbSuccess, cbFailure));

    if (startConsuming)
    {
        consume();
    }
}

void hx::asys::libuv::stream::StreamWriter::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    //
}

std::unique_ptr<hx::asys::libuv::stream::QueuedWrite> hx::asys::libuv::stream::StreamWriter::popFront()
{
    auto front = std::unique_ptr<hx::asys::libuv::stream::QueuedWrite>(std::move(queue.front()));

    queue.pop_front();

    return front;
}

void hx::asys::libuv::stream::StreamWriter::consume()
{
    if (queue.empty())
    {
        return;
    }

    auto& front   = queue.front();
    auto  request = std::make_unique<uv_write_t>();
    auto  size    = std::min(CHUNK_SIZE, front->length - front->progress);

    front->staging.resize(size);

    auto buffer = uv_buf_init(front->staging.data(), size);

    std::memcpy(front->staging.data(), front->array.rooted->GetBase() + front->offset + front->progress, size);

    auto result = uv_write(request.get(), stream, &buffer, 1, onWriteCallback);
    if (result < 0)
    {
        Dynamic(front->cbFailure.rooted)(uv_err_to_enum(result));

        queue.pop_front();

        return;
    }

    request.release()->data = this;
}