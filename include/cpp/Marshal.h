#pragma once

#include <type_traits>

namespace cpp
{
	namespace marshal
	{
		// Boxed class is used to promote a value type to the GC heap.

		template<class T>
		class Boxed_obj final : public ::hx::Object
		{
            static void finalise(::hx::Object* obj)
            {
                auto ptr = reinterpret_cast<Boxed_obj<T>*>(obj);

                ptr->value.~T();
            }

        public:
			T value;

            Boxed_obj() : value()
            {
                if constexpr (std::is_destructible<T>::value)
                {
                    ::hx::GCSetFinalizer(this, finalise);
                }
            }

            Boxed_obj(T* ptr) : value(*ptr)
            {
                if constexpr (std::is_destructible<T>::value)
                {
                    ::hx::GCSetFinalizer(this, finalise);
                }
            }

            template<class... TArgs>
            Boxed_obj(TArgs... args) : value(std::forward<TArgs>(args)...)
            {
                if constexpr (std::is_destructible<T>::value)
                {
                    ::hx::GCSetFinalizer(this, finalise);
                }
            }
		};

		template<class T>
        using Boxed = ::hx::ObjectPtr<Boxed_obj<T>>;

		//

        template<class T>
        struct ValueTypeStructHandler
        {
            static inline const char* getName() { return "cpp.ValueType"; }
            static inline ::String toString(const void* inValue) { return HX_CSTRING("cpp.ValueType"); }
            static inline void handler(DynamicHandlerOp op, void* inValue, int inSize, void* outResult)
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

        template<class T>
        class Reference;

        template<class T>
        class ValueType;

        template<class T>
        class Reference final : public ::cpp::Reference<T>
        {
            using Super = ::cpp::Reference<T>;

            static T* FromDynamic(const Dynamic& inRHS)
            {
                return FromBoxed(inRHS.Cast<Boxed<T>>());
                //auto boxed = inRHS.Cast<Boxed<T>>(); // Boxed<T>(reinterpret_cast<Boxed_obj<T>*>());
                //auto ptr   = &boxed->value;

                //return ptr;
            }

            static T* FromBoxed(const Boxed<T>& inRHS)
            {
                if (nullptr == inRHS.mPtr)
                {
                    return nullptr;
                }

                return const_cast<T*>(&inRHS->value);
            }

        public:
            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            Reference(const T* inRHS) : Super(inRHS) {}
            Reference(const ValueType<T>& inRHS) : Super(inRHS) { }
            Reference(const Boxed<T>& inRHS) : Super(FromBoxed(inRHS)) {}
            Reference(const Variant& inRHS) : Super(FromDynamic(inRHS.asDynamic())) {}
            Reference(const Dynamic& inRHS) : Super(FromDynamic(inRHS)) {}

            operator Dynamic () const
            {
                return Boxed<T>(new Boxed_obj<T>(Super::ptr));
            }

            operator Variant() const
            {
                return Boxed<T>(new Boxed_obj<T>(Super::ptr));
            }

            T* operator->() const
            {
                if (nullptr == Super::ptr)
                {
                    ::hx::NullReference("ValueType", true);
                }

                return Super::ptr;
            }
        };

        template<class T>
        class ValueType final : public Struct<T>
        {
            static T* FromDynamic(const Dynamic& inRHS)
            {
                auto boxed = inRHS.Cast<Boxed<T>>();
                auto ptr   = &boxed.mPtr->value;

                return ptr;
            }

        public:
            using ::cpp::Struct<T>::value;

            // This allows 'StaticCast' to be used from arrays
            using Ptr = Dynamic;

            ValueType() : Struct<T>() {}
            ValueType(const Reference<T>& inRHS) : Struct<T>(inRHS) {}
            ValueType(const Boxed<T>& inRHS) : Struct<T>(inRHS->value) {}
            ValueType(const Variant& inRHS) : Struct<T>(::cpp::Reference<T>(FromDynamic(inRHS.asDynamic()))) {}
            ValueType(const Dynamic& inRHS) : Struct<T>(::cpp::Reference<T>(FromDynamic(inRHS))) {}

            template<class... TArgs>
            ValueType(TArgs... args) : Struct<T>(std::forward<TArgs>(args)...) {}

            ValueType<T>& operator=(const Reference<T>& inRHS)
            {
                value = *inRHS.ptr;
                
                return *this;
            }

            operator Dynamic () const
            {
                return Boxed<T>(new Boxed_obj<T>(&value));
            }

            operator Variant() const
            {
                return Boxed<T>(new Boxed_obj<T>(&value));
            }
        };
	}
}