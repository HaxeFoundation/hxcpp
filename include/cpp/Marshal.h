#ifndef CPP_MARSHAL_H
#define CPP_MARSHAL_H

#include <type_traits>

namespace cpp
{
    namespace marshal
    {
        /// <summary>
        /// Templated class to hold a non GC object on the heap.
        /// If T has a destructor a finaliser is added so it is called when this object is collected.
        /// </summary>
        template<class T>
        class Boxed_obj final : public ::hx::Object
        {
            static void finalise(::hx::Object* obj);

            void setFinaliser(std::true_type);
            void setFinaliser(std::false_type);

        public:
            T value;

            Boxed_obj();

            Boxed_obj(T* ptr);

            template<class... TArgs>
            Boxed_obj(TArgs... args);
        };

        template<class T>
        class ValueType final
        {
            // These true and false variants are called based on if T is a pointer
            // If T is not a pointer trying to assign a value type to null results in a null pointer exception being thrown.
            // But if T is a pointer then the value type holds a null pointer value.

            static T FromReference(const ValueReference<T>& inRHS);
            static T FromBoxed(const Boxed<T>& inRHS);
            static T FromDynamic(const Dynamic& inRHS);

        public:
            T value;

            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            ValueType();
            ValueType(const ValueReference<T>& inRHS);
            ValueType(const null& inRHS);
            ValueType(const Boxed<T>& inRHS);
            ValueType(const Variant& inRHS);
            ValueType(const Dynamic& inRHS);

            template<class... TArgs>
            ValueType(TArgs... args);

            ValueType<T>& operator=(const ValueReference<T>& inRHS);
            ValueType<T>& operator=(const null& inRHS);
        };

        template<class T>
        class PointerType final
        {
        public:
            using TPtr = T*;

        private:
            static TPtr FromReference(const PointerReference<T>& inRHS);
            static TPtr FromBoxed(const Boxed<TPtr>& inRHS);
            static TPtr FromDynamic(const Dynamic& inRHS);

        public:
            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            TPtr value;

            PointerType();
            PointerType(TPtr inRHS);
            PointerType(const PointerReference<T>& inRHS);
            PointerType(const null& inRHS);
            PointerType(const Boxed<TPtr>& inRHS);
            PointerType(const Variant& inRHS);
            PointerType(const Dynamic& inRHS);

            PointerType<T>& operator=(const PointerReference<T>& inRHS);
            PointerType<T>& operator=(const null& inRHS);
        };

        template<class T>
        class ValueReference final : public ::cpp::Reference<T>
        {
            using Super = ::cpp::Reference<T>;

            template<class O>
            static O* FromDynamic(const Dynamic& inRHS);
            template<class O>
            static O* FromBoxed(const Boxed<O>& inRHS);
            Boxed<T> ToBoxed() const;

        public:
            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            ValueReference(const null& inRHS);
            ValueReference(const ValueType<T>& inRHS);
            ValueReference(const Boxed<T>& inRHS);
            ValueReference(const T& inRHS);
            ValueReference(T& inRHS);
            ValueReference(const T* inRHS);

            template<class O>
            ValueReference(const ValueReference<O>& inRHS);
            template<class O>
            ValueReference(const ValueType<O>& inRHS);
            template<class O>
            ValueReference(const Boxed<O>& inRHS);

            ValueReference(const Variant& inRHS);
            ValueReference(const Dynamic& inRHS);

            operator Dynamic() const;
            operator Variant() const;
            operator Boxed<T>() const;

            T* operator->() const;
            T operator*() const;

            bool operator==(const ValueReference<T>& inRHS) const;
            bool operator!=(const ValueReference<T>& inRHS) const;
        };

        template<class T>
        class PointerReference final : public ::cpp::Reference<T*>
        {
        public:
            using TPtr = T*;

        private:
            using Super = ::cpp::Reference<TPtr>;

            template<class O>
            static O* FromDynamic(const Dynamic& inRHS);

            template<class O>
            static O* FromBoxed(const Boxed<O>& inRHS);

            Boxed<TPtr> ToBoxed() const;

        public:
            PointerReference(const null& inRHS);
            PointerReference(const PointerType<T>& inRHS);
            PointerReference(const Boxed<TPtr>& inRHS);

            template<class O>
            PointerReference(const PointerReference<O>& inRHS);
            template<class O>
            PointerReference(const PointerType<O>& inRHS);
            template<class O>
            PointerReference(const Boxed<O>& inRHS);

            PointerReference(const TPtr& inRHS);
            PointerReference(TPtr& inRHS);
            PointerReference(const TPtr* inRHS);
            PointerReference(const Variant& inRHS);
            PointerReference(const Dynamic& inRHS);

            operator Dynamic() const;
            operator Variant() const;
            operator Boxed<TPtr>() const;
            operator Pointer<T>() const;

            operator TPtr&();
            operator TPtr*();
            operator void*();
            operator void**();

            TPtr operator->() const;
        };
    }
}

// Boxed implementation

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
inline cpp::marshal::Boxed_obj<T>::Boxed_obj(TArgs... args) : value( std::forward<TArgs>(args)... )
{
    setFinaliser(std::is_destructible<T>{});
}

// Reference implementation

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

// Pointer implementation

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
inline cpp::marshal::PointerReference<T>::operator TPtr&()
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

//

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

// ValueType implementation

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
inline cpp::marshal::ValueType<T>::ValueType(TArgs... args) : value( std::forward<TArgs>(args)... ) {}

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

// Implement some pointer helpers here

template<typename T>
inline cpp::Pointer<T> cpp::Pointer_obj::addressOf(const ::cpp::marshal::ValueReference<T>& ref)
{
    return Pointer<T>(ref.ptr);
}

#endif