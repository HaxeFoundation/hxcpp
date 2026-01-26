#ifndef HX_FUNCTIONS_H
#define HX_FUNCTIONS_H

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

#if (HXCPP_API_LEVEL>=500)

    template<typename T1>
    bool IsNotNull(const T1& v1);

    template<class TReturn, class... TArgs>
    class HXCPP_EXTERN_CLASS_ATTRIBUTES Callable_obj;

    template<class TReturn, class... TArgs>
    class HXCPP_EXTERN_CLASS_ATTRIBUTES Callable_obj<TReturn(TArgs...)> : public hx::Object
    {
    public:
        HX_IS_INSTANCE_OF enum { _hx_ClassId = ::hx::clsIdClosure };

        int __GetType() const override final
        {
            return vtFunction;
        }

        int __ArgCount() const override final
        {
            return sizeof...(TArgs);
        }

        Dynamic __Run(const Array<Dynamic>& inArgs) override = 0;

        virtual TReturn HX_LOCAL_RUN(TArgs... args) = 0;
    };

    template<class TReturn, class... TArgs>
    class HXCPP_EXTERN_CLASS_ATTRIBUTES AutoCallable_obj;

    template<class TReturn, class... TArgs>
    class HXCPP_EXTERN_CLASS_ATTRIBUTES AutoCallable_obj<TReturn(TArgs...)> : public ::hx::Callable_obj<TReturn(TArgs...)>
    {
        Dynamic __Run(const Array<Dynamic>& inArgs) override final;
    };

    // ::hx::ObjectPtr overloads

    template<class TReturn, class... TArgs>
    class HXCPP_EXTERN_CLASS_ATTRIBUTES Callable;

    template<class TReturn, class... TArgs>
    class Callable<TReturn(TArgs...)> : public ObjectPtr<Callable_obj<TReturn(TArgs...)>>
    {
    public:
        using super = hx::ObjectPtr< Callable_obj<TReturn(TArgs...)> >;
        using OBJ_  = Callable_obj<TReturn(TArgs...)>;
        using Ptr   = Callable_obj<TReturn(TArgs...)>*;

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

        Callable(::cpp::Function<TReturn(TArgs...)> inFunction)
            : super(nullptr)
        {
            struct FunctionCallable final : public AutoCallable_obj<TReturn(TArgs...)>
            {
                ::cpp::Function<TReturn(TArgs...)> func;

                FunctionCallable(::cpp::Function<TReturn(TArgs...)> inFunc) : func(inFunc)
                {
                   HX_OBJ_WB_NEW_MARKED_OBJECT(this);
                }

                TReturn HX_LOCAL_RUN(TArgs... args) override
                {
                    return func(args...);
                }
            };

            super::mPtr = new FunctionCallable(inFunction);
        }

        template<class TOtherReturn, class... TOtherArgs>
        Callable(const Callable<TOtherReturn(TOtherArgs...)>& inCallable)
            : super(nullptr)
        {
            struct AdapterCallable final : public AutoCallable_obj<TReturn(TArgs...)>
            {
                Callable<TOtherReturn(TOtherArgs...)> wrapped;

                AdapterCallable(Callable<TOtherReturn(TOtherArgs...)> _wrapped) : wrapped(_wrapped)
                {
                   HX_OBJ_WB_NEW_MARKED_OBJECT(this);
                }

                TReturn HX_LOCAL_RUN(TArgs... args) override
                {
                    return wrapped(args...);
                }

                void* __GetHandle() const override
                {
                    return wrapped.GetPtr();
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

        template<class... TOtherArgs>
        Callable(const Callable<void(TOtherArgs...)>& inCallable)
            : super(nullptr)
        {
            struct AdapterCallable final : public AutoCallable_obj<TReturn(TArgs...)>
            {
                Callable<void(TOtherArgs...)> wrapped;

                AdapterCallable(Callable<void(TOtherArgs...)> _wrapped) : wrapped(_wrapped)
                {
                   HX_OBJ_WB_NEW_MARKED_OBJECT(this);
                }

                TReturn HX_LOCAL_RUN(TArgs... args) override
                {
                    wrapped(args...);

                    return null();
                }

                void* __GetHandle() const override
                {
                    return wrapped.GetPtr();
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
                if (::hx::IsNotNull(inDynamic) && inDynamic->__GetType() == vtFunction)
                {
                    struct DynamicCallable final : public AutoCallable_obj<TReturn(TArgs...)>
                    {
                        Dynamic wrapped;

                        DynamicCallable(Dynamic _wrapped) : wrapped(_wrapped)
                        {
                           HX_OBJ_WB_NEW_MARKED_OBJECT(this);
                        }

                        TReturn HX_LOCAL_RUN(TArgs... args) override
                        {
                            return wrapped(args...);
                        }

                        void* __GetHandle() const override
                        {
                            return wrapped.GetPtr();
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

    template<class... TArgs>
    class Callable<void(TArgs...)> : public ObjectPtr<Callable_obj<void(TArgs...)>>
    {
    public:
        using super = hx::ObjectPtr< Callable_obj<void(TArgs...)> >;
        using OBJ_ = Callable_obj<void(TArgs...)>;
        using Ptr = Callable_obj<void(TArgs...)>*;

        Callable()
            : super(nullptr) {}

        Callable(const null& inNull)
            : super(nullptr) {}

        Callable(Ptr inPtr) : super(inPtr) {}

        Callable(const hx::ObjectPtr<OBJ_>& inCallable)
            : super(inCallable) {}

        Callable(const Callable<void(TArgs...)>& inCallable)
            : super(inCallable.GetPtr()) {}

        Callable(const ::cpp::Variant& inVariant)
            : Callable(Dynamic(inVariant.asObject())) {}

        Callable(::cpp::Function<void(TArgs...)> inFunction)
            : super(nullptr)
        {
            struct FunctionCallable final : public AutoCallable_obj<void(TArgs... )>
            {
                ::cpp::Function<void(TArgs...)> func;

                FunctionCallable(::cpp::Function<void(TArgs...)> inFunc) : func(inFunc) {}

                void HX_LOCAL_RUN(TArgs... args) override
                {
                    func(args...);
                }
            };

            super::mPtr = new FunctionCallable(inFunction);
        }

        template<class TOtherReturn, class... TOtherArgs>
        Callable(const Callable<TOtherReturn(TOtherArgs...)>& inCallable)
            : super(nullptr)
        {
            struct AdapterCallable final : public AutoCallable_obj<void(TArgs...)>
            {
                Callable<TOtherReturn(TOtherArgs...)> wrapped;

                AdapterCallable(Callable<TOtherReturn(TOtherArgs...)> _wrapped) : wrapped(_wrapped)
                {
                   HX_OBJ_WB_NEW_MARKED_OBJECT(this);
                }

                void HX_LOCAL_RUN(TArgs... args) override
                {
                    wrapped(args...);
                }

                void* __GetHandle() const override
                {
                    return wrapped.GetPtr();
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
                if (::hx::IsNotNull(inDynamic) && inDynamic->__GetType() == vtFunction)
                {
                    struct DynamicCallable final : public AutoCallable_obj<void(TArgs...)>
                    {
                        Dynamic wrapped;

                        DynamicCallable(Dynamic _wrapped) : wrapped(_wrapped)
                        {
                           HX_OBJ_WB_NEW_MARKED_OBJECT(this);
                        }

                        void HX_LOCAL_RUN(TArgs... args) override
                        {
                            wrapped(args...);
                        }

                        void* __GetHandle() const override
                        {
                            return wrapped.GetPtr();
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
            }
        }

        void operator ()(TArgs... args)
        {
            if (nullptr == super::mPtr)
            {
                ::Dynamic::ThrowBadFunctionError();
            }

            super::mPtr->_hx_run(args...);
        }
    };
#endif
}

#endif
