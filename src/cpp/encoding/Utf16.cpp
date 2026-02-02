#include <hxcpp.h>
#include <array>
#include <algorithm>
#include <simdutf.h>

using namespace cpp::marshal;
using namespace simdutf;

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
	return getCharCount(codepoint) * sizeof(char16_t);
}

int64_t cpp::encoding::Utf16::getByteCount(const String& string)
{
	return getCharCount(string) * sizeof(char16_t);
}

int cpp::encoding::Utf16::getCharCount(const null&)
{
	hx::NullReference("String", false);
	return 0;
}

int cpp::encoding::Utf16::getCharCount(const char32_t& codepoint)
{
	return utf16_length_from_utf32(&codepoint, 1);
}

int64_t cpp::encoding::Utf16::getCharCount(const String& string)
{
	if (null() == string)
	{
		hx::NullReference("String", false);
	}

	if (string.isUTF16Encoded())
	{
		return string.length;
	}
	else
	{
		return utf16_length_from_latin1(string.length);
	}
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
		if (getByteCount(string) > buffer.length)
		{
			return hx::Throw(HX_CSTRING("Buffer too small"));
		}

		return convert_latin1_to_utf16(string.raw_ptr(), string.length, reinterpret_cast<char16_t*>(buffer.ptr.ptr)) * sizeof(char16_t);
	}
}

int cpp::encoding::Utf16::encode(const char32_t& codepoint, const cpp::marshal::View<uint8_t>& buffer)
{
	if (getByteCount(codepoint) > buffer.length)
	{
		return hx::Throw(HX_CSTRING("Buffer too small"));
	}

	return convert_utf32_to_utf16(&codepoint, 1, reinterpret_cast<char16_t*>(buffer.ptr.ptr)) * sizeof(char16_t);
}

String cpp::encoding::Utf16::decode(const cpp::marshal::View<uint8_t>& buffer)
{
	if (buffer.isEmpty())
	{
		return String::emptyString;
	}

	auto chars = buffer.reinterpret<char16_t>();

	if (validate_utf16_as_ascii(chars.ptr.ptr, chars.length))
	{
		auto backing = static_cast<char*>(hx::NewGCPrivate(0, chars.length + sizeof(char)));
		auto written = convert_valid_utf16_to_latin1(chars.ptr.ptr, chars.length, backing);

		return String(backing, written);
	}

	if (validate_utf16(chars.ptr.ptr, chars.length))
	{
		auto backing = String::allocChar16Ptr(chars.length);

		std::memcpy(backing, chars.ptr.ptr, chars.length * sizeof(char16_t));

		return String(backing, chars.length);
	}
	
	return hx::Throw(HX_CSTRING("Buffer does not contain valid Utf16 data"));
}

char32_t cpp::encoding::Utf16::codepoint(const cpp::marshal::View<uint8_t>& buffer)
{
	auto output = std::array<char32_t, 8>();
#if defined(HXCPP_BIG_ENDIAN)
	auto read = convert_utf16be_to_utf32(reinterpret_cast<char16_t*>(buffer.ptr.ptr), std::min(int64_t{ 4 }, buffer.length), output.data());
#else
	auto read = convert_utf16le_to_utf32(reinterpret_cast<char16_t*>(buffer.ptr.ptr), std::min(int64_t{ 4 }, buffer.length), output.data());
#endif

	if (0 == read)
	{
		return int{ hx::Throw(HX_CSTRING("Failed to read codepoint")) };
	}
	else
	{
		return output[0];
	}
}
