#pragma once

#include "Definitions.inc"

template<class T>
inline T* cpp::marshal::PointerType<T>::FromDynamic(const Dynamic& inRHS)
{
    return FromBoxed(inRHS.StaticCast<Boxed<T>>());
}

template<class T>
inline T* cpp::marshal::PointerType<T>::FromBoxed(const Boxed<TPtr>& inRHS)
{
    if (nullptr == inRHS.mPtr)
    {
        return nullptr;
    }

    return inRHS->value;
}

template<class T>
inline T* cpp::marshal::PointerType<T>::FromReference(const PointerReference<T>& inRHS)
{
    if (nullptr == inRHS.ptr)
    {
        return nullptr;
    }

    return *inRHS.ptr;
}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType() : value(nullptr) {}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType(TPtr inRHS) : value(inRHS) {}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType(const PointerReference<T>& inRHS) : value(FromReference(inRHS.ptr)) {}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType(const null&) : value(nullptr) {}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType(const Boxed<TPtr>& inRHS) : value(FromBoxed(inRHS)) {}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType(const Variant& inRHS) : value(FromDynamic(inRHS)) {}

template<class T>
inline cpp::marshal::PointerType<T>::PointerType(const Dynamic& inRHS) : value(FromDynamic(inRHS)) {}

template<class T>
inline cpp::marshal::PointerType<T>& cpp::marshal::PointerType<T>::operator=(const PointerReference<T>& inRHS)
{
    value = *inRHS.ptr;

    return *this;
}

template<class T>
inline cpp::marshal::PointerType<T>& cpp::marshal::PointerType<T>::operator=(const null& inRHS)
{
    value = nullptr;

    return *this;
}