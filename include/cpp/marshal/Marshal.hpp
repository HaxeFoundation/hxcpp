#pragma once

#include "Definitions.inc"
#include <cstring>
#include <string>

inline cpp::marshal::View<char> cpp::marshal::Marshal::asView(const char* cstring)
{
    return cpp::marshal::View<char>(const_cast<char*>(cstring), static_cast<int>(std::char_traits<char>::length(cstring)));
}

inline cpp::marshal::View<char16_t> cpp::marshal::Marshal::asView(const char16_t* cstring)
{
    return cpp::marshal::View<char16_t>(const_cast<char16_t*>(cstring), static_cast<int>(std::char_traits<char16_t>::length(cstring)));
}

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
inline T cpp::marshal::Marshal::read(View<uint8_t> view)
{
    return *(reinterpret_cast<T*>(view.ptr.ptr));
}

template<class T>
inline ::cpp::Pointer<T> cpp::marshal::Marshal::readPointer(View<uint8_t> view)
{
    return read<T*>(view);
}

inline int8_t cpp::marshal::Marshal::readInt8(View<uint8_t> view)
{
    return read<int8_t>(view);
}

inline int16_t cpp::marshal::Marshal::readInt16(View<uint8_t> view)
{
    return read<int16_t>(view);
}

inline int32_t cpp::marshal::Marshal::readInt32(View<uint8_t> view)
{
    return read<int32_t>(view);
}

inline int64_t cpp::marshal::Marshal::readInt64(View<uint8_t> view)
{
    return read<int64_t>(view);
}

inline uint8_t cpp::marshal::Marshal::readUInt8(View<uint8_t> view)
{
    return read<uint8_t>(view);
}

inline uint16_t cpp::marshal::Marshal::readUInt16(View<uint8_t> view)
{
    return read<uint16_t>(view);
}

inline uint32_t cpp::marshal::Marshal::readUInt32(View<uint8_t> view)
{
    return read<uint32_t>(view);
}

inline uint64_t cpp::marshal::Marshal::readUInt64(View<uint8_t> view)
{
    return read<uint64_t>(view);
}

inline float cpp::marshal::Marshal::readFloat32(View<uint8_t> view)
{
    return read<float>(view);
}

inline double cpp::marshal::Marshal::readFloat64(View<uint8_t> view)
{
    return read<double>(view);
}

template<class T>
inline void cpp::marshal::Marshal::write(View<uint8_t> view, const T& value)
{
    std::memcpy(view.ptr, reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}

template<class T>
inline void cpp::marshal::Marshal::writePointer(View<uint8_t> view, const ::cpp::Pointer<T>& value)
{
    write<T*>(view, value.ptr);
}

inline void cpp::marshal::Marshal::writeInt8(View<uint8_t> view, const int8_t& value)
{
    write<int8_t>(view, value);
}

inline void cpp::marshal::Marshal::writeInt16(View<uint8_t> view, const int16_t& value)
{
    write<int16_t>(view, value);
}

inline void cpp::marshal::Marshal::writeInt32(View<uint8_t> view, const int32_t& value)
{
    write<int32_t>(view, value);
}

inline void cpp::marshal::Marshal::writeInt64(View<uint8_t> view, const int64_t& value)
{
    write<int64_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt8(View<uint8_t> view, const uint8_t& value)
{
    write<uint8_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt16(View<uint8_t> view, const uint16_t& value)
{
    write<uint16_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt32(View<uint8_t> view, const uint32_t& value)
{
    write<uint32_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt64(View<uint8_t> view, const uint64_t& value)
{
    write<uint64_t>(view, value);
}

inline void cpp::marshal::Marshal::writeFloat32(View<uint8_t> view, const float& value)
{
    write<float>(view, value);
}

inline void cpp::marshal::Marshal::writeFloat64(View<uint8_t> view, const double& value)
{
    write<double>(view, value);
}