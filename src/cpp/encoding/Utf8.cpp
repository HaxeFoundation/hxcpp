#include <hxcpp.h>
#include <array>
#include <algorithm>
#include <simdutf.h>

using namespace cpp::marshal;
using namespace simdutf;

int cpp::encoding::Utf8::getByteCount(const null&)
{
    hx::NullReference("String", false);
    return 0;
}

int cpp::encoding::Utf8::getByteCount(const char32_t& codepoint)
{
    return utf8_length_from_utf32(&codepoint, 1);
}

int64_t cpp::encoding::Utf8::getByteCount(const String& string)
{
    if (null() == string)
    {
        hx::NullReference("String", false);
    }

    if (string.isAsciiEncoded())
    {
        return string.length;
    }

#if defined(HX_SMART_STRINGS)
    return utf8_length_from_utf16(string.raw_wptr(), string.length);
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int cpp::encoding::Utf8::getCharCount(const null&)
{
    hx::NullReference("String", false);
    return 0;
}

int cpp::encoding::Utf8::getCharCount(const char32_t& codepoint)
{
    return getByteCount(codepoint) / sizeof(char);
}

int64_t cpp::encoding::Utf8::getCharCount(const String& string)
{
    return getByteCount(string) / sizeof(char);
}

int cpp::encoding::Utf8::encode(const null&, const cpp::marshal::View<uint8_t>& buffer)
{
    hx::NullReference("String", false);
    return 0;
}

int64_t cpp::encoding::Utf8::encode(const String& string, const cpp::marshal::View<uint8_t>& buffer)
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

    if (string.isAsciiEncoded())
    {
        auto src = cpp::marshal::View<uint8_t>(reinterpret_cast<uint8_t*>(const_cast<char*>(string.raw_ptr())), string.length);

        if (src.tryCopyTo(buffer))
        {
            return src.length;
        }
        else
        {
            return hx::Throw(HX_CSTRING("Buffer too small"));
        }
    }

#if defined(HX_SMART_STRINGS)
    if (getByteCount(string) > buffer.length)
    {
        hx::Throw(HX_CSTRING("Buffer too small"));
    }

    return convert_valid_utf16_to_utf8(string.raw_wptr(), string.length, reinterpret_cast<char*>(buffer.ptr.ptr));
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int cpp::encoding::Utf8::encode(const char32_t& codepoint, const cpp::marshal::View<uint8_t>& buffer)
{
    if (getByteCount(codepoint) > buffer.length)
    {
        hx::Throw(HX_CSTRING("Buffer too small"));
    }

    return convert_utf32_to_utf8(&codepoint, 1, reinterpret_cast<char*>(buffer.ptr.ptr));
}

String cpp::encoding::Utf8::decode(const cpp::marshal::View<uint8_t>& buffer)
{
    if (buffer.isEmpty())
    {
        return String::emptyString;
    }

    if (validate_ascii(reinterpret_cast<char*>(buffer.ptr.ptr), buffer.length))
    {
        auto backing = hx::NewString(buffer.length);

        std::memcpy(backing, buffer.ptr.ptr, buffer.length);

        return String(backing, buffer.length);
    }

    if (validate_utf8(reinterpret_cast<char*>(buffer.ptr.ptr), buffer.length))
    {
#if defined(HX_SMART_STRINGS)
        auto chars   = utf16_length_from_utf8(reinterpret_cast<char*>(buffer.ptr.ptr), buffer.length);
        auto backing = String::allocChar16Ptr(chars);
        auto written = convert_valid_utf8_to_utf16(reinterpret_cast<char*>(buffer.ptr.ptr), buffer.length, backing);
#else
        auto backing = hx::NewString(buffer.length);
        auto written = buffer.length;

        std::memcpy(backing, buffer.ptr.ptr, buffer.length);
#endif

        return String(backing, static_cast<int>(written));
    }

    return hx::Throw(HX_CSTRING("Buffer was not valid UTF8 bytes"));
}

char32_t cpp::encoding::Utf8::codepoint(const cpp::marshal::View<uint8_t>& buffer)
{
    auto output = std::array<char32_t, 4>();
    auto read   = convert_utf8_to_utf32(reinterpret_cast<char*>(buffer.ptr.ptr), std::min(int64_t{4}, buffer.length), output.data());

    if (0 == read)
    {
        return int{ hx::Throw(HX_CSTRING("Failed to read codepoint")) };
    }
    else
    {
        return output[0];
    }
}
