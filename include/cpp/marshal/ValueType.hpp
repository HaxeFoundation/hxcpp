#pragma once

#include "Definitions.inc"

template<class T>
inline T cpp::marshal::ValueType<T>::FromDynamic(const Dynamic& inRHS)
{
    return FromBoxed(inRHS.StaticCast<Boxed<T>>());
}

template<class T>
inline T cpp::marshal::ValueType<T>::FromBoxed(const Boxed<T>& inRHS)
{
    if (nullptr == inRHS.mPtr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return inRHS->value;
}

template<class T>
inline T cpp::marshal::ValueType<T>::FromReference(const ValueReference<T>& inRHS)
{
    if (nullptr == inRHS.ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return *inRHS.ptr;
}

template<class T>
inline cpp::marshal::ValueType<T>::ValueType() : value() {}

template<class T>
inline cpp::marshal::ValueType<T>::ValueType(const ValueReference<T>& inRHS) : value(FromReference(inRHS.ptr)) {}

template<class T>
inline cpp::marshal::ValueType<T>::ValueType(const null& inRHS) : ValueType<T>(::cpp::marshal::ValueReference<T>(inRHS)) {}

template<class T>
inline cpp::marshal::ValueType<T>::ValueType(const Boxed<T>& inRHS) : ValueType<T>(::cpp::marshal::ValueReference<T>(FromBoxed(inRHS))) {}

template<class T>
inline cpp::marshal::ValueType<T>::ValueType(const Variant& inRHS) : ValueType<T>(::cpp::marshal::ValueReference<T>(FromDynamic(inRHS.asDynamic()))) {}

template<class T>
inline cpp::marshal::ValueType<T>::ValueType(const Dynamic& inRHS) : ValueType<T>(::cpp::marshal::ValueReference<T>(FromDynamic(inRHS))) {}

template<class T>
template<class ...TArgs>
inline cpp::marshal::ValueType<T>::ValueType(TArgs... args) : value(std::forward<TArgs>(args)...) {}

template<class T>
inline cpp::marshal::ValueType<T>& cpp::marshal::ValueType<T>::operator=(const ValueReference<T>& inRHS)
{
    if (nullptr == inRHS.ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    value = *inRHS.ptr;

    return *this;
}

template<class T>
inline cpp::marshal::ValueType<T>& cpp::marshal::ValueType<T>::operator=(const null& inRHS)
{
    ::hx::NullReference("ValueType", true);

    return *this;
}