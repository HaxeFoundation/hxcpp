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

    class LibuvFile_obj : public hx::asys::filesystem::File_obj
    {
    private:
        uv_loop_t* loop;
        uv_file file;
    public:
        LibuvFile_obj(uv_loop_t* _loop, uv_file _file) : loop(_loop), file(_file) {}

        void write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct WriteRequest
            {
                std::unique_ptr<std::vector<char>> data;
                hx::RootedObject cbSuccess;
                hx::RootedObject cbFailure;

                WriteRequest(std::unique_ptr<std::vector<char>> _data, Dynamic _cbSuccess, Dynamic _cbFailure)
                    : data(std::move(_data)), cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr) {}
            };

            auto wrapper = [](uv_fs_t* request) {
                auto gcZone    = hx::AutoGCZone();
                auto spData    = std::unique_ptr<WriteRequest>(static_cast<WriteRequest*>(request->data));
                auto spRequest = unique_fs_req(request);

                if (request->result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(String::create(uv_err_name(request->result)));
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
                cbFailure(String::create(uv_err_name(result)));
            }
            else
            {
                request->data = new WriteRequest(std::move(staging), cbSuccess, cbFailure);
                request.release();
            }
        }
        void read(::cpp::Int64 pos, Array<uint8_t> buffer, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct ReadRequest
            {
                const int offset;
                const hx::RootedObject array;
                const hx::RootedObject cbSuccess;
                const hx::RootedObject cbFailure;
                const std::unique_ptr<std::vector<char>> staging;
                const std::unique_ptr<uv_buf_t> buffer;

                ReadRequest(int _offset, Array<uint8_t> _array, Dynamic _cbSuccess, Dynamic _cbFailure, std::unique_ptr<std::vector<char>> _staging, std::unique_ptr<uv_buf_t> _buffer)
                    : offset(_offset), array(_array.mPtr), cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr), staging(std::move(_staging)), buffer(std::move(_buffer)) { }
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
                else if (request->result == 0)
                {
                    Dynamic(spData->cbFailure.rooted)(HX_CSTRING("More Data"));
                }
                else
                {
                    Dynamic(spData->cbFailure.rooted)(String::create(uv_err_name(request->result)));
                }
            };

            auto staging  = std::make_unique<std::vector<char>>(buffer->length);
            auto uvBuffer = std::make_unique<uv_buf_t>(uv_buf_init(staging->data(), staging->capacity()));
            auto request  = std::make_unique<uv_fs_t>();
            auto result   = uv_fs_read(loop, request.get(), file, uvBuffer.get(), 1, pos, wrapper);
            
            if (result < 0)
            {
                cbFailure(String::create(uv_err_name(result)));
            }
            else
            {
                request->data = new ReadRequest(offset, buffer, cbSuccess, cbFailure, std::move(staging), std::move(uvBuffer));
                request.release();
            }
        }
        void close(Dynamic cbSuccess, Dynamic cbFailure)
        {
            struct CloseRequest
            {
                const hx::RootedObject cbSuccess;
                const hx::RootedObject cbFailure;

                CloseRequest(Dynamic _cbSuccess, Dynamic _cbFailure) : cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr) {}
            };

            auto wrapper = [](uv_fs_t* request) {
                auto spRequest = unique_fs_req(request);
                auto spData    = std::unique_ptr<CloseRequest>(static_cast<CloseRequest*>(request->data));
                auto gcZone    = hx::AutoGCZone();

                if (spRequest->result < 0)
                {
                    Dynamic(spData->cbFailure.rooted)(String::create(uv_err_name(spRequest->result)));
                }
                else
                {
                    Dynamic(spData->cbSuccess.rooted)();
                }
            };

            auto request = std::make_unique<uv_fs_t>();
            auto result  = uv_fs_close(loop, request.get(), file, wrapper);

            if (result < 0)
            {
                cbFailure(String::create(uv_err_name(result)));
            }
            else
            {
                request->data = new CloseRequest(cbSuccess, cbFailure);
                request.release();
            }
        }
    };
}

void hx::asys::filesystem::File_obj::open(Context ctx, String path, int flags, Dynamic cbSuccess, Dynamic cbFailure)
{
    struct OpenRequest
    {
        hx::RootedObject cbSuccess;
        hx::RootedObject cbFailure;

        OpenRequest(Dynamic _cbSuccess, Dynamic _cbFailure) : cbSuccess(_cbSuccess.mPtr), cbFailure(_cbFailure.mPtr) {}
    };

    auto libuvCtx = hx::asys::libuv::LibuvAsysContext_obj::Get(ctx);
    auto wrapper  = [](uv_fs_t* request) {
        auto gcZone    = hx::AutoGCZone();
        auto spData    = std::unique_ptr<OpenRequest>(static_cast<OpenRequest*>(request->data));
        auto spRequest = unique_fs_req(request);

        if (spRequest->result < 0)
        {
            Dynamic(spData->cbFailure.rooted)(String::create(uv_err_name(spRequest->result)));
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
        cbFailure(String::create(uv_err_name(result)));
    }
    else
    {
        request->data = new OpenRequest(cbSuccess, cbFailure);
        request.release();
    }
}
