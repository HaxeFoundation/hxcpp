#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"

namespace
{
    void getCwd(const char* &output, hx::Anon options)
    {
        if (null() == options)
        {
            return;
        }

        auto field = options->__Field(HX_CSTRING("cwd"), HX_PROP_ALWAYS);
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

        auto field = options->__Field(HX_CSTRING("args"), HX_PROP_ALWAYS);
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

        auto field = options->__Field(HX_CSTRING("env"), HX_PROP_ALWAYS);
        if (field.isNull())
        {
            return;
        }

        auto hash = Dynamic(field.asDynamic());
        auto keys = __string_hash_keys(hash);

        environment.resize(static_cast<size_t>(keys->length) + 1);

        for (auto i = size_t(0); i < keys->length; i++)
        {
            auto& key = keys[i];
            auto value = __string_hash_get_string(hash, key);

            if (null() == value)
            {
                environment.at(i) = const_cast<char*>(key.utf8_str());
            }
            else
            {
                environment.at(i) = (std::string(key.utf8_str()) + std::string("=") + std::string(value.utf8_str())).data();
            }
        }
    }

    /*std::unique_ptr<std::vector<uv_stdio_container_t>> getStdioContainers(hx::Anon hxOptions)
    {
        if (null() == hxOptions)
        {
            return;
        }

        auto field = hxOptions->__Field(HX_CSTRING("stdio"), HX_PROP_ALWAYS);
        if (field.isNull())
        {
            return;
        }

        auto stdinOptions = hx::Anon(field.asObject());
        auto stdinField   = stdinOptions->__Field(HX_CSTRING("stdin"), HX_PROP_ALWAYS);
        auto stdoutField  = stdinOptions->__Field(HX_CSTRING("stdout"), HX_PROP_ALWAYS);
        auto stderrField  = stdinOptions->__Field(HX_CSTRING("stderr"), HX_PROP_ALWAYS);
        auto extraPipes   = stdinOptions->__Field(HX_CSTRING("extra"), HX_PROP_ALWAYS);

        switch (hx::EnumBase(stdinField)->_hx_getIndex())
        {
        case 0:
        {
            auto config = uv_stdio_container_t();
            config.flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
            break;
        }
            

        case 1:
            break;

        case 2:
            break;

        case 3:
            break;

        case 4:
            break;

        case 5:
            break;

        case 6:
            break;
        }
    }*/
}

void hx::asys::system::Process::open(Context ctx, String command, hx::Anon options, Dynamic cbSuccess, Dynamic cbFailure)
{
    auto uvContext = hx::asys::libuv::context(ctx);
    auto process   = std::make_unique<hx::asys::libuv::system::LibuvChildProcess>();

    getCwd(process->options.cwd, options);
    getArguments(process->arguments, command, options);
    getEnvironment(process->environment, options);

    process->request.data    = process.get();
    process->options.args    = process->arguments.data();
    process->options.env     = process->environment.empty() ? nullptr : process->environment.data();
    process->options.file    = command.utf8_str();
    process->options.exit_cb = [](uv_process_t* request, int64_t status, int signal) {
        auto process = reinterpret_cast<hx::asys::libuv::system::LibuvChildProcess*>(request->data);

        process->currentExitCode = status;

        auto gcZone   = hx::AutoGCZone();
        auto callback = Dynamic(process->exitCallback);
        if (null() != callback)
        {
            callback(status);
        }
    };

    auto uidOption = options->__Field(HX_CSTRING("user"), hx::PropertyAccess::paccDynamic);
    if (!uidOption.isNull())
    {
        process->options.flags |= UV_PROCESS_SETUID;
        process->options.uid = uidOption.asInt();
    }

    auto gidOption = options->__Field(HX_CSTRING("group"), hx::PropertyAccess::paccDynamic);
    if (!gidOption.isNull())
    {
        process->options.flags |= UV_PROCESS_SETGID;
        process->options.gid = gidOption.asInt();
    }

    auto detachedOption = options->__Field(HX_CSTRING("detached"), hx::PropertyAccess::paccDynamic);
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
        cbSuccess(::cpp::Pointer<ChildProcess>(process.release()));
    }
}