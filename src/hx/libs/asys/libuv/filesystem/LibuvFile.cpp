#include <hxcpp.h>
#include <uv.h>
#include <memory>
#include "../LibuvAsysContext.h"

namespace
{
    std::unique_ptr<uv_fs_t, uv_fs_cb> unique_fs_req(uv_fs_t* request)
    {
        return std::unique_ptr<uv_fs_t, uv_fs_cb>(request, [](uv_fs_t* request) {
            uv_fs_req_cleanup(request);

            delete request;
        });
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

    hx::EnumBase create(const String& name, const int index, int fields)
    {
        auto result = new (fields * sizeof(cpp::Variant)) hx::EnumBase_obj;

        result->_hx_setIdentity(name, index, fields);

        return result;
    }

    hx::EnumBase uv_err_to_enum(int code)
    {
        switch (code)
        {
            case UV_ENOENT:
                return create(HX_CSTRING("FileNotFound"), 0, 0);
            case UV_EEXIST:
                return create(HX_CSTRING("FileExists"), 1, 0);
            case UV_ESRCH:
                return create(HX_CSTRING("ProcessNotFound"), 2, 0);
            case UV_EACCES:
                return create(HX_CSTRING("AccessDenied"), 3, 0);
			case UV_ENOTDIR:
                return create(HX_CSTRING("NotDirectory"), 4, 0);
			case UV_EMFILE:
                return create(HX_CSTRING("TooManyOpenFiles"), 5, 0);
			case UV_EPIPE:
                return create(HX_CSTRING("BrokenPipe"), 6, 0);
			case UV_ENOTEMPTY:
                return create(HX_CSTRING("NotEmpty"), 7, 0);
			case UV_EADDRNOTAVAIL:
                return create(HX_CSTRING("AddressNotAvailable"), 8, 0);
			case UV_ECONNRESET:
                return create(HX_CSTRING("ConnectionReset"), 9, 0);
			case UV_ETIMEDOUT:
                return create(HX_CSTRING("TimedOut"), 10, 0);
			case UV_ECONNREFUSED:
                return create(HX_CSTRING("ConnectionRefused"), 11, 0);
			case UV_EBADF:
                return create(HX_CSTRING("BadFile"), 12, 0);
			default:
                return create(HX_CSTRING("CustomError"), 13, 1)->_hx_init(0, String::create(uv_err_name(code)));
        }
    }

    struct FileRequest
    {
        const hx::RootedObject cbSuccess;
        const hx::RootedObject cbFailure;

        FileRequest(Dynamic _cbSuccess, Dynamic _cbFailure) : cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr) {}
    };

    void basicCallback(uv_fs_t* request)
    {
        auto spRequest = unique_fs_req(request);
        auto spData    = std::unique_ptr<FileRequest>(static_cast<FileRequest*>(request->data));
        auto gcZone    = hx::AutoGCZone();

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(uv_err_to_enum(spRequest->result));
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
    public:
        LibuvFile_obj(uv_loop_t* _loop, uv_file _file) : loop(_loop), file(_file) {}

        void write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct WriteRequest : FileRequest
            {
                std::unique_ptr<std::vector<char>> data;

                WriteRequest(std::unique_ptr<std::vector<char>> _data, Dynamic _cbSuccess, Dynamic _cbFailure)
                    : FileRequest(_cbSuccess, _cbFailure), data(std::move(_data)) {}
            };

            auto wrapper = [](uv_fs_t* request) {
                auto gcZone    = hx::AutoGCZone();
                auto spData    = std::unique_ptr<WriteRequest>(static_cast<WriteRequest*>(request->data));
                auto spRequest = unique_fs_req(request);

                if (request->result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(uv_err_to_enum(spRequest->result));
                }
                else
                {
                    Dynamic(spData->cbSuccess.rooted)(request->result);
                }
            };

            
            auto staging = std::make_unique<std::vector<char>>(length);
            auto buffer  = uv_buf_init(staging->data(), staging->capacity());

            std::memcpy(staging->data(), data->getBase() + offset, length);

            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_write(loop, request.get(), file, &buffer, 1, pos, wrapper);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new WriteRequest(std::move(staging), cbSuccess, cbFailure);
                request.release();
            }
        }
        void read(::cpp::Int64 pos, Array<uint8_t> buffer, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct ReadRequest : FileRequest
            {
                const int offset;
                const hx::RootedObject array;
                const std::unique_ptr<std::vector<char>> staging;
                const std::unique_ptr<uv_buf_t> buffer;

                ReadRequest(int _offset, Array<uint8_t> _array, Dynamic _cbSuccess, Dynamic _cbFailure, std::unique_ptr<std::vector<char>> _staging, std::unique_ptr<uv_buf_t> _buffer)
                    : FileRequest(_cbSuccess, _cbFailure), offset(_offset), array(_array.mPtr), staging(std::move(_staging)), buffer(std::move(_buffer)) { }
            };

            if (pos < 0)
            {
                cbFailure(HX_CSTRING("Position is negative"));
            }
            if (offset < 0 || offset > buffer->length)
            {
                cbFailure(HX_CSTRING("Offset outside of buffer bounds"));
            }

            auto wrapper  = [](uv_fs_t* request) {
                auto spRequest = unique_fs_req(request);
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
                    Dynamic(spData->cbFailure.rooted)(uv_err_to_enum(spRequest->result));
                }
            };

            auto staging  = std::make_unique<std::vector<char>>(buffer->length);
            auto uvBuffer = std::make_unique<uv_buf_t>(uv_buf_init(staging->data(), staging->capacity()));
            auto request  = std::make_unique<uv_fs_t>();
            auto result   = uv_fs_read(loop, request.get(), file, uvBuffer.get(), 1, pos, wrapper);
            
            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
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
                auto spRequest = unique_fs_req(request);
                auto spData    = std::unique_ptr<FileRequest>(static_cast<FileRequest*>(request->data));
                auto gcZone    = hx::AutoGCZone();

                if (spRequest->result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(uv_err_to_enum(spRequest->result));
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
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
        void resize(int size, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_ftruncate(loop, request.get(), file, size, basicCallback);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
        void setPermissions(int permissions, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_fchmod(loop, request.get(), file, permissions, basicCallback);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
        void setOwner(int user, int group, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_fchown(loop, request.get(), file, user, group, basicCallback);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
        void setTimes(int accessTime, int modificationTime, Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_futime(loop, request.get(), file, accessTime, modificationTime, basicCallback);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
        void flush(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_fsync(loop, request.get(), file, basicCallback);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_close(loop, request.get(), file, basicCallback);

            if (result < 0)
            {
                cbFailure(uv_err_to_enum(result));
            }
            else
            {
                request->data = new FileRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
    };
}

void hx::asys::filesystem::File_obj::open(Context ctx, String path, int flags, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto libuvCtx = hx::asys::libuv::LibuvAsysContext_obj::Get(ctx);
    auto wrapper  = [](uv_fs_t* request) {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<FileRequest>(static_cast<FileRequest*>(request->data));
        auto spRequest = unique_fs_req(request);

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(uv_err_to_enum(spRequest->result));
        }
        else
        {
            Dynamic(spData->cbSuccess.rooted)(File(new LibuvFile_obj(spRequest->loop, spRequest->result)));
        }
    };

    auto request = std::make_unique<uv_fs_t>();
    auto result  = uv_fs_open(libuvCtx->uvLoop, request.get(), path.utf8_str(), openFlag(flags), openMode(flags), wrapper);

    if (result < 0)
    {
        cbFailure(uv_err_to_enum(result));
    }
    else
    {
        request->data = new FileRequest(cbSuccess, cbFailure);
        request.release();
    }
}
