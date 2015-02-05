#include <hxcpp.h>
#include <math.h>
#include <hxMath.h>
#include <stdio.h>

using namespace hx;



extern Class __StringClass;
namespace hx
{

extern Class hxEnumBase_obj__mClass;
extern Class Object__mClass;


Class __BoolClass;
Class __IntClass;
Class __FloatClass;
Class __PointerClass;
Class __VoidClass;


Class &GetBoolClass() { return __BoolClass; }
Class &GetIntClass() { return __IntClass; }
Class &GetFloatClass() { return __FloatClass; }
Class &GetPointerClass() { return __PointerClass; }
Class &GetVoidClass() { return __VoidClass; }



// --- "Simple" Data Objects ---------------------------------------------------


Dynamic DynTrue;
Dynamic DynFalse;
Dynamic DynEmptyString;

class IntData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc)
      { return hx::Object::operator new(inSize,inAlloc); }
   IntData(int inValue=0) : mValue(inValue) {};

   Class __GetClass() const { return __IntClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< IntData *>(inClass); }

   virtual int __GetType() const { return vtInt; }

   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return mValue; }

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
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc)
      { return hx::Object::operator new(inSize,inAlloc); }
   BoolData(bool inValue=false) : mValue(inValue) {};

   Class __GetClass() const { return __BoolClass; }
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
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc)
      { return hx::Object::operator new(inSize,inAlloc); }
   DoubleData(double inValue=0) : mValue(inValue) {};

   Class __GetClass() const { return __FloatClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< DoubleData *>(inClass); }

   virtual int __GetType() const { return vtFloat; }
   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return (int)mValue; }

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
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc)
      { return hx::Object::operator new(inSize,inAlloc); }

   PointerData(void *inValue) : mValue(inValue) {};

   Class __GetClass() const { return __PointerClass; }
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
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc)
      { return hx::Object::operator new(inSize,inAlloc); }

   StructData(const void *inValue,int inLength)
   {
      mLength= inLength;
      mValue = InternalNew(inLength,false);
      memcpy(mValue, inValue, inLength);
   }

   Class __GetClass() const { return __PointerClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< StructData *>(inClass); }

   // k_cpp_struct
   int __GetType() const { return vtAbstractBase + 3; }
   void * __GetHandle() const { return mValue; }
   String toString()
   {
      char buf[100];
      sprintf(buf,"Struct(x%d)", mLength);
      return String(buf);
   }
   String __ToString() const { return String(mValue); }

   int __Compare(const hx::Object *inRHS) const
   {
      void *r = inRHS==0 ? 0 : inRHS->__GetHandle();
      return mValue < r ? -1 : mValue==r ? 0 : 1;
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
};





}


namespace cpp
{
// --- Pointer -------------------------------------------------

Dynamic CreateDynamicPointer(void *inValue) { return new hx::PointerData(inValue); }

// --- Struct -------------------------------------------------

Dynamic CreateDynamicStruct(const void *inValue, int inLength) { return new hx::StructData(inValue,inLength); }

}



// --- Dynamic -------------------------------------------------

Dynamic sConstDynamicInts[256+1];


Dynamic::Dynamic(bool inVal) : super( inVal ? hx::DynTrue.mPtr : hx::DynFalse.mPtr ) { }
Dynamic::Dynamic(int inVal)
{
   if (inVal>=-1 && inVal<256)
   {
      int idx = inVal+1;
      mPtr = sConstDynamicInts[idx].mPtr;
      if (!mPtr)
         mPtr = sConstDynamicInts[idx].mPtr = new (hx::NewObjConst)IntData(inVal);
   }
   else
      mPtr = (hx::Object *)new IntData(inVal);
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

Dynamic Dynamic::operator+(const int &i) const
{
   int t = mPtr ? mPtr->__GetType() : vtNull;
   if (t==vtString)
      return Cast<String>() + String(i);
   return Cast<double>() + i;
}

Dynamic Dynamic::operator+(const double &d) const
{
   int t = mPtr ? mPtr->__GetType() : vtNull;
   if (t==vtString)
      return Cast<String>() + String(d);
   return Cast<double>() + d;
}


Dynamic Dynamic::operator+(const float &f) const
{
   int t = mPtr ? mPtr->__GetType() : vtNull;
   if (t==vtString)
      return Cast<String>() + String(f);
   return Cast<double>() + f;
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
   return inPtr && (TCanCast<IntData>(inPtr) || TCanCast<DoubleData>(inPtr));
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
      return false;
   double val = d->__ToDouble();
   return ((int)val == val);
}


static void sMarkStatics(HX_MARK_PARAMS) {
	HX_MARK_MEMBER(__VoidClass);
	HX_MARK_MEMBER(__BoolClass);
	HX_MARK_MEMBER(__IntClass);
	HX_MARK_MEMBER(__FloatClass);
	HX_MARK_MEMBER(__PointerClass);
	HX_MARK_MEMBER(__StringClass);
	HX_MARK_MEMBER(Object__mClass);
	HX_MARK_MEMBER(ArrayBase::__mClass);
	HX_MARK_MEMBER(Math_obj::__mClass);
	HX_MARK_MEMBER(Anon_obj::__mClass);
	HX_MARK_MEMBER(hx::hxEnumBase_obj__mClass);
	HX_MARK_MEMBER(hx::DynEmptyString);
};



#ifdef HXCPP_VISIT_ALLOCS
static void sVisitStatics(HX_VISIT_PARAMS) {
	HX_VISIT_MEMBER(__VoidClass);
	HX_VISIT_MEMBER(__BoolClass);
	HX_VISIT_MEMBER(__IntClass);
	HX_VISIT_MEMBER(__FloatClass);
	HX_VISIT_MEMBER(__PointerClass);
	HX_VISIT_MEMBER(__StringClass);
	HX_VISIT_MEMBER(Object__mClass);
	HX_VISIT_MEMBER(ArrayBase::__mClass);
	HX_VISIT_MEMBER(Math_obj::__mClass);
	HX_VISIT_MEMBER(Anon_obj::__mClass);
	HX_VISIT_MEMBER(hx::hxEnumBase_obj__mClass);
	HX_VISIT_MEMBER(hx::DynEmptyString);
};

#endif


void Dynamic::__boot()
{
   Static(__VoidClass) = hx::RegisterClass(HX_CSTRING("Void"),NoCast,sNone,sNone,0,0,0, 0, sMarkStatics
      #ifdef HXCPP_VISIT_ALLOCS
      ,sVisitStatics
      #endif
   );
   Static(__BoolClass) = hx::RegisterClass(HX_CSTRING("Bool"),TCanCast<BoolData>,sNone,sNone, 0,0, 0);
   Static(__IntClass) = hx::RegisterClass(HX_CSTRING("Int"),IsInt,sNone,sNone,0,0, 0 );
   Static(__FloatClass) = hx::RegisterClass(HX_CSTRING("Float"),IsFloat,sNone,sNone, 0,0,&__IntClass );
   Static(__PointerClass) = hx::RegisterClass(HX_CSTRING("cpp::Pointer"),IsPointer,sNone,sNone, 0,0,&__PointerClass );
   DynTrue = Dynamic( new (hx::NewObjConst) hx::BoolData(true) );
   DynFalse = Dynamic( new (hx::NewObjConst) hx::BoolData(false) );
   DynEmptyString = Dynamic(HX_CSTRING("").__ToObject());
}


