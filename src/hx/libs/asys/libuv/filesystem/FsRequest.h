#pragma once

#include <hxcpp.h>
#include "../LibuvUtils.h"

namespace hx::asys::libuv::filesystem
{
    struct FsRequest : hx::asys::libuv::BaseRequest
    {
    public:
        uv_fs_t uv;

        FsRequest(Dynamic _cbSuccess, Dynamic _cbFailure) : BaseRequest(_cbSuccess, _cbFailure)
        {
            uv.data = this;
        }

        virtual ~FsRequest()
        {
            uv_fs_req_cleanup(&uv);
        }

        static void callback(uv_fs_t* request)
        {
            auto gcZone    = hx::AutoGCZone();
            auto spRequest = std::unique_ptr<FsRequest>(static_cast<FsRequest*>(request->data));

            if (spRequest->uv.result < 0)
            {
                Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
            }
            else
            {
                Dynamic(spRequest->cbSuccess.rooted)(spRequest->uv.result);
            }
        }
    };
}