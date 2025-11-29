#pragma once

#include "Definitions.inc"
#include <cstring>
#include <cmath>

template<class T>
inline cpp::marshal::View<T>::View(::cpp::Pointer<T> _ptr, int _length) : ptr(_ptr), length(_length) {}

template<class T>
inline bool cpp::marshal::View<T>::tryCopyTo(const View<T>& destination)
{
    if (destination.length < length)
    {
        return false;
    }

    std::memcpy(destination.ptr.ptr, ptr.ptr, sizeof(T) * length);

    return true;
}

template<class T>
inline void cpp::marshal::View<T>::clear()
{
    std::memset(ptr, 0, sizeof(T) * length);
}

template<class T>
inline void cpp::marshal::View<T>::fill(T value)
{
    for (auto i = 0; i < length; i++)
    {
        ptr[i] = value;
    }
}

template<class T>
inline bool cpp::marshal::View<T>::isEmpty()
{
    return length == 0;
}

template<class T>
inline cpp::marshal::View<T> cpp::marshal::View<T>::slice(int index)
{
    return View<T>(ptr + index, length - index);
}

template<class T>
inline cpp::marshal::View<T> cpp::marshal::View<T>::slice(int index, int length)
{
    return View<T>(ptr + index, length);
}

template<class T>
template<class K>
inline cpp::marshal::View<K> cpp::marshal::View<T>::reinterpret()
{
    auto newPtr = ::cpp::Pointer<K>{ ptr.reinterpret() };
    auto fromSize = sizeof(T);
    auto toSize = sizeof(K);

    if (toSize == fromSize)
    {
        return cpp::marshal::View<K>(newPtr, length);
    }
    if (toSize < fromSize)
    {
        return cpp::marshal::View<K>(newPtr, length * (fromSize / toSize));
    }

    auto shrink = static_cast<double>(fromSize) / toSize;
    auto newLength = static_cast<int>(std::floor(length * shrink));

    return cpp::marshal::View<K>(newPtr, newLength);
}

template<class T>
inline int cpp::marshal::View<T>::compare(const View<T>& inRHS)
{
    return std::memcmp(ptr.ptr, inRHS.ptr.ptr, sizeof(T) * length);
}

template<class T>
inline bool cpp::marshal::View<T>::operator==(const View<T>& inRHS) const
{
    return length == inRHS.length && ptr.ptr == inRHS.ptr.ptr;
}

template<class T>
inline bool cpp::marshal::View<T>::operator!=(const View<T>& inRHS) const
{
    return length != inRHS.length || ptr.ptr != inRHS.ptr.ptr;
}

template<class T>
inline T& cpp::marshal::View<T>::operator[](int index)
{
    return ptr[index];
}

template<class T>
inline cpp::marshal::View<T>::operator void* ()
{
    return ptr.ptr;
}

template<class T>
inline cpp::marshal::View<T>::operator T* ()
{
    return ptr.ptr;
}

template<class T>
inline cpp::marshal::View<T>::operator cpp::Pointer<T> ()
{
    return ptr;
}