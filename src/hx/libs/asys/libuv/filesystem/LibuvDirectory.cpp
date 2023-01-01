#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include "../LibuvUtils.h"

namespace
{
    class LibuvDirectory_obj : public hx::asys::filesystem::Directory_obj
    {
    public:
        uv_loop_t* loop;
        uv_dir_t* dir;

        LibuvDirectory_obj(uv_loop_t* _loop, uv_dir_t* _dir)
            : loop(_loop), dir(_dir) {}

        void next(const int batch, Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct NextEntriesRequest : public hx::asys::libuv::BaseRequest
            {
                const std::unique_ptr<std::vector<uv_dirent_t>> entries;

                NextEntriesRequest(Dynamic cbSuccess, Dynamic cbFailure, std::unique_ptr<std::vector<uv_dirent_t>> _entries)
                    : BaseRequest(cbSuccess, cbFailure), entries(std::move(_entries)) {}
            };

            auto wrapper = [](uv_fs_t* request) {
                auto gcZone    = hx::AutoGCZone();
                auto spData    = std::unique_ptr<NextEntriesRequest>(static_cast<NextEntriesRequest*>(request->data));
                auto spRequest = hx::asys::libuv::unique_fs_req(request);

                if (spRequest->result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
                }
                else
                {
                    auto entries = Array<String>(0, 0);

                    for (auto i = 0; i < spRequest->result; i++)
                    {
                        if (nullptr != spData->entries->at(i).name)
                        {
                            entries->push(String::create(spData->entries->at(i).name));
                        }
                    }

                    Dynamic(spData->cbSuccess.rooted)(entries);
                }
            };

            auto entries = std::make_unique<std::vector<uv_dirent_t>>(batch);
            dir->nentries = entries->capacity();
            dir->dirents  = entries->data();

            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_readdir(loop, request.get(), dir, wrapper);

            if (result < 0)
            {
                cbFailure(hx::asys::libuv::uv_err_to_enum(result));
            }
            else
            {
                request->data = new NextEntriesRequest(cbSuccess, cbFailure, std::move(entries));
                request.release();
            }
        }
        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto wrapper = [](uv_fs_t* request) {
                auto gcZone    = hx::AutoGCZone();
                auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
                auto spRequest = hx::asys::libuv::unique_fs_req(request);

                if (spRequest->result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
                }
                else
                {
                    Dynamic(spData->cbSuccess.rooted)();
                }
            };

            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_closedir(loop, request.get(), dir, wrapper);

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
    };
}

void hx::asys::filesystem::Directory_obj::open(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto wrapper  = [](uv_fs_t* request) {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto spRequest = hx::asys::libuv::unique_fs_req(request);

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(Directory(new LibuvDirectory_obj(spRequest->loop, static_cast<uv_dir_t*>(spRequest->ptr))));
        }
    };

    auto request = std::make_unique<uv_fs_t>();
    auto result  = uv_fs_opendir(libuvCtx->uvLoop, request.get(), path.utf8_str(), wrapper);

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

void hx::asys::filesystem::Directory_obj::create(Context ctx, String path, int permissions, bool recursive, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();

    if (recursive)
    {
        // TODO : This isn't really async, eventually move this loop into haxe.

        hx::EnterGCFreeZone();

        auto filePath    = std::filesystem::u8path(path.utf8_str());
        auto accumulated = std::filesystem::path();

        for (auto&& part : filePath)
        {
            if (accumulated.empty())
            {
                accumulated = part;
            }
            else
            {
                accumulated = accumulated / part;
            }

            auto result = uv_fs_mkdir(libuvCtx->uvLoop, request.get(), accumulated.u8string().c_str(), permissions, nullptr);
            if (result < 0 && result != EEXIST)
            {
                hx::ExitGCFreeZone();

                cbFailure(result);

                return;
            }
        }

        hx::ExitGCFreeZone();

        cbSuccess();
    }
    else
    {
        auto wrapper = [](uv_fs_t* request) {
            auto gcZone    = hx::AutoGCZone();
            auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
            auto spRequest = hx::asys::libuv::unique_fs_req(request);

            if (spRequest->result < 0)
            {
                Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
            }
            else
            {
                Dynamic(spData->cbSuccess.rooted)();
            }
        };

        auto result = uv_fs_mkdir(libuvCtx->uvLoop, request.get(), path.utf8_str(), permissions, wrapper);
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
}