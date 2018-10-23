#ifndef HX_NULL_H
#define HX_NULL_H



// --- null value  ---------------------------------------------------------
//
// This is used by external operatator and return statments - Most will
//  use operator overloading to convert to the null pointer


// Forward declare ...
class String;
class null;
namespace hx { template<typename O> class ObjectPtr; }

namespace hx { null NullArithmetic(const char *inText); }

#define HX_NULL_COMPARE_OP(op,type,value) \
			bool operator op (const type &inRHS) const { return value; }

#define HX_NULL_COMPARE_OPS(type) \
	HX_NULL_COMPARE_OP(<,type,false) \
	HX_NULL_COMPARE_OP(<=,type,false) \
	HX_NULL_COMPARE_OP(>,type,false) \
	HX_NULL_COMPARE_OP(>=,type,false) \
	HX_NULL_COMPARE_OP(==,type,false) \
	HX_NULL_COMPARE_OP(!=,type,true)

#define HX_NULL_COMPARE_MOST_OPS(type) \
	HX_NULL_COMPARE_OP(<,type,false) \
	HX_NULL_COMPARE_OP(<=,type,false) \
	HX_NULL_COMPARE_OP(>,type,false) \
	HX_NULL_COMPARE_OP(>=,type,false)

#define HX_COMPARE_NULL_OP(op,type,value) \
	   inline bool operator op (type inLHS, const null &) { return value; }

#define HX_COMPARE_NULL_OPS(type) \
	HX_COMPARE_NULL_OP(<,type,false) \
	HX_COMPARE_NULL_OP(<=,type,false) \
	HX_COMPARE_NULL_OP(>,type,false) \
	HX_COMPARE_NULL_OP(>=,type,false) \
	HX_COMPARE_NULL_OP(==,type,false) \
	HX_COMPARE_NULL_OP(!=,type,true)

#define HX_COMPARE_NULL_MOST_OPS(type) \
	HX_COMPARE_NULL_OP(<,type,false) \
	HX_COMPARE_NULL_OP(<=,type,false) \
	HX_COMPARE_NULL_OP(>,type,false) \
	HX_COMPARE_NULL_OP(>=,type,false)


#define HX_NULL_ARITHMETIC_OP(op) \
template<typename T> inline null operator op (T t) const \
   { return hx::NullArithmetic(#op); } \
inline null operator op (const null &) const \
   { return hx::NullArithmetic(#op); }

#define HX_ARITHMETIC_NULL_OP(op) \
template<typename T> inline null operator op (const T &, const null &) \
   { return hx::NullArithmetic(#op); }


class null
{
   struct AnyArg
   {
      template<typename T>
      inline AnyArg(const T&) { }
   };
   public:
     inline null(){ } 

     template<typename T> explicit inline null(const hx::ObjectPtr<T> &){ } 
     template<typename T> explicit inline null(const String &){ } 
     explicit inline null(double){ } 
     explicit inline null(float){ } 
     explicit inline null(int){ } 
     explicit inline null(bool){ } 

     template<typename T>
     T StaticCast() { return null(); }

     operator bool () { return false; }
     operator int () { return 0; }
     operator unsigned int () { return 0; }
     operator double () { return 0; }
     operator float () { return 0; }
     operator char () { return 0; }
     operator unsigned char () { return 0; }
     operator signed char () { return 0; }
     operator short () { return 0; }
     operator unsigned short () { return 0; }
     operator cpp::UInt64 () { return 0; }
     operator cpp::Int64 () { return 0; }
     template<typename T>
     inline operator typename hx::Native<T *> () const { return 0; }


     // Any pointer!
     //operator char * () { return 0; }
     //operator wchar_t * () { return 0; }
     template<typename T> operator T *() { return 0; }

     bool operator == (null inRHS) const { return true; }
     bool operator != (null inRHS) const { return false; }
     bool operator == (null inRHS) { return true; }
     bool operator != (null inRHS) { return false; }

     template<typename T> inline bool operator == (const hx::ObjectPtr<T> &) const;
     template<typename T> inline bool operator != (const hx::ObjectPtr<T> &) const;
     template<typename T> inline bool operator == (const Array<T> &) const;
     template<typename T> inline bool operator != (const Array<T> &) const;
     inline bool operator == (const hx::FieldRef &) const;
     inline bool operator != (const hx::FieldRef &) const;
     inline bool operator == (const hx::IndexRef &) const;
     inline bool operator != (const hx::IndexRef &) const;
     inline bool operator == (const Dynamic &) const;
     inline bool operator != (const Dynamic &) const;
     inline bool operator == (const String &) const;
     inline bool operator != (const String &) const;
     inline bool operator == (const cpp::Variant &v) const { return v.isNull(); }
     inline bool operator != (const cpp::Variant &v) const{ return !v.isNull(); }

     inline null operator - () const { return hx::NullArithmetic("-"); }
     inline null operator ! () const { return hx::NullArithmetic("!"); }

     template<class T> T operator()(const AnyArg &a0=0, const AnyArg &a1=0, const AnyArg &a2=0,
            const AnyArg &a4=0, const AnyArg &a5=0, const AnyArg &a6=0,
            const AnyArg &a7=0, const AnyArg &a8=0, const AnyArg &a9=0 )
            {
                hx::NullReference("Function Call", false);
                T nullDynamic;
                return nullDynamic;
            }

	  HX_NULL_COMPARE_OPS(bool)
	  HX_NULL_COMPARE_OPS(double)
	  HX_NULL_COMPARE_OPS(float)
	  HX_NULL_COMPARE_OPS(int)
	  HX_NULL_COMPARE_OPS(unsigned int)
	  HX_NULL_COMPARE_OPS(short)
	  HX_NULL_COMPARE_OPS(unsigned short)
	  HX_NULL_COMPARE_OPS(signed char)
	  HX_NULL_COMPARE_OPS(unsigned char)
	  HX_NULL_COMPARE_OPS(cpp::Int64)
	  HX_NULL_COMPARE_OPS(cpp::UInt64)
	  HX_NULL_COMPARE_MOST_OPS(String)
	  HX_NULL_COMPARE_MOST_OPS(Dynamic)
	  HX_NULL_COMPARE_MOST_OPS(hx::FieldRef)
	  HX_NULL_COMPARE_MOST_OPS(hx::IndexRef)

	  HX_NULL_COMPARE_OP(<,null,false)
	  HX_NULL_COMPARE_OP(<=,null,true)
	  HX_NULL_COMPARE_OP(>,null,false)
	  HX_NULL_COMPARE_OP(>=,null,true)


	  HX_NULL_ARITHMETIC_OP(+);
	  HX_NULL_ARITHMETIC_OP(*);
	  HX_NULL_ARITHMETIC_OP(-);
	  HX_NULL_ARITHMETIC_OP(/);
	  HX_NULL_ARITHMETIC_OP(%);
	  HX_NULL_ARITHMETIC_OP(&);
	  HX_NULL_ARITHMETIC_OP(|);
	  HX_NULL_ARITHMETIC_OP(^);
	  HX_NULL_ARITHMETIC_OP(>>);
	  HX_NULL_ARITHMETIC_OP(<<);
};

namespace hx
{

template<typename T>
struct Null
{
   inline Null() : isNull(true) { }
   inline Null(const Null<T> &inOther) : isNull(inOther.isNull), value(inOther.value) { }
   inline Null(const T& inVal) : isNull(false), value(inVal) { }
   inline Null(const null &) : isNull(true) { }
   inline Null(const Dynamic &inVal)
   {
      isNull = null() == inVal;
      if (!isNull)
         value = inVal;
   }
   inline Null(const cpp::Variant &inVal)
   {
      isNull = inVal.isNull();
      if (!isNull)
         value = inVal;
   }

   inline operator Dynamic();
   inline T Default(T inDefault) { return isNull ? inDefault : value; }

   bool isNull;
   T    value;
};

} // end namesapce hx

typedef null Void;

HX_COMPARE_NULL_OPS(bool)
HX_COMPARE_NULL_OPS(double)
HX_COMPARE_NULL_OPS(float)
HX_COMPARE_NULL_OPS(int)
HX_COMPARE_NULL_OPS(unsigned int)
HX_COMPARE_NULL_OPS(short)
HX_COMPARE_NULL_OPS(unsigned short)
HX_COMPARE_NULL_OPS(signed char)
HX_COMPARE_NULL_OPS(unsigned char)
HX_COMPARE_NULL_OPS(cpp::UInt64)
HX_COMPARE_NULL_OPS(cpp::Int64)

HX_ARITHMETIC_NULL_OP(+)
HX_ARITHMETIC_NULL_OP(*)
HX_ARITHMETIC_NULL_OP(-)
HX_ARITHMETIC_NULL_OP(/)
HX_ARITHMETIC_NULL_OP(%)
HX_ARITHMETIC_NULL_OP(&)
HX_ARITHMETIC_NULL_OP(|)
HX_ARITHMETIC_NULL_OP(^)
HX_ARITHMETIC_NULL_OP(>>)
HX_ARITHMETIC_NULL_OP(<<)

// Other ops in Operator.h

inline void *_hx_nullptr() { return 0; }


#endif

