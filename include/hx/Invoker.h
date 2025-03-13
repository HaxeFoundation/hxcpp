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

        // for some reason gcc gives us a "expected a type got std::remove_pointer<T>::type" error if I try and std::remove_pointer<T>::type as a template parameter.
        // but std::remove_pointer_t<T> from C++ 14 works? so just implement that ourselves...
        template< class T >
        using remove_pointer_t = typename std::remove_pointer<T>::type;

        template< class T >
        using remove_const_t = typename std::remove_const<T>::type;

        namespace unwrap
        {
            template<typename T>
            T __hx_struct(Dynamic value, std::true_type)
            {
                return value;
            }

            template<typename T>
            T __hx_struct(Dynamic value, std::false_type)
            {
                return ::cpp::Struct<T>(value);
            }

            template<typename T>
            T __hx_object_pointer(Dynamic value, std::true_type)
            {
                return value;
            }

            template<typename T>
            T __hx_object_pointer(Dynamic value, std::false_type)
            {
                return ::cpp::Pointer<remove_pointer_t<T>>(value);
            }

            template<typename T>
            T __hx_pointer(Dynamic value, std::true_type)
            {
                return __hx_object_pointer<T>(value, std::is_base_of<remove_pointer_t<T>, ::hx::Object>{});
            }

            template<typename T>
            T __hx_pointer(Dynamic value, std::false_type)
            {
                return __hx_struct<T>(value, std::is_constructible<Dynamic, T>{});
            }

            template<typename T>
            T fromDynamic(Dynamic value)
            {
                return __hx_pointer<T>(value, std::is_pointer<T>{});
            }
        }

        namespace wrap
        {
            template<typename T>
            Dynamic __hx_struct(T value, std::true_type)
            {
                return value;
            }

            template<typename T>
            Dynamic __hx_struct(T value, std::false_type)
            {
                return cpp::Struct<T>(value);
            }

            template<typename T>
            Dynamic __hx_object_pointer(T value, std::true_type)
            {
                return Dynamic(value);
            }

            template<typename T>
            Dynamic __hx_object_pointer_strip_const(T value, std::false_type)
            {
                return Dynamic(cpp::Pointer<remove_pointer_t<T>>(value));
            }

            template<typename T>
            Dynamic __hx_object_pointer_strip_const(T value, std::true_type)
            {
                return Dynamic(cpp::Pointer<remove_const_t<remove_pointer_t<T>>>(value));
            }

            template<typename T>
            Dynamic __hx_object_pointer(T value, std::false_type)
            {
                return __hx_object_pointer_strip_const(value, std::is_const<remove_pointer_t<T>>{});
            }

            template<typename T>
            Dynamic __hx_pointer(T value, std::true_type)
            {
                return __hx_object_pointer(value, std::is_base_of<remove_pointer_t<T>, ::hx::Object>{});
            }

            template<typename T>
            Dynamic __hx_pointer(T value, std::false_type)
            {
                return __hx_struct(value, std::is_constructible<Dynamic, T>{});
            }

            template<typename T>
            Dynamic toDynamic(T value)
            {
                return __hx_pointer(value, std::is_pointer<T>{});
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
::Dynamic hx::Callable_obj<TReturn(TArgs...)>::__Run(const Array<Dynamic>& inArgs)
{
    return invoker::Invoker<std::is_void<TReturn>::value, TReturn, TArgs...>::call(this, inArgs, invoker::index::Helper::index_sequence_for<TArgs...>());
}

#endif