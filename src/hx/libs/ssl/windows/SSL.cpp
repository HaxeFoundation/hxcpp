#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"

#define NOMINMAX

#include <Windows.h>
#include <SubAuth.h>

#define SCHANNEL_USE_BLACKLISTS
#define SECURITY_WIN32

#include <schannel.h>
#include <security.h>
#include <shlwapi.h>
#include <comdef.h>
#include <array>
#include <algorithm>
#include <assert.h>

#define printf(...)

namespace
{
	const int TLS_MAX_PACKET_SIZE = std::numeric_limits<uint16_t>::max() + 512;

	struct SocketWrapper : public hx::Object
	{
		HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdSocket };
		SOCKET socket;
	};

	struct SChannelContext : public ::hx::Object
	{
		::String host;
		::Dynamic socket;

		/**
		 * Buffer for handshake or encrypted data.
		 * This buffer is a fixed sized.
		 */
		::Array<uint8_t> input;
		/**
		 * Number of bytes in the input array.
		 */
		int received;

		/**
		 * buffered message data.
		 * Array is expanded and shrunk as data is added and read.
		 */
		::Array<uint8_t> decrypted;

		CredHandle credHandle;
		TimeStamp credTimestamp;
		CtxtHandle ctxtHandle;
		TimeStamp ctxtTimestamp;

		DWORD requestFlags;
		DWORD contextFlags;

		SecPkgContext_StreamSizes sizes;

		SChannelContext(::String inHost)
			: host(inHost)
			, socket(null())
			, input(10000, 10000)
			, decrypted(0, 0)
			, received(0)
			, credHandle()
			, credTimestamp()
			, ctxtHandle()
			, ctxtTimestamp()
			, requestFlags(ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM)
			, contextFlags(0)
			, sizes()
		{
			HX_OBJ_WB_NEW_MARKED_OBJECT(this);
		}

		void __Mark(HX_MARK_PARAMS) override
		{
			HX_MARK_MEMBER(host);
			HX_MARK_MEMBER(socket);
			HX_MARK_MEMBER(input);
			HX_MARK_MEMBER(decrypted);
		}

#ifdef  HXCPP_VISIT_ALLOCS
		void __Visit(HX_VISIT_PARAMS) override
		{
			HX_VISIT_MEMBER(host);
			HX_VISIT_MEMBER(socket);
			HX_VISIT_MEMBER(input);
			HX_VISIT_MEMBER(decrypted);
		}
#endif

	};

	void DestroyCert(Dynamic obj)
	{
		auto cert = obj.Cast<hx::ssl::windows::Cert>();

		CertFreeCertificateContext(cert->ctx);

		cert->ctx = nullptr;
	}

	void DestroyKey(Dynamic obj)
	{
		auto key = obj.Cast<hx::ssl::windows::Key>();

		NCryptFreeObject(key->ctx);
	}

	static void init_sec_buffer(SecBuffer* buffer, unsigned long type, void* data, unsigned long size)
	{
		buffer->cbBuffer = size;
		buffer->BufferType = type;
		buffer->pvBuffer = data;
	}

	static void init_sec_buffer_desc(SecBufferDesc* desc, SecBuffer* buffers, unsigned long buffer_count)
	{
		desc->ulVersion = SECBUFFER_VERSION;
		desc->pBuffers = buffers;
		desc->cBuffers = buffer_count;
	}

	void block_error()
	{
		auto err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK || err == WSAEALREADY)
		{
			hx::Throw(HX_CSTRING("Blocking"));
		}
		else
		{
			hx::Throw(HX_CSTRING("EOF"));
		}
	}
}

hx::ssl::windows::Cert_obj::Cert_obj(PCCERT_CONTEXT inCtx) : ctx(inCtx)
{
	_hx_set_finalizer(this, DestroyCert);
}

String hx::ssl::windows::Cert_obj::toString()
{
	return HX_CSTRING("cert");
}

hx::ssl::windows::Key_obj::Key_obj(NCRYPT_KEY_HANDLE inCtx) : ctx(inCtx)
{
	_hx_set_finalizer(this, DestroyKey);
}

String hx::ssl::windows::Key_obj::toString()
{
	return HX_CSTRING("key");
}

void _hx_ssl_init()
{
	//
}

Dynamic _hx_ssl_new(Dynamic hconf)
{
	printf("creating new schannel context\n");

	return new SChannelContext(HX_CSTRING(""));
}

void _hx_ssl_close(Dynamic hssl)
{
	//
}

void _hx_ssl_debug_set(int i)
{
	//
}

void _hx_ssl_handshake(Dynamic handle)
{
	auto ctx     = (SChannelContext*)handle.mPtr;
	auto wrapper = (SocketWrapper*)ctx->socket.mPtr;
	auto result  = SECURITY_STATUS{ SEC_E_OK };

	auto credentials = SCHANNEL_CRED();
	credentials.dwFlags   = SCH_CRED_AUTO_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS | SCH_USE_STRONG_CRYPTO;
	credentials.dwVersion = SCHANNEL_CRED_VERSION;
	credentials.grbitEnabledProtocols = SP_PROT_TLS1_2;

	if (SEC_E_OK != (result = AcquireCredentialsHandleA(nullptr, LPSTR(UNISP_NAME), SECPKG_CRED_OUTBOUND, nullptr, &credentials, nullptr, nullptr, &ctx->credHandle, &ctx->credTimestamp)))
	{
		hx::Throw(HX_CSTRING("Failed to acquire credentials handle"));
	}

	hx::strbuf hostBuffer;

	auto hostString    = const_cast<SEC_CHAR*>(ctx->host.utf8_str(&hostBuffer));
	auto outputBuffer0 = std::vector<char>(TLS_MAX_PACKET_SIZE);
	auto outputBuffer1 = std::vector<char>(1024);
	auto outputBuffer2 = std::vector<char>(1024);

	auto initial  = true;

	auto outputBuffers = std::array<SecBuffer, 3>();
	auto outputBufferDescription = SecBufferDesc();

	auto inputBuffers = std::array<SecBuffer, 2>();
	auto inputBufferDescription = SecBufferDesc();

	while (true)
	{
		// Input Buffers
		init_sec_buffer(&inputBuffers[0], SECBUFFER_TOKEN, ctx->input->getBase(), ctx->received);
		init_sec_buffer(&inputBuffers[1], SECBUFFER_EMPTY, nullptr, 0);
		init_sec_buffer_desc(&inputBufferDescription, inputBuffers.data(), inputBuffers.size());

		// Output Buffers
		init_sec_buffer(&outputBuffers[0], SECBUFFER_TOKEN, outputBuffer0.data(), outputBuffer0.size());
		init_sec_buffer(&outputBuffers[1], SECBUFFER_ALERT, outputBuffer1.data(), outputBuffer1.size());
		init_sec_buffer(&outputBuffers[2], SECBUFFER_EMPTY, nullptr, 0);
		init_sec_buffer_desc(&outputBufferDescription, outputBuffers.data(), outputBuffers.size());

		result =
			InitializeSecurityContextA(
				&ctx->credHandle,
				initial ? nullptr : &ctx->ctxtHandle,
				hostString,
				ctx->requestFlags,
				0,
				0,
				initial ? nullptr : &inputBufferDescription,
				0,
				&ctx->ctxtHandle,
				&outputBufferDescription,
				&ctx->contextFlags, 
				&ctx->ctxtTimestamp);

		initial = false;

		switch (result)
		{
		case SEC_E_OK:
		{
			printf("handshake complete\n");

			if (SECBUFFER_EXTRA == inputBuffers[1].BufferType)
			{
				std::memmove(ctx->input->getBase(), ctx->input->getBase() + ctx->received - inputBuffers[1].cbBuffer, inputBuffers[1].cbBuffer);

				ctx->received = inputBuffers[1].cbBuffer;

				printf("%i bytes of extra data found\n", inputBuffers[1].cbBuffer);
			}
			else
			{
				ctx->received = 0;
			}

			QueryContextAttributes(&ctx->ctxtHandle, SECPKG_ATTR_STREAM_SIZES, &ctx->sizes);

			ctx->input->EnsureSize(ctx->sizes.cbMaximumMessage);

			return;
		}

		case SEC_E_INCOMPLETE_MESSAGE:
		{
			// According to MSDN when we get SEC_E_INCOMPLETE_MESSAGE the empty buffer
			// should contain how much data we need for the call to pass.
			// https://learn.microsoft.com/en-us/windows/win32/secauthn/acceptsecuritycontext--schannel
			if (SECBUFFER_MISSING != inputBuffers[1].BufferType)
			{
				auto targetReceived = ctx->received + inputBuffers[1].cbBuffer;
				
				// Loop until we've read at least the required amount to avoid excessive calls to InitializeSecurityContextA
				while (ctx->received < targetReceived)
				{
					auto read = recv(wrapper->socket, ctx->input->getBase() + ctx->received, ctx->input->length - ctx->received, 0);
					if (read <= 0)
					{
						hx::Throw(HX_CSTRING("Failed to read handshake message"));
					}

					ctx->received += read;
				}
			}
			break;
		}

		case SEC_I_CONTINUE_NEEDED:
		{
			// First check if we have data which was not consumed by the previous InitializeSecurityContextA call.
			// If so shuffle it to the front of the input buffer and set received accordingly.
			// Otherwise we can just set received to zero as we've consumed all data in the buffer.
			if (SECBUFFER_EXTRA == inputBuffers[1].BufferType)
			{
				std::memmove(ctx->input->getBase(), ctx->input->getBase() + ctx->received - inputBuffers[1].cbBuffer, inputBuffers[1].cbBuffer);

				ctx->received = inputBuffers[1].cbBuffer;
			}
			else
			{
				ctx->received = 0;
			}

			// Send all data in the output token buffer to the remote end.
			// Apparently the third buffer can sometimes be extra data, do we send that along as well?
			auto toSend    = static_cast<char*>(outputBuffers[0].pvBuffer);
			auto remaining = outputBuffers[0].cbBuffer;

			while (0 != remaining)
			{
				auto sent = send(wrapper->socket, toSend, remaining, 0);
				if (sent <= 0)
				{
					hx::Throw(HX_CSTRING("Failed to send handshake message"));
				}

				remaining -= sent;
				toSend += sent;
			}

			// Read more data from the remote end and loop.
			auto read = recv(wrapper->socket, ctx->input->getBase() + ctx->received, ctx->input->length - ctx->received, 0);
			if (read <= 0)
			{
				hx::Throw(HX_CSTRING("Failed to read handshake message"));
			}

			ctx->received += read;

			break;
		}

		default:
			hx::Throw(String::create(_com_error(result).ErrorMessage()));
		}
	}
}

void _hx_ssl_set_socket(Dynamic hssl, Dynamic hsocket)
{
	auto ctx = (SChannelContext*)hssl.mPtr;

	ctx->socket = hsocket;
}

void _hx_ssl_set_hostname(Dynamic hssl, String hostname)
{
	auto ctx = (SChannelContext*)hssl.mPtr;

	ctx->host = hostname;
}

Dynamic _hx_ssl_get_peer_certificate(Dynamic hssl)
{
	return null();
}

bool _hx_ssl_get_verify_result(Dynamic hssl)
{
	return false;
}

void _hx_ssl_send_char(Dynamic hssl, int v)
{
	// Lazy...

	auto buffer = Array<unsigned char>(1, 1);
	buffer[0] = static_cast<unsigned char>(v);

	_hx_ssl_write(hssl, buffer);
}

int _hx_ssl_send(Dynamic hssl, Array<unsigned char> buf, int p, int l)
{
	auto ctx       = (SChannelContext*)hssl.mPtr;
	auto wrapper   = (SocketWrapper*)ctx->socket.mPtr;
	auto remaining = buf->length;

	auto stagingBuffer = std::vector<unsigned char>(ctx->sizes.cbHeader + ctx->sizes.cbMaximumMessage + ctx->sizes.cbTrailer);
	auto outputBuffers = std::array<SecBuffer, 3>();
	auto outputBufferDescription = SecBufferDesc();

	while (remaining > 0)
	{
		auto usage  = std::min(static_cast<size_t>(remaining), static_cast<size_t>(ctx->sizes.cbMaximumMessage));
		auto cursor = buf->length - remaining;

		init_sec_buffer(&outputBuffers[0], SECBUFFER_STREAM_HEADER, stagingBuffer.data(), ctx->sizes.cbHeader);
		init_sec_buffer(&outputBuffers[1], SECBUFFER_DATA, stagingBuffer.data() + ctx->sizes.cbHeader, usage);
		init_sec_buffer(&outputBuffers[2], SECBUFFER_STREAM_TRAILER, stagingBuffer.data() + ctx->sizes.cbHeader + usage, ctx->sizes.cbTrailer);
		init_sec_buffer_desc(&outputBufferDescription, outputBuffers.data(), outputBuffers.size());

		std::memcpy(outputBuffers[1].pvBuffer, buf->getBase() + cursor, usage);

		auto result = SECURITY_STATUS{ SEC_E_OK };
		if (SEC_E_OK != (result = EncryptMessage(&ctx->ctxtHandle, 0, &outputBufferDescription, 0)))
		{
			hx::Throw(String::create(_com_error(result).ErrorMessage()));
		}

		printf("encrypted %zi bytes\n", usage);

		auto sent  = 0;
		auto total = ctx->sizes.cbHeader + usage + ctx->sizes.cbTrailer;
		while (sent < total)
		{
			auto count = send(wrapper->socket, reinterpret_cast<char*>(stagingBuffer.data()) + sent, total - sent, 0);
			if (count <= 0)
			{
				hx::Throw(HX_CSTRING("Socket send error"));
			}

			sent += count;
		}

		remaining -= sent;
	}

	printf("sent %i bytes\n", l);

	return l;
}

void _hx_ssl_write(Dynamic hssl, Array<unsigned char> buf)
{
	auto _ = _hx_ssl_send(hssl, buf, 0, buf->length);
}

int _hx_ssl_recv_char(Dynamic hssl)
{
	return hx::Throw(HX_CSTRING("Not Implemented"));
}

int _hx_ssl_recv(Dynamic hssl, Array<unsigned char> buf, int p, int l)
{
	auto ctx     = (SChannelContext*)hssl.mPtr;
	auto wrapper = (SocketWrapper*)ctx->socket.mPtr;

	auto buffers = std::array<SecBuffer, 4>();
	auto bufferDescription = SecBufferDesc();

	while (true)
	{
		if (ctx->decrypted->length > 0)
		{
			auto taking = std::min(l, ctx->decrypted->length);

			printf("taking %i cached bytes\n", taking);

			buf->memcpy(p, &ctx->decrypted[0], taking);

			ctx->decrypted->removeRange(0, taking);

			return taking;
		}

		if (ctx->received > 0)
		{
			auto result = SECURITY_STATUS{ SEC_E_OK };

			init_sec_buffer(&buffers[0], SECBUFFER_DATA, ctx->input->getBase(), ctx->received);
			init_sec_buffer(&buffers[1], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer(&buffers[2], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer(&buffers[3], SECBUFFER_EMPTY, nullptr, 0);
			init_sec_buffer_desc(&bufferDescription, buffers.data(), buffers.size());

			switch (result = DecryptMessage(&ctx->ctxtHandle, &bufferDescription, 0, nullptr))
			{
			case SEC_E_OK:
			{
				assert(buffers[0].BufferType == SECBUFFER_STREAM_HEADER);
				assert(buffers[1].BufferType == SECBUFFER_DATA);
				assert(buffers[2].BufferType == SECBUFFER_STREAM_TRAILER);

				printf("decrypting successful! (given %i bytes)\n", ctx->received);
				printf("header  ( ptr : %p, len : %i)\n", buffers[0].pvBuffer, buffers[0].cbBuffer);
				printf("message ( ptr : %p, len : %i)\n", buffers[1].pvBuffer, buffers[1].cbBuffer);
				printf("trailer ( ptr : %p, len : %i)\n", buffers[2].pvBuffer, buffers[2].cbBuffer);

				if (buffers[3].BufferType == SECBUFFER_EXTRA)
				{
					printf("extra   ( ptr : %p, len : %i)\n", buffers[3].pvBuffer, buffers[3].cbBuffer);
				}

				for (auto i = 0; i < buffers[1].cbBuffer; i++)
				{
					ctx->decrypted->push(static_cast<uint8_t*>(buffers[1].pvBuffer)[i]);
				}

				if (SECBUFFER_EXTRA == buffers[3].BufferType)
				{
					printf("moving %i to extra buffer\n", buffers[3].cbBuffer);

					std::memmove(ctx->input->getBase(), buffers[3].pvBuffer, buffers[3].cbBuffer);

					ctx->received = buffers[3].cbBuffer;
				}
				else
				{
					printf("no extra buffer, resetting recieved\n");

					ctx->received = 0;
				}

				break;
			}
			case SEC_E_INCOMPLETE_MESSAGE:
			{
				assert(buffers[0].BufferType == SECBUFFER_MISSING);
				assert(buffers[0].cbBuffer > 0);

				printf("incomplete message\n");
				printf("\tSECBUFFER_MISSING indicates it wants %i more bytes\n", buffers[0].cbBuffer);
				printf("\tcurrent receive position is %i, so %i free space\n", ctx->received, ctx->input->length - ctx->received);

				if (ctx->received + buffers[0].cbBuffer > ctx->input->length)
				{
					printf("\t\tgrowing input buffer\n");

					ctx->input->EnsureSize(ctx->received + buffers[0].cbBuffer);
				}

				auto count = recv(wrapper->socket, ctx->input->getBase() + ctx->received, buffers[0].cbBuffer, 0);
				if (count <= 0)
				{
					printf("about to throw leaving behind %i encrypted and %i decrypted bytes\n", ctx->received, ctx->decrypted->length);

					block_error();
				}

				ctx->received += count;

				printf("socket read, added %i\n", count);

				break;
			}
			default:
				// Could we error and have potentially decoded data?
				hx::Throw(String::create(_com_error(result).ErrorMessage()));
			}
		}
		else
		{
			printf("no buffered input, reading block from socket (%i)\n", ctx->sizes.cbBlockSize);

			auto count = recv(wrapper->socket, ctx->input->getBase(), ctx->input->length, 0);
			if (count <= 0)
			{
				block_error();
			}

			printf("added to received buffer (total %i)\n", count);

			ctx->received = count;
		}
	}

	return 0;
}

Array<unsigned char> _hx_ssl_read(Dynamic hssl)
{
	return hx::Throw(HX_CSTRING("Not Implemented"));
}

Dynamic _hx_ssl_conf_new(bool server)
{
	return null();
}

void _hx_ssl_conf_close(Dynamic hconf)
{
	//
}

void _hx_ssl_conf_set_ca(Dynamic hconf, Dynamic hcert)
{
	//
}

void _hx_ssl_conf_set_verify(Dynamic hconf, int mode)
{
	//
}

void _hx_ssl_conf_set_cert(Dynamic hconf, Dynamic hcert, Dynamic hpkey)
{
	//
}

void _hx_ssl_conf_set_servername_callback(Dynamic hconf, Dynamic obj)
{
	//
}

Dynamic _hx_ssl_cert_load_defaults()
{
	return null();
}