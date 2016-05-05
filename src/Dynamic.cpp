#include <hxcpp.h>
#include <math.h>
#include <hxMath.h>
#include <stdio.h>

using namespace hx;



extern hx::Class __StringClass;
namespace hx
{

extern hx::Class hxEnumBase_obj__mClass;
extern hx::Class Object__mClass;


hx::Class __BoolClass;
hx::Class __IntClass;
hx::Class __FloatClass;
hx::Class __Int64Class;
hx::Class __PointerClass;
hx::Class __VoidClass;
#ifdef HXCPP_OBJC
hx::Class __ObjcClass;
#endif


hx::Class &GetBoolClass() { return __BoolClass; }
hx::Class &GetIntClass() { return __IntClass; }
hx::Class &GetFloatClass() { return __FloatClass; }
hx::Class &GetInt64Class() { return __Int64Class; }
hx::Class &GetPointerClass() { return __PointerClass; }
hx::Class &GetVoidClass() { return __VoidClass; }



// --- "Simple" Data Objects ---------------------------------------------------


Dynamic DynTrue;
Dynamic DynFalse;
Dynamic DynEmptyString;

class IntData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc, const char *inName="Int")
      { return hx::Object::operator new(inSize,inAlloc,inName); }
   IntData(int inValue=0) : mValue(inValue) {};

   hx::Class __GetClass() const { return __IntClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< IntData *>(inClass); }

   virtual int __GetType() const { return vtInt; }

   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return mValue; }
   cpp::Int64 __ToInt64() const { return mValue; }

   int __Compare(const hx::Object *inRHS) const
   {
      double diff = mValue - inRHS->__ToDouble();
      return diff < 0 ? -1 : diff==0 ? 0 : 1;
   }


   int mValue;
};


class BoolData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc,const char *inName="Bool")
      { return hx::Object::operator new(inSize,inAlloc,"Bool"); }
   BoolData(bool inValue=false) : mValue(inValue) {};

   hx::Class __GetClass() const { return __BoolClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< BoolData *>(inClass); }

   virtual int __GetType() const { return vtBool; }

   String __ToString() const { return mValue ? HX_CSTRING("true") : HX_CSTRING("false"); }
   String toString() { return mValue ? HX_CSTRING("true") : HX_CSTRING("false"); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return mValue; }

   int __Compare(const hx::Object *inRHS) const
   {
      double diff = (double)mValue - inRHS->__ToDouble();
      return diff < 0 ? -1 : diff==0 ? 0 : 1;
   }


   bool mValue;
};



class DoubleData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc,const char *inName="Float")
      { return hx::Object::operator new(inSize,inAlloc,inName); }
   DoubleData(double inValue=0) : mValue(inValue) {};

   hx::Class __GetClass() const { return __FloatClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< DoubleData *>(inClass); }

   virtual int __GetType() const { return vtFloat; }
   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return (int)mValue; }
   cpp::Int64 __ToInt64() const { return mValue; }

   int __Compare(const hx::Object *inRHS) const
   {
      double rval = inRHS->__ToDouble();
      if (rval==mValue)
         return 0;

      return mValue < rval ? -1 :  1;
   }


   double mValue;
};


class Int64Data : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc,const char *inName="Int64")
      { return hx::Object::operator new(inSize,inAlloc,inName); }
   Int64Data(cpp::Int64 inValue=0) : mValue(inValue) {};

   hx::Class __GetClass() const { return __Int64Class; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< DoubleData *>(inClass); }

   virtual int __GetType() const { return vtFloat; }
   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return (int)mValue; }
   cpp::Int64 __ToInt64() const { return mValue; }

   int __Compare(const hx::Object *inRHS) const
   {
      double rval = inRHS->__ToDouble();
      if (rval==mValue)
         return 0;

      return mValue < rval ? -1 :  1;
   }


   double mValue;
};



class PointerData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc,const char *inName="cpp.Pointer")
      { return hx::Object::operator new(inSize,inAlloc,inName); }

   PointerData(void *inValue) : mValue(inValue) {};

   hx::Class __GetClass() const { return __PointerClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< PointerData *>(inClass); }

   // k_cpp_pointer
   int __GetType() const { return vtAbstractBase + 2; }
   void * __GetHandle() const { return mValue; }
   String toString()
   {
      char buf[100];
      sprintf(buf,"Pointer(%p)", mValue);
      return String(buf);
   }
   String __ToString() const { return String(mValue); }

   int __Compare(const hx::Object *inRHS) const
   {
      void *r = inRHS==0 ? 0 : inRHS->__GetHandle();
      return mValue < r ? -1 : mValue==r ? 0 : 1;
   }


   void *mValue;
};


class StructData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjContainer,const char *inName="cpp.Struct")
      { return hx::Object::operator new(inSize,inAlloc,inName); }

   StructData(const void *inValue,int inLength, cpp::DynamicHandlerFunc inHandler)
   {
      mLength= inLength;
      mValue = InternalNew(inLength,false);
      memcpy(mValue, inValue, inLength);
      mHandler = inHandler;
   }

   hx::Class __GetClass() const { return __PointerClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< StructData *>(inClass); }

   // k_cpp_struct
   int __GetType() const { return vtAbstractBase + 3; }
   void * __GetHandle() const { return mValue; }
   String toString()
   {
      return __ToString();
   }
   String __ToString() const
   {
      String result;
      mHandler(cpp::dhoToString, mValue, &result );
      return result;
   }
   const char *__CStr() const
   {
      const char *result = "unknown";
      mHandler(cpp::dhoGetClassName, mValue, &result );
      return result;
   }

   int __Compare(const hx::Object *inRHS) const
   {
      if (!inRHS)
         return 1;

      int diff = __length() - inRHS->__length();
      if (diff==0)
         diff = __GetType() - inRHS->__GetType();
      if (diff==0)
         diff = memcmp( mValue, inRHS->__GetHandle(), mLength );
       
      if (diff<0) return -1;
      if (diff>0) return 1;
      return 0;
   }

   int __length() const { return mLength; }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_ARRAY(mValue);
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_ARRAY(mValue);
   }
   #endif

   int  mLength;
   void *mValue;
   cpp::DynamicHandlerFunc mHandler;
};





}


namespace cpp
{
// --- Pointer -------------------------------------------------

Dynamic CreateDynamicPointer(void *inValue) { return new hx::PointerData(inValue); }

// --- Struct -------------------------------------------------

Dynamic CreateDynamicStruct(const void *inValue, int inSize, DynamicHandlerFunc inFunc)

{
   return new hx::StructData(inValue,inSize,inFunc); }
}



// --- Dynamic -------------------------------------------------

Dynamic sConstDynamicInts[256+1];

static hx::Object *fromInt(int inVal)
{
   hx::Object *result = 0;
   if (inVal>=-1 && inVal<256)
   {
      int idx = inVal+1;
      result = sConstDynamicInts[idx].mPtr;
      if (!result)
         result = sConstDynamicInts[idx].mPtr = new (hx::NewObjConst)IntData(inVal);
   }
   else
      result = (hx::Object *)new IntData(inVal);
   return result;
}

Dynamic::Dynamic(bool inVal) : super( inVal ? hx::DynTrue.mPtr : hx::DynFalse.mPtr ) { }
Dynamic::Dynamic(int inVal)
{
   mPtr = fromInt(inVal);
}

Dynamic::Dynamic(short inVal)
{
   mPtr = fromInt(inVal);
}

Dynamic::Dynamic(unsigned int inVal)
{
   mPtr = fromInt(inVal);
}
Dynamic::Dynamic(unsigned short inVal)
{
   mPtr = fromInt(inVal);
}
Dynamic::Dynamic(unsigned char inVal)
{
   mPtr = fromInt(inVal);
}
Dynamic::Dynamic(signed char inVal)
{
   mPtr = fromInt(inVal);
}



Dynamic::Dynamic(double inVal)
{
   if ( (int)inVal==inVal && inVal>=-1 && inVal<256 )
   {
      int idx = inVal+1;
      mPtr = sConstDynamicInts[idx].mPtr;
      if (!mPtr)
         mPtr = sConstDynamicInts[idx].mPtr = new (hx::NewObjConst)IntData(inVal);
   }
   mPtr = (hx::Object *)new DoubleData(inVal);
}


Dynamic::Dynamic(cpp::Int64 inVal)
{
   if ( (int)inVal==inVal && inVal>=-1 && inVal<256 )
   {
      int idx = inVal+1;
      mPtr = sConstDynamicInts[idx].mPtr;
      if (!mPtr)
         mPtr = sConstDynamicInts[idx].mPtr = new (hx::NewObjConst)IntData(inVal);
   }
   mPtr = (hx::Object *)new Int64Data(inVal);
}


Dynamic::Dynamic(cpp::UInt64 inVal)
{
   if ( (int)inVal==inVal && inVal<256 )
   {
      int idx = inVal+1;
      mPtr = sConstDynamicInts[idx].mPtr;
      if (!mPtr)
         mPtr = sConstDynamicInts[idx].mPtr = new (hx::NewObjConst)IntData(inVal);
   }
   mPtr = (hx::Object *)new Int64Data(inVal);
}





Dynamic::Dynamic(float inVal)
{
   mPtr = Dynamic( (double) inVal ).mPtr;
}

Dynamic::Dynamic(const cpp::CppInt32__ &inVal) :
  super(  Dynamic(inVal.mValue).mPtr ) { }

Dynamic::Dynamic(const String &inVal) :
  super( inVal.__s ? (inVal.length==0 ? DynEmptyString.mPtr : inVal.__ToObject() ) : 0 ) { }

Dynamic::Dynamic(const HX_CHAR *inVal) :
  super( inVal ? String(inVal).__ToObject() : 0 ) { }


Dynamic Dynamic::operator+(const Dynamic &inRHS) const
{
   int t1 = mPtr ? mPtr->__GetType() : vtNull;
   int t2 = inRHS.mPtr ? inRHS.mPtr->__GetType() : vtNull;

   if ( (t1==vtInt || t1==vtFloat)  &&  (t2==vtInt || t2==vtFloat) )
   {
      return mPtr->__ToDouble() + inRHS.mPtr->__ToDouble();
   }
   if (!mPtr)
      return String() + inRHS;
   if (!inRHS.mPtr)
      return *this + String();

   return const_cast<hx::Object*>(mPtr)->toString() + const_cast<Dynamic&>(inRHS)->toString();
}

#define DYN_OP_ADD(TYPE) \
Dynamic Dynamic::operator+(const TYPE &i) const \
{ \
   int t = mPtr ? mPtr->__GetType() : vtNull; \
   if (t==vtString) \
      return Cast<String>() + String(i); \
   return Cast<double>() + i; \
}

DYN_OP_ADD(double)
DYN_OP_ADD(float)
DYN_OP_ADD(int)
DYN_OP_ADD(unsigned int)
DYN_OP_ADD(short)
DYN_OP_ADD(unsigned short)
DYN_OP_ADD(signed char)
DYN_OP_ADD(unsigned char)
DYN_OP_ADD(cpp::Int64)
DYN_OP_ADD(cpp::UInt64)

Dynamic Dynamic::operator+(const cpp::Variant &v) const
{
   int t = mPtr ? mPtr->__GetType() : vtNull;
   if (t==vtString || v.type == cpp::Variant::typeString)
      return Cast<String>() + v.asString();
   return Cast<double>() + v.asDouble();
}



double Dynamic::operator%(const Dynamic &inRHS) const
{
   if (mPtr->__GetType()==vtInt && inRHS.mPtr->__GetType()==vtInt)
      return mPtr->__ToInt() % inRHS->__ToInt();
   double lhs = mPtr->__ToDouble();
   double rhs = inRHS->__ToDouble();
   int even = (int)(lhs/rhs);
   double remain = lhs - even * rhs;
   if (remain<0) remain += fabs(rhs);
   return remain;
}

hx::IndexRef Dynamic::operator[](int inIndex)
{
   return hx::IndexRef(mPtr,inIndex);
}


void Dynamic::ThrowBadFunctionError()
{
   #ifdef HXCPP_DEBUGGER
   NullReference("Function", true);
   #endif
   hx::Throw( HX_NULL_FUNCTION_POINTER );
}

#include <hx/DynamicImpl.h>

namespace cpp
{
CppInt32__::CppInt32__(const Dynamic &inD) : mValue(inD->__ToInt()) { }
}

namespace hx {
null BadCast()
{
   hx::Throw(HX_INVALID_CAST);
   return null();
}
}


static bool NoCast(hx::Object *) { return false; }
static bool IsFloat(hx::Object *inPtr)
{
   return inPtr && (TCanCast<IntData>(inPtr) || TCanCast<DoubleData>(inPtr) || TCanCast<Int64Data>(inPtr) );
}
static bool IsInt64(hx::Object *inPtr)
{
   return inPtr && (TCanCast<Int64Data>(inPtr) || TCanCast<IntData>(inPtr));
}

static bool IsPointer(hx::Object *inPtr)
{
   return inPtr && inPtr->__GetType() >= vtAbstractBase;
}

static bool IsInt(hx::Object *inPtr)
{
   if (!inPtr)
      return false;
   if (TCanCast<IntData>(inPtr))
      return true;
   DoubleData *d = dynamic_cast<DoubleData *>(inPtr);
   if (!d)
   {
      Int64Data *i64 = dynamic_cast<Int64Data *>(inPtr);
      if (i64)
      {
         int val = i64->mValue;
         return val==i64->mValue;
      }
      return false;
   }
   double val = d->__ToDouble();
   return ((int)val == val);
}


static void sMarkStatics(HX_MARK_PARAMS) {
	HX_MARK_MEMBER(__VoidClass);
	HX_MARK_MEMBER(__BoolClass);
	HX_MARK_MEMBER(__IntClass);
	HX_MARK_MEMBER(__FloatClass);
	HX_MARK_MEMBER(__Int64Class);
	HX_MARK_MEMBER(__PointerClass);
	HX_MARK_MEMBER(__StringClass);
	HX_MARK_MEMBER(Object__mClass);
	HX_MARK_MEMBER(ArrayBase::__mClass);
	HX_MARK_MEMBER(Math_obj::__mClass);
	HX_MARK_MEMBER(Anon_obj::__mClass);
	HX_MARK_MEMBER(hx::hxEnumBase_obj__mClass);
	HX_MARK_MEMBER(hx::DynEmptyString);
#ifdef HXCPP_OBJC
	HX_MARK_MEMBER(__ObjcClass);
#endif
};



#ifdef HXCPP_VISIT_ALLOCS
static void sVisitStatics(HX_VISIT_PARAMS) {
	HX_VISIT_MEMBER(__VoidClass);
	HX_VISIT_MEMBER(__BoolClass);
	HX_VISIT_MEMBER(__IntClass);
	HX_VISIT_MEMBER(__FloatClass);
	HX_VISIT_MEMBER(__Int64Class);
	HX_VISIT_MEMBER(__PointerClass);
	HX_VISIT_MEMBER(__StringClass);
	HX_VISIT_MEMBER(Object__mClass);
	HX_VISIT_MEMBER(ArrayBase::__mClass);
	HX_VISIT_MEMBER(Math_obj::__mClass);
	HX_VISIT_MEMBER(Anon_obj::__mClass);
	HX_VISIT_MEMBER(hx::hxEnumBase_obj__mClass);
	HX_VISIT_MEMBER(hx::DynEmptyString);
#ifdef HXCPP_OBJC
	HX_VISIT_MEMBER(__ObjcClass);
#endif
};

#endif


void Dynamic::__boot()
{
   Static(__VoidClass) = hx::_hx_RegisterClass(HX_CSTRING("Void"),NoCast,sNone,sNone,0,0,0, 0, sMarkStatics
      #ifdef HXCPP_VISIT_ALLOCS
      ,sVisitStatics
      #endif
   );
   Static(__BoolClass) = hx::_hx_RegisterClass(HX_CSTRING("Bool"),TCanCast<BoolData>,sNone,sNone, 0,0, 0);
   Static(__IntClass) = hx::_hx_RegisterClass(HX_CSTRING("Int"),IsInt,sNone,sNone,0,0, 0 );
   Static(__FloatClass) = hx::_hx_RegisterClass(HX_CSTRING("Float"),IsFloat,sNone,sNone, 0,0,&__IntClass );
   Static(__Int64Class) = hx::_hx_RegisterClass(HX_CSTRING("cpp::Int64"),IsInt64,sNone,sNone, 0,0,&__IntClass );
   Static(__PointerClass) = hx::_hx_RegisterClass(HX_CSTRING("cpp::Pointer"),IsPointer,sNone,sNone, 0,0,&__PointerClass );
   DynTrue = Dynamic( new (hx::NewObjConst) hx::BoolData(true) );
   DynFalse = Dynamic( new (hx::NewObjConst) hx::BoolData(false) );
   DynEmptyString = Dynamic(HX_CSTRING("").__ToObject());
#ifdef HXCPP_OBJC
   Static(__ObjcClass) = hx::_hx_RegisterClass(HX_CSTRING("objc::BoxedType"),IsPointer,sNone,sNone, 0,0,&__ObjcClass );
#endif
}


