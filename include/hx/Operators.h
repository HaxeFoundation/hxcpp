#ifndef HX_OPERATORS_H
#define HX_OPERATORS_H



template<typename T> bool null::operator == (const hx::ObjectPtr<T> &O) const { return !O.mPtr; }
template<typename T> inline bool null::operator != (const hx::ObjectPtr<T> &O) const { return O.mPtr; }

template<typename T> inline bool null::operator == (const Array<T> &O) const { return !O.mPtr; }
template<typename T> inline bool null::operator != (const Array<T> &O) const { return O.mPtr; }
inline bool null::operator == (const hx::FieldRef &O) const { return !O.HasPointer(); }
inline bool null::operator != (const hx::FieldRef &O) const { return O.HasPointer(); }
inline bool null::operator == (const hx::IndexRef &O) const { return !O.HasPointer(); }
inline bool null::operator != (const hx::IndexRef &O) const { return O.HasPointer(); }

inline bool null::operator == (const Dynamic &O) const { return !O.mPtr; }
inline bool null::operator != (const Dynamic &O) const { return O.mPtr; }

inline bool null::operator == (const String &O) const { return !O.raw_ptr(); }
inline bool null::operator != (const String &O) const { return O.raw_ptr(); }

namespace hx {
template<typename T> Null<T>::operator Dynamic() { if (isNull) return Dynamic(); return value; }
}

HX_COMPARE_NULL_MOST_OPS(String)
HX_COMPARE_NULL_MOST_OPS(Dynamic)
HX_COMPARE_NULL_MOST_OPS(hx::FieldRef)
HX_COMPARE_NULL_MOST_OPS(hx::IndexRef)

//HX_NULL_DEFINE_COMPARE_MOST_OPS(String)
//HX_NULL_DEFINE_COMPARE_MOST_OPS(Dynamic)
//HX_NULL_DEFINE_COMPARE_MOST_OPS(hx::FieldRef)
//HX_NULL_DEFINE_COMPARE_MOST_OPS(hx::IndexRef)


// Operators for mixing various types ....


inline String operator+(const cpp::UInt64 &i,const String &s) { return String(i) + s; }
inline String operator+(const cpp::Int64 &i,const String &s) { return String(i) + s; }
inline String operator+(const int &i,const String &s) { return String(i) + s; }
inline String operator+(const unsigned int &i,const String &s) { return String(i) + s; }
inline String operator+(const double &d,const String &s) { return String(d) + s; }
inline String operator+(const float &d,const String &s) { return String(d) + s; }
inline String operator+(const bool &b,const String &s) { return String(b) + s; }
inline String operator+(const unsigned char c,const String &s) { return String(c) + s; }
inline String operator+(const signed char c,const String &s) { return String(c) + s; }
inline String operator+(const unsigned short c,const String &s) { return String(c) + s; }
inline String operator+(const signed short c,const String &s) { return String(c) + s; }
inline String operator+(const null &n,const String &s) { return String(n) + s; }
inline String operator+(const cpp::CppInt32__ &i,const String &s) { return String(i) + s; }

template<typename T_>
   inline String operator+(const hx::ObjectPtr<T_> &inLHS,const String &s)
   { return (inLHS.mPtr ? const_cast<hx::ObjectPtr<T_> & >(inLHS)->toString() : HX_CSTRING("null") ) + s; }

/*
template<typename LHS_>
   inline Dynamic operator+(LHS_ inLHS, const hx::FieldRef &inField)
   { return inLHS + inField.operator Dynamic(); }

template<typename LHS_>
   inline Dynamic operator+(LHS_ inLHS,const hx::IndexRef &inIndexRef)
   { return inLHS + inIndexRef.operator Dynamic(); }
*/

// += -= *= /= %= &= |= ^= <<= >>= >>>=

namespace hx
{

template<typename T> inline double ToDouble(T inT) { return 0; }
template<typename T> inline double ToDouble(hx::ObjectPtr<T> inObj)
{
   return inObj.mPtr ? inObj.mPtr->__ToDouble() : 0.0;
}
template<> inline double ToDouble(String inValue) { return __hxcpp_parse_float(inValue); }
template<> inline double ToDouble(double inValue) { return inValue; }
template<> inline double ToDouble(int inValue) { return inValue; }
template<> inline double ToDouble(bool inValue) { return inValue; }
template<> inline double ToDouble(float inValue) { return inValue; }
template<> inline double ToDouble(cpp::UInt64 inValue) { return inValue; }
template<> inline double ToDouble(cpp::Int64 inValue) { return inValue; }
template<> inline double ToDouble(null inValue) { return 0; }



inline int UShr(int inData,int inShift)
{
   return ((unsigned int)inData) >> inShift;
}


HXCPP_EXTERN_CLASS_ATTRIBUTES double DoubleMod(double inLHS,double inRHS);

template<typename TL,typename TR>
double Mod(TL inLHS,TR inRHS) { return hx::DoubleMod(inLHS,inRHS); }

double DivByZero(double d);

#if !defined(_MSC_VER) || _MSC_VER > 1399
inline int Mod(int inLHS,int inRHS)
{
   if (!inRHS)
      hx::Throw(HX_CSTRING("Mod by 0 Error."));
   return inLHS % inRHS;
}
#endif


template<typename L, typename R>
inline L& AddEq(L &inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename L, typename R>
inline L& MultEq(L &inLHS, R inRHS) { inLHS = inLHS * inRHS; return inLHS; }
template<typename L, typename R>
inline L& DivEq(L &inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& SubEq(L &inLHS, R inRHS) { inLHS = inLHS - inRHS; return inLHS; }
template<typename L, typename R>
inline L& AndEq(L &inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& OrEq(L &inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& XorEq(L &inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& ShlEq(L &inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& ShrEq(L &inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& UShrEq(L &inLHS, R inRHS) { inLHS = hx::UShr(inLHS,inRHS); return inLHS; }
template<typename L, typename R>
inline L& ModEq(L &inLHS, R inRHS) { inLHS = DoubleMod(inLHS,inRHS); return inLHS; }

#if defined(__GNUC__) || defined(__SNC__)
template<typename R>
inline hx::FieldRef AddEq(hx::FieldRef inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef MultEq(hx::FieldRef inLHS, R inRHS) { inLHS = inLHS * inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef DivEq(hx::FieldRef inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef SubEq(hx::FieldRef inLHS, R inRHS) { inLHS = inLHS - inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef AndEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef OrEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef XorEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef ShlEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef ShrEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef UShrEq(hx::FieldRef inLHS, R inRHS) { inLHS = hx::UShr(inLHS,inRHS); return inLHS; }
template<typename R>
inline hx::FieldRef ModEq(hx::FieldRef inLHS, R inRHS) { inLHS = DoubleMod(inLHS,inRHS); return inLHS; }


template<typename R>
inline hx::IndexRef AddEq(hx::IndexRef inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef MultEq(hx::IndexRef inLHS, R inRHS) { inLHS = (double)inLHS * (double)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef DivEq(hx::IndexRef inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef SubEq(hx::IndexRef inLHS, R inRHS) { inLHS = (double)inLHS - (double)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef AndEq(hx::IndexRef inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef OrEq(hx::IndexRef inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef XorEq(hx::IndexRef inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef ShlEq(hx::IndexRef inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef ShrEq(hx::IndexRef inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename R>
inline hx::IndexRef UShrEq(hx::IndexRef inLHS, R inRHS) { inLHS = hx::UShr(inLHS,inRHS); return inLHS; }
template<typename R>
inline hx::IndexRef ModEq(hx::IndexRef inLHS, R inRHS) { inLHS = DoubleMod(inLHS,inRHS); return inLHS; }



#endif // __GNUC__ || __SNC__

template<typename R,typename T>
inline hx::__TArrayImplRef<T> AddEq(hx::__TArrayImplRef<T> ref, R inRHS)
   { ref.mObject->__set(ref.mIndex, ref.mObject->__get(ref.mIndex) + inRHS); return ref;}

template<typename R,typename T>
inline hx::__TArrayImplRef<T> MultEq(hx::__TArrayImplRef<T> ref, R inRHS)
   { ref.mObject->__set(ref.mIndex, ref.mObject->__get(ref.mIndex) * inRHS); return ref;}

template<typename R,typename T>
inline hx::__TArrayImplRef<T> DivEq(hx::__TArrayImplRef<T> ref, R inRHS)
   { ref.mObject->__set(ref.mIndex, ref.mObject->__get(ref.mIndex) / inRHS); return ref;}

template<typename R,typename T>
inline hx::__TArrayImplRef<T> SubEq(hx::__TArrayImplRef<T> ref, R inRHS)
   { ref.mObject->__set(ref.mIndex, ref.mObject->__get(ref.mIndex) - inRHS); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> AndEq(hx::__TArrayImplRef<T> ref, int inRHS)
   { ref.mObject->__set(ref.mIndex, (int)ref.mObject->__get(ref.mIndex) & inRHS); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> OrEq(hx::__TArrayImplRef<T> ref, int inRHS)
   { ref.mObject->__set(ref.mIndex, (int)ref.mObject->__get(ref.mIndex) | inRHS); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> XorEq(hx::__TArrayImplRef<T> ref, int inRHS)
   { ref.mObject->__set(ref.mIndex, (int)ref.mObject->__get(ref.mIndex) ^ inRHS); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> ShlEq(hx::__TArrayImplRef<T> ref, int inRHS)
   { ref.mObject->__set(ref.mIndex, (int)ref.mObject->__get(ref.mIndex) << inRHS); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> ShrEq(hx::__TArrayImplRef<T> ref, int inRHS)
   { ref.mObject->__set(ref.mIndex, (int)ref.mObject->__get(ref.mIndex) >> inRHS); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> UShrEq(hx::__TArrayImplRef<T> ref, int inRHS)
   { ref.mObject->__set(ref.mIndex, hx::UShr(ref.mObject->__get(ref.mIndex),inRHS)); return ref;}

template<typename T>
inline hx::__TArrayImplRef<T> UShrEq(hx::__TArrayImplRef<T> ref, double inRHS)
   { ref.mObject->__set(ref.mIndex, DoubleMod(ref.mObject->__get(ref.mIndex),inRHS)); return ref;}







template<typename T> inline T TCastObject(hx::Object *inObj) { return hx::BadCast(); }
template<> inline bool TCastObject<bool>(hx::Object *inObj)
{
   if (!inObj) return false;
   if (inObj->__GetType()!=::vtBool) return hx::BadCast();
   return inObj?inObj->__ToInt():0;
}
template<> inline int TCastObject<int>(hx::Object *inObj)
{
   if (!inObj) return 0;
   if (!(inObj->__GetType()==::vtInt ||
        ((inObj->__GetType()==::vtFloat || inObj->__GetType()==::vtInt64) && inObj->__ToDouble()==inObj->__ToInt()) ) ) return hx::BadCast();
   return inObj->__ToInt();
}
template<> inline double TCastObject<double>(hx::Object *inObj)
{
   if (!inObj) return 0.0;
   if ((inObj->__GetType()!=::vtFloat && inObj->__GetType()!=::vtInt64 && inObj->__GetType()!=::vtInt))
      return hx::BadCast();
   return inObj->__ToDouble();
}
template<> inline float TCastObject<float>(hx::Object *inObj)
{
   if (!inObj || (inObj->__GetType()!=::vtFloat && inObj->__GetType()!=::vtInt64 && inObj->__GetType()!=::vtInt))
      return hx::BadCast();
   return inObj->__ToDouble();
}

template<> inline String TCastObject<String>(hx::Object *inObj)
{
   if (!inObj) return String();
   if (inObj->__GetType()!=::vtString)
      return hx::BadCast();
   return inObj->__ToString();
}

template<> inline null TCastObject<null>(hx::Object *inObj) { return null(); }

// Cast to scalar
template<typename T> struct TCast
{
   template<typename VAL> static inline T cast(VAL inVal ) {
      T result =  TCastObject<T>(Dynamic(inVal).GetPtr());
      //null string from null is ok...
      //if (result==null()) hx::BadCast();
      return result;
   }

   template<typename INOBJ>
   static inline T cast(ObjectPtr<INOBJ> inObj )
   {
      T result =  TCastObject<T>(inObj.GetPtr());
      if (result==null()) hx::BadCast();
      return result;
   }

   template<typename INOBJ>
   static inline T cast(Array<INOBJ> inObj ) { return hx::BadCast(); }

};

// Cast to object
template<typename T> struct TCast< ObjectPtr<T> >
{
   template<typename VAL> static inline ObjectPtr<T> cast(VAL inVal ) {
      ObjectPtr<T> result = Dynamic(inVal);
      if (result==null() && inVal!=null()) BadCast();
      return result;
   }

   template<typename INOBJ>
   static inline ObjectPtr<T> cast(ObjectPtr<INOBJ> inObj )
   {
      ObjectPtr<T> result = ObjectPtr<T>(inObj);
      if (result==null() && inObj!=null()) hx::BadCast();
      return result;
   }
};

#if (HXCPP_API_LEVEL >= 330)
template< > struct TCast< cpp::VirtualArray >
{
   template<typename VAL> static inline cpp::VirtualArray cast(VAL inVal ) {
      return  cpp::VirtualArray(inVal);
   }
};
#endif


// Cast to struct
template<typename T,typename H> struct TCast< cpp::Struct<T,H> >
{
   static inline cpp::Struct<T,H> cast( const cpp::Struct<T,H> &inObj ) { return inObj; }
};


inline Array<Dynamic> TCastToArray(Dynamic inVal)
{
   Dynamic result = inVal;
   if (result==null() && inVal!=null()) hx::BadCast();
   return inVal;
}

template<typename PTRTYPE> struct DynamicConvertType { enum { Convert = aciNotArray }; };

// Always convert ...
template<> struct DynamicConvertType< hx::Interface * > { enum { Convert = aciAlwaysConvert }; };
template<> struct DynamicConvertType< Array_obj<Dynamic> * > { enum { Convert = aciObjectArray }; };
template<> struct DynamicConvertType< Array_obj< ::String> * > { enum { Convert = aciStringArray }; };
template<typename T> struct DynamicConvertType< Array_obj<T> * > { enum { Convert = sizeof(T) }; };
#if (HXCPP_API_LEVEL >= 330)
template<> struct DynamicConvertType< cpp::VirtualArray_obj * > { enum { Convert = aciVirtualArray }; };
#endif

}



template<typename RESULT>
inline RESULT Dynamic::StaticCast() const
{
   typedef typename RESULT::Ptr type;

   const int convertId = (int)hx::DynamicConvertType<type>::Convert;
   if (convertId!=hx::aciNotArray && mPtr && convertId!=((hx::ArrayCommon *)mPtr)->getArrayConvertId())
   {
      // Constructing the result from the Dynamic value will check for a conversion
      //  using something like dynamic_cast
      return *this;
   }
   else
   {
      // Simple reinterpret_cast
      return (typename RESULT::Ptr)mPtr;
   }
}

namespace hx
{
inline bool IsInterfacePtr(...) { return false; }
inline bool IsInterfacePtr(const hx::Interface *) { return true; }
}
  
template<typename VALUE>
inline void __hxcpp_unsafe_set(hx::ObjectPtr<VALUE> &outForced, const Dynamic &inD)
{
   if (hx::IsInterfacePtr(outForced.mPtr))
   {
      hx::Throw(HX_CSTRING("unsafe set of interfaces not supported yet."));
      outForced.mPtr = (VALUE *)(inD.mPtr);
   }
   else
   {
      #if (HXCPP_API_LEVEL >= 331)
      outForced.mPtr = (VALUE *)(inD.mPtr);
      #else
      outForced.mPtr = (VALUE *)(inD.mPtr ? inD.mPtr->__GetRealObject() : 0);
      #endif
   }
}

namespace hx
{
class HXCPP_EXTERN_CLASS_ATTRIBUTES StringValueIterator : public cpp::StringIterator<int>
{
public:
   StringValueIterator(const String &inValue) : StringIterator(inValue) { }

   int next() { return value.cca(pos++); }
};

class HXCPP_EXTERN_CLASS_ATTRIBUTES StringKeyValueIterator : public cpp::StringIterator<Dynamic>
{
public:
   StringKeyValueIterator(const String &inValue) : StringIterator(inValue) { }

   Dynamic next() {
      int p = pos;
      return
        hx::AnonStruct2_obj< int,int >::Create(HX_("key",9f,89,51,00),p,
                                               HX_("value",71,7f,b8,31), value.cca(pos++) );
   }
};
}


Dynamic String::iterator()
{
   return new hx::StringValueIterator(*this);
}


Dynamic String::keyValueIterator()
{
   return new hx::StringKeyValueIterator(*this);
}

namespace hx
{
template<typename FROM, typename TO> Dynamic hx::ArrayKeyValueIterator<FROM, TO>::next()
{
   int p = mIdx++;
   return
     hx::AnonStruct2_obj< int,TO >::Create(HX_("key",9f,89,51,00),p,
                                           HX_("value",71,7f,b8,31), toTo(mArray->__get(p)) );
}
}




#endif
