#pragma once

#include <hxcpp.h>

namespace hx
{
	struct Coroutine
	{
		static Dynamic suspend(Dynamic f, Dynamic cont)
		{
			struct SuspendCaller final : public hx::Object
			{
				Dynamic f;
				Dynamic cont;

				SuspendCaller(Dynamic inF, Dynamic inCont) : f(inF), cont(inCont)
				{
					HX_OBJ_WB_NEW_MARKED_OBJECT(this);
				}

				Dynamic HX_LOCAL_RUN()
				{
					return f(cont);
				}

				Dynamic __run(const Dynamic&, const Dynamic&) override
				{
					return HX_LOCAL_RUN();
				}

				Dynamic __Run(const Array<Dynamic>& args) override
				{
					return HX_LOCAL_RUN();
				}

				void __Mark(hx::MarkContext* __inCtx) override
				{
					HX_MARK_MEMBER(__inCtx);
				}

#ifdef HXCPP_VISIT_ALLOCS
				void __Visit(hx::VisitContext* __inCtx) override
				{
					HX_VISIT_MEMBER(__inCtx);
				}
#endif
			};

			return new SuspendCaller(f, cont);
		}
	};
}