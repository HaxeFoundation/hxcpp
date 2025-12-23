#include <hxcpp.h>
#include <array>

using namespace cpp::marshal;

namespace
{
    bool isAsciiBuffer(View<uint8_t>& buffer)
    {
        for (auto i = int64_t{ 0 }; i < buffer.length; i++)
        {
            if (buffer.ptr[i] > 127)
            {
                return false;
            }
        }

        return true;
    }
}

int64_t cpp::encoding::Utf8::getByteCount(const char32_t& codepoint)
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
    auto source     = View<char16_t>(string.raw_wptr(), string.length).reinterpret<uint8_t>();
    auto length     = source.length;
    auto codepoint  = char32_t{ 0 };
    auto bytes      = int64_t{ 0 };

    while (false == source.isEmpty())
    {
        source = source.slice(Utf16::decode(source, codepoint));
        bytes += getByteCount(codepoint);
    }

    return bytes;
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int64_t cpp::encoding::Utf8::getCharCount(const char32_t& codepoint)
{
    return getByteCount(codepoint) / sizeof(char);
}

int64_t cpp::encoding::Utf8::getCharCount(const String& string)
{
    return getByteCount(string) / sizeof(char);
}

int64_t cpp::encoding::Utf8::encode(const String& string, cpp::marshal::View<uint8_t> buffer)
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
        auto src = cpp::marshal::View<uint8_t>(reinterpret_cast<uint8_t*>(const_cast<char*>(string.raw_ptr())), string.length * sizeof(char));

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
    auto codepoint  = char32_t{ 0 };

    while (false == source.isEmpty())
    {
        source = source.slice(Utf16::decode(source, codepoint));
        buffer = buffer.slice(encode(codepoint, buffer));
    }

    return buffer.ptr.ptr - initialPtr;
#else
    return hx::Throw(HX_CSTRING("Unexpected encoding error"));
#endif
}

int64_t cpp::encoding::Utf8::encode(const char32_t& codepoint, cpp::marshal::View<uint8_t> buffer)
{
    if (codepoint <= 0x7F)
    {
        buffer[0] = codepoint;

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

String cpp::encoding::Utf8::decode(cpp::marshal::View<uint8_t> buffer)
{
    if (buffer.isEmpty())
    {
        return String::emptyString;
    }

    if (isAsciiBuffer(buffer))
    {
        return Ascii::decode(buffer);
    }

    auto bytes     = int64_t{ 0 };
    auto codepoint = char32_t{ 0 };
    auto i         = int64_t{ 0 };

    while (i < buffer.length)
    {
        i     += decode(buffer.slice(i), codepoint);
        bytes += Utf16::getByteCount(codepoint);
    }

    auto backing = static_cast<uint8_t*>(hx::NewGCPrivate(0, bytes + sizeof(char16_t)));
    auto output  = View<uint8_t>(backing, bytes);

    while (false == buffer.isEmpty())
    {
        buffer = buffer.slice(decode(buffer, codepoint));
        output = output.slice(Utf16::encode(codepoint, output));
    }

    reinterpret_cast<uint32_t*>(backing)[-1] |= HX_GC_STRING_CHAR16_T;

    return String(reinterpret_cast<char16_t*>(backing), bytes / sizeof(char16_t));
}

int64_t cpp::encoding::Utf8::decode(cpp::marshal::View<uint8_t> buffer, char32_t& codepoint)
{
    if (0 == buffer.length)
    {
        return hx::Throw(HX_CSTRING("Empty view"));
    }

    auto b0 = buffer[0];

    if ((b0 & 0x80) == 0)
    {
        codepoint = b0;

        return 1;
    }
    else if ((b0 & 0xE0) == 0xC0)
    {
        codepoint = (static_cast<char32_t>(b0 & 0x1F) << 6) | static_cast<char32_t>(buffer[1] & 0x3F);

        return 2;
    }
    else if ((b0 & 0xF0) == 0xE0)
    {
        codepoint = (static_cast<char32_t>(b0 & 0x0F) << 12) | (static_cast<char32_t>(buffer[1] & 0x3F) << 6) | static_cast<char32_t>(buffer[2] & 0x3F);

        return 3;
    }
    else if ((b0 & 0xF8) == 0xF0)
    {
        codepoint =
            (static_cast<char32_t>(b0 & 0x07) << 18) |
            (static_cast<char32_t>(buffer[1] & 0x3F) << 12) |
            (static_cast<char32_t>(buffer[2] & 0x3F) << 6) |
            static_cast<char32_t>(buffer[3] & 0x3F);

        return 4;
    }
    else
    {
        return hx::Throw(HX_CSTRING("Failed to read codepoint"));
    }
}