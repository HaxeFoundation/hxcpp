
::foreach PARAMS:: ::if (ARG>=6)::
Dynamic Dynamic::NS::operator()(::DYNAMIC_ARG_LIST::)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::NS::__new(::ARG::)::DYNAMIC_ADDS::);
}

namespace cpp
{
::NS::Dynamic Variant::NS::operator()(::DYNAMIC_ARG_LIST::)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::NS::__new(::ARG::)::DYNAMIC_ADDS::);
}
}


::else::

namespace hx {

struct CMemberFunction::ARG:: : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction::ARG:: mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::NS::clsIdCMember::ARG:: };


   CMemberFunction::ARG::(const char *inName, hx::Object *inObj, MemberFunction::ARG:: inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction::ARG:: *other = dynamic_cast<const CMemberFunction::ARG:: *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return ::ARG::; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction::ARG::.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      ::if (ARG>0)::
      return mFunction(mThis.GetPtr(), ::ARR_LIST::);
      ::else::
      return mFunction(mThis.GetPtr());
      ::end::
   } 
   Dynamic __run(::DYNAMIC_ARG_LIST::) 
   { 
      ::if (ARG>0)::
      return mFunction(mThis.GetPtr(), ::ARG_LIST::);
      ::else::
      return mFunction(mThis.GetPtr());
      ::end::
   } 
}; 



struct CStaticFunction::ARG:: : public hx::Object 
{ 
   StaticFunction::ARG:: mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::NS::clsIdCStatic::ARG:: };


   CStaticFunction::ARG::(const char *inName,StaticFunction::ARG:: inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction::ARG:: *other = dynamic_cast<const CStaticFunction::ARG:: *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return ::ARG::; } 
   ::String __ToString() const{ return String(mName); } 
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(::ARR_LIST::);
   } 
   Dynamic __run(::DYNAMIC_ARG_LIST::) 
   { 
      return mFunction(::ARG_LIST::);
   } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction::ARG::(const char *inName,hx::Object *inObj, MemberFunction::ARG:: inFunc)
   { return new CMemberFunction::ARG::(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction::ARG::(const char *inName,StaticFunction::ARG:: inFunc)
   { return new CStaticFunction::ARG::(inName,inFunc); }

}

::end::
::end::

namespace hx
{


struct CMemberFunctionVar : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunctionVar mFunction;
   const char *mName;
   int N;


   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMemberVar };


   CMemberFunctionVar(const char *inName,hx::Object *inObj, MemberFunctionVar inFunction,int inN)
   {
      mThis = inObj;
      mFunction = inFunction;
      mName = inName;
      N = inN;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunctionVar *other = dynamic_cast<const CMemberFunctionVar *>(inRHS);
      if (!other)
         return -1;
      return (mFunction==other->mFunction && mName==other->mName && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }


   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return N; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunctionVar.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(mThis.GetPtr(), inArgs);
   } 
}; 



struct CStaticFunctionVar : public hx::Object 
{ 
   StaticFunctionVar mFunction;
   const char *mName;
   int N;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStaticVar };

   CStaticFunctionVar(const char *inName,StaticFunctionVar inFunction,int inN)
   {
      mFunction = inFunction;
      mName = inName;
      N = inN;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunctionVar *other = dynamic_cast<const CStaticFunctionVar *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction ? 0 : -1;
   }


   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return N; } 
   ::String __ToString() const { return String(mName); } 
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(inArgs);
   } 
}; 


Dynamic CreateMemberFunctionVar(const char *inName, hx::Object *inObj, MemberFunctionVar inFunc,int inN)
   { return new CMemberFunctionVar(inName, inObj,inFunc,inN); }

Dynamic CreateStaticFunctionVar(const char *inName,StaticFunctionVar inFunc,int inN)
   { return new CStaticFunctionVar(inName, inFunc,inN); }

}


