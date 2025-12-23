#include <hxcpp.h>
#include <array>

using namespace cpp::marshal;

namespace
{
	bool isSurrogate(char32_t codepoint)
	{
		return codepoint >= 0xd800 && codepoint < 0xe000;
	}

	bool isLowSurrogate(char32_t codepoint)
	{
		return codepoint >= 0xdc00 && codepoint < 0xe000;
	}

	bool isHighSurrogate(char32_t codepoint)
	{
		return codepoint >= 0xd800 && codepoint < 0xdc00;
	}
}

bool cpp::encoding::Utf16::isEncoded(const String& string)
{
	if (null() == string)
	{
		hx::NullReference("String", false);
	}

	return string.isUTF16Encoded();
}

int32_t cpp::encoding::Utf16::getByteCount(const char32_t& codepoint)
{
	if (codepoint >= 0x10000)
	{
		if (codepoint < 0x110000)
		{
			return 4;
		}
	}

	return 2;
}

int64_t cpp::encoding::Utf16::getByteCount(const String& string)
{
	if (null() == string)
	{
		hx::NullReference("String", false);
	}

	if (string.isUTF16Encoded())
	{
		return string.length * sizeof(char16_t);
	}
	else
	{
		auto bytes = int64_t{ 0 };
		for (auto i = 0; i < string.length; i++)
		{
			bytes += getByteCount(static_cast<char32_t>(string.raw_ptr()[i]));
		}

		return bytes;
	}
}

int64_t cpp::encoding::Utf16::encode(const String& string, cpp::marshal::View<uint8_t> buffer)
{
	if (null() == string)
	{
		hx::NullReference("String", false);
	}

	if (0 == string.length)
	{
		return 0;
	}

	if (buffer.isEmpty())
	{
		return hx::Throw(HX_CSTRING("Buffer too small"));
	}

	if (string.isUTF16Encoded())
	{
		auto src = cpp::marshal::View<uint8_t>(reinterpret_cast<uint8_t*>(const_cast<char16_t*>(string.raw_wptr())), string.length * sizeof(char16_t));

		if (src.tryCopyTo(buffer))
		{
			return src.length;
		}
		else
		{
			return hx::Throw(HX_CSTRING("Buffer too small"));
		}
	}
	else
	{
		auto bytes = int64_t{ 0 };
		for (auto i = 0; i < string.length; i++)
		{
			bytes += getByteCount(static_cast<char32_t>(string.raw_ptr()[i]));
		}

		if (bytes > buffer.length)
		{
			return hx::Throw(HX_CSTRING("Buffer too small"));
		}

		for (auto i = 0; i < string.length; i++)
		{
			buffer = buffer.slice(encode(static_cast<char32_t>(string.raw_ptr()[i]), buffer));
		}

		return bytes;
	}
}

int64_t cpp::encoding::Utf16::encode(const char32_t& codepoint, cpp::marshal::View<uint8_t> buffer)
{
	if (codepoint >= 0x10000)
	{
		auto over = codepoint - 0x10000;
		if (over >= 0x10000)
		{
			Marshal::writeUInt16(buffer, 0xFFFD);

			return 2;
		}
		else
		{
			auto staging = std::array<uint16_t, 2>();
			staging[0] = (over >> 10) + 0xD800;
			staging[1] = (over & 0x3FF) + 0xDC00;

			Marshal::writeUInt32(buffer, *reinterpret_cast<uint32_t*>(staging.data()));

			return 4;
		}
	}
	else if (isSurrogate(codepoint))
	{
		Marshal::writeUInt16(buffer, 0xFFFD);

		return 2;
	}
	else
	{
		Marshal::writeUInt16(buffer, static_cast<uint16_t>(codepoint));

		return 2;
	}
}

String cpp::encoding::Utf16::decode(cpp::marshal::View<uint8_t> buffer)
{
	if (buffer.isEmpty())
	{
		return String::emptyString;
	}

	auto bytes     = int64_t{ 0 };
    auto codepoint = char32_t{ 0 };
    auto i         = int64_t{ 0 };

    while (i < buffer.length)
    {
        i     += decode(buffer.slice(i), codepoint);
        bytes += getByteCount(codepoint);
    }

	auto backing = static_cast<uint8_t*>(hx::NewGCPrivate(0, bytes + sizeof(char16_t)));
	auto output  = View<uint8_t>(backing, bytes);

	while (false == buffer.isEmpty())
	{
		buffer = buffer.slice(decode(buffer, codepoint));
		output = output.slice(encode(codepoint, output));
	}

	reinterpret_cast<uint32_t*>(backing)[-1] |= HX_GC_STRING_CHAR16_T;

	return String(reinterpret_cast<char16_t*>(backing), bytes / sizeof(char16_t));
}

int64_t cpp::encoding::Utf16::decode(cpp::marshal::View<uint8_t> buffer, char32_t& codepoint)
{
	auto first = static_cast<char16_t>(Marshal::readUInt16(buffer));

	if (0xD800 <= first && first < 0xDc00)
	{
		auto second = static_cast<char16_t>(Marshal::readUInt16(buffer.slice(2)));
		if (0xDC00 <= second && second < 0xE000)
		{
			codepoint = ((((first - 0xD800) << 10) | (second - 0xDC00)) + 0x10000);

			return 4;
		}

		return hx::Throw(HX_CSTRING("Invalid UTF16"));
	}
	else
	{
		codepoint = first;

		return 2;
	}
}
