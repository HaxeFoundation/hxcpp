#include <hx/schannel/SChannelSession.h>
#include <array>

namespace
{
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

hx::schannel::SChannelContext::SChannelContext(const char* inHost)
	: host(inHost)
{
}

Array<uint8_t> hx::schannel::SChannelContext::startHandshake()
{
	/*auto parameter = TLS_PARAMETERS{ 0 };
	parameter.grbitDisabledProtocols = SP_PROT_TLS1_3_CLIENT | SP_PROT_TLS1_3_SERVER;*/

	auto credential = SCH_CREDENTIALS{ 0 };
	credential.dwFlags   = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_USE_STRONG_CRYPTO | SCH_CRED_NO_DEFAULT_CREDS;
	credential.dwVersion = SCH_CREDENTIALS_VERSION;
	/*credential.cTlsParameters = 1;
	credential.pTlsParameters = &parameter;*/

	if (SEC_E_OK != AcquireCredentialsHandle(NULL, UNISP_NAME, SECPKG_CRED_OUTBOUND, NULL, &credential, NULL, NULL, &credHandle, &credTimestamp))
	{
		hx::Throw(HX_CSTRING("Failed to aquire credentials"));
	}

	auto outputBuffer            = SecBuffer();
	auto outputBufferDescription = SecBufferDesc();

	init_sec_buffer(&outputBuffer, SECBUFFER_EMPTY, nullptr, 0);
	init_sec_buffer_desc(&outputBufferDescription, &outputBuffer, 1);

	requestFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

	if (SEC_I_CONTINUE_NEEDED != InitializeSecurityContext(&credHandle, nullptr, const_cast<char*>(host), requestFlags, 0, 0, nullptr, 0, &ctxtHandle, &outputBufferDescription, &contextFlags, &ctxtTimestamp))
	{
		hx::Throw(HX_CSTRING("Failed to generate initial handshake"));
	}

	auto buffer = Array<uint8_t>(outputBuffer.cbBuffer, outputBuffer.cbBuffer);

	std::memcpy(buffer->getBase(), outputBuffer.pvBuffer, outputBuffer.cbBuffer);

	FreeContextBuffer(outputBuffer.pvBuffer);

	return buffer;
}

hx::Anon hx::schannel::SChannelContext::handshake(Array<uint8_t> input)
{
	auto outputBuffers           = std::array<SecBuffer, 3>();
	auto outputBufferDescription = SecBufferDesc();
	auto inputBuffers            = std::array<SecBuffer, 2>();
	auto inputBufferDescription  = SecBufferDesc();

	// Input Buffers
	init_sec_buffer(&inputBuffers[0], SECBUFFER_TOKEN, input->getBase(), input->length);
	init_sec_buffer(&inputBuffers[1], SECBUFFER_EMPTY, nullptr, 0);
	init_sec_buffer_desc(&inputBufferDescription, inputBuffers.data(), inputBuffers.size());

	// Output Buffers
	init_sec_buffer(&outputBuffers[0], SECBUFFER_TOKEN, nullptr, 0);
	init_sec_buffer(&outputBuffers[1], SECBUFFER_ALERT, nullptr, 0);
	init_sec_buffer(&outputBuffers[2], SECBUFFER_EMPTY, nullptr, 0);
	init_sec_buffer_desc(&outputBufferDescription, outputBuffers.data(), outputBuffers.size());

	auto result = SEC_E_OK;

	switch ((result = InitializeSecurityContext(&credHandle, &ctxtHandle, const_cast<char*>(host), requestFlags, 0, 0, &inputBufferDescription, 0, NULL, &outputBufferDescription, &contextFlags, &ctxtTimestamp)))
	{
	case SEC_E_OK:
	{
		QueryContextAttributes(&ctxtHandle, SECPKG_ATTR_STREAM_SIZES, &sizes);

		return hx::Anon_obj::Create(2)->setFixed(0, "result", 0)->setFixed(1, "data", null());
	}
	case SEC_E_INCOMPLETE_MESSAGE:
	{
		return hx::Anon_obj::Create(2)->setFixed(0, "result", -1)->setFixed(1, "data", null());
	}
	case SEC_I_INCOMPLETE_CREDENTIALS:
	{
		hx::Throw(HX_CSTRING("Credentials requested"));
	}
	case SEC_I_CONTINUE_NEEDED:
	{
		for (auto&& buffer : outputBuffers)
		{
			if (SECBUFFER_TOKEN == buffer.BufferType)
			{
				auto output = Array<uint8_t>(buffer.cbBuffer, buffer.cbBuffer);

				std::memcpy(output->getBase(), buffer.pvBuffer, buffer.cbBuffer);

				FreeContextBuffer(buffer.pvBuffer);

				return hx::Anon_obj::Create(2)->setFixed(0, "result", 0)->setFixed(1, "data", output);
			}
		}
	}
	case SEC_E_WRONG_PRINCIPAL:
	{
		hx::Throw("SNI or certificate check failed");
	}
	default:
	{
		hx::Throw("Creating security context failed");
	}
	}
}

cpp::Pointer<hx::schannel::SChannelContext> hx::schannel::SChannelContext::create(::String host)
{
	return cpp::Pointer<SChannelContext>(new SChannelContext(host.utf8_str()));
}