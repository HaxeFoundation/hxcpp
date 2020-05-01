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
         typeInt64,
         typeBool,
      };

      union
      {
         // Although this is typed as 'char', it might be char16_t in the case of smart strings
         const char *valStringPtr;
         hx::Object *valObject;
         double valDouble;
         cpp::Int64 valInt64;
         int valInt;
         bool valBool;
      };
      Type type;
      unsigned int valStringLen;



      inline bool isNull() const {
          return (type==typeObject && !valObject) || (type==typeString && !valStringPtr); }
      inline bool isNumeric() const;
      inline bool isBool() const;
      inline int asInt() const;
      inline bool isInt() const;
      inline cpp::Int64 asInt64() const;
      inline bool isInt64() const;
      inline bool isString() const;
      inline double asDouble() const;
      inline hx::Object *asObject() const { return type==typeObject ? valObject : 0; }
      inline hx::Object *asDynamic() const{ return type==typeObject ? valObject : toDynamic(); }
      inline hx::Object *toDynamic() const; // later
      inline String asString() const;
      inline String getString() const;

      inline Variant() : valInt64(0), type(typeObject) { }
      //inline Variant() { copyBuf.b[0] = copyBuf.b[1] = 0; }
      inline Variant(const null &) : type(typeObject), valObject(0) { }
      inline Variant(bool inValue) : type(typeBool), valBool(inValue) { }
      inline Variant(double inValue) : type(typeDouble), valDouble(inValue) { }
      inline Variant(const ::String &inValue); // later

      inline Variant(cpp::Int64 inValue) : type(typeInt64), valInt64(inValue) { }
      inline Variant(cpp::UInt64 inValue) : type(typeInt64), valInt64(inValue) { }
      inline Variant(int inValue) : type(typeInt), valInt(inValue) { }
      inline Variant(cpp::UInt32 inValue) : type(typeInt), valInt(inValue) { }
      inline Variant(cpp::Int16 inValue) : type(typeInt), valInt(inValue) { }
      inline Variant(cpp::UInt16 inValue) : type(typeInt), valInt(inValue) { }
      inline Variant(cpp::Int8 inValue) : type(typeInt), valInt(inValue) { }
      inline Variant(cpp::UInt8 inValue) : type(typeInt), valInt(inValue) { }
      #if defined(__OBJC__) && defined(HXCPP_OBJC)
      inline Variant(const id inObjc);
      inline operator id() const;
      #endif


      template<typename SOURCE_>
      Variant(const hx::ObjectPtr<SOURCE_> &inObjectPtr);

      inline Variant(const Dynamic &inRHS); // later
      inline Variant(hx::Object *inValue) : type(typeObject), valObject(inValue) { }

      template<typename T,typename H>
      explicit inline Variant(const cpp::Struct<T,H> &inVal);
      template<typename T>
      explicit inline Variant(const cpp::Pointer<T> &inRHS) ;
      template<typename T>
      explicit inline Variant(const cpp::Function<T> &inRHS) ;
      template<typename T>
      explicit inline Variant(const hx::Native<T> &inRHS) ;

      //inline operator Dynamic() const; // later
      //inline operator String() const;
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
      inline operator cpp::Int64 () const { return asInt64(); }
      inline operator cpp::UInt64 () const { return asInt64(); }
      inline bool operator !() const { return !asInt(); }

      inline int Compare(hx::Object *inRHS) const;
      inline int Compare(const Dynamic &inRHS) const;
      inline int Compare(const cpp::Variant &inRHS) const;

      inline double set(const double &inValue) { type=typeDouble; return valDouble=inValue; }
      inline double set(const float &inValue) { type=typeDouble; return valDouble=inValue; }

      inline void mark(hx::MarkContext *__inCtx); // later
      #ifdef HXCPP_VISIT_ALLOCS
      inline void visit(hx::VisitContext *__inCtx); // later
      #endif


      //inline Variant &operator=(const Variant &inRhs) { copyBuf = inRhs.copyBuf; return *this; }

      template<typename T>
      bool operator==(const T &inRHS) const;

      template<typename T>
      bool operator==(const hx::ObjectPtr<T> &inRHS) const
         { return Compare(inRHS.mPtr)==0; }


      template<typename T>
      bool operator!=(const hx::ObjectPtr<T> &inRHS) const
         { return Compare(inRHS.mPtr)!=0; }



      inline bool operator==(const null &inRHS) const { return isNull(); }
      inline bool operator==(const String &inRHS) const;

      inline bool operator!=(const null &inRHS) const { return !isNull(); }
      inline bool operator!=(const Variant &inRHS) const { return !operator==(inRHS); }
      inline bool operator!=(const String &inRHS)  const;


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
       if (::cpp::isIntType(inRHS) && isInt() )
          return asInt() - (int)inRHS;
       return asDouble() - (double)inRHS;
    }

    template<typename T>
    inline cpp::Variant operator * (const T &inRHS) const
    {
       if (::cpp::isIntType(inRHS) && isInt())
          return asInt() * (int)inRHS;
       return asDouble() * (double)inRHS;
    }

   inline bool operator < (const String &inRHS) const;
   inline bool operator <= (const String &inRHS) const;
   inline bool operator > (const String &inRHS) const;
   inline bool operator >= (const String &inRHS) const;



   #define HX_VARIANT_COMPARE_OP( op ) \
   inline bool operator op (double inRHS)  const { return isNumeric() && (asDouble() op inRHS); } \
   inline bool operator op (cpp::Int64 inRHS)  const { return isNumeric() && (asInt64() op inRHS); } \
   inline bool operator op (cpp::UInt64 inRHS)  const { return isNumeric() && ((cpp::UInt64)(asInt64()) op inRHS); } \
   inline bool operator op (float inRHS)  const { return isNumeric() && (asDouble() op inRHS); } \
   inline bool operator op (int inRHS)  const { return isNumeric() && (asDouble() op (double)inRHS); } \
   inline bool operator op (unsigned int inRHS)  const { return isNumeric() && (asDouble() op (double)inRHS); } \
   inline bool operator op (short inRHS)  const { return isNumeric() && (asDouble() op (double)inRHS); } \
   inline bool operator op (unsigned short inRHS)  const { return isNumeric() && (asDouble() op (double)inRHS); } \
   inline bool operator op (signed char inRHS)  const { return isNumeric() && (asDouble() op (double)inRHS); } \
   inline bool operator op (unsigned char inRHS)  const { return isNumeric() && (asDouble() op (double)inRHS); } \
   inline bool operator op (bool inRHS)  const { return isBool() && (asDouble() op (double)inRHS); } \
   inline bool operator op (const Dynamic &inRHS)  const { return Compare(inRHS) op 0; } \

   #define HX_VARIANT_COMPARE_OP_ALL( op ) \
      inline bool operator op (const null &inRHS)  const { return false; } \
      inline bool operator op (const cpp::Variant &inRHS)  const { return Compare(inRHS) op 0; } \
      HX_VARIANT_COMPARE_OP(op)

   HX_VARIANT_COMPARE_OP( == )
   HX_VARIANT_COMPARE_OP( != )
   HX_VARIANT_COMPARE_OP_ALL( < )
   HX_VARIANT_COMPARE_OP_ALL( <= )
   HX_VARIANT_COMPARE_OP_ALL( >= )
   HX_VARIANT_COMPARE_OP_ALL( >  )


   };

#else // Second time ...
   #define CPP_VARIANT_TWICE_H

   bool Variant::isInt() const
   {
      return type==typeInt || (type==typeObject && valObject && valObject->__GetType()==vtInt);
   }
   bool Variant::isInt64() const
   {
      return type==typeInt64 || (type==typeObject && valObject && valObject->__GetType()==vtInt64);
   }
   bool Variant::isString() const
   {
      return type==typeString || (type==typeObject && valObject && valObject->__GetType()==vtString);
   }


   #if defined(__OBJC__) && defined(HXCPP_OBJC)
   // Variant type neither adds nor releases references counts while holding the value as an id on the stack
   // The Dynamic created here owns the id, and we refer to the Dynamic and use his reference count to keep the id alive
   inline Variant::Variant(const id inObjc) { type=typeObject; valObject = Dynamic(inObjc).mPtr; }
   #ifdef OBJC_ARC
      inline Variant::operator id () const {  return type==typeObject && valObject ? (__bridge id)valObject->__GetHandle() : 0;  }
   #else
      inline Variant::operator id () const {  return type==typeObject && valObject ? (id)valObject->__GetHandle() : 0;  }
   #endif
   #endif



   template<> inline bool isIntType(const Dynamic &inRHS) { return inRHS->__GetType()==vtInt; }
   template<> inline bool isIntType(const cpp::Variant &inRHS) { return inRHS.isInt(); }
   template<> inline bool isStringType(const Dynamic &inRHS) { return inRHS.mPtr && inRHS->__GetType()==vtString; }
   template<> inline bool isStringType(const cpp::Variant &inRHS) { return inRHS.isString(); }

   template<typename T,typename H>
   Variant::Variant(const cpp::Struct<T,H> &inVal) :
           type(typeObject), valObject(Dynamic(inVal).mPtr) { }

   template<typename T>
   Variant::Variant(const cpp::Pointer<T> &inRHS) : type(typeObject), valObject( Dynamic(inRHS).mPtr ) { }
   template<typename T>
   Variant::Variant(const cpp::Function<T> &inRHS) : type(typeObject), valObject( Dynamic(inRHS).mPtr ) { }
   template<typename T>
   Variant::Variant(const hx::Native<T> &inRHS) : type(typeObject), valObject( CreateDynamicPointer(inRHS.ptr).mPtr ) { }


#define HX_ARITH_VARIANT( op ) \
   inline double operator op (const double &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const float &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const int &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const unsigned int &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const signed char &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const unsigned char &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const signed short &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const unsigned short &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const cpp::Int64 &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \
   inline double operator op (const cpp::UInt64 &inLHS,const cpp::Variant &inRHS) { return inLHS op (double)inRHS; } \

   HX_ARITH_VARIANT( - )
   HX_ARITH_VARIANT( + )
   HX_ARITH_VARIANT( / )
   HX_ARITH_VARIANT( * )

   inline bool Variant::operator==(const String &inString) const
   {
      if (isNull()) return inString==null();
      return type==typeString && asString()==inString;
   }
   inline bool Variant::operator!=(const String &inString) const
   {
      if (isNull()) return inString!=null();
      return type!=typeString || asString()!=inString;
   }
   inline bool Variant::operator < (const String &inRHS)  const { return asString() < inRHS; }
   inline bool Variant::operator <= (const String &inRHS)  const { return asString() < inRHS; }
   inline bool Variant::operator > (const String &inRHS)  const { return asString() > inRHS; }
   inline bool Variant::operator >= (const String &inRHS)  const { return asString() >= inRHS; }





   Variant::Variant(const ::String &inValue) :
      type(typeString), valStringPtr(inValue.raw_ptr()), valStringLen(inValue.length) { }

   Variant::Variant(const Dynamic &inRHS) : type(typeObject), valObject(inRHS.mPtr) { }

   template<typename SOURCE_>
   Variant::Variant(const hx::ObjectPtr<SOURCE_> &inObjectPtr) :
       type(typeObject), valObject(inObjectPtr.mPtr) { }

   inline void Variant::CheckFPtr()
   {
      if (isNull())  Dynamic::ThrowBadFunctionError();
   }

   HX_IMPLEMENT_INLINE_VARIANT_FUNCTIONS


   int Variant::asInt() const
   {
      if (type==typeInt)
         return valInt;

      switch(type)
      {
         case typeDouble: return valDouble;
         case typeInt64: return (int)valInt64;
         case typeBool: return valBool;
         case typeObject: return valObject ? valObject->__ToInt() : 0;
         default: ;
      }
      return 0;
   }



   cpp::Int64 Variant::asInt64() const
   {
      if (type==typeInt64)
         return valInt64;

      switch(type)
      {
         case typeDouble: return valDouble;
         case typeInt: return valInt;
         case typeBool: return valBool;
         case typeObject: return valObject ? valObject->__ToInt64() : 0;
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
      else if (type==typeInt64)
         return valInt64;
      else if (type==typeObject)
         return valObject ? valObject->__ToDouble() : 0.0;
      return 0.0;
   }


   inline hx::Object *Variant::toDynamic() const
   {
      switch(type)
      {
         case typeInt: return Dynamic(valInt).mPtr;
         case typeDouble: return Dynamic(valDouble).mPtr;
         case typeBool: return Dynamic(valBool).mPtr;
         case typeString: return Dynamic(String(valStringPtr, valStringLen)).mPtr;
         case typeInt64: return Dynamic(valInt64).mPtr;
         case typeObject: return valObject;
         default: ;
      }
      return 0;
   }


   /*
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
   */


   bool Variant::isNumeric() const
   {
      if (type==typeInt || type==typeDouble || type==typeInt64)
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


   String Variant::getString() const { return String(valStringPtr, valStringLen); }
   String Variant::asString() const
   {
      switch(type)
      {
         case typeInt: return String(valInt);
         case typeDouble: return String(valDouble);
         case typeBool: return String(valBool);
         case typeString: return String(valStringPtr, valStringLen);
         case typeInt64: return String(valInt64);
         case typeObject: return valObject ? valObject->toString() : String();
         default: ;
      }
      return String();
   }
   //Variant::operator String() const { return asString(); }


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

   template<typename T>
   bool Variant::operator==(const T &inRHS) const
   {
      switch(type)
      {
         case typeInt: return valInt==(double)inRHS;
         case typeDouble:return valDouble==(double)inRHS;
         case typeBool: return valBool==(bool)inRHS;
         case typeInt64: return valInt64==(cpp::Int64)inRHS;
         case typeString: return getString()==String(inRHS);
         case typeObject:
               if (!valObject)
                  return inRHS == null();
               return valObject->__Compare( Dynamic(inRHS).mPtr )==0;
      }
      return false;
   }


   int Variant::Compare(hx::Object *inPtr) const
   {
      if (!inPtr)
         return isNull() ? 0 : 1;

      switch(type)
      {
         case typeInt:
           {
              double diff = valInt - inPtr->__ToDouble();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeDouble:
           {
              double diff = valDouble - inPtr->__ToDouble();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeInt64:
           {
              cpp::Int64 diff = valInt64 - inPtr->__ToInt64();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeBool:
            if (!inPtr) return 1;
            return valBool==(bool)(inPtr->__ToInt()) ? 1 : 0;
         case typeString:
            if (!inPtr) return valStringPtr ? 1 : 0;
            if (inPtr->__GetType()!=vtString)
               return 1;
            return String(valStringPtr, valStringLen)==inPtr->toString() ? 1 : 0;
         case typeObject:
            #if (HXCPP_API_LEVEL>=331)
            return valObject->__Compare( inPtr );
            #else
            return valObject->__Compare( inPtr->__GetRealObject() );
            #endif
         default: ;

      }
      return 0;
   }
   int Variant::Compare(const Dynamic &inD) const { return Compare(inD.mPtr); }
   int Variant::Compare(const cpp::Variant &inVar) const
   {
      if (inVar.type==typeObject)
         return Compare(inVar.valObject);

      switch(type)
      {
         case typeInt:
           {
              double diff = valInt - inVar.asDouble();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeDouble:
           {
              double diff = valDouble - inVar.asDouble();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeInt64:
           {
              cpp::Int64 diff = valInt64 - inVar.asInt64();
              return diff<0 ? -1 : diff==0 ? 0 : 1;
           }
         case typeBool:
            return valBool==(bool)(inVar.asInt()) ? 1 : 0;
         case typeString:
            if (inVar.type!=typeString)
               return 1;
            return String(valStringPtr, valStringLen)==inVar.asString();
         case typeObject:
            if (!valObject)
               return 1;
            return - inVar.Compare(*this);
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





#define HX_VARIANT_OP_ISEQ(T) \
inline bool operator == (const T &inLHS,const cpp::Variant &inRHS) { return inRHS==inLHS; } \
inline bool operator != (const T &inLHS,const cpp::Variant &inRHS) { return inRHS!=inLHS; }


#define HX_VARIANT_OP_ISEQ(T) \
inline bool operator == (const T &inLHS,const cpp::Variant &inRHS) { return inRHS==inLHS; } \
inline bool operator != (const T &inLHS,const cpp::Variant &inRHS) { return inRHS!=inLHS; }

HX_VARIANT_OP_ISEQ(String)
HX_VARIANT_OP_ISEQ(double)
HX_VARIANT_OP_ISEQ(float)
HX_VARIANT_OP_ISEQ(cpp::Int64)
HX_VARIANT_OP_ISEQ(cpp::UInt64)
HX_VARIANT_OP_ISEQ(int)
HX_VARIANT_OP_ISEQ(unsigned int)
HX_VARIANT_OP_ISEQ(short)
HX_VARIANT_OP_ISEQ(unsigned short)
HX_VARIANT_OP_ISEQ(signed char)
HX_VARIANT_OP_ISEQ(unsigned char)
HX_VARIANT_OP_ISEQ(bool)

inline bool operator < (bool inLHS,const cpp::Variant &inRHS) { return false; }
inline bool operator <= (bool inLHS,const cpp::Variant &inRHS) { return false; }
inline bool operator >= (bool inLHS,const cpp::Variant &inRHS) { return false; }
inline bool operator > (bool inLHS,const cpp::Variant &inRHS) { return false; }


#define HX_COMPARE_VARIANT_OP( op ) \
   inline bool operator op (double inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (float inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && ((double)inLHS op (double)inRHS); } \
   inline bool operator op (cpp::Int64 inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (cpp::UInt64 inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (int inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (unsigned int inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (short inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (unsigned short inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (signed char inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (unsigned char inLHS,const ::cpp::Variant &inRHS) \
      { return inRHS.isNumeric() && (inLHS op (double)inRHS); } \
   inline bool operator op (const null &,const ::cpp::Variant &inRHS) \
      { return false; } \

HX_COMPARE_VARIANT_OP( < )
HX_COMPARE_VARIANT_OP( <= )
HX_COMPARE_VARIANT_OP( >= )
HX_COMPARE_VARIANT_OP( >  )






} // close cpp
namespace hx {
   template<typename T>
   bool ObjectPtr<T>::operator==(const cpp::Variant &inRHS) const {
       return inRHS.Compare(mPtr)==0;
   }
   template<typename T>
   bool ObjectPtr<T>::operator!=(const cpp::Variant &inRHS) const {
       return inRHS.Compare(mPtr)!=0;
   }

} // close hx
namespace cpp {

#endif // not twice



} // end namespace cpp




#endif // CPP_VARIANT_TWICE_H
