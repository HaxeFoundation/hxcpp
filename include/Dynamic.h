#ifndef HX_DYNAMIC_H
#define HX_DYNAMIC_H

// --- Dynamic ---------------------------------------------------------------
//
// The Dynamic class views all classes through the hx::Object interface, and
//  provides generic access to its pointer.
// It uses dynamic_cast to provide strongly-typed access to the real class.

class Dynamic : public hx::ObjectPtr<hx::Object>
{
   typedef  hx::ObjectPtr<hx::Object> super;

public:

   Dynamic() {};
   Dynamic(int inVal);
   Dynamic(const cpp::CppInt32__ &inVal);
   Dynamic(bool inVal);
   Dynamic(double inVal);
   Dynamic(hx::Object *inObj) : super(inObj) { }
   Dynamic(const String &inString);
   Dynamic(const null &inNull) : super(0) { }
   Dynamic(const Dynamic &inRHS) : super(inRHS.mPtr) { }
   Dynamic(const wchar_t *inStr);

    void Set(bool inVal);
    void Set(int inVal);
    void Set(double inVal);

   operator double () const { return mPtr ? mPtr->__ToDouble() : 0.0; }
   operator int () const { return mPtr ? mPtr->__ToInt() : 0; }
   operator unsigned char () const { return mPtr ? mPtr->__ToInt() : 0; }
   operator bool() const { return mPtr && mPtr->__ToInt(); }
   bool operator !() const { return !mPtr || !mPtr->__ToInt(); }

   hx::IndexRef operator[](int inIndex);
   inline Dynamic __get(int inIndex) const { return mPtr->__GetItem(inIndex); }

   template<typename SOURCE_>
   Dynamic(const hx::ObjectPtr<SOURCE_> &inObjectPtr) :
          hx::ObjectPtr<hx::Object>(inObjectPtr.mPtr) { }

   Dynamic Default(const Dynamic &inDef) { return mPtr ? *this : inDef; }

   template<typename RETURN_>
   RETURN_ Cast() const { return RETURN_(*this); }

   template<typename CLASS_>
   bool IsClass() { return CLASS_(mPtr).mPtr; }

	static void __boot();

   int Compare(const Dynamic &inRHS) const
   {
      if (mPtr==inRHS.mPtr) return 0;
      if (mPtr==0) return -1;
      if (inRHS.mPtr==0) return -1;
      return mPtr->__Compare(inRHS.mPtr);
   }

   bool operator==(const null &inRHS) const { return mPtr==0; }
   bool operator!=(const null &inRHS) const { return mPtr!=0; }

   bool operator != (const Dynamic &inRHS) const { return (Compare(inRHS) != 0); }
   bool operator != (const String &inRHS)  const { return !mPtr || ((String)(*this) != inRHS); }
   bool operator != (double inRHS)  const { return !mPtr || ((double)(*this) != inRHS); }
   bool operator != (int inRHS)  const { return !mPtr || ((double)(*this) != (double)inRHS); }
   bool operator != (bool inRHS)  const { return !mPtr || ((double)(*this) != (double)inRHS); }

   bool operator == (const Dynamic &inRHS) const
   {
      if (mPtr==inRHS.mPtr) return true;
      if (!mPtr || !inRHS.mPtr) return false;
      return mPtr->__Compare(inRHS.mPtr)==0;
   }

   #define DYNAMIC_COMPARE_OP( op ) \
      bool operator op (const String &inRHS)  const { return mPtr && ((String)(*this) op inRHS); } \
      bool operator op (double inRHS)  const { return mPtr && ((double)(*this) op inRHS); } \
      bool operator op (int inRHS)  const { return mPtr && ((double)(*this) op (double)inRHS); } \
      bool operator op (bool inRHS)  const { return mPtr && ((double)(*this) op (double)inRHS); }

   #define DYNAMIC_COMPARE_OP_ALL( op ) \
      bool operator op (const Dynamic &inRHS) const { return mPtr && (Compare(inRHS) op 0); } \
      DYNAMIC_COMPARE_OP(op)


   DYNAMIC_COMPARE_OP( == )
   DYNAMIC_COMPARE_OP_ALL( < )
   DYNAMIC_COMPARE_OP_ALL( <= )
   DYNAMIC_COMPARE_OP_ALL( >= )
   DYNAMIC_COMPARE_OP_ALL( >  )

   template<typename T_>
   bool operator==(const hx::ObjectPtr<T_> &inRHS) const { return mPtr == inRHS.mPtr; }
   template<typename T_>
   bool operator!=(const hx::ObjectPtr<T_> &inRHS) const { return mPtr != inRHS.GetPtr(); }


   // Operator + is different, since it must consider strings too...
    Dynamic operator+(const Dynamic &inRHS) const;
   inline String operator+(const String &s) const;
    Dynamic operator+(const int &i) const;
    Dynamic operator+(const double &d) const;

   double operator%(const Dynamic &inRHS) const;
   double operator-() const { return mPtr ? - mPtr->__ToDouble() : 0.0; }


   #define DYNAMIC_ARITH( op ) \
      double operator op (const Dynamic &inRHS) const { return (double)(*this) op (double)inRHS; } \
      double operator op (const double &inRHS) const { return (double)(*this) op (double)inRHS; } \
      double operator op (const int &inRHS) const { return (double)(*this) op (double)inRHS; }

   DYNAMIC_ARITH( - )
   DYNAMIC_ARITH( * )
   DYNAMIC_ARITH( / )

   void ThrowBadFunctionError();
   inline void CheckFPtr() { if (!mPtr) ThrowBadFunctionError(); }


   // Hmm, ugly.
   typedef const Dynamic &D;
   inline Dynamic operator()() { CheckFPtr(); return mPtr->__run(); }
   inline Dynamic operator()(D a) { CheckFPtr(); return mPtr->__run(a); }
   inline Dynamic operator()(D a,D b) { CheckFPtr(); return mPtr->__run(a,b); }
   inline Dynamic operator()(D a,D b,D c) { CheckFPtr(); return mPtr->__run(a,b,c); }
   inline Dynamic operator()(D a,D b,D c,D d) { CheckFPtr(); return mPtr->__run(a,b,c,d); }
   inline Dynamic operator()(D a,D b,D c,D d,D e) { CheckFPtr(); return mPtr->__run(a,b,c,d,e); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f) { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h,D i)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h,i); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h,i,j); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j,D k)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h,i,j,k); }

};


template<>
inline int Dynamic::Cast<int>() const { return mPtr ? mPtr->__ToInt() : 0; }
template<>
inline bool Dynamic::Cast<bool>() const { return mPtr ? mPtr->__ToInt() : 0; }
template<>
inline double Dynamic::Cast<double>() const { return mPtr ? mPtr->__ToDouble() : 0; }
template<>
inline String Dynamic::Cast<String>() const { return mPtr ? mPtr->toString() : String(null()); }



//
// Gets the class definition that relates to a specific type.
// Most classes have their own class data, by the standard types (non-classes)
//  use the template traits to get the class

namespace hx
{
Class &GetIntClass();
Class &GetFloatClass();
Class &GetBoolClass();
Class &GetVoidClass();
Class &GetStringClass();
}

template<>
inline bool Dynamic::IsClass<int>() { return mPtr && mPtr->__GetClass()==hx::GetIntClass(); }
template<>
inline bool Dynamic::IsClass<double>() { return mPtr && mPtr->__GetClass()==hx::GetFloatClass(); }
template<>
inline bool Dynamic::IsClass<bool>() { return mPtr && mPtr->__GetClass()==hx::GetBoolClass(); }
template<>
inline bool Dynamic::IsClass<null>() { return !mPtr; }
template<>
inline bool Dynamic::IsClass<String>() { return mPtr && mPtr->__GetClass()==hx::GetStringClass(); }

inline String Dynamic::operator+(const String &s) const { return Cast<String>() + s; }


inline bool operator != (double inLHS,const Dynamic &inRHS) \
   { return !inRHS.GetPtr() || (inLHS != (double)inRHS); } \
inline bool operator != (int inLHS,const Dynamic &inRHS) \
   { return !inRHS.GetPtr() || (inLHS != (double)inRHS); } \
inline bool operator != (bool inLHS,const Dynamic &inRHS) \
   { return !inRHS.GetPtr() || ((double)inLHS != (double)inRHS); }


#define COMPARE_DYNAMIC_OP( op ) \
   inline bool operator op (double inLHS,const Dynamic &inRHS) \
      { return inRHS.GetPtr() && (inLHS op (double)inRHS); } \
   inline bool operator op (int inLHS,const Dynamic &inRHS) \
      { return inRHS.GetPtr() && (inLHS op (double)inRHS); } \
   inline bool operator op (bool inLHS,const Dynamic &inRHS) \
      { return inRHS.GetPtr() && ((double)inLHS op (double)inRHS); }

COMPARE_DYNAMIC_OP( == )
COMPARE_DYNAMIC_OP( < )
COMPARE_DYNAMIC_OP( <= )
COMPARE_DYNAMIC_OP( >= )
COMPARE_DYNAMIC_OP( >  )


#define ARITH_DYNAMIC( op ) \
   inline double operator op (const double &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const int &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \

ARITH_DYNAMIC( - )
ARITH_DYNAMIC( + )
ARITH_DYNAMIC( / )
ARITH_DYNAMIC( * )

double operator%(const int &inLHS,const Dynamic &inRHS);
double operator%(const double &inLHS,const Dynamic &inRHS);


#endif
