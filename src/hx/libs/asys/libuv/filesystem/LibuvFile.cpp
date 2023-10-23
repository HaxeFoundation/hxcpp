#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include "LibuvFile.h"

namespace
{
    struct ChunkedRequest final : hx::asys::libuv::BaseRequest
    {
        std::unique_ptr<std::vector<char>> staging;

        // The current offset into the array.
        // May differ from array offset if multiple smaller chunks are needed to complete the overall request.
        const int currentOffset;

        // The starting offset into the array for the overall request.
        const int arrayOffset;

        // The total length for the overall request.
        const int arrayLength;

        // The starting position into the file for the overall request.
        const int filePos;

        const hx::RootedObject<Array_obj<uint8_t>> array;

        ChunkedRequest(
            std::unique_ptr<std::vector<char>> _staging,
            const int _currentOffset,
            const int _arrayOffset,
            const int _arrayLength,
            const int _filePos,
            const Array<uint8_t> _array,
            Dynamic _cbSuccess,
            Dynamic _cbFailure)
            : BaseRequest(_cbSuccess, _cbFailure)
            , staging(std::move(_staging))
            , currentOffset(_currentOffset)
            , arrayOffset(_arrayOffset)
            , arrayLength(_arrayLength)
            , filePos(_filePos)
            , array(_array.mPtr) {}
    };

    void onWriteCallback(uv_fs_t* request)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<ChunkedRequest>(static_cast<ChunkedRequest*>(request->data));
        auto spRequest = hx::asys::libuv::unique_fs_req(request);

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            auto newArrayOffset = static_cast<int>(spData->currentOffset + spRequest->result);

            if (0 == spRequest->result || newArrayOffset >= spData->arrayOffset + spData->arrayLength)
            {
                Dynamic(spData->cbSuccess.rooted)(newArrayOffset - spData->arrayOffset);
            }
            else
            {
                auto array         = Array<uint8_t>(spData->array.rooted);
                auto amountWritten = newArrayOffset - spData->arrayOffset;
                auto batchSize     = std::min(spData->staging->capacity(), static_cast<size_t>(spData->arrayLength - amountWritten));

                std::memcpy(spData->staging->data(), array->getBase() + newArrayOffset, batchSize);

                auto buffer     = uv_buf_init(spData->staging->data(), batchSize);
                auto newFilePos = spData->filePos + amountWritten;
                auto result     = uv_fs_write(spRequest->loop, spRequest.get(), spRequest->file.fd, &buffer, 1, newFilePos, onWriteCallback);

                if (result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));
                }
                else
                {
                    spRequest->data =
                        new ChunkedRequest(
                            std::move(spData->staging),
                            newArrayOffset,
                            spData->arrayOffset,
                            spData->arrayLength,
                            spData->filePos,
                            array,
                            spData->cbSuccess.rooted,
                            spData->cbFailure.rooted);
                    spRequest.release();
                }
            }
        }
    }

    void onReadCallback(uv_fs_t* request)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spRequest = hx::asys::libuv::unique_fs_req(request);
        auto spData    = std::unique_ptr<ChunkedRequest>(static_cast<ChunkedRequest*>(spRequest->data));

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            auto newArrayOffset  = static_cast<int>(spData->currentOffset + spRequest->result);
            auto totalAmountRead = newArrayOffset - spData->arrayOffset;

            if (spRequest->result == 0)
            {
                Dynamic(spData->cbSuccess.rooted)(totalAmountRead);
            }
            else
            {
                auto array = Array<uint8_t>(spData->array.rooted);

                array->memcpy(spData->currentOffset, reinterpret_cast<uint8_t*>(spData->staging->data()), spRequest->result);

                if (newArrayOffset >= spData->arrayOffset + spData->arrayLength)
                {
                    Dynamic(spData->cbSuccess.rooted)(totalAmountRead);
                }
                else
                {
                    auto batchSize  = std::min(static_cast<int>(spData->staging->capacity()), spData->arrayLength - totalAmountRead);
                    auto newFilePos = spData->filePos + totalAmountRead;
                    auto buffer     = uv_buf_init(spData->staging->data(), batchSize);
                    auto result     = uv_fs_read(spRequest->loop, spRequest.get(), spRequest->file.fd, &buffer, 1, newFilePos, onReadCallback);

                    if (result < 0)
                    {
                        Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(result));
                    }
                    else
                    {
                        spRequest->data =
                            new ChunkedRequest(
                                std::move(spData->staging),
                                newArrayOffset,
                                spData->arrayOffset,
                                spData->arrayLength,
                                spData->filePos,
                                array,
                                spData->cbSuccess.rooted,
                                spData->cbFailure.rooted);
                        spRequest.release();
                    }
                }
            }
        }
    }

    void onOpenCallback(uv_fs_t* request)
    {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto spRequest = hx::asys::libuv::unique_fs_req(request);

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(hx::asys::filesystem::File(new hx::asys::libuv::filesystem::LibuvFile_obj(spRequest->loop, spRequest->result, String::create(spRequest->path))));
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
    auto request  = std::make_unique<uv_fs_t>();
    auto result   = uv_fs_open(libuvCtx->uvLoop, request.get(), path.utf8_str(), openFlag(flags), openMode(flags), onOpenCallback);

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
    auto request  = std::make_unique<uv_fs_t>();

    result = uv_fs_mkstemp(libuvCtx->uvLoop, request.get(), path.u8string().c_str(), onOpenCallback);

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

hx::asys::libuv::filesystem::LibuvFile_obj::LibuvFile_obj(uv_loop_t* _loop, uv_file _file, const String _path)
    : File_obj(_path)
    , loop(_loop)
    , file(_file) {}

void hx::asys::libuv::filesystem::LibuvFile_obj::write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    // Create an intermediate buffer so libuv isn't handling GC memory which could be moved from under it.
    // In the case the data being written is larger than the batch buffer multiple write requests are made.
    auto batchSize = static_cast<int>(std::numeric_limits<uint16_t>::max());
    auto bufferSize = std::min(batchSize, length);
    auto staging = std::make_unique<std::vector<char>>(bufferSize);
    auto buffer = uv_buf_init(staging->data(), staging->capacity());

    std::memcpy(staging->data(), data->getBase() + offset, bufferSize);

    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_write(loop, request.get(), file, &buffer, 1, pos, onWriteCallback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request->data = new ChunkedRequest(std::move(staging), offset, offset, length, pos, data, cbSuccess, cbFailure);
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::read(::cpp::Int64 pos, Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
{
    if (pos < 0)
    {
        cbFailure(hx::asys::libuv::create(HX_CSTRING("CustomError"), 13, 1)->_hx_init(0, HX_CSTRING("Position is negative")));
    }
    if (offset < 0 || offset > output->length)
    {
        cbFailure(hx::asys::libuv::create(HX_CSTRING("CustomError"), 13, 1)->_hx_init(0, HX_CSTRING("Offset outside of buffer bounds")));
    }

    auto batchSize = static_cast<int>(std::numeric_limits<uint16_t>::max());
    auto bufferSize = std::min(batchSize, length);
    auto staging = std::make_unique<std::vector<char>>(bufferSize);
    auto buffer = uv_buf_init(staging->data(), staging->capacity());

    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_read(loop, request.get(), file, &buffer, 1, pos, onReadCallback);

    if (result < 0)
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        request->data = new ChunkedRequest(std::move(staging), offset, offset, length, pos, output, cbSuccess, cbFailure);
        request.release();
    }
}

void hx::asys::libuv::filesystem::LibuvFile_obj::info(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto wrapper = [](uv_fs_t* request) {
        auto spRequest = hx::asys::libuv::unique_fs_req(request);
        auto spData = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto gcZone = hx::AutoGCZone();

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            auto statBuf = hx::Anon_obj::Create();
            statBuf->__SetField(HX_CSTRING("atime"), static_cast<int>(spRequest->statbuf.st_atim.tv_sec), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("mtime"), static_cast<int>(spRequest->statbuf.st_mtim.tv_sec), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("ctime"), static_cast<int>(spRequest->statbuf.st_ctim.tv_sec), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("dev"), static_cast<int>(spRequest->statbuf.st_dev), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("uid"), static_cast<int>(spRequest->statbuf.st_uid), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("gid"), static_cast<int>(spRequest->statbuf.st_gid), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("ino"), static_cast<int>(spRequest->statbuf.st_ino), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("mode"), static_cast<int>(spRequest->statbuf.st_mode), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("nlink"), static_cast<int>(spRequest->statbuf.st_nlink), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("rdev"), static_cast<int>(spRequest->statbuf.st_rdev), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("size"), static_cast<int>(spRequest->statbuf.st_size), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("blksize"), static_cast<int>(spRequest->statbuf.st_blksize), hx::PropertyAccess::paccDynamic);
            statBuf->__SetField(HX_CSTRING("blocks"), static_cast<int>(spRequest->statbuf.st_blocks), hx::PropertyAccess::paccDynamic);

            Dynamic(spData->cbSuccess.rooted)(statBuf);
        }
        };

    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_fstat(loop, request.get(), file, wrapper);

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

void hx::asys::libuv::filesystem::LibuvFile_obj::resize(int size, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_ftruncate(loop, request.get(), file, size, hx::asys::libuv::basic_callback);

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

void hx::asys::libuv::filesystem::LibuvFile_obj::setPermissions(int permissions, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_fchmod(loop, request.get(), file, permissions, hx::asys::libuv::basic_callback);

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

void hx::asys::libuv::filesystem::LibuvFile_obj::setOwner(int user, int group, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_fchown(loop, request.get(), file, user, group, hx::asys::libuv::basic_callback);

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

void hx::asys::libuv::filesystem::LibuvFile_obj::setTimes(int accessTime, int modificationTime, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_futime(loop, request.get(), file, accessTime, modificationTime, hx::asys::libuv::basic_callback);

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

void hx::asys::libuv::filesystem::LibuvFile_obj::flush(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_fsync(loop, request.get(), file, hx::asys::libuv::basic_callback);

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

void hx::asys::libuv::filesystem::LibuvFile_obj::close(Dynamic cbSuccess, Dynamic cbFailure)
{
    auto request = std::make_unique<uv_fs_t>();
    auto result = uv_fs_close(loop, request.get(), file, hx::asys::libuv::basic_callback);

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