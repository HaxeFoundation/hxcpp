#include <hxcpp.h>
#include "Streams.h"
#include "../LibuvUtils.h"

namespace
{
    using uv_write_close_cb = void(*)(uv_write_t*);

    void close_write(uv_write_t* request)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(request), &hx::asys::libuv::clean_handle);
    }

    void close_read(uv_read_t* request)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(request), &hx::asys::libuv::clean_handle);
    }
}

void hx::asys::libuv::stream::write(uv_stream_t* handle, Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto staging = std::vector<char>(length);
    auto buffer  = uv_buf_init(staging.data(), staging.size());

    std::memcpy(staging.data(), input->getBase() + offset, length);

    auto request = std::make_unique<uv_write_t>();
    auto wrapper = [](uv_write_t* request, int status) {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_write_t, uv_write_close_cb>(request, &close_write);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));

        if (status < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)();
        }
    };

    auto result = uv_write(request.get(), handle, &buffer, 1, wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request->data = new hx::asys::libuv::BaseRequest(cbSuccess, cbFailure);
        request.release();
    }
}