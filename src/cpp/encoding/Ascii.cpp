#include <hxcpp.h>
#include <simdutf.h>

using namespace cpp::marshal;
using namespace simdutf;

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

	if (validate_ascii(reinterpret_cast<char*>(view.ptr.ptr), view.length))
	{
		auto backing = hx::NewString(view.length);

		std::memcpy(backing, view.ptr.ptr, view.length);

		return String(static_cast<const char*>(backing), view.length);
	}
	else
	{
		return hx::Throw(HX_CSTRING("Buffer contained invalid ASCII data"));
	}
}
