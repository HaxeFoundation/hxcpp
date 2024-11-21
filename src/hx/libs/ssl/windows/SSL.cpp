#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"

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
		}

#ifdef  HXCPP_VISIT_ALLOCS
		void __Visit(HX_VISIT_PARAMS) override
		{
			HX_VISIT_MEMBER(host);
			HX_VISIT_MEMBER(socket);
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

	//auto credentials = SCH_CREDENTIALS();
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
	auto inputBuffer   = std::vector<char>(TLS_MAX_PACKET_SIZE);
	auto outputBuffer0 = std::vector<char>(TLS_MAX_PACKET_SIZE);
	auto outputBuffer1 = std::vector<char>(1024);
	auto outputBuffer2 = std::vector<char>(1024);

	auto received = 0;
	auto initial  = true;

	auto outputBuffers = std::array<SecBuffer, 3>();
	auto outputBufferDescription = SecBufferDesc();

	auto inputBuffers = std::array<SecBuffer, 2>();
	auto inputBufferDescription = SecBufferDesc();

	while (true)
	{
		// Input Buffers
		init_sec_buffer(&inputBuffers[0], SECBUFFER_TOKEN, inputBuffer.data(), received);
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
			QueryContextAttributes(&ctx->ctxtHandle, SECPKG_ATTR_STREAM_SIZES, &ctx->sizes);

			return;
		}

		case SEC_E_INCOMPLETE_MESSAGE:
		{
			// According to MSDN when we get SEC_E_INCOMPLETE_MESSAGE the empty buffer
			// should contain how much data we need for the call to pass.
			// https://learn.microsoft.com/en-us/windows/win32/secauthn/acceptsecuritycontext--schannel
			if (SECBUFFER_MISSING != inputBuffers[1].BufferType)
			{
				auto targetReceived = received + inputBuffers[1].cbBuffer;
				
				// Loop until we've read at least the required amount to avoid excessive calls to InitializeSecurityContextA
				while (received < targetReceived)
				{
					auto read = recv(wrapper->socket, inputBuffer.data() + received, inputBuffer.size() - received, 0);
					if (read <= 0)
					{
						hx::Throw(HX_CSTRING("Failed to read handshake message"));
					}

					received += read;
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
				inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + received - inputBuffers[1].cbBuffer);
				inputBuffer.resize(TLS_MAX_PACKET_SIZE);

				received = inputBuffers[1].cbBuffer;
			}
			else
			{
				received = 0;
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
			auto read = recv(wrapper->socket, inputBuffer.data() + received, inputBuffer.size() - received, 0);
			if (read <= 0)
			{
				hx::Throw(HX_CSTRING("Failed to read handshake message"));
			}

			received += read;

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
	//
}

int _hx_ssl_send(Dynamic hssl, Array<unsigned char> buf, int p, int l)
{
	return 0;
}

void _hx_ssl_write(Dynamic hssl, Array<unsigned char> buf)
{
	//
}

int _hx_ssl_recv_char(Dynamic hssl)
{
	return 0;
}

int _hx_ssl_recv(Dynamic hssl, Array<unsigned char> buf, int p, int l)
{
	return 0;
}

Array<unsigned char> _hx_ssl_read(Dynamic hssl)
{
	return null();
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