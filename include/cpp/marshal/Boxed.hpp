#pragma once

#include "Definitions.inc"

template<class T>
inline void cpp::marshal::Boxed_obj<T>::finalise(::hx::Object* obj)
{
    auto ptr = reinterpret_cast<Boxed_obj<T>*>(obj);

    ptr->value.~T();
}

template<class T>
inline void cpp::marshal::Boxed_obj<T>::setFinaliser(std::true_type)
{
    ::hx::GCSetFinalizer(this, finalise);
}

template<class T>
inline void cpp::marshal::Boxed_obj<T>::setFinaliser(std::false_type) {}

template<class T>
inline cpp::marshal::Boxed_obj<T>::Boxed_obj() : value()
{
    setFinaliser(std::is_destructible<T>{});
}

template<class T>
inline cpp::marshal::Boxed_obj<T>::Boxed_obj(T* ptr) : value(*ptr)
{
    setFinaliser(std::is_destructible<T>{});
}

template<class T>
template<class ...TArgs>
inline cpp::marshal::Boxed_obj<T>::Boxed_obj(TArgs... args) : value(std::forward<TArgs>(args)...)
{
    setFinaliser(std::is_destructible<T>{});
}