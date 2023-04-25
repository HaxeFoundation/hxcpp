#ifndef HX_FUNCTIONS_H
#define HX_FUNCTIONS_H
#include <hxcpp.h>

namespace hx
{
   struct HXCPP_EXTERN_CLASS_ATTRIBUTES LocalFunc : public hx::Object
   {
      int __GetType() const { return vtFunction; }
      inline void DoMarkThis(hx::MarkContext *__inCtx) { }
#ifdef HXCPP_VISIT_ALLOCS
      inline void DoVisitThis(hx::VisitContext *__inCtx) { }
#endif
   };

   struct HXCPP_EXTERN_CLASS_ATTRIBUTES LocalThisFunc : public LocalFunc
   {
      Dynamic __this;
		void __SetThis(Dynamic inThis) { __this = inThis; }
      inline void DoMarkThis(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(__this); }
#ifdef HXCPP_VISIT_ALLOCS
      inline void DoVisitThis(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(__this); }
#endif
   };

    template<class TReturn, class... TArgs>
    struct HXCPP_EXTERN_CLASS_ATTRIBUTES Callable_obj;

    template<class TReturn, class... TArgs>
    struct HXCPP_EXTERN_CLASS_ATTRIBUTES Callable_obj<TReturn(TArgs...)> : public hx::Object
    {
        int __GetType() const override
        {
            return vtFunction;
        }

        int __ArgCount() const override
        {
            return sizeof...(TArgs);
        }

        virtual TReturn _hx_run(TArgs... args) = 0;
    };

    template<class TReturn, class... TArgs>
    class HXCPP_EXTERN_CLASS_ATTRIBUTES Callable;

    template<class TReturn, class... TArgs>
    class Callable<TReturn(TArgs...)> : public ObjectPtr<Callable_obj<TReturn(TArgs...)>>
    {
    private:
        using super = hx::ObjectPtr< Callable_obj<TReturn(TArgs...)> >;
        using OBJ_  = Callable_obj<TReturn(TArgs...)>;
        using Ptr   = Callable_obj<TReturn(TArgs...)>*;

    public:
        Callable()
            : super(nullptr) {}

        Callable(const null& inNull)
            : super(nullptr) {}

        Callable(Ptr inPtr) : super(inPtr) {}

        Callable(const hx::ObjectPtr<OBJ_>& inCallable)
            : super(inCallable) {}

        Callable(const Callable<TReturn(TArgs...)>& inCallable)
            : super(inCallable.GetPtr()) {}

        Callable(const ::cpp::Variant& inVariant)
            : Callable(Dynamic(inVariant.asObject())) {}

        template<class TOtherReturn, class... TOtherArgs>
        Callable(const Callable<TOtherReturn(TOtherArgs...)>& inCallable)
            : super(nullptr)
        {
            struct AdapterCallable final : public Callable_obj<TReturn(TArgs...)>
            {
                Callable<TOtherReturn(TOtherArgs...)> wrapped;

                AdapterCallable(Callable<TOtherReturn(TOtherArgs...)> _wrapped) : wrapped(_wrapped) {}

                TReturn _hx_run(TArgs... args) override
                {
                    if constexpr (std::is_void<TReturn>())
                    {
                        wrapped(args...);
                    }
                    else
                    {
                        if constexpr (std::is_void<TOtherReturn>())
                        {
                            wrapped(args...);

                            return null();
                        }
                        else
                        {
                            return wrapped(args...);
                        }
                    }
                }

                inline void __Mark(hx::MarkContext* __inCtx) override
                {
                    HX_MARK_MEMBER(wrapped);
                }
#ifdef HXCPP_VISIT_ALLOCS
                inline void __Visit(hx::VisitContext* __inCtx) override
                {
                    HX_VISIT_MEMBER(wrapped);
                }
#endif
            };

            super::mPtr = new AdapterCallable(inCallable);
        }

        Callable(const Dynamic& inDynamic)
            : super(nullptr)
        {
            auto casted = dynamic_cast<Ptr>(inDynamic.GetPtr());
            if (nullptr != casted)
            {
                super::mPtr = casted;
            }
            else
            {
                if (inDynamic->__GetType() == vtFunction)
                {
                    struct DynamicCallable final : public Callable_obj<TReturn(TArgs...)>
                    {
                        Dynamic wrapped;

                        DynamicCallable(Dynamic _wrapped) : wrapped(_wrapped) {}

                        TReturn _hx_run(TArgs... args) override
                        {
                            if constexpr (std::is_void<TReturn>())
                            {
                                wrapped(args...);
                            }
                            else
                            {
                                return wrapped(args...);
                            }
                        }

                        Dynamic __Run(const Array<Dynamic>& inArgs) override
                        {
                            return wrapped(inArgs);
                        }

                        inline void __Mark(hx::MarkContext* __inCtx) override
                        {
                            HX_MARK_MEMBER(wrapped);
                        }
#ifdef HXCPP_VISIT_ALLOCS
                        inline void __Visit(hx::VisitContext* __inCtx) override
                        {
                            HX_VISIT_MEMBER(wrapped);
                        }
#endif
                    };

                    super::mPtr = new DynamicCallable(inDynamic);
                }
                else
                {
                    ::hx::Throw(HX_CSTRING("Dynamic is not a function"));
                }
            }
        }

        TReturn operator ()(TArgs... args)
        {
            if (nullptr == super::mPtr)
            {
                ::Dynamic::ThrowBadFunctionError();
            }

            return super::mPtr->_hx_run(args...);
        }
    };
}

#endif
