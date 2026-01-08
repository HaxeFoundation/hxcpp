#ifndef HX_CLOSURE_H
#define HX_CLOSURE_H
#include <hxcpp.h>
#include <type_traits>

namespace hx
{
	namespace invoker
	{
		namespace index
		{
            struct Helper
            {
                template<typename T, T... Ints>
                struct integer_sequence
                {
                    typedef T value_type;
                    static constexpr std::size_t size() { return sizeof...(Ints); }
                };

                template<std::size_t... Ints>
                using index_sequence = integer_sequence<std::size_t, Ints...>;

                template<typename T, std::size_t N, T... Is>
                struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Is...> {};

                template<typename T, T... Is>
                struct make_integer_sequence<T, 0, Is...> : integer_sequence<T, Is...> {};

                template<std::size_t N>
                using make_index_sequence = make_integer_sequence<std::size_t, N>;

                template<typename... T>
                using index_sequence_for = make_index_sequence<sizeof...(T)>;
            };
		}

        template<class T>
        struct ConversionTrait
        {
            inline static Dynamic _hx_struct(T value, std::true_type)
            {
                return Dynamic{ value };
            }
            inline static Dynamic _hx_struct(T value, std::false_type)
            {
                return ::cpp::Struct<T>(value);
            }

            inline static Dynamic toDynamic(T v) { return _hx_struct(v, std::is_constructible<Dynamic, T>{}); }
            inline static T fromDynamic(Dynamic d) { return T{ d }; }
        };

        template<class T>
        struct ConversionTrait<T*>
        {
            inline static Dynamic toDynamic(T* v) { return Dynamic{ ::cpp::Pointer<T>(v)}; }
            inline static T* fromDynamic(Dynamic d) { return ::cpp::Pointer<T>(d).ptr; }
        };

        template<class T>
        struct ConversionTrait<::cpp::Pointer<T>>
        {
            inline static Dynamic toDynamic(::cpp::Pointer<T> v) { return v; }
            inline static ::cpp::Pointer<T> fromDynamic(Dynamic d) { return cpp::Pointer<T>(d); }
        };

        template<class T>
        struct ConversionTrait<::cpp::Struct<T>>
        {
            inline static Dynamic toDynamic(::cpp::Struct<T> v) { return v; }
            inline static ::cpp::Struct<T> fromDynamic(Dynamic d) { return cpp::Struct<T>(d); }
        };

        template<class T>
        struct ConversionTrait<::cpp::marshal::ValueReference<T>>
        {
            inline static Dynamic toDynamic(::cpp::marshal::ValueReference<T> v) { return v; }
            inline static ::cpp::marshal::ValueReference<T> fromDynamic(Dynamic d) { return ::cpp::marshal::ValueReference<T>{ ::cpp::marshal::ValueType<T>{ d } }; }
        };

        template<class T>
        struct ConversionTrait<::cpp::marshal::ValueType<T>>
        {
            inline static Dynamic toDynamic(::cpp::marshal::ValueType<T> v) { return ::cpp::marshal::ValueReference<T>{ v }; }
            inline static ::cpp::marshal::ValueType<T> fromDynamic(Dynamic d) { return ::cpp::marshal::ValueType<T>{ d }; }
        };

        template<class T>
        struct ConversionTrait<::cpp::marshal::PointerReference<T>>
        {
            inline static Dynamic toDynamic(::cpp::marshal::PointerReference<T> v) { return v; }
            inline static ::cpp::marshal::PointerReference<T> fromDynamic(Dynamic d) { return ::cpp::marshal::PointerReference<T>{ ::cpp::marshal::PointerType<T>{ d } }; }
        };

        template<class T>
        struct ConversionTrait<::cpp::marshal::PointerType<T>>
        {
            inline static Dynamic toDynamic(::cpp::marshal::PointerType<T> v) { return ::cpp::marshal::PointerReference<T>{ v }; }
            inline static ::cpp::marshal::PointerType<T> fromDynamic(Dynamic d) { return ::cpp::marshal::PointerType<T>{ d }; }
        };

        namespace unwrap
        {
            template<typename T>
            T fromDynamic(Dynamic value)
            {
                using traits = ConversionTrait<T>;

                return traits::fromDynamic(value);
            }
        }

        namespace wrap
        {
            template<typename T>
            Dynamic toDynamic(T value)
            {
                using traits = ConversionTrait<T>;

                return traits::toDynamic(value);
            }
        }

        template<bool void_return, class TReturn, class... TArgs>
        struct Invoker;

        template<class TReturn, class... TArgs>
        struct Invoker<true, TReturn, TArgs...>
        {
            template<size_t... I>
            static Dynamic call(hx::Callable<TReturn(TArgs...)> callable, const Array<Dynamic>& inArgs, index::Helper::index_sequence<I...>)
            {
                callable(invoker::unwrap::fromDynamic<TArgs>(inArgs[I]) ...);

                return null();
            }
        };

        template<class TReturn, class... TArgs>
        struct Invoker<false, TReturn, TArgs...>
        {
            template<size_t... I>
            static Dynamic call(hx::Callable<TReturn(TArgs...)> callable, const Array<Dynamic>& inArgs, index::Helper::index_sequence<I...>)
            {
                return invoker::wrap::toDynamic(callable(invoker::unwrap::fromDynamic<TArgs>(inArgs[I]) ...));
            }
        };

        inline Dynamic invoke(hx::Object* obj)
        {
            return obj->__Run(Array_obj<::Dynamic>::__new(0, 0));
        }

        template<class ...TArgs>
        inline Dynamic invoke(hx::Object* obj, const TArgs & ...args)
        {
            using unused = int[];

            auto arr = Array_obj<::Dynamic>::__new(0, sizeof...(args));

            (void)unused {
                0, (arr->push(invoker::wrap::toDynamic(args)), 0)...
            };

            return obj->__Run(arr);
        }
	}
}

template<class ...TArgs>
::Dynamic Dynamic::operator()(const TArgs & ...args)
{
    CheckFPtr();

    return hx::invoker::invoke(mPtr, args...);
}

template<class ...TArgs>
::Dynamic cpp::Variant::operator()(const TArgs & ...args)
{
    CheckFPtr();

    return hx::invoker::invoke(valObject, args...);
}

template<class TReturn, class... TArgs>
::Dynamic hx::AutoCallable_obj<TReturn(TArgs...)>::__Run(const Array<Dynamic>& inArgs)
{
    return invoker::Invoker<std::is_void<TReturn>::value, TReturn, TArgs...>::call(this, inArgs, invoker::index::Helper::index_sequence_for<TArgs...>());
}

#endif