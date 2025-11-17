#ifndef CPP_MARSHAL_H
#define CPP_MARSHAL_H

#include <type_traits>
#include <cstring>
#include <string>
#include <cmath>

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

            template<class K>
            K get(int index);

            template<class K>
            void set(int index, K value);
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

            template<class K>
            K get(int index);

            template<class K>
            void set(int index, K value);
        };

        template<class T>
        struct View final
        {
            ::cpp::Pointer<T> ptr;
            int length;

            View(::cpp::Pointer<T> _ptr, int _length);

            void clear();
            void fill(T value);
            bool isEmpty();
            View<T> slice(int index);
            View<T> slice(int index, int length);
            bool tryCopyTo(const View<T>& destination);
            template<class K> View<K> reinterpret();

            bool operator==(const View<T>& inRHS) const;
            bool operator!=(const View<T>& inRHS) const;

            T& operator[] (int index);
        };

        struct Marshal final
        {
            static View<char> toCharView(::String string);
            static int toCharView(::String, View<char> buffer);

            static View<char16_t> toWideCharView(::String string);
            static int toWideCharView(::String, View<char16_t> buffer);

            static ::String toString(View<char> buffer);
            static ::String toString(View<char16_t> buffer);

            template<class T> static T read(View<uint8_t> view);
            template<class T> static void write(View<uint8_t> view, const T& value);
        };
    }
}

//

inline cpp::marshal::View<char> cpp::marshal::Marshal::toCharView(::String string)
{
    auto length = 0;
    auto ptr    = string.utf8_str(nullptr, true, &length);

    return View<char>(const_cast<char*>(ptr), length + 1);
}

inline int cpp::marshal::Marshal::toCharView(::String string, View<char> buffer)
{
    auto length = 0;

    if (string.utf8_str(buffer, &length))
    {
        return length;
    }
    else
    {
        hx::Throw(HX_CSTRING("Not enough space in the view to write the string"));

        return 0;
    }
}

inline cpp::marshal::View<char16_t> cpp::marshal::Marshal::toWideCharView(::String string)
{
    auto length = 0;
    auto ptr    = string.wc_str(nullptr, &length);

    return View<char16_t>(const_cast<char16_t*>(ptr), length + 1);
}

inline int cpp::marshal::Marshal::toWideCharView(::String string, View<char16_t> buffer)
{
    auto length = 0;

    if (string.wc_str(buffer, &length))
    {
        return length;
    }
    else
    {
        hx::Throw(HX_CSTRING("Not enough space in the view to write the string"));

        return 0;
    }
}

inline ::String cpp::marshal::Marshal::toString(View<char> buffer)
{
    return ::String::create(buffer);
}

inline ::String cpp::marshal::Marshal::toString(View<char16_t> buffer)
{
    return ::String::create(buffer);
}

template<class T>
inline void cpp::marshal::Marshal::write(View<uint8_t> view, const T& value)
{
    std::memcpy(view.ptr, reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}

template<class T>
inline T cpp::marshal::Marshal::read(View<uint8_t> view)
{
    return *(reinterpret_cast<T*>(view.ptr.ptr));
}

//

template<class T>
inline cpp::marshal::View<T>::View(::cpp::Pointer<T> _ptr, int _length) : ptr(_ptr), length(_length) {}

template<class T>
inline bool cpp::marshal::View<T>::tryCopyTo(const View<T>& destination)
{
    auto requiredSize = sizeof(T) * length;
    if (destination.length < requiredSize)
    {
        return false;
    }

    std::memcpy(destination.ptr, ptr, requiredSize);

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
template<class K>
inline K cpp::marshal::ValueReference<T>::get(int index)
{
    return (*Super::ptr)[index];
}

template<class T>
template<class K>
inline void cpp::marshal::ValueReference<T>::set(int index, K value)
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
template<class K>
inline K cpp::marshal::PointerReference<T>::get(int index)
{
    return (*Super::ptr)[index];
}

template<class T>
template<class K>
inline void cpp::marshal::PointerReference<T>::set(int index, K value)
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

template<typename T>
inline cpp::Pointer<T*> cpp::Pointer_obj::addressOf(const ::cpp::marshal::PointerReference<T>& ref)
{
    return Pointer<T*>(ref.ptr);
}

// I'm not sure why I need this pointer ctor overload, I'm sure it was working without it at some point
template<typename T>
inline cpp::Pointer<T>::Pointer(const ::cpp::marshal::PointerReference<T> ref)
{
    ptr = *ref.ptr;
}

#endif