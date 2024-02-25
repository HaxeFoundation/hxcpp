#pragma once

#include <hxcpp.h>

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <SubAuth.h>

#define SCHANNEL_USE_BLACKLISTS
#define SECURITY_WIN32

#include <schannel.h>
#include <security.h>
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
		void handshake(Array<uint8_t> input, Dynamic cbSuccess, Dynamic cbFailure);
		void encode(Array<uint8_t> input, int offset, int length, Dynamic cbSuccess, Dynamic cbFailure);

		static cpp::Pointer<SChannelContext> create(::String host);
	};
}