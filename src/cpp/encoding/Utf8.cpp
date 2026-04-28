#include <hxcpp.h>
#include <array>

using namespace cpp::marshal;

namespace
{
    bool isAsciiBuffer(const View<uint8_t>& buffer)
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

int cpp::encoding::Utf8::getByteCount(const char32_t& codepoint)
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

int cpp::encoding::Utf8::encode(const char32_t& codepoint, const cpp::marshal::View<uint8_t>& buffer)
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

    if (isAsciiBuffer(buffer))
    {
        return Ascii::decode(buffer);
    }

#if defined(HX_SMART_STRINGS)
    auto chars = int64_t{ 0 };
    auto i     = int64_t{ 0 };

    while (i < buffer.length)
    {
        auto p = codepoint(buffer.slice(i));

        i     += getByteCount(p);
        chars += Utf16::getCharCount(p);
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
