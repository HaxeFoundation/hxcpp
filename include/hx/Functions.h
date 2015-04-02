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

}

#endif
