#pragma once

#include "Definitions.inc"

template<class T>
template<class O>
inline O* cpp::marshal::ValueReference<T>::FromDynamic(const Dynamic& inRHS)
{
    return FromBoxed(inRHS.StaticCast<Boxed<O>>());
}

template<class T>
template<class O>
inline O* cpp::marshal::ValueReference<T>::FromBoxed(const Boxed<O>& inRHS)
{
    if (nullptr == inRHS.mPtr)
    {
        return nullptr;
    }

    return const_cast<O*>(&inRHS->value);
}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference() : Super(nullptr) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const null& inRHS) : Super(inRHS) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const ValueType<T>& inRHS) : Super(inRHS.value) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const Boxed<T>& inRHS) : Super(FromBoxed<T>(inRHS)) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const T& inRHS) : Super(inRHS) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(T& inRHS) : Super(inRHS) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const T* inRHS) : Super(inRHS) {}

template<class T>
template<class O>
inline cpp::marshal::ValueReference<T>::ValueReference(const ValueReference<O>& inRHS) : Super(reinterpret_cast<T*>(const_cast<O*>(inRHS.ptr))) {}

template<class T>
template<class O>
inline cpp::marshal::ValueReference<T>::ValueReference(const ValueType<O>& inRHS) : Super(reinterpret_cast<T*>(const_cast<O*>(&inRHS.value))) {}

template<class T>
template<class O>
inline cpp::marshal::ValueReference<T>::ValueReference(const Boxed<O>& inRHS) : Super(reinterpret_cast<T*>(FromBoxed<O>(inRHS))) {}

template<class T>
template<class K>
inline K cpp::marshal::ValueReference<T>::get(int64_t index)
{
    return (*Super::ptr)[index];
}

template<class T>
template<class K>
inline void cpp::marshal::ValueReference<T>::set(int64_t index, K value)
{
    (*Super::ptr)[index] = value;
}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const Variant& inRHS) : Super(FromDynamic<T>(inRHS)) {}

template<class T>
inline cpp::marshal::ValueReference<T>::ValueReference(const Dynamic& inRHS) : Super(FromDynamic<T>(inRHS)) {}

template<class T>
inline cpp::marshal::Boxed<T> cpp::marshal::ValueReference<T>::ToBoxed() const
{
    if (Super::ptr)
    {
        return Boxed<T>(new Boxed_obj<T>(Super::ptr));
    }
    else
    {
        return Boxed<T>();
    }
}

template<class T>
inline cpp::marshal::ValueReference<T>::operator ::Dynamic() const
{
    return ToBoxed();
}

template<class T>
inline cpp::marshal::ValueReference<T>::operator ::cpp::Variant() const
{
    return ToBoxed();
}

template<class T>
inline cpp::marshal::ValueReference<T>::operator ::cpp::marshal::Boxed<T>() const
{
    return ToBoxed();
}

template<class T>
inline T* cpp::marshal::ValueReference<T>::operator ->() const
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return Super::ptr;
}

template<class T>
inline bool cpp::marshal::ValueReference<T>::operator==(const ValueReference<T>& inRHS) const
{
    return (*Super::ptr) == (*inRHS.ptr);
}

template<class T>
inline bool cpp::marshal::ValueReference<T>::operator!=(const ValueReference<T>& inRHS) const
{
    return (*Super::ptr) != (*inRHS.ptr);
}

template<class T>
inline T cpp::marshal::ValueReference<T>::operator*() const
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return *Super::ptr;
}