#pragma once

namespace cpp
{
	namespace encoding
	{
        struct Ascii final
        {
            static bool isEncoded(const String& string);

            /// <summary>
            /// Encode the provided string to ASCII bytes and write them to the buffer.
            /// If the provided string is UTF16 encoded an exception is raised and nothing is written to the buffer.
            /// </summary>
            /// <returns>Number of chars written to the buffer.</returns>
            static int64_t encode(const String& string, cpp::marshal::View<uint8_t> buffer);

            /// <summary>
            /// Create a string from the provided ASCII bytes.
            /// </summary>
            static String decode(cpp::marshal::View<uint8_t> string);
        };
	}
}