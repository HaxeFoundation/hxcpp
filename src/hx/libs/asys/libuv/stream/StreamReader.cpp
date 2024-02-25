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
        if (!buffer.empty())
        {
            queue.emplace_back(output, offset, length, cbSuccess, cbFailure);

            consume();

            return;
        }

        auto alloc = [](uv_handle_t* handle, size_t suggested, uv_buf_t* buffer) {
            auto reader = static_cast<StreamReader*>(handle->data);

            buffer->base = reader->staging.data();
            buffer->len  = reader->staging.size();
        };

        auto read = [](uv_stream_t* stream, ssize_t len, const uv_buf_t* read) {
            auto gcZone = hx::AutoGCZone();
            auto reader = static_cast<StreamReader*>(stream->data);

            if (len <= 0)
            {
                reader->reject(len);

                return;
            }

            reader->buffer.insert(reader->buffer.end(), reader->staging.begin(), reader->staging.begin() + len);
            reader->consume();
        };

        auto result = uv_read_start(stream, alloc, read);
        if (result < 0 && result != UV_EALREADY)
        {
            cbFailure(uv_err_to_enum(result));

            return;
        }
    }

    queue.emplace_back(output, offset, length, cbSuccess, cbFailure);
}

void hx::asys::libuv::stream::StreamReader::consume()
{
    while (!buffer.empty() && !queue.empty())
    {
        auto& request = queue.front();
        auto size     = std::min(int(buffer.size()), request.length);

        request.array.rooted->memcpy(request.offset, buffer.data(), size);

        buffer.erase(buffer.begin(), buffer.begin() + size);

        queue.pop_front();

        Dynamic(request.cbSuccess.rooted)(size);
    }

    if (queue.empty())
    {
        uv_read_stop(stream);
    }
}

void hx::asys::libuv::stream::StreamReader::reject(int code)
{
    buffer.clear();

    while (!queue.empty())
    {
        auto& request = queue.front();

        Dynamic(request.cbFailure.rooted)(asys::libuv::uv_err_to_enum(code));

        queue.pop_front();
    }
}