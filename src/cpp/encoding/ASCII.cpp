#include <hxcpp.h>

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
