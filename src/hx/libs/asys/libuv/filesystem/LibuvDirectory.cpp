#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include <functional>
#include <array>
#include "FsRequest.h"

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
        auto spRequest = std::unique_ptr<hx::asys::libuv::filesystem::FsRequest>(static_cast<hx::asys::libuv::filesystem::FsRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->uv.result == UV_ENOENT)
        {
            Dynamic(spRequest->cbSuccess.rooted)(false);
        }
        else if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            Dynamic(spRequest->cbSuccess.rooted)((spRequest->uv.statbuf.st_mode & S_IFMT) == type);
        }
    }

    void path_callback(uv_fs_t* request)
    {
        auto spRequest = std::unique_ptr<hx::asys::libuv::filesystem::FsRequest>(static_cast<hx::asys::libuv::filesystem::FsRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            Dynamic(spRequest->cbSuccess.rooted)(String::create(static_cast<const char*>(spRequest->uv.ptr)));
        }
    }

    class NextEntriesRequest : public hx::asys::libuv::filesystem::FsRequest
    {
    public:
        std::vector<uv_dirent_t> entries;

        NextEntriesRequest(Dynamic cbSuccess, Dynamic cbFailure, const int _batch)
            : FsRequest(cbSuccess, cbFailure)
            , entries(_batch) {}
    };

    class LibuvDirectory_obj : public hx::asys::filesystem::Directory_obj
    {
    public:
        uv_loop_t* loop;
        uv_dir_t* dir;

        LibuvDirectory_obj(uv_loop_t* _loop, uv_dir_t* _dir, String _path)
            : Directory_obj(_path), loop(_loop), dir(_dir) {}

        void next(const int batch, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto wrapper = [](uv_fs_t* request) {
                auto gcZone    = hx::AutoGCZone();
                auto spRequest = static_cast<NextEntriesRequest*>(request->data);

                if (spRequest->uv.result < 0)
                {
                    Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
                }
                else
                {
                    auto entries = Array<String>(spRequest->uv.result, 0);

                    for (auto i = 0; i < spRequest->uv.result; i++)
                    {
                        if (nullptr != spRequest->entries.at(i).name)
                        {
                            entries[i] = String::create(spRequest->entries.at(i).name);
                        }
                    }

                    Dynamic(spRequest->cbSuccess.rooted)(entries);
                }
            };

            auto request = std::make_unique<NextEntriesRequest>(cbSuccess, cbFailure, batch);
            dir->nentries = request->entries.capacity();
            dir->dirents  = request->entries.data();

            auto result = uv_fs_readdir(loop, &request->uv, dir, wrapper);

            if (result < 0)
            {
                cbFailure(hx::asys::libuv::uv_err_to_enum(result));
            }
            else
            {
                request.release();
            }
        }

        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
            auto result  = uv_fs_closedir(loop, &request->uv, dir, hx::asys::libuv::filesystem::FsRequest::callback);

            if (result < 0)
            {
                cbFailure(hx::asys::libuv::uv_err_to_enum(result));
            }
            else
            {
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
        auto spRequest = std::unique_ptr<hx::asys::libuv::filesystem::FsRequest>(static_cast<hx::asys::libuv::filesystem::FsRequest*>(request->data));

        if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            Dynamic(spRequest->cbSuccess.rooted)(Directory(new LibuvDirectory_obj(spRequest->uv.loop, static_cast<uv_dir_t*>(spRequest->uv.ptr), String::create(spRequest->uv.path))));
        }
    };

    auto request = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_opendir(libuvCtx->uvLoop, &request->uv, path.utf8_str(), wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::create(Context ctx, String path, int permissions, bool recursive, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<uv_fs_t>();

    // Maybe TODO : this is not async and would be a ball ache to do so.

    auto separator = std::array<char, 2>();

#if HX_WINDOWS
    wcstombs(separator.data(), &std::filesystem::path::preferred_separator, 1);
#else
    separator[0] = std::filesystem::path::preferred_separator;
#endif

    auto items       = path.split(separator.data());
    auto accumulated = std::filesystem::path();
    auto result      = 0;

    for (auto i = 0; i < items->length - 1; i++)
    {
        if (accumulated.empty())
        {
            accumulated = items[i].utf8_str();
        }
        else
        {
            accumulated = accumulated / items[i].utf8_str();
        }

        if (!recursive)
        {
            hx::EnterGCFreeZone();

            if ((result = uv_fs_stat(libuvCtx->uvLoop, request.get(), accumulated.u8string().c_str(), nullptr)) < 0 && result != UV_EEXIST)
            {
                hx::ExitGCFreeZone();

                cbFailure(hx::asys::libuv::uv_err_to_enum(result));

                return;
            }

            hx::ExitGCFreeZone();
        }

        hx::EnterGCFreeZone();

        if ((result = uv_fs_mkdir(libuvCtx->uvLoop, request.get(), accumulated.u8string().c_str(), permissions, nullptr)) < 0 && result != UV_EEXIST)
        {
            hx::ExitGCFreeZone();

            cbFailure(hx::asys::libuv::uv_err_to_enum(result));

            return;
        }

        hx::ExitGCFreeZone();
    }

    accumulated = accumulated / items[items->length - 1].utf8_str();

    hx::EnterGCFreeZone();

    if ((result = uv_fs_mkdir(libuvCtx->uvLoop, request.get(), accumulated.u8string().c_str(), permissions, nullptr)) < 0 && result != UV_EEXIST)
    {
        hx::ExitGCFreeZone();

        cbFailure(hx::asys::libuv::uv_err_to_enum(result));

        return;
    }
    
    hx::ExitGCFreeZone();

    cbSuccess();
}

void hx::asys::filesystem::Directory_obj::rename(Context ctx, String oldPath, String newPath, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_rename(libuvCtx->uvLoop, &request->uv, oldPath.utf8_str(), newPath.utf8_str(), hx::asys::libuv::filesystem::FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::check(Context ctx, String path, FileAccessMode accessMode, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto wrapper  = [](uv_fs_t* request) {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<hx::asys::libuv::filesystem::FsRequest>(static_cast<hx::asys::libuv::filesystem::FsRequest*>(request->data));

        if (spRequest->uv.result == UV_ENOENT || spRequest->uv.result == UV_EACCES)
        {
            Dynamic(spRequest->cbSuccess.rooted)(false);
        }
        else if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            Dynamic(spRequest->cbSuccess.rooted)(true);
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

    auto result = uv_fs_access(libuvCtx->uvLoop, &request->uv, path.utf8_str(), mode, wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::deleteFile(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_unlink(libuvCtx->uvLoop, &request->uv, path.utf8_str(), hx::asys::libuv::filesystem::FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::deleteDirectory(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_rmdir(libuvCtx->uvLoop, &request->uv, path.utf8_str(), hx::asys::libuv::filesystem::FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::isDirectory(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto wrapper  = [](uv_fs_t* request) { check_type_callback(S_IFDIR, request); };

    auto result = uv_fs_stat(libuvCtx->uvLoop, &request->uv, path.utf8_str(), wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::isFile(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto wrapper  = [](uv_fs_t* request) { check_type_callback(S_IFREG, request); };

    auto result = uv_fs_stat(libuvCtx->uvLoop, &request->uv, path.utf8_str(), wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::isLink(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto wrapper  = [](uv_fs_t* request) { check_type_callback(S_IFLNK, request); };

    auto result = uv_fs_lstat(libuvCtx->uvLoop, &request->uv, path.utf8_str(), wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::setLinkOwner(Context ctx, String path, int user, int group, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_lchown(libuvCtx->uvLoop, &request->uv, path.utf8_str(), user, group, hx::asys::libuv::filesystem::FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::link(Context ctx, String target, String path, int type, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = type == 0
        ? uv_fs_link(libuvCtx->uvLoop, &request->uv, target.utf8_str(), path.utf8_str(), hx::asys::libuv::filesystem::FsRequest::callback)
        : uv_fs_symlink(libuvCtx->uvLoop, &request->uv, target.utf8_str(), path.utf8_str(), 0, hx::asys::libuv::filesystem::FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::linkInfo(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto wrapper  = [](uv_fs_t* request) {
        auto spRequest = std::unique_ptr<hx::asys::libuv::filesystem::FsRequest>(static_cast<hx::asys::libuv::filesystem::FsRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            auto statBuf = hx::Anon_obj::Create(13);
            statBuf->setFixed( 0, HX_CSTRING("atime"), static_cast<int>(spRequest->uv.statbuf.st_atim.tv_sec));
            statBuf->setFixed( 1, HX_CSTRING("mtime"), static_cast<int>(spRequest->uv.statbuf.st_mtim.tv_sec));
            statBuf->setFixed( 2, HX_CSTRING("ctime"), static_cast<int>(spRequest->uv.statbuf.st_ctim.tv_sec));
            statBuf->setFixed( 3, HX_CSTRING("dev"), static_cast<int>(spRequest->uv.statbuf.st_dev));
            statBuf->setFixed( 4, HX_CSTRING("uid"), static_cast<int>(spRequest->uv.statbuf.st_uid));
            statBuf->setFixed( 5, HX_CSTRING("gid"), static_cast<int>(spRequest->uv.statbuf.st_gid));
            statBuf->setFixed( 6, HX_CSTRING("ino"), static_cast<int>(spRequest->uv.statbuf.st_ino));
            statBuf->setFixed( 7, HX_CSTRING("mode"), static_cast<int>(spRequest->uv.statbuf.st_mode));
            statBuf->setFixed( 8, HX_CSTRING("nlink"), static_cast<int>(spRequest->uv.statbuf.st_nlink));
            statBuf->setFixed( 9, HX_CSTRING("rdev"), static_cast<int>(spRequest->uv.statbuf.st_rdev));
            statBuf->setFixed(10, HX_CSTRING("size"), static_cast<int>(spRequest->uv.statbuf.st_size));
            statBuf->setFixed(11, HX_CSTRING("blksize"), static_cast<int>(spRequest->uv.statbuf.st_blksize));
            statBuf->setFixed(12, HX_CSTRING("blocks"), static_cast<int>(spRequest->uv.statbuf.st_blocks));

            Dynamic(spRequest->cbSuccess.rooted)(statBuf);
        }
    };

    auto result = uv_fs_lstat(libuvCtx->uvLoop, &request->uv, path.utf8_str(), wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::readLink(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_readlink(libuvCtx->uvLoop, &request->uv, path.utf8_str(), path_callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::copyFile(Context ctx, String source, String destination, bool overwrite, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_copyfile(libuvCtx->uvLoop, &request->uv, source.utf8_str(), destination.utf8_str(), overwrite ? 0 : UV_FS_COPYFILE_EXCL, hx::asys::libuv::filesystem::FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::Directory_obj::realPath(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_realpath(libuvCtx->uvLoop, &request->uv, path.utf8_str(), path_callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}