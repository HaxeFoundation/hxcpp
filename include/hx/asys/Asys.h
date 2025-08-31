#pragma once

#include <cstdint>

HX_DECLARE_CLASS2(hx, asys, Context)
HX_DECLARE_CLASS2(hx, asys, Writable)
HX_DECLARE_CLASS2(hx, asys, Readable)
HX_DECLARE_CLASS3(hx, asys, filesystem, File)
HX_DECLARE_CLASS3(hx, asys, filesystem, Directory)
HX_DECLARE_CLASS3(hx, asys, net, TcpServer)
HX_DECLARE_CLASS3(hx, asys, net, TcpSocket)
HX_DECLARE_CLASS3(hx, asys, net, SecureSession)
HX_DECLARE_CLASS3(hx, asys, net, IpcSocket)
HX_DECLARE_CLASS3(hx, asys, system, Process)
HX_DECLARE_CLASS3(hx, asys, system, CurrentProcess)
HX_DECLARE_CLASS3(hx, asys, system, ChildProcess)

namespace hx
{
    namespace asys
    {
        using Ipv4Address = int;
        using Ipv6Address = Array<uint8_t>;
        using Pid         = int;

        class Context_obj : public Object
        {
        protected:
            Context_obj(system::CurrentProcess _process) : process(_process) {}

        public:
            static Context create();

            const system::CurrentProcess process;

            virtual bool loop() = 0;
            virtual void close() = 0;
        };

        class Writable_obj : public Object
        {
        public:
            virtual void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            virtual void flush(Dynamic cbSuccess, Dynamic cbFailure) = 0;
        };

        class Readable_obj : public Object
        {
        public:
            virtual void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
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
                static void info(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);

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
                const String path;

                Directory_obj(String _path) : path(_path) {}

                static void open(Context ctx, String path, Dynamic cbSuccess, Dynamic cbFailure);
                static void create(Context ctx, String path, int permissions, Dynamic cbSuccess, Dynamic cbFailure);
                static void rename(Context ctx, String oldPath, String newPath, Dynamic cbSuccess, Dynamic cbFailure);
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

            class TcpServer_obj : public Object
            {
            public:
                hx::Anon localAddress;

                static void open_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);
                static void open_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void accept(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;

                virtual void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) = 0;

                virtual void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            class TcpSocket_obj : public Object
            {
            public:
                hx::Anon localAddress;
                hx::Anon remoteAddress;

                Writable writer;
                Readable reader;

                static void connect_ipv4(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);
                static void connect_ipv6(Context ctx, const String host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) = 0;

                virtual void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;

                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            class SecureSession_obj : public Object
            {
            public:
                virtual void encode(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void decode(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;

                static void authenticateAsClient(TcpSocket socket, String host, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);
            };

            class IpcSocket_obj : public Object
            {
            public:
                String socketName;
                String peerName;

                Writable writer;
                Readable reader;

                static void bind(Context ctx, String name, Dynamic cbSuccess, Dynamic cbFailure);
                static void connect(Context ctx, String name, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            // class UdpSocket_obj : public Object
            // {
            // public:
            //     const hx::Anon localAddress;
            //     const hx::Anon remoteAddress;

            //     static void open(Context ctx, const String& host, int port, Dynamic options, Dynamic cbSuccess, Dynamic cbFailure);

            //     virtual void bind(const String& host, int port, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            //     virtual void unbind(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            //     virtual void write(Array<uint8_t> data, int offset, int length, const String& host, int port, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            //     virtual void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            //     virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            // };
        }

        namespace system
        {
            class Process_obj : public Object
            {
            public:
                static void open(Context ctx, String command, hx::Anon options, Dynamic cbSuccess, Dynamic cbFailure);

                virtual Pid pid() = 0;

                virtual void sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            class ChildProcess_obj : public Process_obj
            {
            public:
                ChildProcess_obj(Writable _stdio_in, Readable _stdio_out, Readable _stdio_err)
                    : stdio_in(_stdio_in)
                    , stdio_out(_stdio_out)
                    , stdio_err(_stdio_err) {}

                Writable stdio_in;
                Readable stdio_out;
                Readable stdio_err;

                virtual void exitCode(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };

            class CurrentProcess_obj : public Process_obj
            {
            public:
                CurrentProcess_obj(Readable _stdio_in, Writable _stdio_out, Writable _stdio_err)
                    : stdio_in(_stdio_in)
                    , stdio_out(_stdio_out)
                    , stdio_err(_stdio_err) {}

                virtual void setSignalAction(hx::EnumBase signal, hx::EnumBase action) = 0;

                Readable stdio_in;
                Writable stdio_out;
                Writable stdio_err;
            };
        }
    }
}