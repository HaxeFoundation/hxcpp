#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include <functional>
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

    void check_type_callback(int type, uv_fs_t* request)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto spRequest = hx::asys::libuv::unique_fs_req(request);

        if (spRequest->result == UV_ENOENT)
        {
            Dynamic(spData->cbSuccess.rooted)(false);
        }
        else if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)((spRequest->statbuf.st_mode & S_IFMT) == type);
        }
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

void hx::asys::filesystem::Directory_obj::isDirectory(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto wrapper  = [](uv_fs_t* request) { check_type_callback(S_IFDIR, request); };

    auto result = uv_fs_stat(libuvCtx->uvLoop, request.get(), path.utf8_str(), wrapper);

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

void hx::asys::filesystem::Directory_obj::isFile(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto wrapper  = [](uv_fs_t* request) { check_type_callback(S_IFREG, request); };

    auto result = uv_fs_stat(libuvCtx->uvLoop, request.get(), path.utf8_str(), wrapper);

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

void hx::asys::filesystem::Directory_obj::isLink(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto wrapper  = [](uv_fs_t* request) { check_type_callback(S_IFLNK, request); };

    auto result = uv_fs_stat(libuvCtx->uvLoop, request.get(), path.utf8_str(), wrapper);

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

void hx::asys::filesystem::Directory_obj::setLinkOwner(Context ctx, String path, int user, int group, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto result   = uv_fs_lchown(libuvCtx->uvLoop, request.get(), path.utf8_str(), user, group, hx::asys::libuv::basic_callback);

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

void hx::asys::filesystem::Directory_obj::link(Context ctx, String target, String path, int type, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto result   = type == 0
        ? uv_fs_link(libuvCtx->uvLoop, request.get(), target.utf8_str(), path.utf8_str(), hx::asys::libuv::basic_callback)
        : uv_fs_symlink(libuvCtx->uvLoop, request.get(), target.utf8_str(), path.utf8_str(), 0, hx::asys::libuv::basic_callback);

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

void hx::asys::filesystem::Directory_obj::linkInfo(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto wrapper  = [](uv_fs_t* request) {
        auto spRequest = hx::asys::libuv::unique_fs_req(request);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            auto statBuf = hx::Anon_obj::Create(13);
            statBuf->setFixed( 0, HX_CSTRING("atime"), static_cast<int>(spRequest->statbuf.st_atim.tv_sec));
            statBuf->setFixed( 1, HX_CSTRING("mtime"), static_cast<int>(spRequest->statbuf.st_mtim.tv_sec));
            statBuf->setFixed( 2, HX_CSTRING("ctime"), static_cast<int>(spRequest->statbuf.st_ctim.tv_sec));
            statBuf->setFixed( 3, HX_CSTRING("dev"), static_cast<int>(spRequest->statbuf.st_dev));
            statBuf->setFixed( 4, HX_CSTRING("uid"), static_cast<int>(spRequest->statbuf.st_uid));
            statBuf->setFixed( 5, HX_CSTRING("gid"), static_cast<int>(spRequest->statbuf.st_gid));
            statBuf->setFixed( 6, HX_CSTRING("ino"), static_cast<int>(spRequest->statbuf.st_ino));
            statBuf->setFixed( 7, HX_CSTRING("mode"), static_cast<int>(spRequest->statbuf.st_mode));
            statBuf->setFixed( 8, HX_CSTRING("nlink"), static_cast<int>(spRequest->statbuf.st_nlink));
            statBuf->setFixed( 9, HX_CSTRING("rdev"), static_cast<int>(spRequest->statbuf.st_rdev));
            statBuf->setFixed(10, HX_CSTRING("size"), static_cast<int>(spRequest->statbuf.st_size));
            statBuf->setFixed(11, HX_CSTRING("blksize"), static_cast<int>(spRequest->statbuf.st_blksize));
            statBuf->setFixed(12, HX_CSTRING("blocks"), static_cast<int>(spRequest->statbuf.st_blocks));

            Dynamic(spData->cbSuccess.rooted)(statBuf);
        }
    };

    auto result = uv_fs_lstat(libuvCtx->uvLoop, request.get(), path.utf8_str(), wrapper);

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

void hx::asys::filesystem::Directory_obj::readLink(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();
    auto wrapper  = [](uv_fs_t* request) {
        auto spRequest = hx::asys::libuv::unique_fs_req(request);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(String::create(static_cast<const char*>(spRequest->ptr)));
        }
    };

    auto result = uv_fs_readlink(libuvCtx->uvLoop, request.get(), path.utf8_str(), wrapper);

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