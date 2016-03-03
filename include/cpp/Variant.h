#ifndef CPP_VARIANT_TWICE_H

namespace cpp
{
#ifndef CPP_VARIANT_ONCE_H
#define CPP_VARIANT_ONCE_H

   template<typename T>
   inline bool isIntType(const T &inRHS) { return false; }
   template<> inline bool isIntType(const int &inRHS) { return true; }
   template<> inline bool isIntType(const Dynamic &inRHS);
   template<> inline bool isIntType(const cpp::Variant &inRHS);

   template<typename T>
   inline bool isStringType(const T &inRHS) { return false; }
   template<> inline bool isStringType(const String &inRHS) { return true; }
   template<> inline bool isStringType(const Dynamic &inRHS);
   template<> inline bool isStringType(const cpp::Variant &inRHS);


   struct Variant
   {
      enum Type
      {
         typeObject = 0,
         typeString,
         typeDouble,
         typeInt,
         typeBool,
      };

      union
      {
         struct
         {
            const char *valStringPtr;
            unsigned int valStringLen;
         };
         hx::Object *valObject;
         double valDouble;
         int valInt;
         bool valBool;
      };
      Type type;

      inline bool isNull() const { return type==typeObject && !valObject; }
      inline bool isNumeric() const;
      inline bool isBool() const;
      inline int asInt() const;
      inline bool isInt() const;
      inline bool isString() const;
      inline double asDouble() const;
      inline String asString() const;

      inline Variant() : type(typeObject), valObject(0) { }
      inline Variant(const null &) : type(typeObject), valObject(0) { }
      inline Variant(bool inValue) : type(typeBool), valBool(inValue) { }
      inline Variant(int inValue) : type(typeInt), valInt(inValue) { }
      inline Variant(double inValue) : type(typeDouble), valDouble(inValue) { }
      inline Variant(const ::String &inValue); // later

      template<typename SOURCE_>
      Variant(const hx::ObjectPtr<SOURCE_> &inObjectPtr);

      inline Variant(Dynamic &inRHS); // later
      inline Variant(hx::Object *inValue) : type(typeObject), valObject(inValue) { }
      inline operator Dynamic() const; // later
      inline operator String() const;
      inline operator double() const { return asDouble(); }
      inline operator int() const { return asInt(); }
      inline operator bool() const { return asInt(); }
      inline operator float () const { return asDouble(); }
      inline operator unsigned int () const { return asInt(); }
      inline operator short () const { return asInt(); }
      inline operator unsigned short () const { return asInt(); }
      inline operator unsigned char () const { return asInt(); }
      inline operator char () const { return asInt(); }
      inline operator signed char () const { return asInt(); }
      inline bool operator !() const { return !asInt(); }

      inline int Compare(const Dynamic &inRHS) const;

      inline double set(const double &inValue) { type=typeDouble; return valDouble=inValue; }
      inline double set(const float &inValue) { type=typeDouble; return valDouble=inValue; }

      inline void mark(hx::MarkContext *__inCtx); // later
      #ifdef HXCPP_VISIT_ALLOCS
      inline void visit(hx::VisitContext *__inCtx); // later
      #endif



      inline bool operator==(const null &inRHS) const { return isNull(); }
      inline bool operator!=(const null &inRHS) const { return !isNull(); }
      inline bool operator == (const Dynamic &inRHS) const { return Compare(inRHS)==0; }

      inline bool operator != (const Dynamic &inRHS) const { return (Compare(inRHS) != 0); }
      inline bool operator != (const String &inRHS)  const;
      inline bool operator != (double inRHS)  const { return !isNumeric() || asDouble() != inRHS; }
      inline bool operator != (float inRHS)  const { return !isNumeric() || asDouble() != inRHS; }
      inline bool operator != (int inRHS)  const { return !isNumeric() || asDouble() != (double)inRHS; }
      inline bool operator != (bool inRHS)  const { return !isBool() || (bool)asInt() != inRHS; }

      template<typename RETURN_>
      RETURN_ Cast() const { return RETURN_(*this); }

      void CheckFPtr();
      HX_DECLARE_VARIANT_FUNCTIONS


    // Operator + is different, since it must consider strings too...
    inline String operator+(const String &s) const;
    template<typename T>
    inline cpp::Variant operator + (const T &inRHS) const;

    inline double operator%(const Dynamic &inRHS) const;
    inline double operator-() const { return -asDouble(); }
    inline double operator++() { return set(asDouble()+1); }
    inline double operator++(int) {double val = asDouble(); set(val+1); return val; }
    inline double operator--() { return set(asDouble()-1); }
    inline double operator--(int) {double val = asDouble(); set(val-1); return val; }

    template<typename T>
    inline double operator / (const T &inRHS) const { return asDouble() / (double)inRHS; } \

    template<typename T>
    inline cpp::Variant operator - (const T &inRHS) const
    {
       if (isInt() && ::cpp::isIntType(inRHS))
          return asInt() - (int)inRHS;
       return asDouble() - inRHS.asDouble();
    }

    template<typename T>
    inline cpp::Variant operator * (const T &inRHS) const
    {
       if (isInt() && ::cpp::isIntType(inRHS))
          return asInt() - (int)inRHS;
       return asDouble() - inRHS.asDouble();
    }

   };

#else // Second time ...
   #define CPP_VARIANT_TWICE_H

   bool Variant::isInt() const
   {
      return type==typeInt || (type==typeObject && valObject && valObject->__GetType()==vtInt);
   }
   bool Variant::isString() const
   {
      return type==typeString || (type==typeObject && valObject && valObject->__GetType()==vtString);
   }


   template<> inline bool isIntType(const Dynamic &inRHS) { return inRHS->__GetType()==vtInt; }
   template<> inline bool isIntType(const cpp::Variant &inRHS) { return inRHS.isInt(); }
   template<> inline bool isStringType(const Dynamic &inRHS) { return inRHS.mPtr && inRHS->__GetType()==vtString; }
   template<> inline bool isStringType(const cpp::Variant &inRHS) { return inRHS.isString(); }


#define HX_ARITH_VARIANT( op ) \
   inline double operator op (const double &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const float &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const int &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \

   HX_ARITH_VARIANT( - )
   HX_ARITH_VARIANT( + )
   HX_ARITH_VARIANT( / )
   HX_ARITH_VARIANT( * )

   Variant::Variant(const ::String &inValue) : type(typeString), valStringPtr(inValue.__s), valStringLen(inValue.length) { }

   Variant::Variant(Dynamic &inRHS) : type(typeObject), valObject(inRHS.mPtr) { }

   template<typename SOURCE_>
   Variant::Variant(const hx::ObjectPtr<SOURCE_> &inObjectPtr) :
       type(typeObject), valObject(inObjectPtr.mPtr) { }

   inline void Variant::CheckFPtr()
   {
      if (isNull())  Dynamic::ThrowBadFunctionError();
   }

   HX_IMPLEMNET_INLINE_VARIANT_FUNCTIONS


   int Variant::asInt() const
   {
      if (type==typeInt)
         return valInt;

      switch(type)
      {
         case typeDouble: return valDouble;
         case typeBool: return valBool;
         case typeObject: return valObject ? valObject->__ToInt() : 0;
         default: ;
      }
      return 0;
   }

   double Variant::asDouble() const
   {
      if (type==typeDouble)
         return valDouble;
      else if (type==typeInt)
         return valInt;
      else if (type==typeObject)
         return valObject ? valObject->__ToDouble() : 0.0;
      return 0.0;
   }

   Variant::operator Dynamic() const
   {
      switch(type)
      {
         case typeInt: return valInt;
         case typeDouble: return valDouble;
         case typeBool: return valBool;
         case typeString: return String(valStringPtr, valStringLen);
         case typeObject: return valObject;
         default: ;
      }
      return null();
   }


   bool Variant::isNumeric() const
   {
      if (type==typeInt || type==typeDouble)
         return true;
      if (type!=typeObject || valObject==0)
         return false;

      int t = valObject->__GetType();
      return t==vtInt || t==vtFloat;
   }

   bool Variant::isBool() const
   {
      if (type==typeBool)
         return true;
      if (type!=typeObject || valObject==0)
         return false;

      return valObject->__GetType() == vtBool;
   }


   String Variant::asString() const
   {
      switch(type)
      {
         case typeInt: return String(valInt);
         case typeDouble: return String(valDouble);
         case typeBool: return String(valBool);
         case typeString: return String(valStringPtr, valStringLen);
         case typeObject: return valObject ? valObject->toString() : String();
         default: ;
      }
      return String();
   }
   Variant::operator String() const { return asString(); }


   void Variant::mark(hx::MarkContext *__inCtx)
   {
      if (type==typeString)
      {
         HX_MARK_STRING(valStringPtr);
      }
      else if (type==typeObject)
      {
         HX_MARK_OBJECT(valObject);
      }
   }

   int Variant::Compare(const Dynamic &inRHS) const
   {
      if (inRHS.mPtr==0)
         return isNull() ? 0 : 1;

      switch(type)
      {
         case typeInt: return valInt - inRHS->__ToInt();
         case typeDouble:
           {
              double diff = valDouble - inRHS->__ToDouble();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeBool: return valBool==(bool)inRHS ? 1 : 0;
         case typeString: return String(valStringPtr, valStringLen)==(String)inRHS ? 1 : 0;
         case typeObject:
               return valObject->__Compare( inRHS.mPtr->__GetRealObject() );
         default: ;

      }
      return 0;
   }

   String cpp::Variant::operator+(const String &s) const
   {
      return asString() + s;
   }
   template<typename T>
   cpp::Variant Variant::operator + (const T &inRHS) const
   {
      if (isString() || ::cpp::isStringType(inRHS))
         return asString() + String(inRHS);
      return asDouble() + (double)inRHS;
   }



   #ifdef __OBJC__
   #ifdef HXCPP_OBJC
   inline Variant::asId() const
   {
      return type==typeObjC ?  valObjC : 
            type==valObject && valObject ? valObject->__GetHandle() : 0;
   }
   #endif
   #endif


   #ifdef HXCPP_VISIT_ALLOCS
   void Variant::visit(hx::VisitContext *__inCtx)
   {
      if (type==typeString)
      {
         HX_VISIT_STRING(valStringPtr);
      }
      else if (type==typeObject)
      {
         HX_VISIT_OBJECT(valObject);
      }
   }
   #endif // HXCPP_VISIT_ALLOCS

#endif // not twice

} // end namespace cpp

#endif // CPP_VARIANT_TWICE_H
