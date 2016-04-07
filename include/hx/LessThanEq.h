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
   enum { AS = CompareAsDynamic };

   inline static int toInt(Dynamic inValue) { return inValue; }
   inline static double toDouble(Dynamic inValue) { return inValue; }
   inline static cpp::Int64 toInt64(Dynamic inValue) { return inValue; }
   inline static String toString(Dynamic inValue) { return inValue; }
   inline static hx::Object *toObject(Dynamic inValue) { return inValue.mPtr; }
};


template <> 
struct CompareTraits<int>
{
   enum { AS = CompareAsInt };

   inline static int toInt(int inValue) { return inValue; }
   inline static double toDouble(int inValue) { return inValue; }
   inline static cpp::Int64 toInt64(int inValue) { return inValue; }
   inline static String toString(int inValue) { return String(); }
   inline static hx::Object *toObject(int inValue) { return 0; }
};


template <> 
struct CompareTraits<double>
{
   enum { AS = CompareAsDouble };

   inline static int toInt(double inValue) { return inValue; }
   inline static double toDouble(double inValue) { return inValue; }
   inline static cpp::Int64 toInt64(double inValue) { return inValue; }
   inline static String toString(double inValue) { return String(); }
   inline static hx::Object *toObject(double inValue) { return 0; }
};





template<typename T1>
bool IsNull(const T1 &v1)
{
   return null()==v1;
}

template<typename T1>
bool IsNotNull(const T1 &v1)
{
   return null()!=v1;
}

template<bool LESS, bool EQ, typename T1, typename T2>
inline bool TestLessEq(const T1 &v1, const T2 &v2)
{
   typedef CompareTraits<T1> traits1;
   typedef CompareTraits<T2> traits2;

   if (traits1::AS==CompareAsInt && traits2::AS==CompareAsInt)
   {
      return LESS ? ( EQ ? traits1::toInt(v1) <= traits2::toInt(v2) :
                           traits1::toInt(v1) <  traits2::toInt(v2)  ) :
                    ( EQ ? traits1::toInt(v1) == traits2::toInt(v2) :
                           traits1::toInt(v1) != traits2::toInt(v2)  );
   }
   else if (traits1::AS<=CompareAsInt64 && traits2::AS<=CompareAsInt64)
   {
      return LESS ? ( EQ ? traits1::toInt64(v1) <= traits2::toInt64(v2) :
                           traits1::toInt64(v1) <  traits2::toInt64(v2)  ) :
                    ( EQ ? traits1::toInt64(v1) == traits2::toInt64(v2) :
                           traits1::toInt64(v1) != traits2::toInt64(v2)  );
   }
   else if (traits1::AS<=CompareAsDouble && traits2::AS<=CompareAsDouble)
   {
      return LESS ? ( EQ ? traits1::toDouble(v1) <= traits2::toDouble(v2) :
                           traits1::toDouble(v1) <  traits2::toDouble(v2)  ) :
                    ( EQ ? traits1::toDouble(v1) == traits2::toDouble(v2) :
                           traits1::toDouble(v1) != traits2::toDouble(v2)  );
   }
   else if (traits1::AS==CompareAsString && traits2::AS==CompareAsString)
   {
      return LESS ? ( EQ ? traits1::toString(v1) <= traits2::toString(v2) :
                           traits1::toString(v1) <  traits2::toString(v2)  ) :
                    ( EQ ? traits1::toString(v1) == traits2::toString(v2) :
                           traits1::toString(v1) != traits2::toString(v2)  );
   }
   else if (traits1::AS<=CompareAsString && traits2::AS<=CompareAsString)
   {
      // String with a number...
      return false;
   }
   else if (traits1::AS==CompareAsString || traits2::AS==CompareAsString)
   {
      // String with a object...
      return LESS ? ( EQ ? traits1::toString(v1) <= traits2::toString(v2) :
                           traits1::toString(v1) <  traits2::toString(v2)  ) :
                    ( EQ ? traits1::toString(v1) == traits2::toString(v2) :
                           traits1::toString(v1) != traits2::toString(v2)  );
   }
   else if (traits1::AS<=CompareAsDouble || traits2::AS<=CompareAsDouble)
   {
      // numeric with a object...
      return LESS ? ( EQ ? traits1::toDouble(v1) <= traits2::toDouble(v2) :
                           traits1::toDouble(v1) <  traits2::toDouble(v2)  ) :
                    ( EQ ? traits1::toDouble(v1) == traits2::toDouble(v2) :
                           traits1::toDouble(v1) != traits2::toDouble(v2)  );
   }
   else
   {
      // Object with Object
      hx::Object *o1 = traits1::toObject(v1);
      hx::Object *o2 = traits1::toObject(v2);
      if (!o1 || !o2)
         return EQ ? o1==o2 : o1!=o2;
      else
      {
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


}


#endif
