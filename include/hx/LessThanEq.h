#ifndef HX_LESS_THAN_EQ_INCLUDED
#define HX_LESS_THAN_EQ_INCLUDED

namespace hx
{

enum {
   CompareAsInt,
   CompareAsInt64,
   CompareAsDouble,
   CompareAsString,
   CompareAsDynamic,
};


template <typename T> 
struct CompareTraits
{
   enum { type = (int)CompareAsDynamic };

   inline static int toInt(Dynamic inValue) { return inValue; }
   inline static double toDouble(Dynamic inValue) { return inValue; }
   inline static cpp::Int64 toInt64(Dynamic inValue) { return inValue; }
   inline static String toString(Dynamic inValue) { return inValue; }
   inline static hx::Object *toObject(Dynamic inValue) { return inValue.mPtr; }
   inline static int getDynamicCompareType(const Dynamic &inValue)
   {
      if (!inValue.mPtr)
         return CompareAsDynamic;
      switch(inValue->__GetType())
      {
         case vtInt: case vtBool: return CompareAsInt;
         case vtInt64: return CompareAsInt64;
         case vtFloat: return CompareAsDouble;
         case vtString: return CompareAsString;
         default: return CompareAsDynamic;
      }
   }
   inline static bool isNull(const Dynamic &inValue) { return !inValue.mPtr; }
};



template <> 
struct CompareTraits<null>
{
   enum { type = (int)CompareAsDynamic };

   inline static int toInt(const null &inValue) { return 0; }
   inline static double toDouble(const null & inValue) { return 0; }
   inline static cpp::Int64 toInt64(const null & inValue) { return 0; }
   inline static String toString(const null & inValue) { return String(); }
   inline static hx::Object *toObject(const null & inValue) { return 0; }

   inline static int getDynamicCompareType(const null &) { return type; }
   inline static bool isNull(const null &) { return true; }
};



template <> 
struct CompareTraits<signed int>
{
   enum { type = (int)CompareAsInt };

   inline static int toInt(int inValue) { return inValue; }
   inline static double toDouble(int inValue) { return inValue; }
   inline static cpp::Int64 toInt64(int inValue) { return inValue; }
   inline static String toString(int inValue) { return String(); }
   inline static hx::Object *toObject(int inValue) { return 0; }

   inline static int getDynamicCompareType(int) { return type; }
   inline static bool isNull(int) { return false; }
};

template <> 
struct CompareTraits<unsigned int>
{
   enum { type = (int)CompareAsInt };

   // Return value is unsigned ...
   inline static unsigned int toInt(unsigned int inValue) { return inValue; }
   inline static double toDouble(unsigned int inValue) { return inValue; }
   inline static cpp::Int64 toInt64(unsigned int inValue) { return inValue; }
   inline static String toString(unsigned int inValue) { return String(); }
   inline static hx::Object *toObject(unsigned int inValue) { return 0; }

   inline static int getDynamicCompareType(int) { return type; }
   inline static bool isNull(int) { return false; }
};

template <> struct CompareTraits<signed short> : public CompareTraits<int> { };
template <> struct CompareTraits<unsigned short> : public CompareTraits<int> { };
template <> struct CompareTraits<signed char> : public CompareTraits<int> { };
template <> struct CompareTraits<unsigned char> : public CompareTraits<int> { };
template <> struct CompareTraits<char> : public CompareTraits<int> { };
template <> struct CompareTraits<wchar_t> : public CompareTraits<int> { };
template <> struct CompareTraits<char16_t> : public CompareTraits<int> { };


template <> 
struct CompareTraits<double>
{
   enum { type = (int)CompareAsDouble };

   inline static int toInt(double inValue) { return inValue; }
   inline static double toDouble(double inValue) { return inValue; }
   inline static cpp::Int64 toInt64(double inValue) { return inValue; }
   inline static String toString(double inValue) { return String(); }
   inline static hx::Object *toObject(double inValue) { return 0; }

   inline static int getDynamicCompareType(const double &) { return type; }
   inline static bool isNull(const double &) { return false; }
};
template <> struct CompareTraits<float> : public CompareTraits<double> { };



template <> 
struct CompareTraits<cpp::Int64>
{
   enum { type = (int)CompareAsInt64 };

   inline static int toInt(cpp::Int64 inValue) { return (int)inValue; }
   inline static double toDouble(cpp::Int64 inValue) { return inValue; }
   inline static cpp::Int64 toInt64(cpp::Int64 inValue) { return inValue; }
   inline static String toString(cpp::Int64 inValue) { return String(); }
   inline static hx::Object *toObject(cpp::Int64 inValue) { return 0; }

   inline static int getDynamicCompareType(cpp::Int64) { return type; }
   inline static bool isNull(cpp::Int64) { return false; }
};

template <> 
struct CompareTraits<cpp::UInt64>
{
   enum { type = (int)CompareAsInt64 };

   inline static int toInt(cpp::UInt64 inValue) { return (int)inValue; }
   inline static double toDouble(cpp::UInt64 inValue) { return inValue; }
   // Return value is unsigned ...
   inline static cpp::UInt64 toInt64(cpp::UInt64 inValue) { return inValue; }
   inline static String toString(cpp::UInt64 inValue) { return String(); }
   inline static hx::Object *toObject(cpp::UInt64 inValue) { return 0; }

   inline static int getDynamicCompareType(cpp::UInt64) { return type; }
   inline static bool isNull(cpp::UInt64) { return false; }
};


template <> 
struct CompareTraits< String >
{
   enum { type = (int)CompareAsString };

   inline static int toInt(const String &) { return 0; }
   inline static double toDouble(const String &) { return 0; }
   inline static cpp::Int64 toInt64(const String &) { return 0; }
   inline static String toString(const String &inValue ) { return inValue; }
   inline static hx::Object *toObject(const String &inValue) { return Dynamic(inValue).mPtr; }

   inline static int getDynamicCompareType(const String &) { return type; }
   inline static bool isNull(const String &inValue) { return !inValue.raw_ptr(); }
};


template <> 
struct CompareTraits< cpp::Variant >
{
   enum { type = (int)CompareAsDynamic };

   // Might ne a
   inline static int toInt(const cpp::Variant &inValue) { return inValue; }
   inline static double toDouble(const cpp::Variant &inValue) { return inValue; }
   inline static cpp::Int64 toInt64(const cpp::Variant &inValue) { return inValue; }
   inline static String toString(const cpp::Variant &inValue ) { return inValue; }
   inline static hx::Object *toObject(const cpp::Variant &inValue) {
      if (inValue.type==cpp::Variant::typeObject)
         return inValue.valObject;
      return 0;
   }

   inline static int getDynamicCompareType(const cpp::Variant &inValue)
   {
      switch(inValue.type)
      {
         case cpp::Variant::typeInt: case cpp::Variant::typeBool: return CompareAsInt;
         case cpp::Variant::typeInt64: return CompareAsInt64;
         case cpp::Variant::typeDouble: return CompareAsDouble;
         case cpp::Variant::typeString: return CompareAsString;

         case cpp::Variant::typeObject:
            {
               if (!inValue.valObject)
                  return CompareAsDynamic;
               switch(inValue.valObject->__GetType())
               {
                  case vtInt: case vtBool: return CompareAsInt;
                  case vtInt64: return CompareAsInt64;
                  case vtFloat: return CompareAsDouble;
                  case vtString: return CompareAsString;
                  default: return CompareAsDynamic;
               }
            }
         default:
              return CompareAsDynamic;
      }
   }
   inline static bool isNull(const cpp::Variant &inValue) { return inValue.isNull(); }
};



template <typename T> 
struct CompareTraits< cpp::Pointer<T> >
{
   enum { type = (int)CompareAsDynamic };

   inline static int toInt(Dynamic inValue) { return inValue; }
   inline static double toDouble(Dynamic inValue) { return inValue; }
   inline static cpp::Int64 toInt64(Dynamic inValue) { return inValue; }
   inline static String toString(Dynamic inValue) { return inValue; }
   inline static hx::Object *toObject(Dynamic inValue) { return inValue.mPtr; }
   inline static int getDynamicCompareType(const Dynamic &inValue)
   {
      return CompareAsDynamic;
   }
   inline static bool isNull(const cpp::Pointer<T> &inValue) { return !inValue.ptr; }
};


template <typename T> 
struct CompareTraits< T * >
{
   enum { type = (int)CompareAsInt64 };

   inline static int toInt(T * inValue) { return 0; }
   inline static double toDouble(T * inValue) { return 0; }
   inline static cpp::Int64 toInt64(T * inValue) { return (cpp::Int64)inValue; }
   inline static String toString(T * inValue) { return String(); }
   inline static hx::Object *toObject(T * inValue) { return 0; }
   inline static int getDynamicCompareType(T * inValue)
   {
      return CompareAsInt64;
   }
   inline static bool isNull(T *inValue) { return !inValue; }
};


template<typename T1>
hx::Object *GetExistingObject(const T1 &v1)
{
   typedef CompareTraits<T1> traits1;
   return traits1::toObject(v1);
}


template<typename T1>
bool IsNull(const T1 &v1)
{
   typedef CompareTraits<T1> traits1;
   return traits1::isNull(v1);
}

template<typename T1>
bool IsNotNull(const T1 &v1)
{
   typedef CompareTraits<T1> traits1;
   return !traits1::isNull(v1);
}

template<bool LESS, bool EQ, typename T1, typename T2>
inline bool TestLessEq(const T1 &v1, const T2 &v2)
{
   typedef CompareTraits<T1> traits1;
   typedef CompareTraits<T2> traits2;

   if (traits1::type==(int)CompareAsInt && traits2::type==(int)CompareAsInt)
   {
      return LESS ? ( EQ ? traits1::toInt(v1) <= traits2::toInt(v2) :
                           traits1::toInt(v1) <  traits2::toInt(v2)  ) :
                    ( EQ ? traits1::toInt(v1) == traits2::toInt(v2) :
                           traits1::toInt(v1) != traits2::toInt(v2)  );
   }
   else if (traits1::type<=(int)CompareAsInt64 && traits2::type<=(int)CompareAsInt64)
   {
      return LESS ? ( EQ ? traits1::toInt64(v1) <= traits2::toInt64(v2) :
                           traits1::toInt64(v1) <  traits2::toInt64(v2)  ) :
                    ( EQ ? traits1::toInt64(v1) == traits2::toInt64(v2) :
                           traits1::toInt64(v1) != traits2::toInt64(v2)  );
   }
   else if (traits1::type<=(int)CompareAsDouble && traits2::type<=(int)CompareAsDouble)
   {
      return LESS ? ( EQ ? traits1::toDouble(v1) <= traits2::toDouble(v2) :
                           traits1::toDouble(v1) <  traits2::toDouble(v2)  ) :
                    ( EQ ? traits1::toDouble(v1) == traits2::toDouble(v2) :
                           traits1::toDouble(v1) != traits2::toDouble(v2)  );
   }
   else if (traits1::type==(int)CompareAsString && traits2::type==(int)CompareAsString)
   {
      return LESS ? ( EQ ? traits1::toString(v1) <= traits2::toString(v2) :
                           traits1::toString(v1) <  traits2::toString(v2)  ) :
                    ( EQ ? traits1::toString(v1) == traits2::toString(v2) :
                           traits1::toString(v1) != traits2::toString(v2)  );
   }
   else if (traits1::type<=(int)CompareAsString && traits2::type<=(int)CompareAsString)
   {
      // String with a number...
      return false;
   }
   else if (traits1::type==(int)CompareAsString || traits2::type==(int)CompareAsString)
   {
      // String with a object...
      return LESS ? ( EQ ? traits1::toString(v1) <= traits2::toString(v2) :
                           traits1::toString(v1) <  traits2::toString(v2)  ) :
                    ( EQ ? traits1::toString(v1) == traits2::toString(v2) :
                           traits1::toString(v1) != traits2::toString(v2)  );
   }
   else if (traits1::type<=(int)CompareAsDouble || traits2::type<=(int)CompareAsDouble)
   {
      // numeric with a object...

      // null can only be equal to null...
      bool n1 = traits1::isNull(v1);
      bool n2 = traits2::isNull(v2);
      if (n1 || n2)
         return EQ ? n1==n2 : !LESS && n1!=n2/* false,false = not equal*/;

      return LESS ? ( EQ ? traits1::toDouble(v1) <= traits2::toDouble(v2) :
                           traits1::toDouble(v1) <  traits2::toDouble(v2)  ) :
                    ( EQ ? traits1::toDouble(v1) == traits2::toDouble(v2) :
                           traits1::toDouble(v1) != traits2::toDouble(v2)  );
   }
   else
   {
      // Dynamic compare.
      // This time, one or both types are calculated at run time

      // Check null/not null compare
      bool n1 = traits1::isNull(v1);
      bool n2 = traits2::isNull(v2);
      if (n1 || n2)
         return EQ ? n1==n2 : !LESS && n1!=n2 /* false,false = not equal*/;

      int t1 = traits1::getDynamicCompareType(v1);
      int t2 = traits2::getDynamicCompareType(v2);

      if (t1==(int)CompareAsInt && t2==(int)CompareAsInt)
      {
         return LESS ? ( EQ ? traits1::toInt(v1) <= traits2::toInt(v2) :
                              traits1::toInt(v1) <  traits2::toInt(v2)  ) :
                       ( EQ ? traits1::toInt(v1) == traits2::toInt(v2) :
                              traits1::toInt(v1) != traits2::toInt(v2)  );
      }
      else if (t1<=(int)CompareAsInt64 && t2<=(int)CompareAsInt64)
      {
         return LESS ? ( EQ ? traits1::toInt64(v1) <= traits2::toInt64(v2) :
                              traits1::toInt64(v1) <  traits2::toInt64(v2)  ) :
                       ( EQ ? traits1::toInt64(v1) == traits2::toInt64(v2) :
                              traits1::toInt64(v1) != traits2::toInt64(v2)  );
      }
      else if (t1<=(int)CompareAsDouble && t2<=(int)CompareAsDouble)
      {
         return LESS ? ( EQ ? traits1::toDouble(v1) <= traits2::toDouble(v2) :
                              traits1::toDouble(v1) <  traits2::toDouble(v2)  ) :
                       ( EQ ? traits1::toDouble(v1) == traits2::toDouble(v2) :
                              traits1::toDouble(v1) != traits2::toDouble(v2)  );
      }
      else if (t1==(int)CompareAsString && t2==(int)CompareAsString)
      {
         return LESS ? ( EQ ? traits1::toString(v1) <= traits2::toString(v2) :
                              traits1::toString(v1) <  traits2::toString(v2)  ) :
                       ( EQ ? traits1::toString(v1) == traits2::toString(v2) :
                              traits1::toString(v1) != traits2::toString(v2)  );
      }
      else if (t1<=(int)CompareAsString && t2<=(int)CompareAsString)
      {
         // String with a number...
         return false;
      }
      else if (t1==(int)CompareAsString || t2==(int)CompareAsString)
      {
         // String with a object...
         return LESS ? ( EQ ? traits1::toString(v1) <= traits2::toString(v2) :
                              traits1::toString(v1) <  traits2::toString(v2)  ) :
                       ( EQ ? traits1::toString(v1) == traits2::toString(v2) :
                              traits1::toString(v1) != traits2::toString(v2)  );
      }
      else if (t1<=(int)CompareAsDouble || t2<=(int)CompareAsDouble)
      {
         // numeric with a object only works for not-equal
         return !LESS && !EQ;
      }
      else
      {
         // Object with Object
         hx::Object *o1 = traits1::toObject(v1);
         hx::Object *o2 = traits2::toObject(v2);

         int diff = o1->__Compare(o2);
         return LESS ? ( EQ ? diff <= 0 :
                              diff <  0  ) :
                       ( EQ ? diff == 0 :
                              diff != 0  );
      }
   }
}


template<typename T1, typename T2>
bool IsEq(const T1 &v1, const T2 &v2) { return TestLessEq<false,true,T1,T2>(v1,v2); }

template<typename T1, typename T2>
bool IsNotEq(const T1 &v1, const T2 &v2) { return TestLessEq<false,false,T1,T2>(v1,v2); }

template<typename T1, typename T2>
bool IsLess(const T1 &v1, const T2 &v2) { return TestLessEq<true,false,T1,T2>(v1,v2); }

template<typename T1, typename T2>
bool IsLessEq(const T1 &v1, const T2 &v2) { return TestLessEq<true,true,T1,T2>(v1,v2); }


template<typename T1, typename T2>
bool IsGreater(const T1 &v1, const T2 &v2) { return TestLessEq<true,false,T2,T1>(v2,v1); }

template<typename T1, typename T2>
bool IsGreaterEq(const T1 &v1, const T2 &v2) { return TestLessEq<true,true,T2,T1>(v2,v1); }



template<typename T1, typename T2>
bool IsPointerEq(const T1 &v1, const T2 &v2)
{
   return GetExistingObject(v1) == GetExistingObject(v2);
}

template<typename T1, typename T2>
bool IsPointerNotEq(const T1 &v1, const T2 &v2)
{
   return GetExistingObject(v1) != GetExistingObject(v2);
}


template<typename T1, typename T2>
bool IsInstanceEq(const T1 &v1, const T2 &v2)
{
   hx::Object *p1 = GetExistingObject(v1);
   hx::Object *p2 = GetExistingObject(v2);
   if (p1==p2)
      return true;
   if (!p1 || !p2)
      return false;
   return !p1->__Compare(p2);
}

template<typename T1, typename T2>
bool IsInstanceNotEq(const T1 &v1, const T2 &v2)
{
   hx::Object *p1 = GetExistingObject(v1);
   hx::Object *p2 = GetExistingObject(v2);
   if (p1==p2)
      return false;
   if (!p1 || !p2)
      return true;
   return p1->__Compare(p2);
}



}


#endif
