#pragma once

namespace cpp
{
	namespace encoding
	{
        struct Utf8 final
        {
            static int64_t getByteCount(const char32_t& codepoint);
            static int64_t getByteCount(const String& string);

            static int64_t getCharCount(const char32_t& codepoint);
            static int64_t getCharCount(const String& string);

            static int64_t encode(const String& string, cpp::marshal::View<uint8_t> buffer);
            static int64_t encode(const char32_t& codepoint, cpp::marshal::View<uint8_t> buffer);

            static String decode(cpp::marshal::View<uint8_t> buffer);
            static int64_t decode(cpp::marshal::View<uint8_t> buffer, char32_t& out);
        };
	}
}