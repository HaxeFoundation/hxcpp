#pragma once

#include "Definitions.inc"
#include <cstring>

inline cpp::marshal::View<char> cpp::marshal::Marshal::toCharView(const ::String& string)
{
    auto length = 0;
    auto ptr    = string.utf8_str(nullptr, true, &length);

    return View<char>(const_cast<char*>(ptr), length + 1);
}

inline int cpp::marshal::Marshal::toCharView(const ::String& string, View<char> buffer)
{
    auto length = 0;

    if (string.utf8_str(buffer, &length))
    {
        return length;
    }
    else
    {
        hx::Throw(HX_CSTRING("Not enough space in the view to write the string"));

        return 0;
    }
}

inline cpp::marshal::View<char16_t> cpp::marshal::Marshal::toWideCharView(const ::String& string)
{
    auto length = 0;
    auto ptr    = string.wc_str(nullptr, &length);

    return View<char16_t>(const_cast<char16_t*>(ptr), length + 1);
}

inline int cpp::marshal::Marshal::toWideCharView(const ::String& string, View<char16_t> buffer)
{
    auto length = 0;

    if (string.wc_str(buffer, &length))
    {
        return length;
    }
    else
    {
        hx::Throw(HX_CSTRING("Not enough space in the view to write the string"));

        return 0;
    }
}

inline ::String cpp::marshal::Marshal::toString(View<char> buffer)
{
    return ::String::create(buffer);
}

inline ::String cpp::marshal::Marshal::toString(View<char16_t> buffer)
{
    return ::String::create(buffer);
}

template<class T>
inline void cpp::marshal::Marshal::write(View<uint8_t> view, const T& value)
{
    std::memcpy(view.ptr, reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}

template<class T>
inline T cpp::marshal::Marshal::read(View<uint8_t> view)
{
    return *(reinterpret_cast<T*>(view.ptr.ptr));
}