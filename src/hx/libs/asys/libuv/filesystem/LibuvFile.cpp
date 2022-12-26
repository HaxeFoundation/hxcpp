#include <hxcpp.h>
#include <memory>
#include <filesystem>
#include "../LibuvUtils.h"

namespace
{
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

    void basicCallback(uv_fs_t* request)
    {
        auto spRequest = hx::asys::libuv::unique_fs_req(request);
        auto spData    = std::unique_ptr<hx::asys::libuv::BaseRequest>(static_cast<hx::asys::libuv::BaseRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)();
        }
    }

    class LibuvFile_obj : public hx::asys::filesystem::File_obj
    {
    private:
        uv_loop_t* loop;
        uv_file file;

        struct WriteRequest : hx::asys::libuv::BaseRequest
        {
            std::unique_ptr<std::vector<char>> staging;

            // The current offset into the array.
            // May differ from array offset if multiple smaller requests are needed to complete the overall write.
            const int currentOffset;

            // The starting offset into the array for the overall write.
            const int arrayOffset;

            // The total length for the overall write.
            const int arrayLength;

            // The starting position into the file for the overall write.
            const int filePos;

            const hx::RootedObject<Array_obj<uint8_t>> array;

            WriteRequest(
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

        static void onWriteCallback(uv_fs_t* request)
        {
            auto gcZone    = hx::AutoGCZone();
            auto spData    = std::unique_ptr<WriteRequest>(static_cast<WriteRequest*>(request->data));
            auto spRequest = hx::asys::libuv::unique_fs_req(request);

            if (spRequest->result < 0)
            {
                Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
            }
            else
            {
                auto array          = Array<uint8_t>(spData->array.rooted);
                auto newArrayOffset = spData->currentOffset + spRequest->result;

                if (newArrayOffset >= spData->arrayOffset + spData->arrayLength)
                {
                    Dynamic(spData->cbSuccess.rooted)(spData->arrayLength);
                }
                else
                {
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
                            new WriteRequest(
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
    public:
        LibuvFile_obj(uv_loop_t* _loop, uv_file _file, const String _path) : File_obj(_path), loop(_loop), file(_file) {}

        void write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            // Create an intermediate buffer so libuv isn't handling GC memory which could be moved from under it.
            // In the case the data being written is larger than the batch buffer multiple write requests are made.
            auto batchSize  = static_cast<int>(std::numeric_limits<uint16_t>::max());
            auto bufferSize = std::min(batchSize, length);
            auto staging    = std::make_unique<std::vector<char>>(bufferSize);
            auto buffer     = uv_buf_init(staging->data(), staging->capacity());

            std::memcpy(staging->data(), data->getBase() + offset, bufferSize);

            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_write(loop, request.get(), file, &buffer, 1, pos, &LibuvFile_obj::onWriteCallback);

            if (result < 0)
            {
                cbFailure(hx::asys::libuv::uv_err_to_enum(result));
            }
            else
            {
                request->data = new WriteRequest(std::move(staging), offset, offset, length, pos, data, cbSuccess, cbFailure);
                request.release();
            }
        }
        void read(::cpp::Int64 pos, Array<uint8_t> buffer, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct ReadRequest : hx::asys::libuv::BaseRequest
            {
                const int offset;
                const hx::RootedObject<hx::Object> array;
                const std::unique_ptr<std::vector<char>> staging;
                const std::unique_ptr<uv_buf_t> buffer;

                ReadRequest(int _offset, Array<uint8_t> _array, Dynamic _cbSuccess, Dynamic _cbFailure, std::unique_ptr<std::vector<char>> _staging, std::unique_ptr<uv_buf_t> _buffer)
                    : BaseRequest(_cbSuccess, _cbFailure), offset(_offset), array(_array.mPtr), staging(std::move(_staging)), buffer(std::move(_buffer)) { }
            };

            if (pos < 0)
            {
                cbFailure(hx::asys::libuv::create(HX_CSTRING("CustomError"), 13, 1)->_hx_init(0, HX_CSTRING("Position is negative")));
            }
            if (offset < 0 || offset > buffer->length)
            {
                cbFailure(hx::asys::libuv::create(HX_CSTRING("CustomError"), 13, 1)->_hx_init(0, HX_CSTRING("Offset outside of buffer bounds")));
            }

            auto wrapper  = [](uv_fs_t* request) {
                auto spRequest = hx::asys::libuv::unique_fs_req(request);
                auto spData    = std::unique_ptr<ReadRequest>(static_cast<ReadRequest*>(request->data));
                auto gcZone    = hx::AutoGCZone();

                if (request->result > 0)
                {
                    auto src    = reinterpret_cast<uint8_t*>(spData->staging->data());
                    auto dest   = Array<uint8_t>(Dynamic(spData->array.rooted));
                    auto length = spData->staging->capacity();

                    dest->memcpy(spData->offset, src, length);

                    Dynamic(spData->cbSuccess.rooted)(request->result);
                }
                else
                {
                    Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(spRequest->result));
                }
            };

            auto staging  = std::make_unique<std::vector<char>>(buffer->length);
            auto uvBuffer = std::make_unique<uv_buf_t>(uv_buf_init(staging->data(), staging->capacity()));
            auto request  = std::make_unique<uv_fs_t>();
            auto result   = uv_fs_read(loop, request.get(), file, uvBuffer.get(), 1, pos, wrapper);
            
            if (result < 0)
            {
                cbFailure(hx::asys::libuv::uv_err_to_enum(result));
            }
            else
            {
                request->data = new ReadRequest(offset, buffer, cbSuccess, cbFailure, std::move(staging), std::move(uvBuffer));
                request.release();
            }
        }
        void info(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto wrapper = [](uv_fs_t* request) {
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
                    statBuf->setFixed( 0, HX_CSTRING("atime"), spRequest->statbuf.st_atim.tv_sec);
                    statBuf->setFixed( 1, HX_CSTRING("mtime"), spRequest->statbuf.st_mtim.tv_sec);
                    statBuf->setFixed( 2, HX_CSTRING("ctime"), spRequest->statbuf.st_ctim.tv_sec);
                    statBuf->setFixed( 3, HX_CSTRING("dev"), spRequest->statbuf.st_dev);
                    statBuf->setFixed( 4, HX_CSTRING("uid"), spRequest->statbuf.st_uid);
                    statBuf->setFixed( 5, HX_CSTRING("gid"), spRequest->statbuf.st_gid);
                    statBuf->setFixed( 6, HX_CSTRING("ino"), spRequest->statbuf.st_ino);
                    statBuf->setFixed( 7, HX_CSTRING("mode"), spRequest->statbuf.st_mode);
                    statBuf->setFixed( 8, HX_CSTRING("nlink"), spRequest->statbuf.st_nlink);
                    statBuf->setFixed( 9, HX_CSTRING("rdev"), spRequest->statbuf.st_rdev);
                    statBuf->setFixed(10, HX_CSTRING("size"), spRequest->statbuf.st_size);
                    statBuf->setFixed(11, HX_CSTRING("blksize"), spRequest->statbuf.st_blksize);
                    statBuf->setFixed(12, HX_CSTRING("blocks"), spRequest->statbuf.st_blocks);

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
        void resize(int size, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_ftruncate(loop, request.get(), file, size, basicCallback);

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
        void setPermissions(int permissions, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_fchmod(loop, request.get(), file, permissions, basicCallback);

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
        void setOwner(int user, int group, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_fchown(loop, request.get(), file, user, group, basicCallback);

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
        void setTimes(int accessTime, int modificationTime, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_futime(loop, request.get(), file, accessTime, modificationTime, basicCallback);

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
        void flush(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_fsync(loop, request.get(), file, basicCallback);

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
        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_close(loop, request.get(), file, basicCallback);

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

        void __Mark(hx::MarkContext *__inCtx)
        {
            HX_MARK_MEMBER(path);
        }
#ifdef HXCPP_VISIT_ALLOCS
        void __Visit(hx::VisitContext *__inCtx)
        {
            HX_VISIT_MEMBER(path);
        }
#endif
    };
}

void hx::asys::filesystem::File_obj::open(Context ctx, String path, int flags, Dynamic cbSuccess, Dynamic cbFailure)
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
            Dynamic(spData->cbSuccess.rooted)(File(new LibuvFile_obj(spRequest->loop, spRequest->result, String::create(spRequest->path))));
        }
    };

    auto request = std::make_unique<uv_fs_t>();
    auto result  = uv_fs_open(libuvCtx->uvLoop, request.get(), path.utf8_str(), openFlag(flags), openMode(flags), wrapper);

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
            Dynamic(spData->cbSuccess.rooted)(File(new LibuvFile_obj(spRequest->loop, spRequest->result, String::create(spRequest->path))));
        }
    };

    auto request = std::make_unique<uv_fs_t>();
    result = uv_fs_mkstemp(libuvCtx->uvLoop, request.get(), path.u8string().c_str(), wrapper);

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
