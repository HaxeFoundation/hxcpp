#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include "../LibuvUtils.h"

namespace
{
    hx::asys::filesystem::FileAccessMode operator&(hx::asys::filesystem::FileAccessMode lhs, hx::asys::filesystem::FileAccessMode rhs)
    {
        return
            static_cast<hx::asys::filesystem::FileAccessMode>(
                static_cast<std::underlying_type_t<hx::asys::filesystem::FileAccessMode>>(lhs) &
                static_cast<std::underlying_type_t<hx::asys::filesystem::FileAccessMode>>(rhs));
    }

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
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_closedir(loop, request.get(), dir, hx::asys::libuv::basic_callback);

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
        auto result = uv_fs_mkdir(libuvCtx->uvLoop, request.get(), path.utf8_str(), permissions, hx::asys::libuv::basic_callback);
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

void hx::asys::filesystem::Directory_obj::move(Context ctx, String oldPath, String newPath, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto result   = uv_fs_rename(libuvCtx->uvLoop, request.get(), oldPath.utf8_str(), newPath.utf8_str(), hx::asys::libuv::basic_callback);

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

void hx::asys::filesystem::Directory_obj::check(Context ctx, String path, FileAccessMode accessMode, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto wrapper  = [](uv_fs_t* request) {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto spRequest = hx::asys::libuv::unique_fs_req(request);

        if (spRequest->result == UV_ENOENT || spRequest->result == UV_EACCES)
        {
            Dynamic(spData->cbSuccess.rooted)(false);
        }
        else if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(true);
        }
    };

    auto mode = 0;

    if (static_cast<bool>(accessMode & hx::asys::filesystem::FileAccessMode::exists))
    {
        mode |= F_OK;
    }
    if (static_cast<bool>(accessMode & hx::asys::filesystem::FileAccessMode::executable))
    {
        mode |= X_OK;
    }
    if (static_cast<bool>(accessMode & hx::asys::filesystem::FileAccessMode::writable))
    {
        mode |= W_OK;
    }
    if (static_cast<bool>(accessMode & hx::asys::filesystem::FileAccessMode::readable))
    {
        mode |= R_OK;
    }

    auto result = uv_fs_access(libuvCtx->uvLoop, request.get(), path.utf8_str(), mode, wrapper);

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

void hx::asys::filesystem::Directory_obj::deleteFile(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto result   = uv_fs_unlink(libuvCtx->uvLoop, request.get(), path.utf8_str(), hx::asys::libuv::basic_callback);

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

void hx::asys::filesystem::Directory_obj::deleteDirectory(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto result   = uv_fs_rmdir(libuvCtx->uvLoop, request.get(), path.utf8_str(), hx::asys::libuv::basic_callback);

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