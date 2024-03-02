#include <hxcpp.h>
#include <string>
#include "LibuvChildProcess.h"
#include "../stream/ReadablePipe.h"
#include "../stream/WritablePipe.h"
#include "../filesystem/LibuvFile.h"
#include "LibuvCurrentProcess.h"

namespace
{
	class WriteRequest final : hx::asys::libuv::BaseRequest
	{
		std::unique_ptr<hx::ArrayPin> pin;

	public:
		uv_write_t request;
		uv_buf_t buffer;

		WriteRequest(hx::ArrayPin* _pin, char* _base, int _length, Dynamic _cbSuccess, Dynamic _cbFailure)
			: BaseRequest(_cbSuccess, _cbFailure)
			, pin(_pin)
			, buffer(uv_buf_init(_base, _length))
		{
			request.data = this;
		}

		static void callback(uv_write_t* request, int status)
		{
			auto gcZone = hx::AutoGCZone();
			auto spData = std::unique_ptr<WriteRequest>(static_cast<WriteRequest*>(request->data));

			if (status < 0)
			{
				Dynamic(spData->cbFailure.rooted)(hx::asys::libuv::uv_err_to_enum(status));
			}
			else
			{
				Dynamic(spData->cbSuccess.rooted)(spData->buffer.len);
			}
		}
	};

	class TtyWriter final : public hx::asys::Writable_obj
	{
		uv_tty_t* tty;

	public:
		TtyWriter(uv_loop_t* loop, uv_file fd) : tty(new uv_tty_t())
		{
			uv_tty_init(loop, tty, fd, 0);
		}
		void write(Array<uint8_t> data, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			auto request = std::make_unique<WriteRequest>(data->Pin(), data->GetBase() + offset, length, cbSuccess, cbFailure);
			auto result  = uv_write(&request->request, reinterpret_cast<uv_stream_t*>(tty), &request->buffer, 1, WriteRequest::callback);

			if (result < 0)
			{
				cbFailure(hx::asys::libuv::uv_err_to_enum(result));
			}
			else
			{
				request.release();
			}
		}
		void flush(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess();
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess();
		}
	};

	class TtyReader final : public hx::asys::Readable_obj
	{
		uv_tty_t* tty;
		hx::asys::libuv::stream::StreamReader* reader;

	public:
		TtyReader(uv_loop_t* loop, uv_file fd)
			: tty(new uv_tty_t())
			, reader(new hx::asys::libuv::stream::StreamReader(reinterpret_cast<uv_stream_t*>(tty)))
		{
			tty->data = reader;

			uv_tty_init(loop, tty, fd, 0);
		}
		void read(Array<uint8_t> output, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			reader->read(output, offset, length, cbSuccess, cbFailure);
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			cbSuccess();
		}
	};

	class SignalActionRequest final : public hx::asys::libuv::BaseRequest
	{
	public:
		uv_signal_t request;

		SignalActionRequest(Dynamic callback) : BaseRequest(callback, null()) {}

		~SignalActionRequest() override
		{
			uv_signal_stop(&request);

			uv_close(reinterpret_cast<uv_handle_t*>(&request), nullptr);
		}
	};

	int getSignalId(hx::EnumBase signal)
	{
		switch (signal->_hx_getIndex())
		{
		case 0:
			return SIGTERM;
		case 1:
			return SIGKILL;
		case 2:
			return SIGINT;
		case 3:
#if HX_WINDOWS
			hx::Throw(HX_CSTRING("Platform not supported"));
#else
			return SIGSTOP;
#endif
		case 4:
#if HX_WINDOWS
			hx::Throw(HX_CSTRING("Platform not supported"));
#else
			return SIGCONT;
#endif
		case 5:
#if HX_WINDOWS
			hx::Throw(HX_CSTRING("Platform not supported"));
#else
			return hx::asys::libuv::toPosixCode(signal->_hx_getInt(0));
#endif
		}
	}
}

hx::asys::libuv::system::LibuvCurrentProcess::LibuvCurrentProcess(LibuvAsysContext ctx)
	: signalActions(std::make_unique<std::unordered_map<int, std::unique_ptr<BaseRequest>>>(0))
	, ctx(ctx)
{
	hx::GCSetFinalizer(this, [](hx::Object* obj) -> void {
		reinterpret_cast<hx::asys::libuv::system::LibuvCurrentProcess*>(obj)->~LibuvCurrentProcess();
	});

	stdio_in  = hx::asys::Readable(new TtyReader(ctx->uvLoop, 0));
	stdio_out = hx::asys::Writable(new TtyWriter(ctx->uvLoop, 1));
	stdio_err = hx::asys::Writable(new TtyWriter(ctx->uvLoop, 2));
}

hx::asys::Pid hx::asys::libuv::system::LibuvCurrentProcess::pid()
{
	return uv_os_getpid();
}

void hx::asys::libuv::system::LibuvCurrentProcess::sendSignal(hx::EnumBase signal, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto result = 0;
	if ((result = uv_kill(pid(), getSignalId(signal))) < 0)
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
	// TODO : case 0 and 2 are fairly similar, could remove duplicated code.

	switch (action->_hx_getIndex())
	{
	case 0:
		{
			auto signum = getSignalId(signal);
			auto handle = std::make_unique<SignalActionRequest>(null());
			auto func   = [](uv_signal_t* handle, int signum) {
				// do nothing!
			};

			if (uv_signal_init(ctx->uvLoop, &handle->request) < 0)
			{
				hx::Throw(HX_CSTRING("Failed to init signal"));
			}

			if (uv_signal_start(&handle->request, func, signum))
			{
				hx::Throw(HX_CSTRING("Failed to start signal"));
			}

			handle->request.data = handle.get();

			signalActions->erase(signum);
			signalActions->emplace(signum, std::move(handle));
		}
		break;

	case 1:
		{
			signalActions->erase(getSignalId(signal));
		}
		break;

	case 2:
		{
			auto signum = getSignalId(signal);
			auto handle = std::make_unique<SignalActionRequest>(action->_hx_getObject(0));
			auto func   = [](uv_signal_t* handle, int signum) {
				auto gcZone   = AutoGCZone();
				auto request  = reinterpret_cast<SignalActionRequest*>(handle->data);
				auto callback = Dynamic(request->cbSuccess.rooted);

				if (null() != callback)
				{
					callback();
				}
			};

			if (uv_signal_init(ctx->uvLoop, &handle->request) < 0)
			{
				hx::Throw(HX_CSTRING("Failed to init signal"));
			}

			if (uv_signal_start(&handle->request, func, signum))
			{
				hx::Throw(HX_CSTRING("Failed to start signal"));
			}

			handle->request.data = handle.get();

			signalActions->erase(signum);
			signalActions->emplace(signum, std::move(handle));
		}
		break;
	}
}

void hx::asys::libuv::system::LibuvCurrentProcess::__Mark(hx::MarkContext* __inCtx)
{
	HX_MARK_MEMBER(ctx);
	HX_MARK_MEMBER(signalActions);
}

#ifdef HXCPP_VISIT_ALLOCS
void hx::asys::libuv::system::LibuvCurrentProcess::__Visit(hx::VisitContext* __inCtx)
{
	HX_VISIT_MEMBER(ctx);
	HX_VISIT_MEMBER(signalActions);
}
#endif
