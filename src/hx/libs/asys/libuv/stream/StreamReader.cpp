#include <hxcpp.h>
#include "StreamReader.h"

hx::asys::libuv::stream::StreamReader::QueuedRead::QueuedRead(const Array<uint8_t> _array, const int _offset, const int _length, const Dynamic _cbSuccess, const Dynamic _cbFailure)
    : BaseRequest(_cbSuccess, _cbFailure)
    , array(_array.mPtr)
    , offset(_offset)
    , length(_length)
{
}

hx::asys::libuv::stream::StreamReader::StreamReader(uv_stream_t* _stream) : stream(_stream) {}

void hx::asys::libuv::stream::StreamReader::read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    if (queue.empty())
    {
        auto alloc = [](uv_handle_t* handle, size_t suggested, uv_buf_t* buffer) {
            auto reader = static_cast<StreamReader*>(handle->data);

            buffer->base = reader->staging.data();
            buffer->len  = reader->staging.size();
        };

        auto read = [](uv_stream_t* stream, ssize_t len, const uv_buf_t* read) {
            if (read <= 0)
            {
                // TODO : what do we do? Reject all pending reads and clear the buffer?

                return;
            }

            auto gcZone = hx::AutoGCZone();
            auto reader = static_cast<StreamReader*>(stream->data);

            reader->consume(len);
        };

        auto result = uv_read_start(stream, alloc, read);
        if (result < 0)
        {
            cbFailure(uv_err_to_enum(result));

            return;
        }
    }

    queue.emplace_back(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::stream::StreamReader::consume(int length)
{
    buffer.insert(buffer.end(), staging.begin(), staging.begin() + length);

    auto& request = queue.front();

    if (buffer.size() >= request.length)
    {
        request.array.rooted->memcpy(request.offset, buffer.data(), request.length);

        buffer.erase(buffer.begin(), buffer.begin() + request.length);

        Dynamic(request.cbSuccess.rooted)(request.length);

        queue.pop_front();

        if (queue.empty())
        {
            uv_read_stop(stream);
        }
    }
}