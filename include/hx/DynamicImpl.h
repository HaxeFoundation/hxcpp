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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      
      return mFunction(mThis.GetPtr());
      
   } 
   Dynamic __run() 
   { 
      
      return mFunction(mThis.GetPtr());
      
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction();
   } 
   Dynamic __run() 
   { 
      return mFunction();
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      
      return mFunction(mThis.GetPtr(), inArgs[0]);
      
   } 
   Dynamic __run(const Dynamic &inArg0) 
   { 
      
      return mFunction(mThis.GetPtr(), inArg0);
      
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(inArgs[0]);
   } 
   Dynamic __run(const Dynamic &inArg0) 
   { 
      return mFunction(inArg0);
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      
      return mFunction(mThis.GetPtr(), inArgs[0],inArgs[1]);
      
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1) 
   { 
      
      return mFunction(mThis.GetPtr(), inArg0,inArg1);
      
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(inArgs[0],inArgs[1]);
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1) 
   { 
      return mFunction(inArg0,inArg1);
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      
      return mFunction(mThis.GetPtr(), inArgs[0],inArgs[1],inArgs[2]);
      
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2) 
   { 
      
      return mFunction(mThis.GetPtr(), inArg0,inArg1,inArg2);
      
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(inArgs[0],inArgs[1],inArgs[2]);
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2) 
   { 
      return mFunction(inArg0,inArg1,inArg2);
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      
      return mFunction(mThis.GetPtr(), inArgs[0],inArgs[1],inArgs[2],inArgs[3]);
      
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3) 
   { 
      
      return mFunction(mThis.GetPtr(), inArg0,inArg1,inArg2,inArg3);
      
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(inArgs[0],inArgs[1],inArgs[2],inArgs[3]);
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3) 
   { 
      return mFunction(inArg0,inArg1,inArg2,inArg3);
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      
      return mFunction(mThis.GetPtr(), inArgs[0],inArgs[1],inArgs[2],inArgs[3],inArgs[4]);
      
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4) 
   { 
      
      return mFunction(mThis.GetPtr(), inArg0,inArg1,inArg2,inArg3,inArg4);
      
   } 
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
   Dynamic __Run(const Array<Dynamic> &inArgs) 
   { 
      return mFunction(inArgs[0],inArgs[1],inArgs[2],inArgs[3],inArgs[4]);
   } 
   Dynamic __run(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4) 
   { 
      return mFunction(inArg0,inArg1,inArg2,inArg3,inArg4);
   } 
}; 


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction5(const char *inName,hx::Object *inObj, MemberFunction5 inFunc)
   { return new CMemberFunction5(inName,inObj,inFunc); }

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction5(const char *inName,StaticFunction5 inFunc)
   { return new CStaticFunction5(inName,inFunc); }

}


 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(6)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(6)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(7)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(7)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(8)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(8)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(9)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(9)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(10)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(10)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(11)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(11)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(12)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(12)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(13)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(13)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(14)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(14)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(15)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(15)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(16)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(16)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(17)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(17)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(18)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(18)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(19)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(19)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(20)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(20)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(21)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(21)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(22)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(22)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(23)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(23)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22,const Dynamic &inArg23)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(24)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22)->init(23,inArg23));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22,const Dynamic &inArg23)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(24)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22)->init(23,inArg23));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22,const Dynamic &inArg23,const Dynamic &inArg24)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(25)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22)->init(23,inArg23)->init(24,inArg24));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22,const Dynamic &inArg23,const Dynamic &inArg24)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(25)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22)->init(23,inArg23)->init(24,inArg24));
}
}



 
Dynamic Dynamic::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22,const Dynamic &inArg23,const Dynamic &inArg24,const Dynamic &inArg25)
{
   CheckFPtr();
   return mPtr->__Run(Array_obj<Dynamic>::__new(26)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22)->init(23,inArg23)->init(24,inArg24)->init(25,inArg25));
}

namespace cpp
{
::Dynamic Variant::operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4,const Dynamic &inArg5,const Dynamic &inArg6,const Dynamic &inArg7,const Dynamic &inArg8,const Dynamic &inArg9,const Dynamic &inArg10,const Dynamic &inArg11,const Dynamic &inArg12,const Dynamic &inArg13,const Dynamic &inArg14,const Dynamic &inArg15,const Dynamic &inArg16,const Dynamic &inArg17,const Dynamic &inArg18,const Dynamic &inArg19,const Dynamic &inArg20,const Dynamic &inArg21,const Dynamic &inArg22,const Dynamic &inArg23,const Dynamic &inArg24,const Dynamic &inArg25)
{
   if (isNull()) Dynamic::ThrowBadFunctionError();
   return valObject->__Run(Array_obj<Dynamic>::__new(26)->init(0,inArg0)->init(1,inArg1)->init(2,inArg2)->init(3,inArg3)->init(4,inArg4)->init(5,inArg5)->init(6,inArg6)->init(7,inArg7)->init(8,inArg8)->init(9,inArg9)->init(10,inArg10)->init(11,inArg11)->init(12,inArg12)->init(13,inArg13)->init(14,inArg14)->init(15,inArg15)->init(16,inArg16)->init(17,inArg17)->init(18,inArg18)->init(19,inArg19)->init(20,inArg20)->init(21,inArg21)->init(22,inArg22)->init(23,inArg23)->init(24,inArg24)->init(25,inArg25));
}
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


