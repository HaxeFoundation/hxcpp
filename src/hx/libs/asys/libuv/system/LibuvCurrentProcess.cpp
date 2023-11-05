#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"
#include "../stream/ReadablePipe.h"
#include "../stream/WritablePipe.h"
#include "../filesystem/LibuvFile.h"
#include "LibuvCurrentProcess.h"

namespace
{
	class TtyWriter final : public hx::asys::Writable_obj
	{
	private:
		std::unique_ptr<hx::asys::libuv::stream::StreamWriter> writer;

	public:
		TtyWriter(uv_tty_t* tty)
			: writer(std::make_unique<hx::asys::libuv::stream::StreamWriter>(reinterpret_cast<uv_stream_t*>(tty)))
		{
			tty->data = writer.get();
		}
		~TtyWriter() = default;
		void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			writer->write(data, offset, length, cbSuccess, cbFailure);
		}
		void flush(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			writer->flush(cbSuccess, cbFailure);
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess();
		}
	};

	class TtyReader final : public hx::asys::Readable_obj
	{
		std::unique_ptr<hx::asys::libuv::stream::StreamReader> reader;

	public:
		TtyReader(uv_tty_t* tty)
			: reader(std::make_unique<hx::asys::libuv::stream::StreamReader>(reinterpret_cast<uv_stream_t*>(tty)))
		{
			tty->data = reader.get();
		}
		~TtyReader() = default;
		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			reader->read(output, offset, length, cbSuccess, cbFailure);
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess();
		}
	};
}

hx::asys::libuv::system::LibuvCurrentProcess::LibuvCurrentProcess(std::unique_ptr<std::array<uv_tty_t, 3>> inTtys)
	: ttys(std::move(inTtys))
{
	stdio_in  = Readable(new TtyReader(&(ttys->at(0))));
	stdio_out = Writable(new TtyWriter(&(ttys->at(1))));
	stdio_err = Writable(new TtyWriter(&(ttys->at(2))));

	hx::GCSetFinalizer(this, [](hx::Object* obj) -> void {
		reinterpret_cast<hx::asys::libuv::system::LibuvCurrentProcess*>(obj)->~LibuvCurrentProcess();
	});
}

hx::asys::libuv::system::LibuvCurrentProcess::~LibuvCurrentProcess()
{
	for (auto&& tty : *ttys)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(&tty), nullptr);
	}
}

int hx::asys::libuv::system::LibuvCurrentProcess::pid()
{
	return uv_os_getpid();
}

void hx::asys::libuv::system::LibuvCurrentProcess::sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto signum = 0;
	switch (signal->_hx_getIndex())
	{
	case 0:
		signum = SIGTERM;
		break;
	case 1:
		signum = SIGKILL;
		break;
	case 2:
		signum = SIGINT;
		break;
	case 3:
#if HX_WINDOWS
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));

		return;
#else
		signum = SIGSTOP;
#endif
		break;
	case 4:
#if HX_WINDOWS
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));

		return;
#else
		signum = SIGCONT;
#endif
		break;
	case 5:
#if HX_WINDOWS
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));

		return;
#else
		signum = hx::asys::libuv::toPosixCode(signal->_hx_getInt(5));
#endif
		break;
	default:
		cbFailure(hx::asys::libuv::uv_err_to_enum(-1));
		return;
	}

	auto result = 0;
	if ((result = uv_kill(pid(), signum)) < 0)
	{
		cbFailure(hx::asys::libuv::uv_err_to_enum(result));
	}
	else
	{
		cbSuccess();
	}
}

void hx::asys::libuv::system::LibuvCurrentProcess::setSignalAction(hx::EnumBase signal, hx::EnumBase action)
{
	switch (action->_hx_getIndex())
	{
	case 0:
		break;

	case 1:
		break;

	case 2:
		break;
	}
}

void hx::asys::libuv::system::LibuvCurrentProcess::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(stdio_in);
	HX_MARK_MEMBER(stdio_out);
	HX_MARK_MEMBER(stdio_err);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::system::LibuvCurrentProcess::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(stdio_in);
	HX_VISIT_MEMBER(stdio_out);
	HX_VISIT_MEMBER(stdio_err);
}
#endif