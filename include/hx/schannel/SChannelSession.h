#pragma once

#include <hxcpp.h>

#include <winsock2.h>
#include <windows.h>
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <shlwapi.h>

namespace hx::schannel
{
	class SChannelContext
	{
		const int TLS_MAX_PACKET_SIZE = (16384 + 512);

		SChannelContext(const char* inHost);
	public:
		const char* host;

		CredHandle credHandle;
		TimeStamp credTimestamp;
		CtxtHandle ctxtHandle;
		TimeStamp ctxtTimestamp;

		DWORD requestFlags;
		DWORD contextFlags;

		SecPkgContext_StreamSizes sizes;

		Array<uint8_t> startHandshake();
		hx::Anon handshake(Array<uint8_t> input);

		static cpp::Pointer<SChannelContext> create(::String host);
	};
}