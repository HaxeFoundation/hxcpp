#pragma once

#include <hxcpp.h>
#include <memory>
#include <deque>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"
#include "../LibuvUtils.h"

namespace hx::asys::libuv::net
{
    class LibuvSocket final : public hx::asys::net::Socket_obj
    {
    private:
        bool keepAliveEnabled;
        uv_stream_t* const handle;
        const std::unique_ptr<stream::StreamReader> reader;
        const std::unique_ptr<stream::StreamWriter> writer;

    public:
        LibuvSocket(uv_stream_t* const _handle);

        ~LibuvSocket();

        void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
        void write(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
        void close(Dynamic cbSuccess, Dynamic cbFailure) override;
        void flush(Dynamic cbSuccess, Dynamic cbFailure) override;

        void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) override;
        void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;
        void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;

        void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) override;
        void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;
        void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;
    };
}