#pragma once

#include "Definitions.inc"
#include <cstring>
#include <cmath>

template<class T>
inline cpp::marshal::View<T>::View(::cpp::Pointer<T> _ptr, int64_t _length) : ptr(_ptr), length(_length) {}

template<class T>
inline bool cpp::marshal::View<T>::tryCopyTo(const View<T>& destination) const
{
    if (destination.length < length)
    {
        return false;
    }

    if (ptr.ptr + length < destination.ptr.ptr || destination.ptr.ptr + length < ptr.ptr)
        std::memcpy(destination.ptr.ptr, ptr.ptr, sizeof(T) * length);
    else
        std::memmove(destination.ptr.ptr, ptr.ptr, sizeof(T) * length);

    return true;
}

template<class T>
inline void cpp::marshal::View<T>::copyTo(const View<T>& destination) const
{
    if (tryCopyTo(destination) == false)
    {
        hx::Throw(HX_CSTRING("View OOB"));
    }
}

template<class T>
inline void cpp::marshal::View<T>::clear() const
{
    std::memset(ptr.ptr, 0, sizeof(T) * length);
}

template<class T>
inline void cpp::marshal::View<T>::fill(T value) const
{
    for (auto i = 0; i < length; i++)
    {
        ptr.ptr[i] = value;
    }
}

template<class T>
inline bool cpp::marshal::View<T>::isEmpty() const
{
    return length == 0;
}

template<class T>
inline cpp::marshal::View<T> cpp::marshal::View<T>::slice(int64_t index) const
{
    if (index < 0 || index > length)
    {
        hx::Throw(HX_CSTRING("View OOB"));
    }

    return View<T>(ptr.ptr + index, length - index);
}

template<class T>
inline cpp::marshal::View<T> cpp::marshal::View<T>::slice(int64_t inIndex, int64_t inLength) const
{
    if (inIndex < 0 || inLength < 0 || inIndex > length || inIndex + inLength > length)
    {
        hx::Throw(HX_CSTRING("View OOB"));
    }

    return View<T>(ptr.ptr + inIndex, inLength);
}

template<class T>
template<class K>
inline cpp::marshal::View<K> cpp::marshal::View<T>::reinterpret() const
{
    auto newPtr   = ::cpp::Pointer<K>(reinterpret_cast<K*>(ptr.ptr));
    auto fromSize = sizeof(T);
    auto toSize   = sizeof(K);

    if (toSize == fromSize)
    {
        return cpp::marshal::View<K>(newPtr, length);
    }
    if (toSize < fromSize)
    {
        return cpp::marshal::View<K>(newPtr, length * (fromSize / toSize));
    }

    auto shrink    = static_cast<double>(fromSize) / toSize;
    auto newLength = static_cast<int>(std::floor(length * shrink));

    return cpp::marshal::View<K>(newPtr, newLength);
}

template<class T>
inline int cpp::marshal::View<T>::compare(const View<T>& inRHS) const
{
    auto common = length < inRHS.length ? length : inRHS.length;
    auto result = std::memcmp(ptr.ptr, inRHS.ptr.ptr, sizeof(T) * common);

    if (result)
    {
        return result;
    }

    return length - inRHS.length;
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
inline T& cpp::marshal::View<T>::operator[](int64_t index) const
{
    if (index < 0 || index >= length)
    {
        hx::Throw(HX_CSTRING("View OOB"));
    }

    return ptr.ptr[index];
}
