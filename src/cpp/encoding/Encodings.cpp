#include <hxcpp.h>
#include <array>
#include <simdutf.h>
#include <hx/thread/Scratch.hpp>

using namespace cpp::marshal;

bool cpp::encoding::Ascii::isEncoded(const String& string)
{
    if (null() == string)
    {
        hx::NullReference("String", false);
    }

    return string.isAsciiEncoded();
}

int64_t cpp::encoding::Ascii::encode(const String& string, View<uint8_t> buffer)
{
    if (null() == string)
    {
        hx::NullReference("String", false);
    }

    if (string.isUTF16Encoded())
    {
        hx::Throw(HX_CSTRING("String cannot be encoded to ASCII"));
    }

    auto src = cpp::marshal::View<char>(string.raw_ptr(), string.length).reinterpret<uint8_t>();

    if (src.tryCopyTo(buffer))
    {
        return src.length;
    }
    else
    {
        return hx::Throw(HX_CSTRING("Buffer too small"));
    }
}

String cpp::encoding::Ascii::decode(View<uint8_t> view)
{
    if (view.isEmpty())
    {
        return String::emptyString;
    }

    auto chars = view.reinterpret<char>();
    if (simdutf::validate_ascii(chars.ptr, chars.length))
    {
        auto output = hx::NewString(view.length);

        std::memcpy(output, view.ptr, view.length);

        return String(static_cast<const char*>(output), view.length);
    }
    else
    {
        return hx::Throw(HX_CSTRING("Buffer contained invalid Ascii data"));
    }
}

int cpp::encoding::Utf8::getByteCount(const null&)
{
    hx::NullReference("String", false);
    return 0;
}

int cpp::encoding::Utf8::getByteCount(char32_t codepoint)
{
    return simdutf::utf8_length_from_utf32(&codepoint, 1);
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
    return simdutf::utf8_length_from_utf16(string.raw_wptr(), string.length);
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int cpp::encoding::Utf8::getCharCount(const null&)
{
    hx::NullReference("String", false);
    return 0;
}

int cpp::encoding::Utf8::getCharCount(char32_t codepoint)
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

    return simdutf::convert_valid_utf16_to_utf8(string.raw_wptr(), string.length, reinterpret_cast<char*>(buffer.ptr.ptr));
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

Array<uint8_t> cpp::encoding::Utf8::encode(const String& string)
{
    if (null() == string)
    {
        hx::NullReference("String", false);
    }

    if (0 == string.length)
    {
        return 0;
    }

    if (string.isAsciiEncoded())
    {
        Array<uint8_t> out(string.length, 0);

        View<uint8_t> src(reinterpret_cast<uint8_t*>(const_cast<char*>(string.raw_ptr())), string.length);
        View<uint8_t> buffer(out->Pointer(), out->length);

        src.copyTo(buffer);

        return out;
    }

#if defined(HX_SMART_STRINGS)
    auto out   = Array<uint8_t>(simdutf::utf8_length_from_utf16(string.raw_wptr(), string.length), 0);
    auto count = simdutf::convert_utf16_to_utf8(string.raw_wptr(), string.length, out->getBase());

    if (0 == count)
    {
        hx::Throw(HX_CSTRING("Provided string was not valid utf16"));
    }

    return out;
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int cpp::encoding::Utf8::encode(char32_t codepoint, const cpp::marshal::View<uint8_t>& buffer)
{
    if (getByteCount(codepoint) > buffer.length)
    {
        hx::Throw(HX_CSTRING("Buffer too small"));
    }

    return static_cast<int>(simdutf::convert_utf32_to_utf8(&codepoint, 1, buffer.reinterpret<char>().ptr.ptr));
}

String cpp::encoding::Utf8::decode(const cpp::marshal::View<uint8_t>& buffer)
{
    if (buffer.isEmpty())
    {
        return String::emptyString;
    }

    auto chars = buffer.reinterpret<char>();
    if (simdutf::validate_ascii(chars.ptr.ptr, chars.length))
    {
        auto backing = hx::NewString(buffer.length);

        std::memcpy(backing, buffer.ptr.ptr, buffer.length);

        return String(backing, buffer.length);
    }

    if (simdutf::validate_utf8(reinterpret_cast<char*>(buffer.ptr.ptr), buffer.length))
    {
#if defined(HX_SMART_STRINGS)
        auto length = simdutf::utf16_length_from_utf8(chars.ptr.ptr, chars.length) * sizeof(char16_t);
        auto output = String::allocChar16Ptr(length);
        auto count  = simdutf::convert_valid_utf8_to_utf16(chars.ptr, chars.length, output);

        return String(output, count);
#else
        auto backing = View<char>(hx::InternalNew(buffer.length, false), buffer.length);

        std::memcpy(backing.ptr.ptr, buffer.ptr.ptr, buffer.length);

        return String(backing.ptr.ptr, static_cast<int>(buffer.length));
#endif
    }
    else
    {
        return hx::Throw(HX_CSTRING("Buffer was not valid UTF8 bytes"));
    }
}

char32_t cpp::encoding::Utf8::codepoint(const cpp::marshal::View<uint8_t>& buffer)
{
    auto output = std::array<char32_t, 4>();
    auto read   = simdutf::convert_utf8_to_utf32(reinterpret_cast<char*>(buffer.ptr.ptr), std::min(int64_t{ 4 }, buffer.length), output.data());

    if (0 == read)
    {
        return int{ hx::Throw(HX_CSTRING("Failed to read codepoint")) };
    }
    else
    {
        return output[0];
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

int cpp::encoding::Utf16::getByteCount(char32_t codepoint)
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

int cpp::encoding::Utf16::getCharCount(char32_t codepoint)
{
    return simdutf::utf16_length_from_utf32(&codepoint, 1);
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
        return simdutf::utf16_length_from_latin1(string.length);
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
        if (getByteCount(string) > buffer.length)
        {
            return hx::Throw(HX_CSTRING("Buffer too small"));
        }

        return simdutf::convert_latin1_to_utf16(string.raw_ptr(), string.length, reinterpret_cast<char16_t*>(buffer.ptr.ptr)) * sizeof(char16_t);
    }
}

int cpp::encoding::Utf16::encode(char32_t codepoint, const cpp::marshal::View<uint8_t>& buffer)
{
    if (getByteCount(codepoint) > buffer.length)
    {
        return hx::Throw(HX_CSTRING("Buffer too small"));
    }

    return simdutf::convert_utf32_to_utf16(&codepoint, 1, reinterpret_cast<char16_t*>(buffer.ptr.ptr)) * sizeof(char16_t);
}

String cpp::encoding::Utf16::decode(const cpp::marshal::View<uint8_t>& buffer)
{
    if (buffer.isEmpty())
    {
        return String::emptyString;
    }

    auto chars = buffer.reinterpret<char16_t>();
    if (simdutf::validate_utf16_as_ascii(chars.ptr, chars.length))
    {
        auto backing = hx::NewString(chars.length);
        auto count   = simdutf::convert_valid_utf16_to_latin1(chars.ptr, chars.length, backing);

        return String(backing, count);
    }

    if (simdutf::validate_utf16(chars.ptr, chars.length))
    {
#if defined(HX_SMART_STRINGS)
        auto backing = String::allocChar16Ptr(chars.length);
        auto written = chars.length;

        std::memcpy(backing, chars.ptr, chars.length * sizeof(char16_t));
#else
        auto size    = simdutf::utf8_length_from_utf16(chars.ptr, chars.length);
        auto backing = hx::NewString(size);
        auto written = simdutf::convert_valid_utf16_to_utf8(chars.ptr, chars.length, backing);
#endif

        return String(backing, static_cast<int>(written));
    }
    else
    {
        return hx::Throw(HX_CSTRING("Buffer contains invalid utf16"));
    }
}

char32_t cpp::encoding::Utf16::codepoint(const cpp::marshal::View<uint8_t>& buffer)
{
    auto output = std::array<char32_t, 8>();
#if defined(HXCPP_BIG_ENDIAN)
    auto read = simdutf::convert_utf16be_to_utf32(reinterpret_cast<char16_t*>(buffer.ptr.ptr), std::min(int64_t{ 4 }, buffer.length), output.data());
#else
    auto read = simdutf::convert_utf16le_to_utf32(reinterpret_cast<char16_t*>(buffer.ptr.ptr), std::min(int64_t{ 4 }, buffer.length), output.data());
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
