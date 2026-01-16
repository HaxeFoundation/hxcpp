#pragma once

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

            ValueReference();
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
            K get(int64_t index);

            template<class K>
            void set(int64_t index, K value);
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
            K get(int64_t index);

            template<class K>
            void set(int64_t index, K value);
        };

        template<class T>
        struct View final
        {
            ::cpp::Pointer<T> ptr;
            int64_t length;

            View(::cpp::Pointer<T> _ptr, int64_t _length);

            void clear() const;
            void fill(T value) const;
            bool isEmpty() const;
            View<T> slice(int64_t index) const;
            View<T> slice(int64_t index, int64_t length) const;
            void copyTo(const View<T>& destination) const;
            bool tryCopyTo(const View<T>& destination) const;
            template<class K> View<K> reinterpret() const;
            int compare(const View<T>& inRHS) const;

            bool operator==(const View<T>& inRHS) const;
            bool operator!=(const View<T>& inRHS) const;

            T& operator[] (int64_t index) const;
        };

        class RootHandle
        {
            ::hx::Object** object;

            RootHandle(hx::Object** obj);

        public:
            static RootHandle create(Dynamic obj);
            static RootHandle fromVoidPointer(::cpp::Pointer<void> ptr);

            ::cpp::Pointer<void> toVoidPointer();
            Dynamic getObject();
            void close();
        };

        struct Marshal final
        {
#ifdef HXCPP_BIG_ENDIAN
            static const bool isBigEndian = true;
#else
            static const bool isBigEndian = false;
#endif

            static View<char> asCharView(const ::String& string);
            static View<char16_t> asWideCharView(const ::String& string);

            template<class T> static T read(const View<uint8_t>& view);
            template<class T> static ::cpp::Pointer<T> readPointer(const View<uint8_t>& view);
            static int8_t readInt8(const View<uint8_t>& view);
            static int16_t readInt16(const View<uint8_t>& view);
            static int32_t readInt32(const View<uint8_t>& view);
            static int64_t readInt64(const View<uint8_t>& view);
            static uint8_t readUInt8(const View<uint8_t>& view);
            static uint16_t readUInt16(const View<uint8_t>& view);
            static uint32_t readUInt32(const View<uint8_t>& view);
            static uint64_t readUInt64(const View<uint8_t>& view);
            static float readFloat32(const View<uint8_t>& view);
            static double readFloat64(const View<uint8_t>& view);

            template<class T> static ::cpp::Pointer<T> readLittleEndianPointer(const View<uint8_t>& view);
            static int16_t readLittleEndianInt16(const View<uint8_t>& view);
            static int32_t readLittleEndianInt32(const View<uint8_t>& view);
            static int64_t readLittleEndianInt64(const View<uint8_t>& view);
            static uint16_t readLittleEndianUInt16(const View<uint8_t>& view);
            static uint32_t readLittleEndianUInt32(const View<uint8_t>& view);
            static uint64_t readLittleEndianUInt64(const View<uint8_t>& view);
            static float readLittleEndianFloat32(const View<uint8_t>& view);
            static double readLittleEndianFloat64(const View<uint8_t>& view);

            template<class T> static ::cpp::Pointer<T> readBigEndianPointer(const View<uint8_t>& view);
            static int16_t readBigEndianInt16(const View<uint8_t>& view);
            static int32_t readBigEndianInt32(const View<uint8_t>& view);
            static int64_t readBigEndianInt64(const View<uint8_t>& view);
            static uint16_t readBigEndianUInt16(const View<uint8_t>& view);
            static uint32_t readBigEndianUInt32(const View<uint8_t>& view);
            static uint64_t readBigEndianUInt64(const View<uint8_t>& view);
            static float readBigEndianFloat32(const View<uint8_t>& view);
            static double readBigEndianFloat64(const View<uint8_t>& view);

            template<class T> static void write(const View<uint8_t>& view, const T& value);
            template<class T> static void writePointer(const View<uint8_t>& view, const Pointer<T>& value);
            static void writeInt8(const View<uint8_t>& view, const int8_t& value);
            static void writeInt16(const View<uint8_t>& view, const int16_t& value);
            static void writeInt32(const View<uint8_t>& view, const int32_t& value);
            static void writeInt64(const View<uint8_t>& view, const int64_t& value);
            static void writeUInt8(const View<uint8_t>& view, const uint8_t& value);
            static void writeUInt16(const View<uint8_t>& view, const uint16_t& value);
            static void writeUInt32(const View<uint8_t>& view, const uint32_t& value);
            static void writeUInt64(const View<uint8_t>& view, const uint64_t& value);
            static void writeFloat32(const View<uint8_t>& view, const float& value);
            static void writeFloat64(const View<uint8_t>& view, const double& value);

            template<class T> static void writeLittleEndianPointer(const View<uint8_t>& view, const Pointer<T>& value);
            static void writeLittleEndianInt16(const View<uint8_t>& view, const int16_t& value);
            static void writeLittleEndianInt32(const View<uint8_t>& view, const int32_t& value);
            static void writeLittleEndianInt64(const View<uint8_t>& view, const int64_t& value);
            static void writeLittleEndianUInt16(const View<uint8_t>& view, const uint16_t& value);
            static void writeLittleEndianUInt32(const View<uint8_t>& view, const uint32_t& value);
            static void writeLittleEndianUInt64(const View<uint8_t>& view, const uint64_t& value);
            static void writeLittleEndianFloat32(const View<uint8_t>& view, const float& value);
            static void writeLittleEndianFloat64(const View<uint8_t>& view, const double& value);

            template<class T> static void writeBigEndianPointer(const View<uint8_t>& view, const Pointer<T>& value);
            static void writeBigEndianInt16(const View<uint8_t>& view, const int16_t& value);
            static void writeBigEndianInt32(const View<uint8_t>& view, const int32_t& value);
            static void writeBigEndianInt64(const View<uint8_t>& view, const int64_t& value);
            static void writeBigEndianUInt16(const View<uint8_t>& view, const uint16_t& value);
            static void writeBigEndianUInt32(const View<uint8_t>& view, const uint32_t& value);
            static void writeBigEndianUInt64(const View<uint8_t>& view, const uint64_t& value);
            static void writeBigEndianFloat32(const View<uint8_t>& view, const float& value);
            static void writeBigEndianFloat64(const View<uint8_t>& view, const double& value);
        };
    }
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