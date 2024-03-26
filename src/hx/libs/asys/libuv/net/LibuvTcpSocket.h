#include <hxcpp.h>
#include "../stream/StreamReader.h"
#include "../stream/StreamWriter.h"

namespace hx::asys::libuv::net
{
	const int KEEP_ALIVE_VALUE = 5;

	class LibuvTcpSocket final : public hx::asys::net::TcpSocket_obj
	{
		uv_tcp_t* tcp;
		int keepAlive;
		hx::asys::libuv::stream::StreamWriter writer;
		hx::asys::libuv::stream::StreamReader reader;

	public:
		LibuvTcpSocket(uv_tcp_t* _socket);

		void getKeepAlive(Dynamic cbSuccess, Dynamic cbFailure) override;
		void getSendBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;
		void getRecvBufferSize(Dynamic cbSuccess, Dynamic cbFailure) override;

		void setKeepAlive(bool keepAlive, Dynamic cbSuccess, Dynamic cbFailure) override;
		void setSendBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;
		void setRecvBufferSize(int size, Dynamic cbSuccess, Dynamic cbFailure) override;

		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override;
		void flush(Dynamic cbSuccess, Dynamic cbFailure) override;
		void close(Dynamic cbSuccess, Dynamic cbFailure) override;

		void __Mark(hx::MarkContext* __inCtx) override;
#if HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override;
#endif
	};
}
