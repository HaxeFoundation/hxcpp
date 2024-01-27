#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include <cstring>
#include <limits>
#include "LibuvFile.h"
#include "FsRequest.h"

namespace
{
    struct WriteRequest final : hx::asys::libuv::filesystem::FsRequest
    {
    private:
        hx::ArrayPin* pin;

    public:
        const uv_buf_t buffer;

        WriteRequest(hx::ArrayPin* _pin, int _offset, int _length, Dynamic _cbSuccess, Dynamic _cbFailure)
            : FsRequest(_cbSuccess, _cbFailure)
            , pin(_pin)
            , buffer(uv_buf_init(pin->GetBase() + _offset, _length))
        {
            //
        }

        ~WriteRequest()
        {
           delete pin;
        }
    };

    struct ReadRequest final : hx::asys::libuv::filesystem::FsRequest
    {
    private:
        std::vector<char> staging;

    public:
        const uv_buf_t buffer;
        const hx::RootedObject<Array_obj<std::uint8_t>> rooted;
        const int offset;
        const int length;
        const int64_t pos;

        int progress;

        ReadRequest(Dynamic _cbSuccess, Dynamic _cbFailure, Array<std::uint8_t> _array, const int _offset, const int _length, const int64_t _pos)
            : FsRequest(_cbSuccess, _cbFailure)
            , staging(std::numeric_limits<uint16_t>::max())
            , buffer(uv_buf_init(staging.data(), staging.capacity()))
            , rooted(_array.mPtr)
            , offset(_offset)
            , length(_length)
            , pos(_pos)
            , progress(0)
        {
            //
        }
    };

    void onReadCallback(uv_fs_t* request)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<ReadRequest>(static_cast<ReadRequest*>(request->data));

        if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            if (spRequest->uv.result == 0)
            {
                Dynamic(spRequest->cbSuccess.rooted)(spRequest->progress);
            }
            else
            {
                auto count = std::min(spRequest->uv.result, static_cast<ssize_t>(spRequest->length) - spRequest->progress);

                spRequest->rooted.rooted->memcpy(spRequest->offset + spRequest->progress, reinterpret_cast<uint8_t*>(spRequest->buffer.base), count);

                if (spRequest->progress + count >= static_cast<int64_t>(spRequest->length))
                {
                    Dynamic(spRequest->cbSuccess.rooted)(spRequest->progress + count);
                }
                else
                {
                    spRequest->progress += count;

                    auto newFilePos = spRequest->pos + spRequest->progress;
                    auto result     = uv_fs_read(spRequest->uv.loop, &spRequest->uv, static_cast<uv_file>(spRequest->uv.file.fd), &spRequest->buffer, 1, newFilePos, onReadCallback);

                    if (result < 0)
                    {
                        Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));
                    }
                    else
                    {
                        spRequest.release();
                    }
                }
            }
        }
    }

    void onOpenCallback(uv_fs_t* request)
    {
        auto gcZone = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<hx::asys::libuv::filesystem::FsRequest>(static_cast<hx::asys::libuv::filesystem::FsRequest*>(request->data));

        if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            Dynamic(spRequest->cbSuccess.rooted)(hx::asys::filesystem::File(new hx::asys::libuv::filesystem::LibuvFile_obj(spRequest->uv.loop, spRequest->uv.result, String::create(spRequest->uv.path))));
        }
    }

    int openFlag(int flag)
    {
        switch (flag)
        {
        case 0:
            return O_WRONLY | O_APPEND | O_CREAT;
        case 1:
            return O_RDONLY;
        case 2:
            return O_RDWR;
        case 3:
            return O_WRONLY | O_CREAT | O_TRUNC;
        case 4:
            return O_WRONLY | O_CREAT | O_EXCL;
        case 5:
            return O_RDWR | O_CREAT | O_TRUNC;
        case 6:
            return O_RDWR | O_CREAT | O_EXCL;
        case 7:
            return O_WRONLY | O_CREAT;
        case 8:
            return O_RDWR | O_CREAT;
        default:
            hx::Throw(HX_CSTRING("Unknown open flag"));

            return 0;
        }
    }

    int openMode(int flag)
    {
        switch (flag)
        {
        case 0:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            return 420;
        default:
            return 0;
        }
    }
}

void hx::asys::filesystem::File_obj::open(Context ctx, String path, int flags, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);
    auto result   = uv_fs_open(libuvCtx->uvLoop, &request->uv, path.utf8_str(), openFlag(flags), openMode(flags), onOpenCallback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::filesystem::File_obj::temp(Context ctx, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto size     = size_t(1);
    auto nullchar = '\0';
    auto result   = uv_os_tmpdir(&nullchar, &size);
    if (result < 0 && result != UV_ENOBUFS)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }

    auto buffer = std::vector<char>(size);
    result = uv_os_tmpdir(buffer.data(), &size);
    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }

    auto path     = std::filesystem::path(buffer.data()) / std::filesystem::path("XXXXXX");
    auto libuvCtx = hx::asys::libuv::context(ctx);
    auto request  = std::make_unique<hx::asys::libuv::filesystem::FsRequest>(cbSuccess, cbFailure);

    result = uv_fs_mkstemp(libuvCtx->uvLoop, &request->uv, path.u8string().c_str(), onOpenCallback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

hx::asys::libuv::filesystem::LibuvFile_obj::LibuvFile_obj(uv_loop_t* _loop, uv_file _file, const String _path)
    : File_obj(_path)
    , loop(_loop)
    , file(_file) {}

void hx::asys::libuv::filesystem::LibuvFile_obj::write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<WriteRequest>(data->Pin(), offset, length, cbSuccess, cbFailure);
    auto result  = uv_fs_write(loop, &request->uv, file, &request->buffer, 1, pos, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::read(::cpp::Int64 pos, Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<ReadRequest>(cbSuccess, cbFailure, output, offset, length, pos);
    auto result  = uv_fs_read(loop, &request->uv, file, &request->buffer, 1, pos, onReadCallback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::info(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto wrapper = [](uv_fs_t* request) {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = std::unique_ptr<FsRequest>(static_cast<FsRequest*>(request->data));

        if (spRequest->uv.result < 0)
        {
            Dynamic(spRequest->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->uv.result));
        }
        else
        {
            auto statBuf = hx::Anon_obj::Create();
            statBuf->__SetField(HX_CSTRING("atime"), static_cast<int>(spRequest->uv.statbuf.st_atim.tv_sec), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("mtime"), static_cast<int>(spRequest->uv.statbuf.st_mtim.tv_sec), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("ctime"), static_cast<int>(spRequest->uv.statbuf.st_ctim.tv_sec), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("dev"), static_cast<int>(spRequest->uv.statbuf.st_dev), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("uid"), static_cast<int>(spRequest->uv.statbuf.st_uid), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("gid"), static_cast<int>(spRequest->uv.statbuf.st_gid), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("ino"), static_cast<int>(spRequest->uv.statbuf.st_ino), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("mode"), static_cast<int>(spRequest->uv.statbuf.st_mode), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("nlink"), static_cast<int>(spRequest->uv.statbuf.st_nlink), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("rdev"), static_cast<int>(spRequest->uv.statbuf.st_rdev), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("size"), static_cast<int>(spRequest->uv.statbuf.st_size), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("blksize"), static_cast<int>(spRequest->uv.statbuf.st_blksize), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("blocks"), static_cast<int>(spRequest->uv.statbuf.st_blocks), hx::PropertyAccess::paccDynamic);

            Dynamic(spRequest->cbSuccess.rooted)(statBuf);
        }
        };

    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result = uv_fs_fstat(loop, &request->uv, file, wrapper);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::resize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_ftruncate(loop, &request->uv, file, size, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::setPermissions(int permissions, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_fchmod(loop, &request->uv, file, permissions, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::setOwner(int user, int group, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_fchown(loop, &request->uv, file, user, group, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::setTimes(int accessTime, int modificationTime, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_futime(loop, &request->uv, file, accessTime, modificationTime, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_fsync(loop, &request->uv, file, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::close(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<FsRequest>(cbSuccess, cbFailure);
    auto result  = uv_fs_close(loop, &request->uv, file, FsRequest::callback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::__Mark(hx::MarkContext* __inCtx)
{
    HX_MARK_MEMBER(path);
}

#ifdef HXCPP_VISIT_ALLOCS

void hx::asys::libuv::filesystem::LibuvFile_obj::__Visit(hx::VisitContext* __inCtx)
{
    HX_VISIT_MEMBER(path);
}

#endif