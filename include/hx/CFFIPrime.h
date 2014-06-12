#ifndef HX_CFFIPRIME_INCLUDED
#define HX_CFFIPRIME_INCLUDED

#include "CFFI.h"


namespace cffi
{

struct HxString
{
   int length;
   const char *__s;
};

inline value alloc_pointer(void *inPtr) { return alloc_abstract((vkind)(0x100 + 2),inPtr); }


template<typename T> struct SigType { enum { Char='?' }; };
template<> struct SigType<bool> { enum { Char='b' }; };
template<> struct SigType<int> { enum { Char='i' }; };
template<> struct SigType<float> { enum { Char='f' }; };
template<> struct SigType<double> { enum { Char='d' }; };
template<> struct SigType<value> { enum { Char='o' }; };
template<> struct SigType<void> { enum { Char='v' }; };
template<> struct SigType<const char *> { enum { Char='c' }; };
template<> struct SigType<HxString> { enum { Char='s' }; };

template<typename RET>
bool CheckSig0( RET (func)(), const char *inSig)
{
   return SigType<RET>::Char==inSig[0] &&
          0 == inSig[1];
}


template<typename RET, typename A0>
bool CheckSig2( RET (func)(A0), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<RET>::Char==inSig[1] &&
          0 == inSig[2];
}


template<typename RET, typename A0, typename A1>
bool CheckSig2( RET (func)(A0,A1), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<RET>::Char==inSig[2] &&
          0 == inSig[3];
}


template<typename RET, typename A0, typename A1, typename A2>
bool CheckSig3( RET (func)(A0,A1,A2), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<RET>::Char==inSig[3] &&
          0 == inSig[4];
}


template<typename RET, typename A0, typename A1, typename A2, typename A3>
bool CheckSig4( RET (func)(A0,A1,A2,A3), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<RET>::Char==inSig[4] &&
          0 == inSig[5];
}


template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4>
bool CheckSig5( RET (func)(A0,A1,A2,A3,A4), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<RET>::Char==inSig[5] &&
          0 == inSig[6];
}



inline value ToValue(int inVal) { return alloc_int(inVal); }
inline value ToValue(float inVal) { return alloc_float(inVal); }
inline value ToValue(double inVal) { return alloc_float(inVal); }
inline value ToValue(value inVal) { return inVal; }
inline value ToValue(bool inVal) { return alloc_bool(inVal); }
//inline value ToValue(HxString inVal) { return 0; }

struct AutoValue
{
   value mValue;
   
   inline operator int()  { return val_int(mValue); }
   inline operator value() { return mValue; }
   inline operator double() { return val_number(mValue); }
   inline operator bool() { return val_bool(mValue); }
   //inline operator HxString() { return HxString(); }
};



} // end namespace cffi

#define PRIME_ARG_DECL0
#define PRIME_ARG_DECL1 cffi::AutoValue a0
#define PRIME_ARG_DECL2 PRIME_ARG_DECL1, cffi::AutoValue a1
#define PRIME_ARG_DECL3 PRIME_ARG_DECL2, cffi::AutoValue a2
#define PRIME_ARG_DECL4 PRIME_ARG_DECL3, cffi::AutoValue a3
#define PRIME_ARG_DECL5 PRIME_ARG_DECL4, cffi::AutoValue a4

#define PRIME_ARG_LIST0
#define PRIME_ARG_LIST1 a0
#define PRIME_ARG_LIST2 PRIME_ARG_LIST1, a1
#define PRIME_ARG_LIST3 PRIME_ARG_LIST2, a2
#define PRIME_ARG_LIST4 PRIME_ARG_LIST3, a3
#define PRIME_ARG_LIST5 PRIME_ARG_LIST4, a4


#define DEFINE_PRIME0(func) extern "C" { \
  EXPORT value func##__prime(const char *inSig) { \
     if (!cffi::CheckSig0(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap() { return cffi::ToValue( func() ); } \
  EXPORT void *func##__0() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME0v(func) extern "C" { \
  EXPORT value func##__prime(const char *inSig) { \
     if (!cffi::CheckSig0(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap() { func(); return alloc_null(); } \
  EXPORT void *func##__0() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME1(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig1(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL1) { return cffi::ToValue( func(PRIME_ARG_LIST1) ); } \
  EXPORT void *func##__1() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME1v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig1(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL1) { func(PRIME_ARG_LIST1); return alloc_null(); } \
  EXPORT void *func##__1() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME2(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig2(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL2) { return cffi::ToValue( func(PRIME_ARG_LIST2) ); } \
  EXPORT void *func##__2() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME2v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig2(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL2) { func(PRIME_ARG_LIST2); return alloc_null(); } \
  EXPORT void *func##__2() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME3(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig3(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL3) { return cffi::ToValue( func(PRIME_ARG_LIST3) ); } \
  EXPORT void *func##__3() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME3v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig3(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL3) { func(PRIME_ARG_LIST3); return alloc_null(); } \
  EXPORT void *func##__3() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME4(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig4(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL4) { return cffi::ToValue( func(PRIME_ARG_LIST4) ); } \
  EXPORT void *func##__4() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME4v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig4(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL4) { func(PRIME_ARG_LIST4); return alloc_null(); } \
  EXPORT void *func##__4() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME5(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig5(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL5) { return cffi::ToValue( func(PRIME_ARG_LIST5) ); } \
  EXPORT void *func##__5() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME5v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig5(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL5) { func(PRIME_ARG_LIST5); return alloc_null(); } \
  EXPORT void *func##__5() { return (void*)(&func##__wrap); } \
}


#endif


