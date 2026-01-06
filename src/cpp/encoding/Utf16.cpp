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

	bool isAsciiBuffer(const View<uint8_t>& buffer)
	{
		auto i = int64_t{ 0 };
		while (i < buffer.length)
		{
			auto p = cpp::encoding::Utf16::codepoint(buffer.slice(i));

			if (p > 127)
			{
				return false;
			}

			i += cpp::encoding::Utf16::getByteCount(p);
		}

		return true;
	}

	String toAsciiString(const View<uint8_t>& buffer)
	{
		auto bytes  = buffer.length / sizeof(char16_t);
		auto chars  = View<char>(hx::InternalNew(bytes + 1, false), bytes * sizeof(char));
		auto i      = int64_t{ 0 };
		auto k      = int64_t{ 0 };

		while (i < buffer.length)
		{
			auto p = cpp::encoding::Utf16::codepoint(buffer.slice(i));

			chars[k++] = static_cast<char>(p);

			i += cpp::encoding::Utf16::getByteCount(p);
		}
		
		return String(chars.ptr.ptr, chars.length);
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

int cpp::encoding::Utf16::getByteCount(const null&)
{
	hx::NullReference("String", false);
	return 0;
}

int cpp::encoding::Utf16::getByteCount(const char32_t& codepoint)
{
	return codepoint <= 0xFFFF ? 2 : 4;
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

int cpp::encoding::Utf16::getCharCount(const null&)
{
	hx::NullReference("String", false);
	return 0;
}

int cpp::encoding::Utf16::getCharCount(const char32_t& codepoint)
{
	return getByteCount(codepoint) / sizeof(char16_t);
}

int64_t cpp::encoding::Utf16::getCharCount(const String& string)
{
	return getByteCount(string) / sizeof(char16_t);
}

int cpp::encoding::Utf16::encode(const null&, const cpp::marshal::View<uint8_t>& buffer)
{
	hx::NullReference("String", false);
	return 0;
}

int64_t cpp::encoding::Utf16::encode(const String& string, const cpp::marshal::View<uint8_t>& buffer)
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

#if defined(HX_SMART_STRINGS)
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
#endif
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

		auto i = int64_t{ 0 };
		for (auto k = 0; k < string.length; k++)
		{
			i += encode(static_cast<char32_t>(string.raw_ptr()[k]), buffer.slice(i));
		}

		return bytes;
	}
}

int cpp::encoding::Utf16::encode(const char32_t& codepoint, const cpp::marshal::View<uint8_t>& buffer)
{
	if (codepoint < 0xD800)
	{
		Marshal::writeUInt16(buffer, static_cast<char16_t>(codepoint));

		return 2;
	}
	else if (codepoint < 0xE000)
	{
		// D800 - DFFF is invalid

		return hx::Throw(HX_CSTRING("Invalid UTF16"));
	}
	else if (codepoint < 0x10000)
	{
		Marshal::writeUInt16(buffer, static_cast<char16_t>(codepoint));

		return 2;
	}
	else if (codepoint < 0x110000)
	{
		auto staging = std::array<uint8_t, 4>();
		auto fst     = View<uint8_t>(staging.data(), 2);
		auto snd     = View<uint8_t>(staging.data() + 2, 2);
		auto all     = View<uint8_t>(staging.data(), staging.size());

		Marshal::writeUInt16(fst, 0xD800 + (((codepoint - 0x10000) >> 10) & 0x3FF));
		Marshal::writeUInt16(snd, 0xDC00 + ((codepoint - 0x10000) & 0x3FF));

		all.copyTo(buffer);

		return 4;
	}

	return 0;
}

String cpp::encoding::Utf16::decode(const cpp::marshal::View<uint8_t>& buffer)
{
	if (buffer.isEmpty())
	{
		return String::emptyString;
	}

	if (isAsciiBuffer(buffer))
	{
		return toAsciiString(buffer);
	}

#if defined(HX_SMART_STRINGS)
	auto i = int64_t{ 0 };
    while (i < buffer.length)
    {
		auto p = codepoint(buffer.slice(i));

		i += getByteCount(p);
    }

	auto chars   = i / sizeof(char16_t);
	auto backing = View<char16_t>(::String::allocChar16Ptr(chars), chars);
	auto output  = backing.reinterpret<uint8_t>();
	auto k       = int64_t{ 0 };

	i = 0;
	while (i < buffer.length)
	{
		auto p = codepoint(buffer.slice(i));

		i += getByteCount(p);
		k += encode(p, output.slice(k));
	}

	return String(backing.ptr.ptr, chars);
#else
	return hx::Throw(HX_CSTRING("Not Implemented : UTF16 decode when HX_SMART_STRINGS is not defined"));
#endif
}

char32_t cpp::encoding::Utf16::codepoint(const cpp::marshal::View<uint8_t>& buffer)
{
	auto first = static_cast<char16_t>(Marshal::readUInt16(buffer));

	if (0xD800 <= first && first < 0xDc00)
	{
		auto second = static_cast<char16_t>(Marshal::readUInt16(buffer.slice(2)));
		if (0xDC00 <= second && second < 0xE000)
		{
			return static_cast<char32_t>((((first - 0xD800) << 10) | (second - 0xDC00)) + 0x10000);
		}

		return int{ hx::Throw(HX_CSTRING("Invalid UTF16")) };
	}
	else
	{
		return static_cast<char32_t>(first);
	}
}
