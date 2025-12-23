#pragma once

namespace cpp
{
	namespace encoding
	{
        struct Utf8 final
        {
            /// <summary>
            /// Returns the number of bytes required to store the codepoint in it's UTF8 form.
            /// </summary>
            static int64_t getByteCount(const char32_t& codepoint);

            /// <summary>
            /// Returns the number of bytes required to store the string in it's UTF8 form.
            /// </summary>
            static int64_t getByteCount(const String& string);

            /// <summary>
            /// Writes the provided string in it's UTF8 form to the buffer.
            /// </summary>
            /// <returns>Number of byte written into the buffer</returns>
            static int64_t encode(const String& string, cpp::marshal::View<uint8_t> buffer);

            /// <summary>
            /// Writes the provided code point in it's UTF8 form to the buffer.
            /// </summary>
            /// <returns>Number of byte written into the buffer</returns>
            static int64_t encode(const char32_t& codepoint, cpp::marshal::View<uint8_t> buffer);

            static String decode(cpp::marshal::View<uint8_t> buffer);
            static int64_t decode(cpp::marshal::View<uint8_t> buffer, char32_t& out);
        };
	}
}