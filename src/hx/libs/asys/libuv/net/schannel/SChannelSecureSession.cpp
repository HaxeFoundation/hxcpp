#include <hxcpp.h>

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <SubAuth.h>

#define SCHANNEL_USE_BLACKLISTS
#define SECURITY_WIN32

#include <schannel.h>
#include <security.h>
#include <shlwapi.h>
#include <array>
#include <algorithm>

namespace
{
	const int TLS_MAX_PACKET_SIZE = (16384 + 512);

	static void init_sec_buffer(SecBuffer* buffer, unsigned long type, void* data, unsigned long size)
	{
		buffer->cbBuffer   = size;
		buffer->BufferType = type;
		buffer->pvBuffer   = data;
	}

	static void init_sec_buffer_desc(SecBufferDesc* desc, SecBuffer* buffers, unsigned long buffer_count)
	{
		desc->ulVersion = SECBUFFER_VERSION;
		desc->pBuffers  = buffers;
		desc->cBuffers  = buffer_count;
	}

	class SChannelContext final
	{
		const int TLS_MAX_PACKET_SIZE = (16384 + 512);
	public:
		SChannelContext(const char* inHost)
			: host(inHost)
			, requestFlags(ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM) {}

		~SChannelContext()
		{
			DeleteSecurityContext(&ctxtHandle);
			FreeCredentialHandle(&credHandle);
		}

		const char* host;

		CredHandle credHandle;
		TimeStamp credTimestamp;
		CtxtHandle ctxtHandle;
		TimeStamp ctxtTimestamp;

		DWORD requestFlags;
		DWORD contextFlags;

		SecPkgContext_StreamSizes sizes;
	};

	HX_DECLARE_CLASS0(Handshake)
	HX_DECLARE_CLASS0(SChannelSecureSession)

	class SChannelSecureSession_obj final : public hx::asys::net::SecureSession_obj
	{
		SChannelContext* ctx;

	public:
		SChannelSecureSession_obj(SChannelContext* inCtx) : ctx(inCtx) {}

		void encode(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			auto use                = std::min(static_cast<unsigned long>(length), ctx->sizes.cbMaximumMessage);
			auto buffer             = std::vector<uint8_t>(TLS_MAX_PACKET_SIZE);
			auto buffers            = std::array<SecBuffer, 4>();
			auto buffersDescription = SecBufferDesc();

			init_sec_buffer(&buffers[0], SECBUFFER_STREAM_HEADER, buffer.data(), ctx->sizes.cbHeader);
			init_sec_buffer(&buffers[1], SECBUFFER_DATA, buffer.data() + ctx->sizes.cbHeader, use);
			init_sec_buffer(&buffers[2], SECBUFFER_STREAM_TRAILER, buffer.data() + ctx->sizes.cbHeader + use, ctx->sizes.cbTrailer);
			init_sec_buffer(&buffers[3], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&buffersDescription, buffers.data(), buffers.size());

			std::memcpy(buffers[1].pvBuffer, input->getBase() + offset, use);

			auto result = SEC_E_OK;
			if (SEC_E_OK != (result = EncryptMessage(&ctx->ctxtHandle, 0, &buffersDescription, 0)))
			{
				cbFailure(HX_CSTRING("Failed to encrypt message"));

				return;
			}

			auto total  = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
			auto output = Array<uint8_t>(total, total);

			std::memcpy(output->getBase(), buffer.data(), total);

			cbSuccess(output);
		}
		void decode(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure) override
		{
			auto buffer             = std::vector<uint8_t>(length);
			auto buffers            = std::array<SecBuffer, 4>();
			auto buffersDescription = SecBufferDesc();

			std::memcpy(buffer.data(), input->getBase() + offset, length);

			init_sec_buffer(&buffers[0], SECBUFFER_DATA, buffer.data(), length);
			init_sec_buffer(&buffers[1], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer(&buffers[2], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer(&buffers[3], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&buffersDescription, buffers.data(), buffers.size());

			auto result = SEC_E_OK;
			if (SEC_E_OK != (result = DecryptMessage(&ctx->ctxtHandle, &buffersDescription, 0, 0)))
			{
				cbFailure(HX_CSTRING("Failed to decrypt message"));

				return;
			}

			auto output = Array<uint8_t>(buffers[1].cbBuffer, buffers[1].cbBuffer);

			std::memcpy(output->GetBase(), buffers[1].pvBuffer, buffers[1].cbBuffer);

			cbSuccess(output);
		}
		void close(Dynamic cbSuccess, Dynamic cbFailure) override
		{
			auto type                    = SCHANNEL_SHUTDOWN;
			auto inputBuffer             = SecBuffer();
			auto inputBufferDescription  = SecBufferDesc();

			init_sec_buffer(&inputBuffer, SECBUFFER_TOKEN, &type, sizeof(type));
			init_sec_buffer_desc(&inputBufferDescription, &inputBuffer, 1);

			if (SEC_E_OK != ApplyControlToken(&ctx->ctxtHandle, &inputBufferDescription))
			{
				cbFailure(HX_CSTRING("Failed to apply control token"));

				return;
			}

			auto outputBuffer            = SecBuffer();
			auto outputBufferDescription = SecBufferDesc();

			init_sec_buffer(&outputBuffer, SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&outputBufferDescription, &outputBuffer, 1);

			auto result = InitializeSecurityContext(&ctx->credHandle, &ctx->ctxtHandle, const_cast<char*>(ctx->host), ctx->requestFlags, 0, 0, nullptr, 0, nullptr, &outputBufferDescription, &ctx->contextFlags, &ctx->ctxtTimestamp);
			if (result == SEC_E_OK || result == SEC_I_CONTEXT_EXPIRED)
			{
				auto output = Array<uint8_t>(outputBuffer.cbBuffer, outputBuffer.cbBuffer);

				std::memcpy(output->getBase(), outputBuffer.pvBuffer, outputBuffer.cbBuffer);

				FreeContextBuffer(outputBuffer.pvBuffer);

				cbSuccess(output);
			}
			else
			{
				cbFailure(HX_CSTRING("Unexpected InitializeSecurityContext result"));
			}
		}
	};

	class Handshake_obj final : public hx::Object
	{
		HX_BEGIN_LOCAL_FUNC_S1(hx::LocalFunc, on_handshake_read, Handshake, handshake) HXARGC(2)
			void _hx_run(int count, String error)
		{
			if (hx::IsNull(error))
			{
				handshake->handshake(count);
			}
			else
			{
				handshake->cbFailure(error);
			}
		}
		HX_END_LOCAL_FUNC2((void))

			HX_BEGIN_LOCAL_FUNC_S1(hx::LocalFunc, on_handshake_sent, Handshake, handshake) HXARGC(2)
			void _hx_run(int count, String error)
		{
			if (hx::IsNull(error))
			{
				handshake->socket->read(
					handshake->buffer,
					handshake->offset,
					handshake->buffer->length - handshake->offset,
					new on_handshake_read(handshake),
					handshake->cbFailure);
			}
			else
			{
				handshake->cbFailure(error);
			}
		}
		HX_END_LOCAL_FUNC2((void))

	public:
		hx::asys::net::TcpSocket socket;
		SChannelContext* ctx;
		int offset;
		Dynamic cbSuccess;
		Dynamic cbFailure;
		Array<uint8_t> buffer;

		Handshake_obj(hx::asys::net::TcpSocket socket, Dynamic cbSuccess, Dynamic cbFailure, SChannelContext* ctx)
			: socket(socket)
			, cbSuccess(cbSuccess)
			, cbFailure(cbFailure)
			, offset(0)
			, buffer(Array<uint8_t>(std::numeric_limits<uint16_t>::max(), std::numeric_limits<uint16_t>::max()))
			, ctx(ctx) {}

		void __Mark(hx::MarkContext* __inCtx) override
		{
			HX_MARK_MEMBER(socket);
			HX_MARK_MEMBER(ctx);
			HX_MARK_MEMBER(offset);
			HX_MARK_MEMBER(cbSuccess);
			HX_MARK_MEMBER(cbFailure);
			HX_MARK_MEMBER(buffer);
		}

#if HXCPP_VISIT_ALLOCS
		void __Visit(hx::VisitContext* __inCtx) override
		{
			HX_VISIT_MEMBER(socket);
			HX_VISIT_MEMBER(ctx);
			HX_VISIT_MEMBER(offset);
			HX_VISIT_MEMBER(cbSuccess);
			HX_VISIT_MEMBER(cbFailure);
			HX_VISIT_MEMBER(buffer);
		}
#endif

		void startHandshake()
		{
			hx::EnterGCFreeZone();

			auto credential = SCH_CREDENTIALS{ 0 };
			credential.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_USE_STRONG_CRYPTO | SCH_CRED_NO_DEFAULT_CREDS;
			credential.dwVersion = SCH_CREDENTIALS_VERSION;

			if (SEC_E_OK != AcquireCredentialsHandle(NULL, UNISP_NAME, SECPKG_CRED_OUTBOUND, NULL, &credential, NULL, NULL, &ctx->credHandle, &ctx->credTimestamp))
			{
				hx::ExitGCFreeZone();

				cbFailure(HX_CSTRING("Failed to aquire credentials"));

				return;
			}

			auto outputBuffer = SecBuffer();
			auto outputBufferDescription = SecBufferDesc();

			init_sec_buffer(&outputBuffer, SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&outputBufferDescription, &outputBuffer, 1);

			if (SEC_I_CONTINUE_NEEDED != InitializeSecurityContext(&ctx->credHandle, nullptr, const_cast<char*>(ctx->host), ctx->requestFlags, 0, 0, nullptr, 0, &ctx->ctxtHandle, &outputBufferDescription, &ctx->contextFlags, &ctx->ctxtTimestamp))
			{
				hx::ExitGCFreeZone();

				cbFailure(HX_CSTRING("Failed to generate initial handshake"));

				return;
			}

			hx::ExitGCFreeZone();

			auto output = Array<uint8_t>(outputBuffer.cbBuffer, outputBuffer.cbBuffer);

			std::memcpy(output->getBase(), outputBuffer.pvBuffer, outputBuffer.cbBuffer);

			FreeContextBuffer(outputBuffer.pvBuffer);

			socket->write(output, 0, output->length, new on_handshake_sent(this), cbFailure);
		}

		void handshake(int count)
		{
			auto outputBuffers = std::array<SecBuffer, 3>();
			auto outputBufferDescription = SecBufferDesc();
			auto inputBuffers = std::array<SecBuffer, 2>();
			auto inputBufferDescription = SecBufferDesc();

			// Input Buffers
			init_sec_buffer(&inputBuffers[0], SECBUFFER_TOKEN, buffer->getBase(), count);
			init_sec_buffer(&inputBuffers[1], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&inputBufferDescription, inputBuffers.data(), inputBuffers.size());

			// Output Buffers
			init_sec_buffer(&outputBuffers[0], SECBUFFER_TOKEN, nullptr, 0);
			init_sec_buffer(&outputBuffers[1], SECBUFFER_ALERT, nullptr, 0);
			init_sec_buffer(&outputBuffers[2], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&outputBufferDescription, outputBuffers.data(), outputBuffers.size());

			auto result = SEC_E_OK;

			switch ((result = InitializeSecurityContext(&ctx->credHandle, &ctx->ctxtHandle, const_cast<char*>(ctx->host), ctx->requestFlags, 0, 0, &inputBufferDescription, 0, nullptr, &outputBufferDescription, &ctx->contextFlags, &ctx->ctxtTimestamp)))
			{
			case SEC_E_OK:
			{
				QueryContextAttributes(&ctx->ctxtHandle, SECPKG_ATTR_STREAM_SIZES, &ctx->sizes);

				cbSuccess(hx::asys::net::SecureSession(new SChannelSecureSession_obj(ctx)));

				break;
			}
			case SEC_E_INCOMPLETE_MESSAGE:
			{
				offset += count;

				socket->read(
					buffer,
					offset,
					buffer->length - offset,
					new on_handshake_read(this),
					cbFailure);

				break;
			}
			case SEC_I_INCOMPLETE_CREDENTIALS:
			{
				cbFailure(HX_CSTRING("Credentials requested"));

				break;
			}
			case SEC_I_CONTINUE_NEEDED:
			{
				if (outputBuffers[0].BufferType != SECBUFFER_TOKEN)
				{
					cbFailure(HX_CSTRING("Expected buffer to be a token type"));

					break;
				}
				if (outputBuffers[0].cbBuffer <= 0)
				{
					cbFailure(HX_CSTRING("Token buffer contains no data"));

					break;
				}

				auto output = Array<uint8_t>(outputBuffers[0].cbBuffer, outputBuffers[0].cbBuffer);

				std::memcpy(output->getBase(), outputBuffers[0].pvBuffer, outputBuffers[0].cbBuffer);

				FreeContextBuffer(outputBuffers[0].pvBuffer);

				socket->write(output, 0, output->length, new on_handshake_sent(this), cbFailure);

				break;
			}
			case SEC_E_WRONG_PRINCIPAL:
			{
				cbFailure(HX_CSTRING("SNI or certificate check failed"));

				break;
			}
			default:
			{
				cbFailure(HX_CSTRING("Creating security context failed"));

				break;
			}
			}
		}
	};
}

void hx::asys::net::SecureSession_obj::authenticateAsClient(TcpSocket socket, String host, Dynamic cbSuccess, Dynamic cbFailure)
{
	auto handshake = new Handshake_obj(socket, cbSuccess, cbFailure, new SChannelContext(host.utf8_str()));

	handshake->startHandshake();
}