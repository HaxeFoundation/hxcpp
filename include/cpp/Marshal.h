#ifndef CPP_MARSHAL_H
#define CPP_MARSHAL_H

#include <type_traits>

namespace cpp
{
    namespace marshal
    {
        template<class T>
        class Boxed_obj final : public ::hx::Object
        {
            static void finalise(::hx::Object* obj);

        public:
            T value;

            Boxed_obj();

            Boxed_obj(T* ptr);

            template<class... TArgs>
            Boxed_obj(TArgs... args);
        };

        template<class T>
        class Reference final : public ::cpp::Reference<T>
        {
            using Super = ::cpp::Reference<T>;

            static T* FromDynamic(const Dynamic& inRHS);

            static T* FromBoxed(const Boxed<T>& inRHS);

            Boxed<T> ToBoxed() const;

        public:
            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            Reference(const ValueType<T>& inRHS);
            Reference(const Boxed<T>& inRHS);
            Reference(const Variant& inRHS);
            Reference(const Dynamic& inRHS);
            Reference(const T& inRHS);
            Reference(T& inRHS);
            Reference(const T* inRHS);

            operator Dynamic() const;
            operator Variant() const;
            operator Boxed<T>() const;

            T* operator->() const;


            bool operator==(const Reference<T>& inRHS) const;
            bool operator!=(const Reference<T>& inRHS) const;
        };

        template<class T>
        class ValueType final
        {
            static T* FromReference(const Reference<T>& inRHS);
            static T* FromDynamic(const Dynamic& inRHS);
            static T* FromBoxed(const Boxed<T>& inRHS);

        public:
            T value;

            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            ValueType(const null& inRHS) = delete;

            ValueType();
            ValueType(const Reference<T>& inRHS);
            ValueType(const Boxed<T>& inRHS);
            ValueType(Boxed_obj<T>* inRHS);
            ValueType(const Variant& inRHS);
            ValueType(const Dynamic& inRHS);

            template<class... TArgs>
            ValueType(TArgs... args);

            ValueType<T>& operator=(const Reference<T>& inRHS);
            ValueType<T>& operator=(const null& inRHS) = delete;
        };
    }
}

// Boxed implementation

template<class T>
void cpp::marshal::Boxed_obj<T>::finalise(::hx::Object* obj)
{
    auto ptr = reinterpret_cast<Boxed_obj<T>*>(obj);

    ptr->value.~T();
}

template<class T>
cpp::marshal::Boxed_obj<T>::Boxed_obj() : value()
{
    if constexpr (std::is_destructible<T>::value)
    {
        ::hx::GCSetFinalizer(this, finalise);
    }
}

template<class T>
cpp::marshal::Boxed_obj<T>::Boxed_obj(T* ptr) : value(*ptr)
{
    if constexpr (std::is_destructible<T>::value)
    {
        ::hx::GCSetFinalizer(this, finalise);
    }
}

template<class T>
template<class ...TArgs>
cpp::marshal::Boxed_obj<T>::Boxed_obj(TArgs... args) : value(std::forward<TArgs>(args)...)
{
    if constexpr (std::is_destructible<T>::value)
    {
        ::hx::GCSetFinalizer(this, finalise);
    }
}

// Reference implementation

template<class T>
T* cpp::marshal::Reference<T>::FromDynamic(const Dynamic& inRHS)
{
    return FromBoxed(inRHS.StaticCast<Boxed<T>>());
}

template<class T>
T* cpp::marshal::Reference<T>::FromBoxed(const Boxed<T>& inRHS)
{
    if (nullptr == inRHS.mPtr)
    {
        return nullptr;
    }

    return const_cast<T*>(&inRHS->value);
}

template<class T>
cpp::marshal::Reference<T>::Reference(const T& inRHS) : Super(inRHS) {}

template<class T>
cpp::marshal::Reference<T>::Reference(T& inRHS) : Super(inRHS) {}

template<class T>
cpp::marshal::Reference<T>::Reference(const T* inRHS) : Super(inRHS) {}

template<class T>
cpp::marshal::Reference<T>::Reference(const ValueType<T>& inRHS) : Super(inRHS.value) {}

template<class T>
cpp::marshal::Reference<T>::Reference(const Boxed<T>& inRHS) : Super(FromBoxed(inRHS)) {}

template<class T>
cpp::marshal::Reference<T>::Reference(const Variant& inRHS) : Super(FromDynamic(inRHS)) {}

template<class T>
cpp::marshal::Reference<T>::Reference(const Dynamic& inRHS) : Super(FromDynamic(inRHS)) {}

template<class T>
cpp::marshal::Boxed<T> cpp::marshal::Reference<T>::ToBoxed() const
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
cpp::marshal::Reference<T>::operator ::Dynamic() const
{
    return ToBoxed();
}

template<class T>
cpp::marshal::Reference<T>::operator ::cpp::Variant() const
{
    return ToBoxed();
}

template<class T>
cpp::marshal::Reference<T>::operator ::cpp::marshal::Boxed<T>() const
{
    return ToBoxed();
}

template<class T>
T* cpp::marshal::Reference<T>::operator ->() const
{
    if (nullptr == Super::ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return Super::ptr;
}

template<class T>
inline bool cpp::marshal::Reference<T>::operator==(const Reference<T>& inRHS) const
{
    return (*Super::ptr) == (*inRHS->ptr);
}

template<class T>
inline bool cpp::marshal::Reference<T>::operator!=(const Reference<T>& inRHS) const
{
    return (*Super::ptr) != (*inRHS->ptr);
}

// ValueType implementation

template<class T>
T* cpp::marshal::ValueType<T>::FromDynamic(const Dynamic& inRHS)
{
    return FromBoxed(inRHS.StaticCast<Boxed<T>>());
}

template<class T>
T* cpp::marshal::ValueType<T>::FromBoxed(const Boxed<T>& inRHS)
{
    if (nullptr == inRHS.mPtr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return const_cast<T*>(&inRHS->value);
}

template<class T>
T* cpp::marshal::ValueType<T>::FromReference(const Reference<T>& inRHS)
{
    if (nullptr == inRHS.ptr)
    {
        ::hx::NullReference("ValueType", true);
    }

    return inRHS.ptr;
}

template<class T>
cpp::marshal::ValueType<T>::ValueType() : value() {}

template<class T>
cpp::marshal::ValueType<T>::ValueType(const Reference<T>& inRHS) : value(*FromReference(inRHS.ptr)) {}

template<class T>
cpp::marshal::ValueType<T>::ValueType(const Boxed<T>& inRHS) : ValueType<T>(::cpp::marshal::Reference<T>(FromBoxed(inRHS))) {}

template<class T>
cpp::marshal::ValueType<T>::ValueType(Boxed_obj<T>* inRHS) : ValueType<T>(::cpp::marshal::Reference<T>(FromBoxed(inRHS))) {}

template<class T>
cpp::marshal::ValueType<T>::ValueType(const Variant& inRHS) : ValueType<T>(::cpp::marshal::Reference<T>(FromDynamic(inRHS.asDynamic()))) {}

template<class T>
cpp::marshal::ValueType<T>::ValueType(const Dynamic& inRHS) : ValueType<T>(::cpp::marshal::Reference<T>(FromDynamic(inRHS))) {}

template<class T>
template<class ...TArgs>
cpp::marshal::ValueType<T>::ValueType(TArgs ...args) : value(std::forward<TArgs>(args)...) {}

template<class T>
cpp::marshal::ValueType<T>& cpp::marshal::ValueType<T>::operator=(const Reference<T>& inRHS)
{
    value = *inRHS.ptr;

    return *this;
}

// Implement some pointer helpers here

template<typename T>
inline cpp::Pointer<T> cpp::Pointer_obj::addressOf(const ::cpp::marshal::Reference<T>& ref)
{
    return Pointer<T>(ref.ptr);
}

#endif