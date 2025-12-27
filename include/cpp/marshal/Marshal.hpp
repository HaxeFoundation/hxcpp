#pragma once

#include "Definitions.inc"
#include <cstring>
#include <string>
#include <algorithm>

namespace
{
    template <class T>
    T reverse(T val)
    {
        auto view = cpp::marshal::View<uint8_t>(reinterpret_cast<uint8_t*>(&val), sizeof(T));

        std::reverse(view.ptr.ptr, view.ptr.ptr + view.length);

        return val;
    }
}

inline cpp::marshal::View<char> cpp::marshal::Marshal::asCharView(const ::String& string)
{
    if (null() == string)
    {
        hx::NullReference("string", false);
    }

    if (false == string.isAsciiEncoded())
    {
        hx::Throw(HX_CSTRING("String is not ASCII encoded"));
    }

    return View<char>(const_cast<char*>(string.raw_ptr()), string.length);
}

inline cpp::marshal::View<char16_t> cpp::marshal::Marshal::asWideCharView(const ::String& string)
{
#if defined(HX_SMART_STRINGS)
    if (null() == string)
    {
        hx::NullReference("string", false);
    }

    if (false == string.isUTF16Encoded())
    {
        hx::Throw(HX_CSTRING("String is not ASCII encoded"));
    }

    return View<char16_t>(const_cast<char16_t*>(string.raw_wptr()), string.length);
#else
    hx::Throw(HX_CSTRING("HX_SMART_STRINGS not defined"));

    return View<char16_t>(cpp::Pointer<char16_t>(null()), 0);
#endif
}

template<class T>
inline T cpp::marshal::Marshal::read(const View<uint8_t>& view)
{
    if (view.length < sizeof(T))
    {
        hx::Throw(HX_CSTRING("View too small"));
    }

    T output{};

    std::memcpy(&output, view.ptr.ptr, sizeof(T));

    return output;
}

template<class T>
inline ::cpp::Pointer<T> cpp::marshal::Marshal::readPointer(const View<uint8_t>& view)
{
    return read<T*>(view);
}

inline int8_t cpp::marshal::Marshal::readInt8(const View<uint8_t>& view)
{
    return read<int8_t>(view);
}

inline int16_t cpp::marshal::Marshal::readInt16(const View<uint8_t>& view)
{
    return read<int16_t>(view);
}

inline int32_t cpp::marshal::Marshal::readInt32(const View<uint8_t>& view)
{
    return read<int32_t>(view);
}

inline int64_t cpp::marshal::Marshal::readInt64(const View<uint8_t>& view)
{
    return read<int64_t>(view);
}

inline uint8_t cpp::marshal::Marshal::readUInt8(const View<uint8_t>& view)
{
    return read<uint8_t>(view);
}

inline uint16_t cpp::marshal::Marshal::readUInt16(const View<uint8_t>& view)
{
    return read<uint16_t>(view);
}

inline uint32_t cpp::marshal::Marshal::readUInt32(const View<uint8_t>& view)
{
    return read<uint32_t>(view);
}

inline uint64_t cpp::marshal::Marshal::readUInt64(const View<uint8_t>& view)
{
    return read<uint64_t>(view);
}

inline float cpp::marshal::Marshal::readFloat32(const View<uint8_t>& view)
{
    return read<float>(view);
}

inline double cpp::marshal::Marshal::readFloat64(const View<uint8_t>& view)
{
    return read<double>(view);
}

template<class T>
inline void cpp::marshal::Marshal::write(const View<uint8_t>& view, const T& value)
{
    if (view.length < sizeof(T))
    {
        hx::Throw(HX_CSTRING("View too small"));
    }

    std::memcpy(view.ptr.ptr, reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}

template<class T>
inline void cpp::marshal::Marshal::writePointer(const View<uint8_t>& view, const ::cpp::Pointer<T>& value)
{
    write<T*>(view, value.ptr);
}

inline void cpp::marshal::Marshal::writeInt8(const View<uint8_t>& view, const int8_t& value)
{
    write<int8_t>(view, value);
}

inline void cpp::marshal::Marshal::writeInt16(const View<uint8_t>& view, const int16_t& value)
{
    write<int16_t>(view, value);
}

inline void cpp::marshal::Marshal::writeInt32(const View<uint8_t>& view, const int32_t& value)
{
    write<int32_t>(view, value);
}

inline void cpp::marshal::Marshal::writeInt64(const View<uint8_t>& view, const int64_t& value)
{
    write<int64_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt8(const View<uint8_t>& view, const uint8_t& value)
{
    write<uint8_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt16(const View<uint8_t>& view, const uint16_t& value)
{
    write<uint16_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt32(const View<uint8_t>& view, const uint32_t& value)
{
    write<uint32_t>(view, value);
}

inline void cpp::marshal::Marshal::writeUInt64(const View<uint8_t>& view, const uint64_t& value)
{
    write<uint64_t>(view, value);
}

inline void cpp::marshal::Marshal::writeFloat32(const View<uint8_t>& view, const float& value)
{
    write<float>(view, value);
}

inline void cpp::marshal::Marshal::writeFloat64(const View<uint8_t>& view, const double& value)
{
    write<double>(view, value);
}

template<class T>
inline ::cpp::Pointer<T> cpp::marshal::Marshal::readBigEndianPointer(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readPointer<T*>(view);
#else
    return reverse(readPointer<T*>(view));
#endif
}

inline int16_t cpp::marshal::Marshal::readBigEndianInt16(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readInt16(view);
#else
    return reverse(readInt16(view));
#endif
}

inline int32_t cpp::marshal::Marshal::readBigEndianInt32(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readInt32(view);
#else
    return reverse(readInt32(view));
#endif
}

inline int64_t cpp::marshal::Marshal::readBigEndianInt64(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readInt64(view);
#else
    return reverse(readInt64(view));
#endif
}

inline uint16_t cpp::marshal::Marshal::readBigEndianUInt16(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readUInt16(view);
#else
    return reverse(readUInt16(view));
#endif
}

inline uint32_t cpp::marshal::Marshal::readBigEndianUInt32(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readUInt32(view);
#else
    return reverse(readUInt32(view));
#endif
}

inline uint64_t cpp::marshal::Marshal::readBigEndianUInt64(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readInt64(view);
#else
    return reverse(readUInt64(view));
#endif
}

inline float cpp::marshal::Marshal::readBigEndianFloat32(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readFloat32(view);
#else
    return reverse(readFloat32(view));
#endif
}

inline double cpp::marshal::Marshal::readBigEndianFloat64(const View<uint8_t>& view)
{
#ifdef HXCPP_BIG_ENDIAN
    return readFloat64(view);
#else
    return reverse(readFloat64(view));
#endif
}

template<class T>
inline ::cpp::Pointer<T> cpp::marshal::Marshal::readLittleEndianPointer(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readPointer<T*>(view);
#else
    return reverse(readPointer<T*>(view));
#endif
}

inline int16_t cpp::marshal::Marshal::readLittleEndianInt16(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readInt16(view);
#else
    return reverse(readInt16(view));
#endif
}

inline int32_t cpp::marshal::Marshal::readLittleEndianInt32(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readInt32(view);
#else
    return reverse(readInt32(view));
#endif
}

inline int64_t cpp::marshal::Marshal::readLittleEndianInt64(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readInt64(view);
#else
    return reverse(readInt64(view));
#endif
}

inline uint16_t cpp::marshal::Marshal::readLittleEndianUInt16(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readUInt16(view);
#else
    return reverse(readUInt16(view));
#endif
}

inline uint32_t cpp::marshal::Marshal::readLittleEndianUInt32(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readUInt32(view);
#else
    return reverse(readUInt32(view));
#endif
}

inline uint64_t cpp::marshal::Marshal::readLittleEndianUInt64(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readInt64(view);
#else
    return reverse(readUInt64(view));
#endif
}

inline float cpp::marshal::Marshal::readLittleEndianFloat32(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readFloat32(view);
#else
    return reverse(readFloat32(view));
#endif
}

inline double cpp::marshal::Marshal::readLittleEndianFloat64(const View<uint8_t>& view)
{
#ifndef HXCPP_BIG_ENDIAN
    return readFloat64(view);
#else
    return reverse(readFloat64(view));
#endif
}

template<class T>
inline void cpp::marshal::Marshal::writeLittleEndianPointer(const View<uint8_t>& view, const ::cpp::Pointer<T>& value)
{
#ifdef HXCPP_BIG_ENDIAN
    write<T*>(view, reverse(value.ptr));
#else
    write<T*>(view, value.ptr);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianInt16(const View<uint8_t>& view, const int16_t& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeInt16(view, reverse(value));
#else
    writeInt16(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianInt32(const View<uint8_t>& view, const int32_t& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeInt32(view, reverse(value));
#else
    writeInt32(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianInt64(const View<uint8_t>& view, const int64_t& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeInt64(view, reverse(value));
#else
    writeInt64(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianUInt16(const View<uint8_t>& view, const uint16_t& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeUInt16(view, reverse(value));
#else
    writeUInt16(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianUInt32(const View<uint8_t>& view, const uint32_t& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeUInt32(view, reverse(value));
#else
    writeUInt32(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianUInt64(const View<uint8_t>& view, const uint64_t& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeUInt16(view, reverse(value));
#else
    writeUInt16(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianFloat32(const View<uint8_t>& view, const float& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeFloat32(view, reverse(value));
#else
    writeFloat32(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeLittleEndianFloat64(const View<uint8_t>& view, const double& value)
{
#ifdef HXCPP_BIG_ENDIAN
    writeFloat64(view, reverse(value));
#else
    writeFloat64(view, value);
#endif
}

template<class T>
inline void cpp::marshal::Marshal::writeBigEndianPointer(const View<uint8_t>& view, const ::cpp::Pointer<T>& value)
{
#ifndef HXCPP_BIG_ENDIAN
    write<T*>(view, reverse(value.ptr));
#else
    write<T*>(view, value.ptr);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianInt16(const View<uint8_t>& view, const int16_t& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeInt16(view, reverse(value));
#else
    writeInt16(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianInt32(const View<uint8_t>& view, const int32_t& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeInt32(view, reverse(value));
#else
    writeInt32(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianInt64(const View<uint8_t>& view, const int64_t& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeInt64(view, reverse(value));
#else
    writeInt64(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianUInt16(const View<uint8_t>& view, const uint16_t& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeUInt16(view, reverse(value));
#else
    writeUInt16(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianUInt32(const View<uint8_t>& view, const uint32_t& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeUInt32(view, reverse(value));
#else
    writeUInt32(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianUInt64(const View<uint8_t>& view, const uint64_t& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeUInt16(view, reverse(value));
#else
    writeUInt16(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianFloat32(const View<uint8_t>& view, const float& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeFloat32(view, reverse(value));
#else
    writeFloat32(view, value);
#endif
}

inline void cpp::marshal::Marshal::writeBigEndianFloat64(const View<uint8_t>& view, const double& value)
{
#ifndef HXCPP_BIG_ENDIAN
    writeFloat64(view, reverse(value));
#else
    writeFloat64(view, value);
#endif
}