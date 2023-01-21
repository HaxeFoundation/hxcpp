#include <hxcpp.h>
#include "Streams.h"
#include "../LibuvUtils.h"

void hx::asys::libuv::stream::write(uv_stream_t* handle, Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto staging = std::vector<char>(length);
    auto buffer  = uv_buf_init(staging.data(), staging.size());

    std::memcpy(staging.data(), input->getBase() + offset, length);

    auto request = std::make_unique<uv_write_t>();
    auto wrapper = [](uv_write_t* request, int status) {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<uv_write_t>(request);
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