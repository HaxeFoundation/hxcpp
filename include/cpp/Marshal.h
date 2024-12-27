#pragma once

#include <type_traits>

namespace cpp
{
	namespace marshal
	{
        template<class T>
        class Reference;

        template<class T>
        class ValueType;

        template<class T>
        class Boxed_obj;

        template<class T>
        using Boxed = ::hx::ObjectPtr<Boxed_obj<T>>;

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

            operator ValueType<T>() const;
        };

        template<class T>
        class Reference final : public ::cpp::Reference<T>
        {
            using Super = ::cpp::Reference<T>;

            static T* FromDynamic(const Dynamic& inRHS);

            static T* FromBoxed(const Boxed<T>& inRHS);

        public:
            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            Reference(const T* inRHS);
            Reference(const ValueType<T>& inRHS);
            Reference(const Boxed<T>& inRHS);
            Reference(const Variant& inRHS);
            Reference(const Dynamic& inRHS);

            operator Dynamic() const;
            operator Variant() const;
            operator Boxed<T>() const;

            T* operator->() const;
        };

        template<class T>
        class ValueType final : public Struct<T>
        {
            static T* FromDynamic(const Dynamic& inRHS);

        public:
            using ::cpp::Struct<T>::value;

            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            ValueType(const null& inRHS) = delete;

            ValueType();
            ValueType(const Reference<T>& inRHS);
            ValueType(const Boxed<T>& inRHS);
            ValueType(const Variant& inRHS);
            ValueType(const Dynamic& inRHS);

            template<class... TArgs>
            ValueType(TArgs... args);

            ValueType<T>& operator=(const Reference<T>& inRHS);
            ValueType<T>& operator=(const null& inRHS) = delete;
        };

        // Boxed implementation

        template<class T>
        void Boxed_obj<T>::finalise(::hx::Object* obj)
        {
            auto ptr = reinterpret_cast<Boxed_obj<T>*>(obj);

            ptr->value.~T();
        }

        template<class T>
        Boxed_obj<T>::Boxed_obj() : value()
        {
            if constexpr (std::is_destructible<T>::value)
            {
                ::hx::GCSetFinalizer(this, finalise);
            }
        }

        template<class T>
        Boxed_obj<T>::Boxed_obj(T* ptr) : value(*ptr)
        {
            if constexpr (std::is_destructible<T>::value)
            {
                ::hx::GCSetFinalizer(this, finalise);
            }
        }

        template<class T>
        template<class ...TArgs>
        Boxed_obj<T>::Boxed_obj(TArgs... args) : value(std::forward<TArgs>(args)...)
        {
            if constexpr (std::is_destructible<T>::value)
            {
                ::hx::GCSetFinalizer(this, finalise);
            }
        }

        template<class T>
        Boxed_obj<T>::operator ValueType<T>() const
        {
            return ValueType<T>(Boxed<T>(this));
        }

		// Reference implementation

        template<class T>
        T* Reference<T>::FromDynamic(const Dynamic& inRHS)
        {
            return FromBoxed(inRHS.Cast<Boxed<T>>());
        }

        template<class T>
        T* Reference<T>::FromBoxed(const Boxed<T>& inRHS)
        {
            if (nullptr == inRHS.mPtr)
            {
                return nullptr;
            }

            return const_cast<T*>(&inRHS->value);
        }

        template<class T>
        Reference<T>::Reference(const T* inRHS) : Super(inRHS) {}

        template<class T>
        Reference<T>::Reference(const ValueType<T>& inRHS) : Super(inRHS) {}

        template<class T>
        Reference<T>::Reference(const Boxed<T>& inRHS) : Super(FromBoxed(inRHS)) {}

        template<class T>
        Reference<T>::Reference(const Variant& inRHS) : Super(FromDynamic(inRHS)) {}

        template<class T>
        Reference<T>::Reference(const Dynamic& inRHS) : Super(FromDynamic(inRHS)) {}

        template<class T>
        Reference<T>::operator Dynamic() const
        {
            return Boxed<T>(new Boxed_obj<T>(Super::ptr));
        }

        template<class T>
        Reference<T>::operator Variant() const
        {
            return Boxed<T>(new Boxed_obj<T>(Super::ptr));
        }

        template<class T>
        Reference<T>::operator Boxed<T>() const
        {
            return Boxed<T>(new Boxed_obj<T>(Super::ptr));
        }

        template<class T>
        T* Reference<T>::operator ->() const
        {
            if (nullptr == Super::ptr)
            {
                ::hx::NullReference("ValueType", true);
            }

            return Super::ptr;
        }

        // ValueType implementation

        template<class T>
        T* ValueType<T>::FromDynamic(const Dynamic& inRHS)
        {
            auto boxed = inRHS.Cast<Boxed<T>>();
            auto ptr   = &boxed.mPtr->value;

            return ptr;
        }

        template<class T>
        ValueType<T>::ValueType() : Struct<T>() {}

        template<class T>
        ValueType<T>::ValueType(const Reference<T>& inRHS) : Struct<T>(inRHS) {}

        template<class T>
        ValueType<T>::ValueType(const Boxed<T>& inRHS) : Struct<T>(inRHS->value) {}

        template<class T>
        ValueType<T>::ValueType(const Variant& inRHS) : Struct<T>(::cpp::Reference<T>(FromDynamic(inRHS.asDynamic()))) {}

        template<class T>
        ValueType<T>::ValueType(const Dynamic& inRHS) : Struct<T>(::cpp::Reference<T>(FromDynamic(inRHS))) {}

        template<class T>
        template<class ...TArgs>
        ValueType<T>::ValueType(TArgs ...args) : Struct<T>(std::forward<TArgs>(args)...) {}

        template<class T>
        ValueType<T>& ValueType<T>::operator=(const Reference<T>& inRHS)
        {
            value = *inRHS.ptr;

            return *this;
        }

        // Value type struct handler

        template<class T>
        struct ValueTypeStructHandler
        {
            static const char* getName() { return "cpp.ValueType"; }
            static ::String toString(const void* inValue) { return HX_CSTRING("cpp.ValueType"); }
            static void handler(DynamicHandlerOp op, void* inValue, int inSize, void* outResult)
            {
                switch (op)
                {
                case dhoToString:
                {
                    *static_cast<::String*>(outResult) = toString(inValue);
                    break;
                }

                case dhoGetClassName:
                {
                    *static_cast<const char**>(outResult) = getName();
                    break;
                }

                case dhoToDynamic:
                {
                    auto ptr = static_cast<T*>(inValue);
                    auto boxed = new Boxed_obj<T>(ptr);

                    outResult = boxed;
                    break;
                }

                case dhoFromDynamic:
                {
                    auto params = static_cast<StructHandlerDynamicParams*>(outResult);
                    auto ptr = static_cast<T*>(inValue);
                    auto wrapped = Boxed<T>(reinterpret_cast<Boxed_obj<T>*>(params->inData));

                    params->outProcessed = true;

                    *ptr = wrapped->value;
                    break;
                }

                case dhoIs:
                    break;
                }
            }
        };
	}
}