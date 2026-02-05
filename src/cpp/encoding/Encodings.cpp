#include <hxcpp.h>
#include <array>

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
        return hx::Throw(HX_CSTRING("View is empty"));
    }

    auto bytes = int64_t{ 0 };
    auto i     = int64_t{ 0 };
    auto chars = view.reinterpret<char>();

    while (i < chars.length && 0 != chars.ptr[i])
    {
        bytes += sizeof(char);
        i++;
    }

    if (0 == bytes)
    {
        return String::emptyString;
    }

    auto backing = hx::NewGCPrivate(0, bytes + sizeof(char));

    std::memcpy(backing, view.ptr.ptr, bytes);

    return String(static_cast<const char*>(backing), bytes / sizeof(char));
}

namespace
{
    bool isAsciiUtf8Buffer(const View<uint8_t>& buffer)
    {
        auto i = int64_t{ 0 };
        while (i < buffer.length)
        {
            auto p = cpp::encoding::Utf8::codepoint(buffer.slice(i));

            if (p > 127)
            {
                return false;
            }

            i += cpp::encoding::Utf8::getByteCount(p);
        }

        return true;
    }
}

int cpp::encoding::Utf8::getByteCount(const null&)
{
    hx::NullReference("String", false);
    return 0;
}

int cpp::encoding::Utf8::getByteCount(char32_t codepoint)
{
    if (codepoint <= 0x7F)
    {
        return 1;
    }
    else if (codepoint <= 0x7FF)
    {
        return 2;
    }
    else if (codepoint <= 0xFFFF)
    {
        return 3;
    }
    else
    {
        return 4;
    }
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
    auto source = View<char16_t>(string.raw_wptr(), string.length).reinterpret<uint8_t>();
    auto length = source.length;
    auto bytes  = int64_t{ 0 };
    auto i      = int64_t{ 0 };

    while (i < source.length)
    {
        auto slice = source.slice(i);
        auto p     = Utf16::codepoint(slice);

        i     += Utf16::getByteCount(p);
        bytes += getByteCount(p);
    }

    return bytes;
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

    auto initialPtr = buffer.ptr.ptr;
    auto source     = View<char16_t>(string.raw_wptr(), string.length).reinterpret<uint8_t>();
    auto i          = int64_t{ 0 };
    auto k          = int64_t{ 0 };

    while (i < source.length)
    {
        auto p = Utf16::codepoint(source.slice(i));

        i += Utf16::getByteCount(p);
        k += encode(p, buffer.slice(k));
    }

    return k;
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
    Array<uint8_t> out(getByteCount(string), 0);
    View<uint8_t> buffer(out->Pointer(), out->length);

    auto initialPtr = buffer.ptr.ptr;
    auto source     = View<char16_t>(string.raw_wptr(), string.length).reinterpret<uint8_t>();
    auto i          = int64_t{ 0 };
    auto k          = int64_t{ 0 };

    while (i < source.length)
    {
        auto p = Utf16::codepoint(source.slice(i));

        i += Utf16::getByteCount(p);
        k += encode(p, buffer.slice(k));
    }

    return out;
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int cpp::encoding::Utf8::encode(char32_t codepoint, const cpp::marshal::View<uint8_t>& buffer)
{
    if (codepoint <= 0x7F)
    {
        buffer[0] = static_cast<uint8_t>(codepoint);

        return 1;
    }
    else if (codepoint <= 0x7FF)
    {
        auto data = std::array<uint8_t, 2>
        { {
            static_cast<uint8_t>(0xC0 | (codepoint >> 6)),
            static_cast<uint8_t>(0x80 | (codepoint & 63))
        } };
        auto src = View<uint8_t>(data.data(), data.size());

        src.copyTo(buffer);

        return data.size();
    }
    else if (codepoint <= 0xFFFF)
    {
        auto data = std::array<uint8_t, 3>
        { {
            static_cast<uint8_t>(0xE0 | (codepoint >> 12)),
            static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 63)),
            static_cast<uint8_t>(0x80 | (codepoint & 63))
        } };

        auto src = View<uint8_t>(data.data(), data.size());

        src.copyTo(buffer);

        return data.size();
    }
    else
    {
        auto data = std::array<uint8_t, 4>
        { {
            static_cast<uint8_t>(0xF0 | (codepoint >> 18)),
            static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 63)),
            static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 63)),
            static_cast<uint8_t>(0x80 | (codepoint & 63))
        } };

        auto src = View<uint8_t>(data.data(), data.size());

        src.copyTo(buffer);

        return data.size();
    }
}

String cpp::encoding::Utf8::decode(const cpp::marshal::View<uint8_t>& buffer)
{
    if (buffer.isEmpty())
    {
        return String::emptyString;
    }

#if defined(HX_SMART_STRINGS)
    auto chars = int64_t{ 0 };
    auto i     = int64_t{ 0 };
    bool isAscii = true;

    while (i < buffer.length)
    {
        auto p = codepoint(buffer.slice(i));

        if (p > 127)
        {
            isAscii = false;
        }

        i     += getByteCount(p);
        chars += Utf16::getCharCount(p);
    }

    if (isAscii)
    {
        return Ascii::decode(buffer);
    }

    auto backing = View<char16_t>(::String::allocChar16Ptr(chars), chars);
    auto output  = backing.reinterpret<uint8_t>();
    auto k       = int64_t{ 0 };

    i = 0;
    while (i < buffer.length)
    {
        auto p = codepoint(buffer.slice(i));

        i += getByteCount(p);
        k += Utf16::encode(p, output.slice(k));
    }

    return String(backing.ptr.ptr, chars);
#else
    if (isAsciiUtf8Buffer(buffer))
    {
        return Ascii::decode(buffer);
    }

    auto backing = View<char>(hx::InternalNew(buffer.length, false), buffer.length);

    std::memcpy(backing.ptr.ptr, buffer.ptr.ptr, buffer.length);

    return String(backing.ptr.ptr, static_cast<int>(buffer.length));
#endif
}

char32_t cpp::encoding::Utf8::codepoint(const cpp::marshal::View<uint8_t>& buffer)
{
    auto b0 = static_cast<char32_t>(buffer[0]);

    if ((b0 & 0x80) == 0)
    {
        return b0;
    }
    else if ((b0 & 0xE0) == 0xC0)
    {
        return (static_cast<char32_t>(b0 & 0x1F) << 6) | static_cast<char32_t>(buffer.slice(1)[0] & 0x3F);
    }
    else if ((b0 & 0xF0) == 0xE0)
    {
        auto staging = std::array<uint8_t, 2>();
        auto dst     = View<uint8_t>(staging.data(), staging.size());

        buffer.slice(1, staging.size()).copyTo(dst);

        return (static_cast<char32_t>(b0 & 0x0F) << 12) | (static_cast<char32_t>(staging[0] & 0x3F) << 6) | static_cast<char32_t>(staging[1] & 0x3F);
    }
    else if ((b0 & 0xF8) == 0xF0)
    {
        auto staging = std::array<uint8_t, 3>();
        auto dst     = View<uint8_t>(staging.data(), staging.size());

        buffer.slice(1, staging.size()).copyTo(dst);

        return
            (static_cast<char32_t>(b0 & 0x07) << 18) |
            (static_cast<char32_t>(staging[0] & 0x3F) << 12) |
            (static_cast<char32_t>(staging[1] & 0x3F) << 6) |
            static_cast<char32_t>(staging[2] & 0x3F);
    }
    else
    {
        return int{ hx::Throw(HX_CSTRING("Failed to read codepoint")) };
    }
}

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

    bool isAsciiUtf16Buffer(const View<uint8_t>& buffer)
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

int cpp::encoding::Utf16::getByteCount(char32_t codepoint)
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

int cpp::encoding::Utf16::getCharCount(char32_t codepoint)
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

int cpp::encoding::Utf16::encode(char32_t codepoint, const cpp::marshal::View<uint8_t>& buffer)
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

#if defined(HX_SMART_STRINGS)
    auto i = int64_t{ 0 };
    bool isAscii = true;
    while (i < buffer.length)
    {
        auto p = codepoint(buffer.slice(i));

        if (p > 127)
        {
            isAscii = false;
        }

        i += getByteCount(p);
    }

    if (isAscii)
    {
        return toAsciiString(buffer);
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
