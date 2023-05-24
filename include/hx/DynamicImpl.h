//   ##  ##  ##   ##   ####   ##  ## ## ##  ##  ####    ##
//   ##  ##  ## ##  ## ##  ## ### ## ## ### ## ##       ##
//    ## ## ##  ###### ###### ###### ## ###### ## ###   ##
//    ## ## ##  ##  ## ## ##  ## ### ## ## ### ##  ##     
//     ## ##    ##  ## ##  ## ##  ## ## ##  ##  ####    ##

// DO NOT EDIT
// This file is generated from the .tpl file

 

namespace hx {

struct CMemberFunction0 : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction0 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMember0 };


   CMemberFunction0(const char *inName, hx::Object *inObj, MemberFunction0 inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction0 *other = dynamic_cast<const CMemberFunction0 *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 0; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction0.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
}; 



struct CStaticFunction0 : public hx::Object 
{ 
   StaticFunction0 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStatic0 };


   CStaticFunction0(const char *inName,StaticFunction0 inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction0 *other = dynamic_cast<const CStaticFunction0 *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 0; } 
   ::String __ToString() const{ return String(mName); } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction0(const char *inName,hx::Object *inObj, MemberFunction0 inFunc)
   { return new CMemberFunction0(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction0(const char *inName,StaticFunction0 inFunc)
   { return new CStaticFunction0(inName,inFunc); }

}


 

namespace hx {

struct CMemberFunction1 : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction1 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMember1 };


   CMemberFunction1(const char *inName, hx::Object *inObj, MemberFunction1 inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction1 *other = dynamic_cast<const CMemberFunction1 *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 1; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction1.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
}; 



struct CStaticFunction1 : public hx::Object 
{ 
   StaticFunction1 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStatic1 };


   CStaticFunction1(const char *inName,StaticFunction1 inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction1 *other = dynamic_cast<const CStaticFunction1 *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 1; } 
   ::String __ToString() const{ return String(mName); } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction1(const char *inName,hx::Object *inObj, MemberFunction1 inFunc)
   { return new CMemberFunction1(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction1(const char *inName,StaticFunction1 inFunc)
   { return new CStaticFunction1(inName,inFunc); }

}


 

namespace hx {

struct CMemberFunction2 : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction2 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMember2 };


   CMemberFunction2(const char *inName, hx::Object *inObj, MemberFunction2 inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction2 *other = dynamic_cast<const CMemberFunction2 *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 2; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction2.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
}; 



struct CStaticFunction2 : public hx::Object 
{ 
   StaticFunction2 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStatic2 };


   CStaticFunction2(const char *inName,StaticFunction2 inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction2 *other = dynamic_cast<const CStaticFunction2 *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 2; } 
   ::String __ToString() const{ return String(mName); } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction2(const char *inName,hx::Object *inObj, MemberFunction2 inFunc)
   { return new CMemberFunction2(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction2(const char *inName,StaticFunction2 inFunc)
   { return new CStaticFunction2(inName,inFunc); }

}


 

namespace hx {

struct CMemberFunction3 : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction3 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMember3 };


   CMemberFunction3(const char *inName, hx::Object *inObj, MemberFunction3 inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction3 *other = dynamic_cast<const CMemberFunction3 *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 3; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction3.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
}; 



struct CStaticFunction3 : public hx::Object 
{ 
   StaticFunction3 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStatic3 };


   CStaticFunction3(const char *inName,StaticFunction3 inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction3 *other = dynamic_cast<const CStaticFunction3 *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 3; } 
   ::String __ToString() const{ return String(mName); } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction3(const char *inName,hx::Object *inObj, MemberFunction3 inFunc)
   { return new CMemberFunction3(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction3(const char *inName,StaticFunction3 inFunc)
   { return new CStaticFunction3(inName,inFunc); }

}


 

namespace hx {

struct CMemberFunction4 : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction4 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMember4 };


   CMemberFunction4(const char *inName, hx::Object *inObj, MemberFunction4 inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction4 *other = dynamic_cast<const CMemberFunction4 *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 4; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction4.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
}; 



struct CStaticFunction4 : public hx::Object 
{ 
   StaticFunction4 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStatic4 };


   CStaticFunction4(const char *inName,StaticFunction4 inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction4 *other = dynamic_cast<const CStaticFunction4 *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 4; } 
   ::String __ToString() const{ return String(mName); } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction4(const char *inName,hx::Object *inObj, MemberFunction4 inFunc)
   { return new CMemberFunction4(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction4(const char *inName,StaticFunction4 inFunc)
   { return new CStaticFunction4(inName,inFunc); }

}


 

namespace hx {

struct CMemberFunction5 : public hx::Object 
{ 
   hx::ObjectPtr<Object> mThis; 
   MemberFunction5 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCMember5 };


   CMemberFunction5(const char *inName, hx::Object *inObj, MemberFunction5 inFunction)
   {
      mName = inName;
      mThis = inObj;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CMemberFunction5 *other = dynamic_cast<const CMemberFunction5 *>(inRHS);
      if (!other)
         return -1;
      return (mName==other->mName && mFunction==other->mFunction && mThis.GetPtr()==other->mThis.GetPtr())? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 5; } 
   ::String __ToString() const{ return String(mName); } 
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mThis,"CMemberFunction5.this"); } 
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); } 
   #endif
   void *__GetHandle() const { return mThis.GetPtr(); } 
}; 



struct CStaticFunction5 : public hx::Object 
{ 
   StaticFunction5 mFunction;
   const char *mName;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdCStatic5 };


   CStaticFunction5(const char *inName,StaticFunction5 inFunction)
   {
      mName = inName;
      mFunction = inFunction;
   }
   int __Compare(const hx::Object *inRHS) const
   {
      const CStaticFunction5 *other = dynamic_cast<const CStaticFunction5 *>(inRHS);
      if (!other)
         return -1;
      return mName==other->mName && mFunction==other->mFunction && mName==other->mName ? 0 : -1;
   }

   int __GetType() const { return vtFunction; } 
   int __ArgCount() const { return 5; } 
   ::String __ToString() const{ return String(mName); } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction5(const char *inName,hx::Object *inObj, MemberFunction5 inFunc)
   { return new CMemberFunction5(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction5(const char *inName,StaticFunction5 inFunc)
   { return new CStaticFunction5(inName,inFunc); }

}


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
}; 


Dynamic CreateMemberFunctionVar(const char *inName, hx::Object *inObj, MemberFunctionVar inFunc,int inN)
   { return new CMemberFunctionVar(inName, inObj,inFunc,inN); }

Dynamic CreateStaticFunctionVar(const char *inName,StaticFunctionVar inFunc,int inN)
   { return new CStaticFunctionVar(inName, inFunc,inN); }

}


