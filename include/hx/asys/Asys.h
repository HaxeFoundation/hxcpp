#pragma once

HX_DECLARE_CLASS2(hx, asys, Context)
HX_DECLARE_CLASS3(hx, asys, filesystem, File)

namespace hx
{
    namespace asys
    {
        class Context_obj : public Object
        {
        public:
            static Context create();

            virtual void enqueue(Dynamic event) = 0;
            virtual Dynamic enqueue(Dynamic event, int intervalMs) = 0;
            virtual void cancel(Dynamic) = 0;
            virtual void loop() = 0;
        };

        namespace filesystem
        {
            class File_obj : public Object
            {
            public:
                static void open(Context ctx, String path, int flags, Dynamic cbSuccess, Dynamic cbFailure);

                virtual void write(::cpp::Int64 pos, Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void read(::cpp::Int64 pos, Array<uint8_t> buffer, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void info(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void resize(int size, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setPermissions(int permissions, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setOwner(int user, int group, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void setTimes(int accessTime, int modificationTime, Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void flush(Dynamic cbSuccess, Dynamic cbFailure) = 0;
                virtual void close(Dynamic cbSuccess, Dynamic cbFailure) = 0;
            };
        }
    }
}