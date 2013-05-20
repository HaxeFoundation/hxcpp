#include <hxcpp.h>
#include <math.h>
#include <hxMath.h>

using namespace hx;



extern Class __StringClass;
namespace hx
{

extern Class hxEnumBase_obj__mClass;
extern Class Object__mClass;


Class __BoolClass;
Class __IntClass;
Class __FloatClass;
Class __VoidClass;


Class &GetBoolClass() { return __BoolClass; }
Class &GetIntClass() { return __IntClass; }
Class &GetFloatClass() { return __FloatClass; }
Class &GetVoidClass() { return __VoidClass; }



// --- "Simple" Data Objects ---------------------------------------------------


Dynamic DynZero;
Dynamic DynOne;
Dynamic DynTrue;
Dynamic DynFalse;
Dynamic DynEmptyString;

class IntData : public hx::Object
{
public:
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
      double diff = mValue - inRHS->__ToDouble();
      return diff < 0 ? -1 : diff==0 ? 0 : 1;
   }


   double mValue;
};




}



// --- Dynamic -------------------------------------------------



Dynamic::Dynamic(bool inVal) : super( inVal ? hx::DynTrue.mPtr : hx::DynFalse.mPtr ) { }
Dynamic::Dynamic(int inVal) :
  super( inVal==0 ? hx::DynZero.mPtr : inVal==1 ? hx::DynOne.mPtr : (hx::Object *)new IntData(inVal) ) { }
Dynamic::Dynamic(double inVal) :
  super(inVal==0 ? hx::DynZero.mPtr : inVal==1 ? hx::DynOne.mPtr :  (hx::Object *)new DoubleData(inVal) ) { }
Dynamic::Dynamic(float inVal) :
  super(inVal==0 ? hx::DynZero.mPtr : inVal==1 ? hx::DynOne.mPtr :  (hx::Object *)new DoubleData(inVal) ) { }
Dynamic::Dynamic(const cpp::CppInt32__ &inVal) :
  super(inVal.mValue==0 ? hx::DynZero.mPtr : inVal.mValue==1 ? hx::DynOne.mPtr :  (hx::Object *)new IntData((int)inVal) ) { }
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
	HX_MARK_MEMBER(__StringClass);
	HX_MARK_MEMBER(Object__mClass);
	HX_MARK_MEMBER(ArrayBase::__mClass);
	HX_MARK_MEMBER(Math_obj::__mClass);
	HX_MARK_MEMBER(Anon_obj::__mClass);
	HX_MARK_MEMBER(hx::hxEnumBase_obj__mClass);
	HX_MARK_MEMBER(hx::DynZero);
	HX_MARK_MEMBER(hx::DynOne);
	HX_MARK_MEMBER(hx::DynTrue);
	HX_MARK_MEMBER(hx::DynFalse);
	HX_MARK_MEMBER(hx::DynEmptyString);
};



#ifdef HXCPP_VISIT_ALLOCS
static void sVisitStatics(HX_VISIT_PARAMS) {
	HX_VISIT_MEMBER(__VoidClass);
	HX_VISIT_MEMBER(__BoolClass);
	HX_VISIT_MEMBER(__IntClass);
	HX_VISIT_MEMBER(__FloatClass);
	HX_VISIT_MEMBER(__StringClass);
	HX_VISIT_MEMBER(Object__mClass);
	HX_VISIT_MEMBER(ArrayBase::__mClass);
	HX_VISIT_MEMBER(Math_obj::__mClass);
	HX_VISIT_MEMBER(Anon_obj::__mClass);
	HX_VISIT_MEMBER(hx::hxEnumBase_obj__mClass);
	HX_VISIT_MEMBER(hx::DynZero);
	HX_VISIT_MEMBER(hx::DynOne);
	HX_VISIT_MEMBER(hx::DynTrue);
	HX_VISIT_MEMBER(hx::DynFalse);
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
   DynZero = Dynamic( new hx::IntData(0) );
   DynOne = Dynamic( new hx::IntData(1) );
   DynTrue = Dynamic( new hx::BoolData(true) );
   DynFalse = Dynamic( new hx::BoolData(false) );
   DynEmptyString = Dynamic(HX_CSTRING("").__ToObject());
}


