#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"
#include "LibuvCurrentProcess.h"
#include "../filesystem/LibuvFile.h"

namespace
{
    void getCwd(const char* &output, hx::Anon options)
    {
        if (null() == options)
        {
            return;
        }

        auto field = options->__Field(HX_CSTRING("cwd"), HX_PROP_DYNAMIC);
        if (field.isNull())
        {
            return;
        }

        output = field.asString().utf8_str();
    }

    void getArguments(std::vector<char*> &arguments, String command, hx::Anon options)
    {
        arguments.push_back(const_cast<char*>(command.utf8_str()));
        arguments.push_back(nullptr);

        if (null() == options)
        {
            return;
        }

        auto field = options->__Field(HX_CSTRING("args"), HX_PROP_DYNAMIC);
        if (field.isNull())
        {
            return;
        }

        auto hxArguments = Dynamic(field.asDynamic()).StaticCast<Array<String>>();
        if (null() == hxArguments)
        {
            return;
        }

        arguments.resize(static_cast<size_t>(hxArguments->length) + 2);

        for (auto i = size_t(0); i < hxArguments->length; i++)
        {
            arguments.at(1 + i) = const_cast<char*>(hxArguments[i].utf8_str());
        }
    }

    void getEnvironment(std::vector<char*>& environment, hx::Anon options)
    {
        if (null() == options)
        {
            return;
        }

        auto field = options->__Field(HX_CSTRING("env"), HX_PROP_DYNAMIC);
        if (field.isNull())
        {
            return;
        }

        auto hash = Dynamic(field.asDynamic()->__Field(HX_CSTRING("h"), HX_PROP_DYNAMIC));
        auto keys = __string_hash_keys(hash);

        environment.resize(static_cast<size_t>(keys->length) + 1);

        for (auto i = size_t(0); i < keys->length; i++)
        {
            auto& key   = keys[i];
            auto  value = __string_hash_get_string(hash, key);

            if (null() == value)
            {
                environment.at(i) = const_cast<char*>(key.utf8_str());
            }
            else
            {
                environment.at(i) = const_cast<char*>((key + HX_CSTRING("=") + value).c_str());
            }
        }
    }

    hx::asys::Writable getWritablePipe(uv_loop_t* loop, uv_stdio_container_t& container, hx::asys::libuv::system::LibuvChildProcess::Stream& stream)
    {
        auto result = 0;

        if ((result = uv_pipe_init(loop, &stream.pipe, 0)) < 0)
        {
            hx::Throw(HX_CSTRING("Failed to init pipe"));
        }

        container.flags       = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
        container.data.stream = reinterpret_cast<uv_stream_t*>(&stream.pipe);

        return new hx::asys::libuv::stream::StreamWriter_obj(reinterpret_cast<uv_stream_t*>(&stream.pipe));
    }

    void onAlloc(uv_handle_t* handle, const size_t suggested, uv_buf_t* buffer)
    {
        auto  ctx     = static_cast<hx::asys::libuv::system::LibuvChildProcess::Stream*>(handle->data);
        auto& staging = ctx->reader.staging.emplace_back(suggested);

        buffer->base = staging.data();
        buffer->len = staging.size();
    }

    void onRead(uv_stream_t* stream, const ssize_t len, const uv_buf_t* read)
    {
        auto  gc  = hx::AutoGCZone();
        auto  ctx = static_cast<hx::asys::libuv::system::LibuvChildProcess::Stream*>(stream->data);

        if (len <= 0)
        {
            ctx->reader.reject(len);

            return;
        }

        ctx->reader.buffer.insert(ctx->reader.buffer.end(), read->base, read->base + len);
        ctx->reader.consume();
    }

    hx::asys::Readable getReadablePipe(uv_loop_t* loop, uv_stdio_container_t& container, hx::asys::libuv::system::LibuvChildProcess::Stream& stream)
    {
        auto result = 0;

        if ((result = uv_pipe_init(loop, &stream.pipe, 0)) < 0)
        {
            hx::Throw(HX_CSTRING("Failed to init pipe"));
        }

        container.flags       = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        container.data.stream = reinterpret_cast<uv_stream_t*>(&stream.pipe);

        return new hx::asys::libuv::stream::StreamReader_obj(&stream.reader, onAlloc, onRead);
    }

    hx::asys::Writable getStdioWritable(uv_loop_t* loop, hx::EnumBase field, uv_stdio_container_t& container, hx::asys::libuv::system::LibuvChildProcess::Stream& stream, const int index)
    {
        switch (field->_hx_getIndex())
        {
        case 0:
        {
            return getWritablePipe(loop, container, stream);
        }

        case 1:
        {
            hx::Throw(HX_CSTRING("Cannot open a writable pipe on stdin"));
        }

        case 2:
        {
            hx::Throw(HX_CSTRING("Cannot open a duplex pipe on stdin"));
        }

        case 3:
        {
            container.flags = UV_INHERIT_FD;
            container.data.fd = index;
            break;
        }

        case 4:
        {
            container.flags = UV_IGNORE;
            break;
        }

        case 5:
        {
            break;
        }

        case 6:
        {
            auto file    = field->_hx_getObject(0);
            auto native  = file->__Field(HX_CSTRING("native"), HX_PROP_DYNAMIC).asObject();
            auto luvFile = reinterpret_cast<hx::asys::libuv::filesystem::LibuvFile_obj*>(native);

            container.flags = UV_INHERIT_FD;
            container.data.fd = luvFile->file;

            break;
        }
        }

        return null();
    }

    hx::asys::Readable getStdioReadable(uv_loop_t* loop, hx::EnumBase field, uv_stdio_container_t& container, hx::asys::libuv::system::LibuvChildProcess::Stream& stream, int index)
    {
        switch (field->_hx_getIndex())
        {
        case 0:
        {
            hx::Throw(HX_CSTRING("Cannot open a readable pipe on stdout"));
        }

        case 1:
        {
            return getReadablePipe(loop, container, stream);
        }

        case 2:
        {
            hx::Throw(HX_CSTRING("Cannot open a duplex pipe on stdout"));
            break;
        }

        case 3:
        {
            container.flags = UV_INHERIT_FD;
            container.data.fd = index;
            break;
        }

        case 4:
        {
            container.flags = UV_IGNORE;
            break;
        }

        case 6:
        {
            auto file    = field->_hx_getObject(0);
            auto native  = file->__Field(HX_CSTRING("native"), HX_PROP_DYNAMIC).asObject();
            auto luvFile = reinterpret_cast<hx::asys::libuv::filesystem::LibuvFile_obj*>(native);

            container.flags = UV_INHERIT_FD;
            container.data.fd = luvFile->file;

            break;
        }
        }

        return null();
    }
}

void hx::asys::system::Process_obj::open(Context ctx, String command, hx::Anon options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto uvContext = hx::asys::libuv::context(ctx);
    auto process   = new hx::asys::libuv::system::LibuvChildProcess::Ctx();

    getCwd(process->options.cwd, options);
    getArguments(process->arguments, command, options);
    getEnvironment(process->environment, options);

    auto o_stdin  = hx::asys::Writable(null());
    auto o_stdout = hx::asys::Readable(null());
    auto o_stderr = hx::asys::Readable(null());

    if (hx::IsNotNull(options))
    {
        auto io = hx::Anon(options->__Field(HX_CSTRING("stdio"), HX_PROP_DYNAMIC));

        o_stdin  = getStdioWritable(uvContext->uvLoop, io->__Field(HX_CSTRING("stdin"), HX_PROP_DYNAMIC), process->containers.at(0), process->streams.at(0), 0);
        o_stdout = getStdioReadable(uvContext->uvLoop, io->__Field(HX_CSTRING("stdout"), HX_PROP_DYNAMIC), process->containers.at(1), process->streams.at(1), 1);
        o_stderr = getStdioReadable(uvContext->uvLoop, io->__Field(HX_CSTRING("stderr"), HX_PROP_DYNAMIC), process->containers.at(2), process->streams.at(2), 2);
    }

    process->request.data        = process;
    process->options.args        = process->arguments.data();
    process->options.env         = process->environment.empty() ? nullptr : process->environment.data();
    process->options.stdio       = process->containers.data();
    process->options.stdio_count = process->containers.size();
    process->options.file        = command.utf8_str();
    process->options.exit_cb     = [](uv_process_t* request, int64_t status, int signal) {
        auto process = reinterpret_cast<hx::asys::libuv::system::LibuvChildProcess::Ctx*>(request->data);

        process->currentExitCode = status;

        auto gcZone   = hx::AutoGCZone();
        auto callback = Dynamic(process->exitCallback.rooted);
        if (null() != callback)
        {
            callback(status);
        }
    };

#if HX_WINDOWS
    process->options.flags |= UV_PROCESS_WINDOWS_FILE_PATH_EXACT_NAME;
#endif

    auto uidOption = options->__Field(HX_CSTRING("user"), HX_PROP_DYNAMIC);
    if (!uidOption.isNull())
    {
        process->options.flags |= UV_PROCESS_SETUID;
        process->options.uid = uidOption.asInt();
    }

    auto gidOption = options->__Field(HX_CSTRING("group"), HX_PROP_DYNAMIC);
    if (!gidOption.isNull())
    {
        process->options.flags |= UV_PROCESS_SETGID;
        process->options.gid = gidOption.asInt();
    }

    auto detachedOption = options->__Field(HX_CSTRING("detached"), HX_PROP_DYNAMIC);
    if (!detachedOption.isNull())
    {
        process->options.flags |= UV_PROCESS_DETACHED;
    }

    auto result = 0;
    if ((result = uv_spawn(uvContext->uvLoop, &process->request, &process->options)))
    {
        cbFailure(hx::asys::libuv::uv_err_to_enum(result));
    }
    else
    {
        cbSuccess(ChildProcess(new hx::asys::libuv::system::LibuvChildProcess(process, o_stdin, o_stdout, o_stderr)));
    }
}
