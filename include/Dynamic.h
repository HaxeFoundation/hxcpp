#ifndef HX_DYNAMIC_H
#define HX_DYNAMIC_H

// --- Dynamic ---------------------------------------------------------------
//
// The Dynamic class views all classes through the hx::Object interface, and
//  provides generic access to its pointer.
// It uses dynamic_cast to provide strongly-typed access to the real class.

namespace hx { class Interface; }



class HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic : public hx::ObjectPtr<hx::Object>
{
   typedef  hx::ObjectPtr<hx::Object> super;

public:

   Dynamic() {};
   Dynamic(int inVal);
   Dynamic(short inVal);
   Dynamic(unsigned int inVal);
   Dynamic(unsigned short inVal);
   Dynamic(unsigned char inVal);
   Dynamic(signed char inVal);
   Dynamic(const cpp::CppInt32__ &inVal);
   Dynamic(bool inVal);
   Dynamic(double inVal);
   Dynamic(float inVal);
   Dynamic(cpp::Int64 inVal);
   Dynamic(cpp::UInt64 inVal);
   Dynamic(hx::Object *inObj) : super(inObj) { }
   Dynamic(const String &inString);
   Dynamic(const null &inNull) : super(0) { }
   Dynamic(const Dynamic &inRHS) : super(inRHS.mPtr) { }
   explicit Dynamic(const HX_CHAR *inStr);
   Dynamic(const cpp::Variant &inRHS) : super(inRHS.asDynamic()) { }
   template<typename T>
   Dynamic(const hx::Native<T *> &inInterface):super(inInterface.ptr ? inInterface->__GetRealObject() : (hx::Object *)0 ) { }
   #if !defined(__GNUC__) || (defined(__WORDSIZE) && (__WORDSIZE != 64))
   Dynamic(long inVal);
   Dynamic(unsigned long inVal);
   #endif
#ifdef __OBJC__
#ifdef HXCPP_OBJC
   Dynamic(const id inObjc);
#endif
#endif

   template<typename T,typename S>
   explicit Dynamic(const cpp::Struct<T,S> &inRHS) { *this = inRHS; }
   template<typename T>
   explicit Dynamic(const cpp::Pointer<T> &inRHS) { *this = inRHS; }


    void Set(bool inVal);
    void Set(int inVal);
    void Set(double inVal);
    void Set(float inVal);


   template<typename RESULT> RESULT StaticCast() const;

   inline operator double () const { return mPtr ? mPtr->__ToDouble() : 0.0; }
   inline operator float () const { return mPtr ? (float)mPtr->__ToDouble() : 0.0f; }
   inline operator int () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator unsigned int () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator short () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator unsigned short () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator unsigned char () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator char () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator signed char () const { return mPtr ? mPtr->__ToInt() : 0; }
   inline operator bool() const { return mPtr && mPtr->__ToInt(); }
   inline operator cpp::Int64() const { return mPtr ? mPtr->__ToInt64() : 0; }
   inline operator cpp::UInt64() const { return mPtr ? mPtr->__ToInt64() : 0; }

   // Conversion to generic pointer requires you to tag the class with a typedef
   template<typename T>
   inline operator typename hx::Native<T *> () const {
      return hx::Native<T *>(dynamic_cast<T *>(mPtr));
   }


   //inline operator cpp::Variant() const { return cpp::Variant(mPtr); }
#ifdef __OBJC__
#ifdef HXCPP_OBJC
   #ifdef OBJC_ARC
   inline operator id() const { return mPtr ? (__bridge id)mPtr->__GetHandle() : 0; }
   #else
   inline operator id() const { return mPtr ? (id)mPtr->__GetHandle() : 0; }
   #endif
#endif
#endif
   inline bool operator !() const { return !mPtr || !mPtr->__ToInt(); }

   hx::IndexRef operator[](int inIndex);
   inline Dynamic __get(int inIndex) const { return mPtr->__GetItem(inIndex); }

   template<typename SOURCE_>
   Dynamic(const hx::ObjectPtr<SOURCE_> &inObjectPtr) :
          hx::ObjectPtr<hx::Object>(inObjectPtr.mPtr) { }

   Dynamic Default(const Dynamic &inDef) { return mPtr ? *this : inDef; }

   template<typename RETURN_>
   RETURN_ Cast() const { return RETURN_(*this); }

   template<typename CLASS_>
   bool IsClass() { return CLASS_(mPtr,false).mPtr; }

	static void __boot();

	inline bool IsNumeric() const
	{
		if (!mPtr) return false;
		int t = mPtr->__GetType();
		return t==vtInt || t==vtFloat;
	}

	inline bool IsBool() const
	{
		if (!mPtr) return false;
		int t = mPtr->__GetType();
		return t==vtBool;
	}


   int Compare(const Dynamic &inRHS) const
   {
      if (mPtr==0) return inRHS.mPtr==0 ? 0 : -1;
      if (inRHS.mPtr==0) return -1;
      #if (HXCPP_API_LEVEL>=331)
      return mPtr->__Compare(inRHS.mPtr);
      #else
      return mPtr->__Compare(inRHS.mPtr->__GetRealObject());
      #endif
   }

   bool operator==(const null &inRHS) const { return mPtr==0; }
   bool operator!=(const null &inRHS) const { return mPtr!=0; }

   bool operator == (const Dynamic &inRHS) const
   {
      // Comparing pointers fails in the case on Nan==Nan
      //if (mPtr==inRHS.mPtr) return true;
      if (!mPtr && !inRHS.mPtr) return true;
      if (!mPtr || !inRHS.mPtr) return false;
      #if (HXCPP_API_LEVEL>=331)
      return mPtr->__Compare(inRHS.mPtr)==0;
      #else
      return mPtr->__Compare(inRHS.mPtr->__GetRealObject())==0;
      #endif
   }

   bool operator != (const Dynamic &inRHS) const
   {
      // Comparing pointers fails in the case on Nan==Nan
      //if (mPtr==inRHS.mPtr) return true;
      if (!mPtr && !inRHS.mPtr) return false;
      if (!mPtr || !inRHS.mPtr) return true;
      #if (HXCPP_API_LEVEL>=331)
      return mPtr->__Compare(inRHS.mPtr)!=0;
      #else
      return mPtr->__Compare(inRHS.mPtr->__GetRealObject())!=0;
      #endif
   }


   bool operator == (const cpp::Variant &inRHS) const { return (*this) == Dynamic(inRHS); }
   bool operator != (const cpp::Variant &inRHS) const { return (*this) != Dynamic(inRHS); }


   #define DYNAMIC_COMPARE_OP( op ) \
      bool operator op (const String &inRHS)  const { return mPtr && ((String)(*this) op inRHS); } \
      bool operator op (double inRHS)  const { return IsNumeric() && ((double)(*this) op inRHS); } \
      bool operator op (cpp::Int64 inRHS)  const { return IsNumeric() && ((cpp::Int64)(*this) op inRHS); } \
      bool operator op (cpp::UInt64 inRHS)  const { return IsNumeric() && ((cpp::Int64)(*this) op inRHS); } \
      bool operator op (float inRHS)  const { return IsNumeric() && ((double)(*this) op inRHS); } \
      bool operator op (int inRHS)  const { return IsNumeric() && ((double)(*this) op (double)inRHS); } \
      bool operator op (unsigned int inRHS)  const { return IsNumeric() && ((double)(*this) op (double)inRHS); } \
      bool operator op (short inRHS)  const { return IsNumeric() && ((double)(*this) op (double)inRHS); } \
      bool operator op (unsigned short inRHS)  const { return IsNumeric() && ((double)(*this) op (double)inRHS); } \
      bool operator op (signed char inRHS)  const { return IsNumeric() && ((double)(*this) op (double)inRHS); } \
      bool operator op (unsigned char inRHS)  const { return IsNumeric() && ((double)(*this) op (double)inRHS); } \
      bool operator op (bool inRHS)  const { return IsBool() && ((double)(*this) op (double)inRHS); } \

   bool operator != (const String &inRHS)  const { return !mPtr || ((String)(*this) != inRHS); }
   bool operator != (double inRHS)  const { return !IsNumeric() || ((double)(*this) != inRHS); }
   bool operator != (cpp::Int64 inRHS)  const { return !IsNumeric() || ((cpp::Int64)(*this) != inRHS); }
   bool operator != (cpp::UInt64 inRHS)  const { return !IsNumeric() || ((cpp::Int64)(*this) != inRHS); }
   bool operator != (float inRHS)  const { return !IsNumeric() || ((double)(*this) != inRHS); }
   bool operator != (int inRHS)  const { return !IsNumeric() || ((double)(*this) != (double)inRHS); }
   bool operator != (unsigned int inRHS)  const { return !IsNumeric() || ((double)(*this) != (double)inRHS); }
   bool operator != (short inRHS)  const { return !IsNumeric() || ((double)(*this) != (double)inRHS); }
   bool operator != (unsigned short inRHS)  const { return !IsNumeric() || ((double)(*this) != (double)inRHS); }
   bool operator != (signed char inRHS)  const { return !IsNumeric() || ((double)(*this) != (double)inRHS); }
   bool operator != (unsigned char inRHS)  const { return !IsNumeric() || ((double)(*this) != (double)inRHS); }
   bool operator != (bool inRHS)  const { return !IsBool() || ((double)(*this) != (double)inRHS); }



   #define DYNAMIC_COMPARE_OP_ALL( op ) \
      bool operator op (const Dynamic &inRHS) const { return mPtr && (Compare(inRHS) op 0); } \
      bool operator op (const cpp::Variant &inRHS) const { return *this op Dynamic(inRHS); } \
      DYNAMIC_COMPARE_OP(op)


   DYNAMIC_COMPARE_OP( == )
   DYNAMIC_COMPARE_OP_ALL( < )
   DYNAMIC_COMPARE_OP_ALL( <= )
   DYNAMIC_COMPARE_OP_ALL( >= )
   DYNAMIC_COMPARE_OP_ALL( >  )

   template<typename T_>
   bool operator==(const hx::ObjectPtr<T_> &inRHS) const
   {
      if (mPtr==inRHS.mPtr) return true;
      if (!mPtr || !inRHS.mPtr) return false;
      #if (HXCPP_API_LEVEL>=331)
      return mPtr == inRHS.mPtr;
      #else
      return mPtr->__GetRealObject() == inRHS.mPtr->__GetRealObject();
      #endif
   }

   template<typename T_>
   bool operator!=(const hx::ObjectPtr<T_> &inRHS) const
   {
      if (mPtr==inRHS.mPtr) return false;
      if (!mPtr || !inRHS.mPtr) return true;
      #if (HXCPP_API_LEVEL>=331)
      return mPtr != inRHS.mPtr;
      #else
      return mPtr->__GetRealObject() != inRHS.mPtr->__GetRealObject();
      #endif
   }


   // Operator + is different, since it must consider strings too...
    Dynamic operator+(const Dynamic &inRHS) const;
   inline String operator+(const String &s) const;

    Dynamic operator+(const cpp::UInt64 &i) const;
    Dynamic operator+(const cpp::Int64 &i) const;
    Dynamic operator+(const int &i) const;
    Dynamic operator+(const unsigned int &i) const;
    Dynamic operator+(const short &i) const;
    Dynamic operator+(const unsigned short &i) const;
    Dynamic operator+(const signed char &i) const;
    Dynamic operator+(const unsigned char &i) const;
    Dynamic operator+(const double &d) const;
    Dynamic operator+(const float &d) const;
    Dynamic operator+(const cpp::Variant &d) const;

   double operator%(const Dynamic &inRHS) const;
   double operator-() const { return mPtr ? - mPtr->__ToDouble() : 0.0; }
   double operator++() { double val = mPtr->__ToDouble() + 1; *this = val; return val; }
   double operator++(int) {double val = mPtr->__ToDouble(); *this = val+1; return val; }
   double operator--() { double val = mPtr->__ToDouble() - 1; *this = val; return val; }
   double operator--(int) {double val = mPtr->__ToDouble(); *this = val-1; return val; }


   double operator / (const cpp::Variant &inRHS) const { return (double)(*this) / (double)inRHS; } \
   double operator / (const Dynamic &inRHS) const { return (double)(*this) / (double)inRHS; } \
   double operator / (const double &inRHS) const { return (double)(*this) / (double)inRHS; } \
   double operator / (const float &inRHS) const { return (double)(*this) / (double)inRHS; } \
   double operator / (const int &inRHS) const { return (double)(*this) / (double)inRHS; }

   #define DYNAMIC_ARITH( op ) \
      Dynamic operator op (const cpp::Variant &inRHS) const \
        { return mPtr->__GetType()==vtInt && inRHS.isInt() ? \
              Dynamic((int)(*this) op (int)inRHS) : \
              Dynamic( (double)(*this) op (double)inRHS); } \
      Dynamic operator op (const Dynamic &inRHS) const \
        { return mPtr->__GetType()==vtInt && inRHS.mPtr->__GetType()==vtInt ? \
              Dynamic((int)(*this) op (int)inRHS) : \
              Dynamic( (double)(*this) op (double)inRHS); } \
      double operator op (const double &inRHS) const { return (double)(*this) op (double)inRHS; } \
      double operator op (const float &inRHS) const { return (double)(*this) op (double)inRHS; } \
      Dynamic operator op (const int &inRHS) const \
        { return mPtr->__GetType()==vtInt ?  Dynamic((int)(*this) op inRHS) : Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const unsigned int &inRHS) const \
        { return mPtr->__GetType()==vtInt ?  Dynamic((int)(*this) op inRHS) : Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const short &inRHS) const \
        { return mPtr->__GetType()==vtInt ?  Dynamic((int)(*this) op inRHS) : Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const unsigned short &inRHS) const \
        { return mPtr->__GetType()==vtInt ?  Dynamic((int)(*this) op inRHS) : Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const signed char &inRHS) const \
        { return mPtr->__GetType()==vtInt ?  Dynamic((int)(*this) op inRHS) : Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const unsigned char &inRHS) const \
        { return mPtr->__GetType()==vtInt ?  Dynamic((int)(*this) op inRHS) : Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const cpp::Int64 &inRHS) const \
        { return Dynamic((double)(*this) op inRHS); } \
      Dynamic operator op (const cpp::UInt64 &inRHS) const \
        { return Dynamic((double)(*this) op inRHS); } \

   DYNAMIC_ARITH( - )
   DYNAMIC_ARITH( * )

   static void ThrowBadFunctionError();
   inline void CheckFPtr() { if (!mPtr) ThrowBadFunctionError(); }

   inline  ::Dynamic operator()() { CheckFPtr(); return mPtr->__run(); }
   inline  ::Dynamic operator()(const Dynamic &inArg0) { CheckFPtr(); return mPtr->__run(inArg0); }
   inline  ::Dynamic operator()(const Dynamic &inArg0,const Dynamic &inArg1) { CheckFPtr(); return mPtr->__run(inArg0,inArg1); }
   inline  ::Dynamic operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2) { CheckFPtr(); return mPtr->__run(inArg0,inArg1,inArg2); }
   inline  ::Dynamic operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3) { CheckFPtr(); return mPtr->__run(inArg0,inArg1,inArg2,inArg3); }
   inline  ::Dynamic operator()(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4) { CheckFPtr(); return mPtr->__run(inArg0,inArg1,inArg2,inArg3,inArg4); }

   HX_DECLARE_DYNAMIC_FUNCTIONS;


   typedef const Dynamic &D;
};



namespace hx
{

inline hx::Object *DynamicPtr(Dynamic inVal) { return inVal.mPtr; }

typedef Dynamic (*MemberFunction0)(hx::Object *inObj);
typedef Dynamic (*MemberFunction1)(hx::Object *inObj,const Dynamic &inArg0);
typedef Dynamic (*MemberFunction2)(hx::Object *inObj,const Dynamic &inArg0,const Dynamic &inArg1);
typedef Dynamic (*MemberFunction3)(hx::Object *inObj,const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2);
typedef Dynamic (*MemberFunction4)(hx::Object *inObj,const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3);
typedef Dynamic (*MemberFunction5)(hx::Object *inObj,const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4);
typedef Dynamic (*MemberFunctionVar)(hx::Object *inObj,const Array<Dynamic> &inArgs);

typedef Dynamic (*StaticFunction0)();
typedef Dynamic (*StaticFunction1)(const Dynamic &inArg0);
typedef Dynamic (*StaticFunction2)(const Dynamic &inArg0,const Dynamic &inArg1);
typedef Dynamic (*StaticFunction3)(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2);
typedef Dynamic (*StaticFunction4)(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3);
typedef Dynamic (*StaticFunction5)(const Dynamic &inArg0,const Dynamic &inArg1,const Dynamic &inArg2,const Dynamic &inArg3,const Dynamic &inArg4);
typedef Dynamic (*StaticFunctionVar)(const Array<Dynamic> &inArgs);


HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction0(const char *,hx::Object *, MemberFunction0);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction1(const char *,hx::Object *, MemberFunction1);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction2(const char *,hx::Object *, MemberFunction2);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction3(const char *,hx::Object *, MemberFunction3);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction4(const char *,hx::Object *, MemberFunction4);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunction5(const char *,hx::Object *, MemberFunction5);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateMemberFunctionVar(const char *,hx::Object *, MemberFunctionVar,int inN);

HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction0(const char *,StaticFunction0);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction1(const char *,StaticFunction1);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction2(const char *,StaticFunction2);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction3(const char *,StaticFunction3);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction4(const char *,StaticFunction4);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunction5(const char *,StaticFunction5);
HXCPP_EXTERN_CLASS_ATTRIBUTES
Dynamic CreateStaticFunctionVar(const char *,StaticFunctionVar,int inN);


}







template<>
inline int Dynamic::Cast<int>() const { return mPtr ? mPtr->__ToInt() : 0; }
template<>
inline bool Dynamic::Cast<bool>() const { return mPtr ? mPtr->__ToInt() : 0; }
template<>
inline double Dynamic::Cast<double>() const { return mPtr ? mPtr->__ToDouble() : 0; }
template<>
inline float Dynamic::Cast<float>() const { return mPtr ? mPtr->__ToDouble() : 0; }
template<>
inline String Dynamic::Cast<String>() const { return mPtr ? mPtr->toString() : String(null()); }



//
// Gets the class definition that relates to a specific type.
// Most classes have their own class data, by the standard types (non-classes)
//  use the template traits to get the class

namespace hx
{
HXCPP_EXTERN_CLASS_ATTRIBUTES hx::Class &GetIntClass();
HXCPP_EXTERN_CLASS_ATTRIBUTES hx::Class &GetFloatClass();
HXCPP_EXTERN_CLASS_ATTRIBUTES hx::Class &GetBoolClass();
HXCPP_EXTERN_CLASS_ATTRIBUTES hx::Class &GetVoidClass();
HXCPP_EXTERN_CLASS_ATTRIBUTES hx::Class &GetStringClass();
}

template<>
inline bool Dynamic::IsClass<int>() { return mPtr && mPtr->__GetClass()==hx::GetIntClass(); }
template<>
inline bool Dynamic::IsClass<double>() { return mPtr && 
   ( mPtr->__GetClass()==hx::GetIntClass() || mPtr->__GetClass()==hx::GetFloatClass()) ; }
template<>
inline bool Dynamic::IsClass<float>() { return mPtr && mPtr->__GetClass()==hx::GetFloatClass(); }
template<>
inline bool Dynamic::IsClass<bool>() { return mPtr && mPtr->__GetClass()==hx::GetBoolClass(); }
template<>
inline bool Dynamic::IsClass<null>() { return !mPtr; }
template<>
inline bool Dynamic::IsClass<String>() { return mPtr && mPtr->__GetClass()==hx::GetStringClass(); }
template<>
inline bool Dynamic::IsClass<Dynamic>() { return true; }

inline String Dynamic::operator+(const String &s) const { return Cast<String>() + s; }

#define HX_DYNAMIC_OP_ISEQ(T) \
inline bool operator == (const T &inLHS,const Dynamic &inRHS) { return inRHS==inLHS; } \
inline bool operator != (const T &inLHS,const Dynamic &inRHS) { return inRHS!=inLHS; }

HX_DYNAMIC_OP_ISEQ(String)
HX_DYNAMIC_OP_ISEQ(double)
HX_DYNAMIC_OP_ISEQ(float)
HX_DYNAMIC_OP_ISEQ(int)
HX_DYNAMIC_OP_ISEQ(bool)

inline bool operator < (bool inLHS,const Dynamic &inRHS) { return false; }
inline bool operator <= (bool inLHS,const Dynamic &inRHS) { return false; }
inline bool operator >= (bool inLHS,const Dynamic &inRHS) { return false; }
inline bool operator > (bool inLHS,const Dynamic &inRHS) { return false; }

#if defined(HX_WINRT) && defined(__cplusplus_winrt)
// Try to avoid the compiler using injected Box::operator int and Dynamic(null) when doing ==
template<typename T>
bool operator==(Platform::Box<T> ^inPtr, nullptr_t)
{
    void* ptr = (void*) reinterpret_cast<void*>(inPtr);
   return ptr==nullptr;
}
#endif



#define COMPARE_DYNAMIC_OP( op ) \
   inline bool operator op (double inLHS,const ::Dynamic &inRHS) \
      { return inRHS.IsNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (float inLHS,const ::Dynamic &inRHS) \
      { return inRHS.IsNumeric() && ((double)inLHS op (double)inRHS); } \
   inline bool operator op (int inLHS,const ::Dynamic &inRHS) \
      { return inRHS.IsNumeric() && (inLHS op (double)inRHS); } 

COMPARE_DYNAMIC_OP( < )
COMPARE_DYNAMIC_OP( <= )
COMPARE_DYNAMIC_OP( >= )
COMPARE_DYNAMIC_OP( >  )


#define ARITH_DYNAMIC( op ) \
   inline double operator op (const cpp::Int64 &inLHS,const Dynamic &inRHS) { return inLHS op (cpp::Int64)inRHS;} \
   inline double operator op (const cpp::UInt64 &inLHS,const Dynamic &inRHS) { return inLHS op (cpp::UInt64)inRHS;} \
   inline double operator op (const double &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const float &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const int &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const unsigned int &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const short &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const unsigned short &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const signed char &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const unsigned char &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \

ARITH_DYNAMIC( - )
ARITH_DYNAMIC( + )
ARITH_DYNAMIC( / )
ARITH_DYNAMIC( * )

double operator%(const int &inLHS,const Dynamic &inRHS);
double operator%(const double &inLHS,const Dynamic &inRHS);
double operator%(const float &inLHS,const Dynamic &inRHS);

template<typename T,typename H> String::String(const cpp::Struct<T,H> &inRHS) { *this = (String)inRHS; }
template<typename OBJ> String::String(const hx::ObjectPtr<OBJ> &inRHS) { *this = Dynamic(inRHS); }



#endif
