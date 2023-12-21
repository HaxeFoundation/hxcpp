#include <hxcpp.h>
#include "StreamWriter.h"
#include <cstring>

namespace
{
    void onWriteCallback(uv_write_t* request, int result)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_write_t>(request);
        auto writer    = static_cast<hx::asys::libuv::stream::StreamWriter*>(spRequest->data);

        if (result < 0)
        {
            auto front = writer->popFront();

            Dynamic(front->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));

            writer->doFlush(front->id);

            return;
        }

        auto& front = writer->writeQueue.front();

        front->progress += front->staging.size();

        if (front->progress < front->length)
        {
            writer->consume();
        }
        else
        {
            Dynamic(front->cbSuccess.rooted)(front->length);

            writer->doFlush(front->id);

            auto _ = writer->popFront();

            writer->consume();
        }
    }
}

hx::asys::libuv::stream::QueuedWrite::QueuedWrite(Array<uint8_t> _array, const int _offset, const int _length, const int _id, Dynamic _cbSuccess, Dynamic _cbFailure)
    : BaseRequest(_cbSuccess, _cbFailure)
    , array(_array.mPtr)
    , offset(_offset)
    , length(_length)
    , id(_id)
    , progress(0)
    , staging(std::vector<char>()) {}

hx::asys::libuv::stream::StreamWriter::StreamWriter(uv_stream_t* const _stream)
    : stream(_stream)
    , writeQueue(std::deque<std::unique_ptr<QueuedWrite>>())
    , flushQueue(std::deque<std::unique_ptr<QueuedFlush>>())
    , seed(0) {}

hx::asys::libuv::stream::QueuedFlush::QueuedFlush(const int _id, Dynamic _cbSuccess, Dynamic _cbFailure)
    : BaseRequest(_cbSuccess, _cbFailure)
    , id(_id) {}

void hx::asys::libuv::stream::StreamWriter::write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto startConsuming = writeQueue.empty();

    writeQueue.push_back(std::make_unique<QueuedWrite>(input, offset, length, seed++, cbSuccess, cbFailure));

    if (startConsuming)
    {
        consume();
    }
}

 //void hx::asys::libuv::stream::StreamWriter::close(Dynamic cbSuccess, Dynamic cbFailure)
 //{
 //    //
 //}

void hx::asys::libuv::stream::StreamWriter::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    if (writeQueue.empty())
    {
        cbSuccess();

        return;
    }

    auto& back = writeQueue.back();
    auto  id   = back->id;

    flushQueue.push_back(std::make_unique<QueuedFlush>(id, cbSuccess, cbFailure));
}

std::unique_ptr<hx::asys::libuv::stream::QueuedWrite> hx::asys::libuv::stream::StreamWriter::popFront()
{
    auto front = std::unique_ptr<hx::asys::libuv::stream::QueuedWrite>(std::move(writeQueue.front()));

    writeQueue.pop_front();

    return front;
}

void hx::asys::libuv::stream::StreamWriter::consume()
{
    if (writeQueue.empty())
    {
        return;
    }

    auto& front   = writeQueue.front();
    auto  request = std::make_unique<uv_write_t>();
    auto  size    = std::min(CHUNK_SIZE, front->length - front->progress);

    front->staging.resize(size);

    auto buffer = uv_buf_init(front->staging.data(), size);

    std::memcpy(front->staging.data(), front->array.rooted->GetBase() + front->offset + front->progress, size);

    auto result = uv_write(request.get(), stream, &buffer, 1, onWriteCallback);
    if (result < 0)
    {
        Dynamic(front->cbFailure.rooted)(uv_err_to_enum(result));

        writeQueue.pop_front();

        return;
    }

    request.release()->data = this;
}

void hx::asys::libuv::stream::StreamWriter::doFlush(int id)
{
    while (!flushQueue.empty() && flushQueue.front()->id <= id)
    {
        auto front = std::unique_ptr<QueuedFlush>(std::move(flushQueue.front()));

        flushQueue.pop_front();

        Dynamic(front->cbSuccess.rooted)();
    }
}