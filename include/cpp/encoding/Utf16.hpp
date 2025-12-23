#pragma once

namespace cpp
{
	namespace encoding
	{
        struct Utf16 final
        {
            static bool isEncoded(const String& string);

            static int getByteCount(const char32_t& codepoint);
            static int64_t getByteCount(const String& string);

            static int getCharCount(const char32_t& codepoint);
            static int64_t getCharCount(const String& string);

            static int encode(const char32_t& codepoint, cpp::marshal::View<uint8_t> buffer);
            static int64_t encode(const String& string, cpp::marshal::View<uint8_t> buffer);

            static char32_t codepoint(cpp::marshal::View<uint8_t> buffer);
            static String decode(cpp::marshal::View<uint8_t> buffer);
        };
	}
}