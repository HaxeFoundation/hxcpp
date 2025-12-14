#pragma once

#include "Definitions.inc"

template<class T>
inline cpp::marshal::Boxed<T*> cpp::marshal::PointerReference<T>::ToBoxed() const
{
    if (nullptr == Super::ptr)
    {
        return new Boxed_obj<TPtr>();
    }
    else
    {
        return new Boxed_obj<TPtr>(Super::ptr);
    }
}

template<class T>
template<class O>
inline O* cpp::marshal::PointerReference<T>::FromBoxed(const Boxed<O>& inRHS)
{
    if (nullptr == inRHS.mPtr)
    {
        return nullptr;
    }

    return const_cast<O*>(&inRHS->value);
}

template<class T>
template<class O>
inline O* cpp::marshal::PointerReference<T>::FromDynamic(const Dynamic& inRHS)
{
    return FromBoxed(inRHS.StaticCast<Boxed<O>>());
}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const null& inRHS) : Super(inRHS) {}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const PointerType<T>& inRHS) : Super(const_cast<TPtr*>(&inRHS.value)) {}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const Boxed<TPtr>& inRHS) : Super(FromBoxed<TPtr>(inRHS)) {}

template<class T>
template<class O>
inline cpp::marshal::PointerReference<T>::PointerReference(const PointerReference<O>& inRHS) : Super(reinterpret_cast<TPtr*>(const_cast<O**>(inRHS.ptr))) {}

template<class T>
template<class O>
inline cpp::marshal::PointerReference<T>::PointerReference(const PointerType<O>& inRHS) : Super(reinterpret_cast<TPtr*>(const_cast<O**>(&inRHS.value))) {}

template<class T>
template<class O>
inline cpp::marshal::PointerReference<T>::PointerReference(const Boxed<O>& inRHS) : Super(reinterpret_cast<TPtr*>(FromBoxed<O>(inRHS))) {}

template<class T>
template<class K>
inline K cpp::marshal::PointerReference<T>::get(int64_t index)
{
    return (*Super::ptr)[index];
}

template<class T>
template<class K>
inline void cpp::marshal::PointerReference<T>::set(int64_t index, K value)
{
    (*Super::ptr)[index] = value;
}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const TPtr& inRHS) : Super(inRHS) {}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(TPtr& inRHS)
{
    if (nullptr != inRHS)
    {
        Super::ptr = &inRHS;
    }
}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const TPtr* inRHS) : Super(inRHS) {}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const Variant& inRHS) : Super(FromDynamic<TPtr>(inRHS)) {}

template<class T>
inline cpp::marshal::PointerReference<T>::PointerReference(const Dynamic& inRHS) : Super(FromDynamic<TPtr>(inRHS)) {}

template<class T>
inline cpp::marshal::PointerReference<T>::operator ::Dynamic() const
{
    return ToBoxed();
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator ::cpp::Variant() const
{
    return ToBoxed();
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator ::cpp::marshal::Boxed<T*>() const
{
    return ToBoxed();
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator ::cpp::Pointer<T>() const
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return ::cpp::Pointer<T>(*Super::ptr);
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator TPtr& ()
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return *Super::ptr;
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator TPtr* ()
{
    return Super::ptr;
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator void* ()
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return *Super::ptr;
}

template<class T>
inline cpp::marshal::PointerReference<T>::operator void** ()
{
    return reinterpret_cast<void**>(Super::ptr);
}

template<class T>
inline T* cpp::marshal::PointerReference<T>::operator->() const
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    if (nullptr == *Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return *Super::ptr;
}