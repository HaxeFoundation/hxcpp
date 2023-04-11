#pragma once

#include <cstdint>

HX_DECLARE_CLASS2(hx, asys, Context)
HX_DECLARE_CLASS3(hx, asys, filesystem, File)
HX_DECLARE_CLASS3(hx, asys, filesystem, Directory)
HX_DECLARE_CLASS3(hx, asys, net, Socket)
HX_DECLARE_CLASS3(hx, asys, net, Server)
HX_DECLARE_CLASS3(hx, asys, system, Process)
HX_DECLARE_CLASS3(hx, asys, system, ChildProcess)

namespace hx
{
    namespace asys
    {
        using Ipv4Address = int;
        using Ipv6Address = Array<uint8_t>;
        using Pid = int;

        class Context_obj : public Object
        {
        public:
            static Context create();

            virtual void enqueue(Dynamic event) = 0;
            virtual Dynamic enqueue(Dynamic event, int intervalMs) = 0;
            virtual void cancel(Dynamic) = 0;
            virtual void loop() = 0;
        };

        struct Writable
        {
            virtual void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
        };

        struct Readable
        {
            virtual void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
        };

        struct Duplex : virtual public Writable, virtual public Readable
        {
            //
        };

        namespace filesystem
        {
            enum class FileAccessMode : std::uint8_t
            {
                exists     = 0b0001,
                executable = 0b0010,
                writable   = 0b0100,
                readable   = 0b1000
            };

            class File_obj : public Object
            {
            public:
                static void open(Context ctx, String path, int flags, Dynamic cbSuccess, Dynamic cbFailure);
                static void temp(Context ctx, Dynamic cbSuccess, Dynamic cbFailure);

                const String path;

                File_obj(String _path) : path(_path) {}

                virtual void write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void read(::cpp::Int64 pos, Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void info(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void resize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setPermissions(int permissions, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setOwner(int user, int group, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setTimes(int accessTime, int modificationTime, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void flush(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            class Directory_obj : public Object
            {
            public:
                static void open(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void create(Context ctx, String path, int permissions, bool recursive, Dynamic cbSuccess, Dynamic cbFailure);
                static void move(Context ctx, String oldPath, String newPath, Dynamic cbSuccess, Dynamic cbFailure);
                static void check(Context ctx, String path, FileAccessMode accessMode, Dynamic cbSuccess, Dynamic cbFailure);
                static void deleteFile(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void deleteDirectory(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void isDirectory(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void isFile(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void isLink(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void setLinkOwner(Context ctx, String path, int user, int group, Dynamic cbSuccess, Dynamic cbFailure);
                static void link(Context ctx, String target, String path, int type, Dynamic cbSuccess, Dynamic cbFailure);
                static void linkInfo(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void readLink(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void copyFile(Context ctx, String source, String destination, bool overwrite, Dynamic cbSuccess, Dynamic cbFailure);
                static void realPath(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void next(int batch, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };
        }

        namespace net
        {
            namespace dns
            {
                void resolve(Context ctx, const String host, Dynamic cbSuccess, Dynamic cbFailure);
                void reverse(Context ctx, const Ipv4Address ip, Dynamic cbSuccess, Dynamic cbFailure);
                void reverse(Context ctx, const Ipv6Address ip, Dynamic cbSuccess, Dynamic cbFailure);
            }

            namespace ip
            {
                hx::EnumBase parse(const String ip);
                String name(const Ipv4Address ip);
                String name(const Ipv6Address ip);
            }

            class Socket_obj : public Object, virtual public Duplex
            {
            protected:
                Socket_obj(hx::EnumBase _name, hx::EnumBase _peer) : name(_name) , peer(_peer) {}

            public:
                const hx::EnumBase name;
                const hx::EnumBase peer;

                static void connect_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);
                static void connect_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);
                static void connect_ipc(Context ctx, const String path, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override = 0;
                virtual void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) override = 0;
                virtual void flush(Dynamic cbSuccess, Dynamic cbFailure) = 0;

                virtual void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) = 0;

                virtual void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            class Server_obj : public Object
            {
            protected:
                Server_obj(hx::EnumBase _name) : name(_name) {}

            public:
                const hx::EnumBase name;

                static void open_ipv4(Context ctx, const String host, int port, Dynamic cbSuccess, Dynamic cbFailure);
                static void open_ipv6(Context ctx, const String host, int port, Dynamic cbSuccess, Dynamic cbFailure);
                static void open_ipc(Context ctx, const String path, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void accept(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };
        }

        namespace system
        {
            class CurrentProcess_obj;

            class Process_obj : public Object
            {
            public:
                static ChildProcess_obj currentProcess();

                const Pid pid;
            };

            class CurrentProcess_obj : public Process_obj
            {
                //
            };
        }
    }
}