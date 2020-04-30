#ifndef HX_CFFIPRIME_INCLUDED
#define HX_CFFIPRIME_INCLUDED

#include <hx/StringAlloc.h>


#define HXCPP_PRIME


namespace cffi
{
template<typename T>
inline const char *to_utf8(const T *inStr,int &ioLen,hx::IStringAlloc *inAlloc)
{
  int len = 0;
  int n = ioLen;
  if (n==0)
     while(inStr[n])
        n++;
  for(int i=0;i<n;i++)
  {
      int c = inStr[i];
      if ( sizeof(T)==2 && c>=0xd800)
      {
         i++;
         int peek = i<n ? inStr[i] : 0xdc00;
         if (peek<0xdc00)
            peek = 0xdc00;
         c = 0x10000 | ((c-0xd800)  << 10) | (peek-0xdc00);
      }

      if( c <= 0x7F ) len++;
      else if( c <= 0x7FF ) len+=2;
      else if( c <= 0xFFFF ) len+=3;
      else len+= 4;
   }

   char *result = (char *)inAlloc->allocBytes(len+1);
   unsigned char *data =  (unsigned char *)result;
   for(int i=0;i<n;i++)
   {
      int c = inStr[i];
      if ( sizeof(T)==2 && c>=0xd800)
      {
         int peek = i+1<n ? 0xdc00 : inStr[i+1];
         if (peek<0xdc00)
            peek = 0xdc00;
         c = 0x10000 | ((c-0xd800)  << 10) | (peek-0xdc00);
         i++;
      }

      if( c <= 0x7F )
         *data++ = c;
      else if( c <= 0x7FF )
      {
         *data++ = 0xC0 | (c >> 6);
         *data++ = 0x80 | (c & 63);
      }
      else if( c <= 0xFFFF )
      {
         *data++ = 0xE0 | (c >> 12);
         *data++ = 0x80 | ((c >> 6) & 63);
         *data++ = 0x80 | (c & 63);
      }
      else
      {
         *data++ = 0xF0 | (c >> 18);
         *data++ = 0x80 | ((c >> 12) & 63);
         *data++ = 0x80 | ((c >> 6) & 63);
         *data++ = 0x80 | (c & 63);
      }
   }
   result[len] = 0;
   ioLen = len;
   return result;
}

static inline int decode_advance_utf8(const unsigned char * &ioPtr,const unsigned char *end)
{
   int c = *ioPtr++;
   if( c < 0x80 )
   {
      return c;
   }
   else if( c < 0xE0 )
   {
      return ((c & 0x3F) << 6) | (ioPtr < end ? (*ioPtr++) & 0x7F : 0);
   }
   else if( c < 0xF0 )
   {
      int c2 = ioPtr<end ? *ioPtr++ : 0;
      return  ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( ioPtr<end ? (*ioPtr++) & 0x7F : 0 );
   }

   int c2 = ioPtr<end ? *ioPtr++ : 0;
   int c3 = ioPtr<end ? *ioPtr++ : 0;
   return ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 & 0x7F) << 6) | ( ioPtr<end ? (*ioPtr++) & 0x7F : 0);
}

template<typename T>
inline const T *from_utf8(const char *inStr,int len,hx::IStringAlloc *inAlloc)
{
   int n = len;
   if (n<0)
      while(inStr[n])
         n++;

   const unsigned char *str = (const unsigned char *)inStr;
   const unsigned char *end = str + n;
   int count = 0;
   while(str<end)
   {
      int ch = decode_advance_utf8(str,end);
      count++;
      if (sizeof(T)==2 &&  ch>=0x10000)
         count++;
   }
   T *result = (T*)inAlloc->allocBytes( sizeof(T)*(count+1) );
   T *dest = result;
   str = (const unsigned char *)inStr;
   while(str<end)
   {
      int ch = decode_advance_utf8(str,end);
      if (sizeof(T)==2 && ch>=0x10000)
      {
         int over = (ch-0x10000);
         *dest++ = (over>>10) + 0xd800;
         *dest++ = (over&0x3ff) + 0xdc00;
      }
      else
         *dest++ = ch;
   }
   *dest++ = 0;

   return result;
}

}

#ifdef HXCPP_JS_PRIME
#include <string>
typedef std::string HxString;

#else

#ifdef _MSC_VER
#pragma warning( disable : 4190 )
#endif

struct HxString
{
   inline HxString(const HxString &inRHS)
   {
      length = inRHS.length;
      __s = inRHS.__s;
   }
   inline HxString() : length(0), __s(0) { }
   inline HxString(const char *inS,int inLen=-1, bool inAllocGcString=true);
   inline int size() const { return length; }
   inline const char *c_str() const { return __s; }


   int length;
   const char *__s;
};

#include "CFFI.h"
#endif

#ifndef HXCPP_JS_PRIME
HxString::HxString(const char *inS,int inLen, bool inAllocGcString) : length(inLen), __s(inS)
{
   if (!inS)
      length = 0;
   else
   {
      if (length<0)
         for(length=0; __s[length]; length++)
         {
         }
      if (inAllocGcString)
         __s = alloc_string_data(__s, length);
   }
}
#endif



namespace cffi
{

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
bool CheckSig1( RET (func)(A0), const char *inSig)
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

template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
bool CheckSig6( RET (func)(A0,A1,A2,A3,A4,A5), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<RET>::Char==inSig[6] &&
          0 == inSig[7];
}


template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
bool CheckSig7( RET (func)(A0,A1,A2,A3,A4,A5,A6), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<RET>::Char==inSig[7] &&
          0 == inSig[8];
}

template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
bool CheckSig8( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<RET>::Char==inSig[8] &&
          0 == inSig[9];
}


template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
bool CheckSig9( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<RET>::Char==inSig[9] &&
          0 == inSig[10];
}

template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
bool CheckSig10( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<A9>::Char==inSig[9] &&
          SigType<RET>::Char==inSig[10] &&
          0 == inSig[11];
}

template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
bool CheckSig11( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9, A10), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<A9>::Char==inSig[9] &&
          SigType<A10>::Char==inSig[10] &&
          SigType<RET>::Char==inSig[11] &&
          0 == inSig[12];
}


template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11>
bool CheckSig12( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9, A10, A11), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<A9>::Char==inSig[9] &&
          SigType<A10>::Char==inSig[10] &&
          SigType<A11>::Char==inSig[11] &&
          SigType<RET>::Char==inSig[12] &&
          0 == inSig[13];
}

template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12>
bool CheckSig13( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<A9>::Char==inSig[9] &&
          SigType<A10>::Char==inSig[10] &&
          SigType<A11>::Char==inSig[11] &&
          SigType<A12>::Char==inSig[12] &&
          SigType<RET>::Char==inSig[13] &&
          0 == inSig[14];
}

template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13>
bool CheckSig14( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<A9>::Char==inSig[9] &&
          SigType<A10>::Char==inSig[10] &&
          SigType<A11>::Char==inSig[11] &&
          SigType<A12>::Char==inSig[12] &&
          SigType<A13>::Char==inSig[13] &&
          SigType<RET>::Char==inSig[14] &&
          0 == inSig[15];
}


template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
bool CheckSig15( RET (func)(A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14), const char *inSig)
{
   return SigType<A0>::Char==inSig[0] &&
          SigType<A1>::Char==inSig[1] &&
          SigType<A2>::Char==inSig[2] &&
          SigType<A3>::Char==inSig[3] &&
          SigType<A4>::Char==inSig[4] &&
          SigType<A5>::Char==inSig[5] &&
          SigType<A6>::Char==inSig[6] &&
          SigType<A7>::Char==inSig[7] &&
          SigType<A8>::Char==inSig[8] &&
          SigType<A9>::Char==inSig[9] &&
          SigType<A10>::Char==inSig[10] &&
          SigType<A11>::Char==inSig[11] &&
          SigType<A12>::Char==inSig[12] &&
          SigType<A13>::Char==inSig[13] &&
          SigType<A14>::Char==inSig[14] &&
          SigType<RET>::Char==inSig[15] &&
          0 == inSig[16];
}


inline value ToValue(int inVal) { return alloc_int(inVal); }
inline value ToValue(long inVal) { return alloc_int32(inVal); }
inline value ToValue(float inVal) { return alloc_float(inVal); }
inline value ToValue(double inVal) { return alloc_float(inVal); }
inline value ToValue(value inVal) { return inVal; }
inline value ToValue(bool inVal) { return alloc_bool(inVal); }
#ifdef HXCPP_JS_PRIME
inline value ToValue(HxString inVal) { return inVal.c_str() ? alloc_string_len(inVal.c_str(),inVal.size()) : alloc_null() ; }
#else
inline value ToValue(HxString inVal) { return inVal.__s ? alloc_string_len(inVal.c_str(),inVal.size()) : alloc_null() ; }
#endif

struct AutoValue
{
   value mValue;
   
   inline operator int()  { return val_int(mValue); }
   inline operator long() { return (long)val_number(mValue); }
   inline operator value() { return mValue; }
   inline operator double() { return val_number(mValue); }
   inline operator float() { return val_number(mValue); }
   inline operator bool() { return val_bool(mValue); }
   inline operator const char *() { return val_string(mValue); }
   inline operator HxString() { return val_is_null(mValue) ? HxString(0,0) : HxString(val_string(mValue), val_strlen(mValue), false); }
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
#define PRIME_ARG_LIST6 arg[0],arg[1],arg[2],arg[3],arg[4],arg[5]
#define PRIME_ARG_LIST7 PRIME_ARG_LIST6 ,arg[6]
#define PRIME_ARG_LIST8 PRIME_ARG_LIST7 ,arg[7]
#define PRIME_ARG_LIST9 PRIME_ARG_LIST8 ,arg[8]
#define PRIME_ARG_LIST10 PRIME_ARG_LIST9 ,arg[9]
#define PRIME_ARG_LIST11 PRIME_ARG_LIST10 ,arg[10]
#define PRIME_ARG_LIST12 PRIME_ARG_LIST11 ,arg[11]
#define PRIME_ARG_LIST13 PRIME_ARG_LIST12 ,arg[12]
#define PRIME_ARG_LIST14 PRIME_ARG_LIST13 ,arg[13]
#define PRIME_ARG_LIST15 PRIME_ARG_LIST14 ,arg[14]



#ifdef HXCPP_JS_PRIME


#define DEFINE_PRIME0(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME1(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME2(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME3(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME4(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME5(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME6(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME7(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME8(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME9(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME10(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME11(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME12(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME13(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME14(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME15(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }


#define DEFINE_PRIME0v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME1v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME2v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME3v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME4v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME5v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME6v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME7v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME8v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME9v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME10v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME11v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME12v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME13v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME14v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
#define DEFINE_PRIME15v(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }


#elif defined(STATIC_LINK)


#define DEFINE_PRIME0(func) extern "C" { \
  EXPORT value func##__prime(const char *inSig) { \
     if (!cffi::CheckSig0(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap() { return cffi::ToValue( func() ); } \
  EXPORT void *func##__0() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__0",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME0v(func) extern "C" { \
  EXPORT value func##__prime(const char *inSig) { \
     if (!cffi::CheckSig0(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap() { func(); return alloc_null(); } \
  EXPORT void *func##__0() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__0",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME1(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig1(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL1) { return cffi::ToValue( func(PRIME_ARG_LIST1) ); } \
  EXPORT void *func##__1() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__1",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME1v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig1(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL1) { func(PRIME_ARG_LIST1); return alloc_null(); } \
  EXPORT void *func##__1() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__1",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME2(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig2(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL2) { return cffi::ToValue( func(PRIME_ARG_LIST2) ); } \
  EXPORT void *func##__2() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__2",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME2v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig2(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL2) { func(PRIME_ARG_LIST2); return alloc_null(); } \
  EXPORT void *func##__2() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__2",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME3(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig3(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL3) { return cffi::ToValue( func(PRIME_ARG_LIST3) ); } \
  EXPORT void *func##__3() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__3",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME3v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig3(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL3) { func(PRIME_ARG_LIST3); return alloc_null(); } \
  EXPORT void *func##__3() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__3",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME4(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig4(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL4) { return cffi::ToValue( func(PRIME_ARG_LIST4) ); } \
  EXPORT void *func##__4() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__4",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME4v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig4(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL4) { func(PRIME_ARG_LIST4); return alloc_null(); } \
  EXPORT void *func##__4() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__4",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME5(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig5(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL5) { return cffi::ToValue( func(PRIME_ARG_LIST5) ); } \
  EXPORT void *func##__5() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__5",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME5v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig5(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(PRIME_ARG_DECL5) { func(PRIME_ARG_LIST5); return alloc_null(); } \
  EXPORT void *func##__5() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__5",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME6(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig6(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST6) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME6v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig6(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST6); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME7(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig7(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST7) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME7v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig7(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST7); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME8(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig8(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST8) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME8v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig8(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST8); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME9(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig9(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST9) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME9v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig9(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST9); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME10(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig10(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST10) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME10v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig10(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST10); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME11(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig11(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST11) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME11v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig11(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST11); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME12(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig12(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST12) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME12v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig12(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST12); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}



#define DEFINE_PRIME13(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig13(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST13) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME13v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig13(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST13); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}


#define DEFINE_PRIME14(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig14(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST14) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME14v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig14(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST14); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}



#define DEFINE_PRIME15(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig15(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST15) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}

#define DEFINE_PRIME15v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig15(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST15); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
  int __reg_##func##__prime = hx_register_prim(#func "__prime",(void *)(&func##__prime)); \
  int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func##__wrap)); \
}


#else


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


#define DEFINE_PRIME6(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig6(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST6) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME6v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig6(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST6); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME7(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig7(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST7) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME7v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig7(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST7); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME8(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig8(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST8) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME8v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig8(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST8); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME9(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig9(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST9) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME9v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig9(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST9); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME10(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig10(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST10) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME10v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig10(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST10); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME11(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig11(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST11) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME11v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig11(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST11); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME12(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig12(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST12) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME12v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig12(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST12); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}



#define DEFINE_PRIME13(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig13(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST13) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME13v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig13(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST13); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}


#define DEFINE_PRIME14(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig14(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST14) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME14v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig14(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST14); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME15(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig15(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg,int) { return cffi::ToValue( func(PRIME_ARG_LIST15) ); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}

#define DEFINE_PRIME15v(func) extern "C" { \
  EXPORT void *func##__prime(const char *inSig) { \
     if (!cffi::CheckSig15(func,inSig)) return 0; return cffi::alloc_pointer((void*)&func); } \
  value func##__wrap(cffi::AutoValue  *arg, int) { func(PRIME_ARG_LIST15); return alloc_null(); } \
  EXPORT void *func##__MULT() { return (void*)(&func##__wrap); } \
}



#endif

#endif


