#include <hxcpp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <string>
#include <hx/Unordered.h>
#include "hx/Hash.h"
#include <hx/Thread.h>
#include <locale>

using namespace hx;

#if defined(BLACKBERRY)
using namespace std;
#endif

// -------- String ----------------------------------------


#ifdef HX_WINDOWS

#ifndef _MSC_VER
#define _MSC_VER 1
#endif

// vc 7...
#if _MSC_VER < 1400

#define SPRINTF _snprintf

#else // vc8+

#define SPRINTF _snprintf_s

#endif

#else // not windows ..

#define SPRINTF snprintf

#endif

#ifdef HX_SMART_STRINGS
#include "hx/Unicase.h"
#endif

namespace hx
{
char HX_DOUBLE_PATTERN[20] = "%.15g";
#define HX_INT_PATTERN "%d"
#define HX_UINT_PATTERN "%ud"
}

void __hxcpp_set_float_format(String inFormat)
{
  int last = inFormat.length < 19 ? inFormat.length : 19;
  memcpy(HX_DOUBLE_PATTERN, inFormat.utf8_str(), last*sizeof(char) );
  HX_DOUBLE_PATTERN[last] = '\0';
}

// --- GC helper

hx::Class __StringClass;

String  String::emptyString = HX_("",00,00,00,00);
static Dynamic sConstEmptyString;
static String  sConstStrings[256];
static Dynamic sConstDynamicStrings[256];
static String *sCharToString[1088] = { 0 };


typedef Hash<TNonGcStringSet> StringSet;
static StringSet *sPermanentStringSet = 0;
static volatile int sPermanentStringSetMutex = 0;

#ifdef HXCPP_COMBINE_STRINGS
static bool sIsIdent[256];
static bool InitIdent()
{
   for(int i=0;i<256;i++)
      sIsIdent[i]= (i>='a' && i<='z') || (i>='A' && i<='Z') || (i>='0' && i<='9') || (i=='_');
   return true;
}
#endif

inline static bool IsUtf16Surrogate(int ch)
{
   return ch>=0xd800 && ch<0xe000;
}
inline static bool IsUtf16LowSurrogate(int ch)
{
   return ch>=0xdc00 && ch<0xe000;
}
inline static bool IsUtf16HighSurrogate(int ch)
{
   return ch>=0xd800 && ch<0xdc00;
}

static int UTF8Bytes(int c)
{
      if( c <= 0x7F )
         return 1;
      else if( c <= 0x7FF )
         return 2;
      else if( c <= 0xFFFF )
         return 3;
      else
         return 4;
}

inline static int UTF16BytesCheck(int ch)
{
   if (ch>=0x10000)
   {
      if (ch<0x110000)
          return 2;
   }
   return 1;
}

static void UTF8EncodeAdvance(char * &ioPtr,int c)
{
      if( c <= 0x7F )
         *ioPtr++ = (c);
      else if( c <= 0x7FF )
      {
         *ioPtr++ = ( 0xC0 | (c >> 6) );
         *ioPtr++ = ( 0x80 | (c & 63) );
      }
      else if( c <= 0xFFFF )
      {
         *ioPtr++  = ( 0xE0 | (c >> 12) );
         *ioPtr++  = ( 0x80 | ((c >> 6) & 63) );
         *ioPtr++  = ( 0x80 | (c & 63) );
      }
      else
      {
         *ioPtr++  = ( 0xF0 | (c >> 18) );
         *ioPtr++  = ( 0x80 | ((c >> 12) & 63) );
         *ioPtr++  = ( 0x80 | ((c >> 6) & 63) );
         *ioPtr++  = ( 0x80 | (c & 63) );
      }
}

static unsigned char *sUtf8LenArray = 0;

static const unsigned char *getUtf8LenArray()
{
   if (!sUtf8LenArray)
   {
      sUtf8LenArray = (unsigned char *)malloc(256);
      for(int i=0;i<256;i++)
         sUtf8LenArray[i] =  i< 0x80 ? 1 : i<0xe0 ? 2 : i<0xf0 ? 3 : 4;
   }
   return sUtf8LenArray;
}

static inline int DecodeAdvanceUTF8(const unsigned char * &ioPtr)
{
   int c = *ioPtr++;
   if( c < 0x80 )
   {
      return c;
   }
   else if( c < 0xE0 )
   {
      return ((c & 0x3F) << 6) | ((*ioPtr++) & 0x7F);
   }
   else if( c < 0xF0 )
   {
      int c2 = *ioPtr++;
      return  ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( (*ioPtr++) & 0x7F);
   }

   int c2 = *ioPtr++;
   int c3 = *ioPtr++;
   return ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 & 0x7F) << 6) | ((*ioPtr++) & 0x7F);
}

static inline int DecodeAdvanceUTF8(const unsigned char * &ioPtr,const unsigned char *end)
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


int _hx_utf8_decode_advance(char *&ioPtr)
{
   return DecodeAdvanceUTF8( (const unsigned char * &) ioPtr );
}


inline int Char16Advance(const char16_t *&ioStr,bool throwOnErr=true)
{
   int ch = *ioStr++;
   if (IsUtf16Surrogate(ch))
   {
      if (IsUtf16LowSurrogate(ch))
      {
         if (throwOnErr)
            hx::Throw(HX_CSTRING("Invalid UTF16"));
         return 0xFFFD;
      }

      int peek = *ioStr++;
      if (IsUtf16HighSurrogate(peek))
      {
         if (throwOnErr)
            hx::Throw(HX_CSTRING("Invalid UTF16"));
         return 0xFFFD;
      }

      ch = 0x10000 + ((ch-0xd800)  << 10) | (peek-0xdc00);
   }
   return ch;
}


void Char16AdvanceSet(char16_t *&ioStr,int inChar)
{
   if (inChar>=0x10000)
   {
      int over = (inChar-0x10000);
      if (over>=0x100000)
      {
         *ioStr++ = 0xfffd;
      }
      else
      {
         *ioStr++ = (over>>10) + 0xd800;
         *ioStr++ = (over&0x3ff) + 0xdc00;
      }
   }
   else if (IsUtf16Surrogate(inChar))
   {
      *ioStr++ = 0xfffd;
   }
   else
      *ioStr++ = inChar;
}


template<typename T>
char *TConvertToUTF8(const T *inStr, int *ioLen, hx::IStringAlloc *inBuffer,bool)
{
   int len = 0;
   int chars = 0;
   if (ioLen==0 || *ioLen==0)
   {
      while(inStr[len])
         chars += UTF8Bytes(inStr[len++]);
   }
   else
   {
      len = *ioLen;
      for(int i=0;i<len;i++)
         chars += UTF8Bytes(inStr[i]);
   }

   char *buf = inBuffer ? (char *)inBuffer->allocBytes(chars+1) :
                          (char *)NewGCPrivate(0,chars+1);
   char *ptr = buf;
   for(int i=0;i<len;i++)
       UTF8EncodeAdvance(ptr,inStr[i]);
   *ptr = 0;
   if (ioLen)
      *ioLen = chars;

   return buf;
}


template<>
char *TConvertToUTF8(const char16_t *inStr, int *ioLen, hx::IStringAlloc *inBuffer,bool throwInvalid)
{
   int len = 0;
   if (ioLen==0 || *ioLen==0)
   {
      const char16_t *s = inStr;
      while( Char16Advance(s,throwInvalid) ) { }
      len = s - inStr - 1;
   }
   else
   {
      len = *ioLen;
   }
   const char16_t *s = inStr;
   const char16_t *end = s + len;
   int chars = 0;
   while(s<end)
      chars += UTF8Bytes( Char16Advance( s,throwInvalid ) );

   char *buf = inBuffer ? (char *)inBuffer->allocBytes(chars+1) :
                          (char *)NewGCPrivate(0,chars+1);
   char *ptr = buf;
   s = inStr;
   while(s<end)
      UTF8EncodeAdvance(ptr,Char16Advance(s,throwInvalid));

   *ptr = 0;
   if (ioLen)
      *ioLen = chars;

   return buf;
}

char16_t *String::allocChar16Ptr(int len)
{
   char16_t *result = (char16_t *)hx::InternalNew( (len+1)*2, false );
   ((unsigned int *)result)[-1] |= HX_GC_STRING_CHAR16_T;
   result[len] = 0;
   return result;
}


template<typename T>
static const char *GCStringDup(const T *inStr,int inLen, int *outLen=0)
{
   if (inStr==0 && inLen<=0)
   {
      if (outLen)
         outLen = 0;
      return String::emptyString.raw_ptr();
   }

   int len = inLen;
   bool allAscii = true;
   if (len==-1)
   {
      len=0;
      while(inStr[len])
      {
         if (sizeof(T)>1 && allAscii)
            allAscii = inStr[len] <= 127;
         len++;
      }
   }
   else if (sizeof(T)>1)
   {
      for(int i=0;i<inLen;i++)
         if (inStr[i]>127)
         {
            allAscii = false;
            break;
         }
   }

   if (outLen)
      *outLen = len;

   if (len==1)
      return String::fromCharCode(inStr[0]).raw_ptr();

   #ifdef HXCPP_COMBINE_STRINGS
   bool ident = len<20 && sIsIdent[inStr[0]] && (inStr[0]<'0' || inStr[0]>'9');
   if (ident && len>3)
      for(int p=1; p<len;p++)
         if (!sIsIdent[ inStr[p] ])
         {
            ident = false;
            break;
         }

   if (ident)
   {
      hx::StackContext *ctx = hx::StackContext::getCurrent();
      if (!ctx->stringSet)
         ctx->stringSet = new WeakStringSet();

      unsigned int hash = 0;
      for(int i=0;i<len;i++)
         hash = hash*223 + ((unsigned char *)inStr)[i];

      struct Finder
      {
         int len;
         const char *ptr;
         Finder(int len, const char *inPtr) : len(len), ptr(inPtr) { }
         bool operator==(const String &inStr) const
         {
            return len == inStr.length && !memcmp(ptr, inStr.__s, len*sizeof(char));
         }
      };
      String found;
      if (ctx->stringSet->findEquivalentKey(found, hash, Finder(len, inStr)))
         return found.__s;

      char *result = hx::NewString(len + 4);
      memcpy(result,inStr,sizeof(char)*(len));
      result[len] = '\0';
      *((unsigned int *)(result + len + 1)) = hash;
      ((unsigned int *)(result))[-1] |= HX_GC_STRING_HASH;

      String asString(result, len);
      ctx->stringSet->TSet( asString, true );

      return result;
   }
   #endif

   if (sizeof(T)==1 || allAscii)
   {
      char *result = hx::NewString( len );
      if (sizeof(T)==1)
         memcpy(result,inStr,sizeof(char)*(len));
      else
         for(int i=0;i<len;i++)
            result[i] = inStr[i];
      return result;
   }


   char16_t *result = String::allocChar16Ptr(len);
   memcpy(result,inStr,2*len);
   if (IsUtf16LowSurrogate(result[0]))
      result[0] = 0xFFFD;
   if (len>1 && IsUtf16HighSurrogate(result[len-1]))
      result[len-1] = 0xFFFD;
   return (const char *)result;
}




template<typename T>
inline String TCopyString(const T *inString,int inLength)
{
   if (!inString)
      return String();

   #ifndef HX_SMART_STRINGS
      if (inLength<0)
         for(inLength=0; !inString[inLength]; inString++) { }

      if (sizeof(T)==1)
      {
         int len = 0;
         const char *res = GCStringDup((const char *)inString,inLength,&len);
         return String(res,len);
      }
      else
      {
         int length = inLength;
         const char *ptr = TConvertToUTF8(inString, &length, 0, true );
         return String(ptr,length);
      }
   #else
      int c16len=0;
      bool hasWChar = false;
      const T *end = inLength>=0 ? inString + inLength : 0;
      if (end)
      {
         for(const T *s = inString; s<end; s++)
         {
            unsigned int c = *s;
            if (c>127)
               hasWChar = true;
            c16len += UTF16BytesCheck(c);
         }
      }
      else
         for(const T *s = inString; *s; s++)
         {
            unsigned int c = *s;
            if (c>127)
               hasWChar = true;
            c16len += UTF16BytesCheck(c);
         }

      if (hasWChar)
      {
         char16_t *result = String::allocChar16Ptr(c16len);
         c16len = 0;
         for(const T *s = inString; ; s++)
         {
            if (end)
            {
               if (s>=end) break;
            }
            else
            {
               if (!*s) break;
            }
            unsigned int c = *s;
            if (c<0x10000)
               result[c16len++] = c;
            else
            {
               int over = (c-0x10000);
               result[c16len++] = (over>>10) + 0xd800;
               result[c16len++] = (over&0x3ff) + 0xdc00;
            }
         }
         return String(result,c16len);
      }
      else
      {
         char *s = hx::NewString(c16len);
         for(int i=0;i<c16len;i++)
            s[i] = (char)inString[i];
         return String(s,c16len);
      }
   #endif
}


String __hxcpp_char_array_to_utf8_string(Array<int> &inChars,int inFirst, int inLen)
{
   int len = inChars->length;
   if (inFirst<0)
     inFirst = 0;
   if (inLen<0) inLen = len;
   if (inFirst+inLen>len)
      inLen = len-inFirst;
   if (inLen<=0)
      return String::emptyString;

   int *base = &inChars[0];
   #ifdef HX_SMART_STRINGS
   bool hasBig = false;
   for(int i=0;i<inLen;i++)
      if (base[i+inFirst]>127)
      {
         hasBig = true;
         break;
      }

   if (hasBig)
   {
      char16_t *ptr = String::allocChar16Ptr(inLen);
      for(int i=0;i<inLen;i++)
         ptr[i] = base[i+inFirst];
      return String(ptr, inLen);
   }
   #endif
   char *result = TConvertToUTF8(base+inFirst,&len,0,true);
   return String(result,len);
}

Array<int> __hxcpp_utf8_string_to_char_array(String &inString)
{
   #ifdef HX_SMART_STRINGS
   Array<int> result = Array_obj<int>::__new(inString.length);
   if (inString.isUTF16Encoded())
   {
      const char16_t *ptr = inString.wc_str();
      for(int i=0;i<inString.length;i++)
         result[i] = ptr[i];
   }
   else
   {
      const char *ptr = inString.raw_ptr();
      for(int i=0;i<inString.length;i++)
         result[i] = ptr[i];
   }
   #else
   Array<int> result = Array_obj<int>::__new(0,inString.length);

   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;
   while(src<end)
       result->push(DecodeAdvanceUTF8(src));

   if (src!=end)
      hx::Throw(HX_CSTRING("Invalid UTF8"));
   #endif

   return result;
}


String __hxcpp_char_bytes_to_utf8_string(String &inBytes)
{
   #ifdef HX_SMART_STRINGS
   // This does not really make much sense
   return inBytes;
   #else
   int len = inBytes.length;
   char *result = TConvertToUTF8((unsigned char *)inBytes.__s,&len,0,true);
   return String(result,len);
   #endif
}


String __hxcpp_utf8_string_to_char_bytes(String &inUTF8)
{
   #ifdef HX_SMART_STRINGS
   // This does not really make much sense
   return inUTF8;
   #else
    const unsigned char *src = (unsigned char *)inUTF8.__s;
    const unsigned char *end = src + inUTF8.length;
    int char_count = 0;
    while(src<end)
    {
        int c = DecodeAdvanceUTF8(src,end);
        char_count++;
        if( c == 8364 ) // euro symbol
           c = 164;
        else if ( c == 0xFEFF ) // BOM
        {
           char_count--;
        }
        else if( c > 255 )
            hx::Throw(HX_CSTRING("Utf8::decode invalid character"));
    }

    if (src!=end)
       hx::Throw(HX_CSTRING("Invalid UTF8"));

    char *result = hx::NewString(char_count);

    src = (unsigned char *)inUTF8.__s;
    char_count = 0;
    while(src<end)
    {
        int c = DecodeAdvanceUTF8(src);
        if( c == 8364 ) // euro symbol
           c = 164;
        if ( c != 0xFEFF ) // BOM
           result[char_count++] = c;
    }

   result[char_count] = '\0';
   return String(result,char_count);
   #endif
}


void _hx_utf8_iter(String inString, Dynamic inIter)
{
   #ifdef HX_SMART_STRINGS
   if (inString.isUTF16Encoded())
      for(int i=0;i<inString.length;i++)
         inIter( (int)inString.raw_wptr()[i] );
   else
      for(int i=0;i<inString.length;i++)
         inIter( (int)inString.raw_ptr()[i] );
   #else
   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;

   while(src<end)
      inIter(DecodeAdvanceUTF8(src,end+1));

   if (src>end)
      hx::Throw(HX_CSTRING("Invalid UTF8"));
   #endif
}

int _hx_utf8_char_code_at(String inString, int inIndex)
{
   #ifdef HX_SMART_STRINGS
   if (!inString.raw_ptr() || inIndex>=inString.length)
      return 0;
   if (inString.isUTF16Encoded())
      return inString.raw_wptr()[inIndex];
   else
      return inString.raw_ptr()[inIndex];
   #else
   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;
   const unsigned char *sLen = getUtf8LenArray();

   for(int i=0;i<inIndex;i++)
   {
      src += sLen[*src];
      if (src==end)
         return 0;
      if (src>end)
         hx::Throw(HX_CSTRING("Invalid UTF8"));
   }
   return DecodeAdvanceUTF8(src,end);
   #endif
}

int _hx_utf8_length(String inString)
{
   #ifdef HX_SMART_STRINGS
   return inString.length;
   #else

   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;

   int len = 0;
   const unsigned char *sLen = getUtf8LenArray();
   while(src<end)
   {
      src += sLen[*src];
      len++;
   }
   if (src>end)
      hx::Throw(HX_CSTRING("Invalid UTF8"));
   return len;
   #endif
}

bool _hx_utf8_is_valid(String inString)
{
   #ifdef HX_SMART_STRINGS
   return true;
   #else
   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;
   const unsigned char *sLen = getUtf8LenArray();
   while(src<end)
      src += sLen[*src];

   return src==end;
   #endif
}

String _hx_utf8_sub(String inString, int inStart, int inLen)
{
   #ifdef HX_SMART_STRINGS
   return inString.substr(inStart,inLen);
   #else
   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;

   const unsigned char *sLen = getUtf8LenArray();
   for(int i=0;i<inStart;i++)
   {
      src += sLen[*src];

      if (src==end)
         return String::emptyString;
      if (src>end)
         hx::Throw(HX_CSTRING("Invalid UTF8"));
   }
   const unsigned char *start = src;
   for(int i=0;i<inLen;i++)
   {
      src += sLen[*src];
      if (src==end)
         break;
      if (src>end)
         hx::Throw(HX_CSTRING("Invalid UTF8"));
   }
   return String::create((const char *)start, src-start);
   #endif
}





String String::create(const wchar_t *inString,int inLength) { return TCopyString(inString,inLength); }
String String::create(const char16_t *inString,int inLength) { return TCopyString(inString,inLength); }
String String::create(const char *inString,int inLength)
{
   if (!inString)
      return String();

   #ifdef HX_SMART_STRINGS
   if (inLength<0)
      for(inLength=0; inString[inLength]; inLength++) { }

   const unsigned char *c = (const unsigned char *)inString;
   for(int i=0;i<inLength;i++)
   {
      if (c[i]>127)
         return _hx_utf8_to_utf16(c, inLength,false);
   }

   #endif

   int len = 0;
   const char *s = GCStringDup(inString,inLength,&len);
   return String(s,len);
}






String::String(const Dynamic &inRHS)
{
   if (inRHS.GetPtr())
      (*this)=const_cast<Dynamic &>(inRHS)->toString();
   else
   {
      __s = 0;
      length = 0;
   }
}

void String::fromInt(int inIdx)
{
   char buf[100];
   SPRINTF(buf,100,HX_INT_PATTERN,inIdx);
   buf[99]='\0';
   __s = GCStringDup(buf,-1,&length);
}

String::String(const int &inRHS)
{
   fromInt(inRHS);
}


String::String(const unsigned int &inRHS)
{
   char buf[100];
   SPRINTF(buf,100,HX_UINT_PATTERN,inRHS);
   buf[99]='\0';
   __s = GCStringDup(buf,-1,&length);
}


String::String(const cpp::CppInt32__ &inRHS)
{
   char buf[100];
   SPRINTF(buf,100,HX_INT_PATTERN,inRHS.mValue);
   __s = GCStringDup(buf,-1,&length);
}



String::String(const double &inRHS)
{
   char buf[100];
   SPRINTF(buf,100,HX_DOUBLE_PATTERN,inRHS);
   buf[99]='\0';
   __s = GCStringDup(buf,-1,&length);
}


String::String(const cpp::Int64 &inRHS)
{
   char buf[100];
   SPRINTF(buf,100,"%lld", (long long int)inRHS);
   buf[99]='\0';
   __s = GCStringDup(buf,-1,&length);
}


String::String(const cpp::UInt64 &inRHS)
{
   char buf[100];
   SPRINTF(buf,100,"%llu", (unsigned long long int)inRHS);
   buf[99]='\0';
   __s = GCStringDup(buf,-1,&length);
}

String::String(const float &inRHS)
{
   char buf[100];
   SPRINTF(buf,100,HX_DOUBLE_PATTERN,inRHS);
   buf[99]='\0';
   __s = GCStringDup(buf,-1,&length);
}

String::String(const bool &inRHS)
{
   if (inRHS)
   {
      *this = HX_CSTRING("true");
   }
   else
   {
      *this = HX_CSTRING("false");
   }
}

void String::fromPointer(const void *p)
{
   char buf[128];
   SPRINTF(buf,128,"Native(%p)",p);
   __s = GCStringDup(buf,-1,&length);
}

#ifdef HX_SMART_STRINGS
#define ADD_HASH(X) \
    result = result*223 + (int)(X)
#endif

unsigned int String::calcSubHash(int start, int inLen) const
{
   unsigned int result = 0;
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      const char16_t *w = __w + start;
      for(int i=0;i<inLen;i++)
      {
         int c = w[i];
         if( c <= 0x7F )
         {
            ADD_HASH(c);
         }
         else if( c <= 0x7FF )
         {
            ADD_HASH(0xC0 | (c >> 6));
            ADD_HASH(0x80 | (c & 63));
         }
         else if( c <= 0xFFFF )
         {
            ADD_HASH(0xE0 | (c >> 12));
            ADD_HASH(0x80 | ((c >> 6) & 63));
            ADD_HASH(0x80 | (c & 63));
         }
         else
         {
            ADD_HASH(0xF0 | (c >> 18));
            ADD_HASH(0x80 | ((c >> 12) & 63));
            ADD_HASH(0x80 | ((c >> 6) & 63) );
            ADD_HASH(0x80 | (c & 63) );
         }
      }
   }
   else
   #endif
   {
      const unsigned char *s = (const unsigned char *)__s + start;
      for(int i=0;i<inLen;i++)
         result = result*223 + s[i];
   }

   return result;

}

unsigned int String::calcHash() const
{
   unsigned int result = 0;
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      for(int i=0;i<length;i++)
      {
         int c = __w[i];
         if( c <= 0x7F )
         {
            ADD_HASH(c);
         }
         else if( c <= 0x7FF )
         {
            ADD_HASH(0xC0 | (c >> 6));
            ADD_HASH(0x80 | (c & 63));
         }
         else if( c <= 0xFFFF )
         {
            ADD_HASH(0xE0 | (c >> 12));
            ADD_HASH(0x80 | ((c >> 6) & 63));
            ADD_HASH(0x80 | (c & 63));
         }
         else
         {
            ADD_HASH(0xF0 | (c >> 18));
            ADD_HASH(0x80 | ((c >> 12) & 63));
            ADD_HASH(0x80 | ((c >> 6) & 63) );
            ADD_HASH(0x80 | (c & 63) );
         }
      }
   }
   else
   #endif
   for(int i=0;i<length;i++)
      result = result*223 + ((unsigned char *)__s)[i];

   return result;
}



// InternalCreateConstBuffer is not uft16 aware whenit come to hashes
static void fixHashPerm16(const String &str)
{
   unsigned int hash = str.calcHash();
   ((unsigned int *)str.raw_ptr())[-2] = hash;
}


static unsigned char safeChars[256];

String String::__URLEncode() const
{
   Array<unsigned char> bytes(0,length);
   // utf8-encode
   __hxcpp_bytes_of_string(bytes,*this);

   int extra = 0;
   int utf8_chars = bytes->__length();
   for(int i=0;i<utf8_chars;i++)
      if (!safeChars[bytes[i]])
         extra++;
   if (extra==0)
      return *this;

   int l = utf8_chars + extra*2;
   char *result = hx::NewString(l);
   char *ptr = result;

   for(int i=0;i<utf8_chars;i++)
   {
      if (!safeChars[bytes[i]])
      {
         static char hex[] = "0123456789ABCDEF";
         unsigned char b = bytes[i];
         *ptr++ = '%';
         *ptr++ = hex[ b>>4 ];
         *ptr++ = hex[ b & 0x0f ];
      }
      else
         *ptr++ = bytes[i];
   }
   return String(result,l);
}

String String::toUpperCase() const
{
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      char16_t *result = String::allocChar16Ptr(length);
      for(int i=0;i<length;i++)
      {
         // Surrogates should already may to themselves - no need to check
         result[i] = unicase_toupper( __w[i] );
      }
      return String(result,length);
   }
   #endif

   char *result = hx::NewString(length);
   for(int i=0;i<length;i++)
      result[i] = toupper( __s[i] );
   return String(result,length);
}

String String::toLowerCase() const
{
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      char16_t *result = String::allocChar16Ptr(length);
      for(int i=0;i<length;i++)
      {
         result[i] = unicase_tolower( __w[i] );
      }
      return String(result,length);
   }
   #endif
   char *result = hx::NewString(length);
   for(int i=0;i<length;i++)
      result[i] = tolower( __s[i] );
   return String(result,length);
}


static int hex(int inChar)
{
   if (inChar>='0' && inChar<='9')
      return inChar-'0';
   if (inChar>='a' && inChar<='f')
      return inChar-'a' + 10;
   if (inChar>='A' && inChar<='F')
      return inChar-'A' + 10;
   return 0;
}


String String::__URLDecode() const
{
   // Create the decoded string; the decoded form might have fewer than
   // [length] characters, but it won't have more.  If it has fewer than
   // [length], some memory will be wasted here, but on the assumption that
   // most URLs have only a few '%NN' encodings in them, don't bother
   // counting the number of characters in the resulting string first.
   char *decoded = NewString(length), *d = decoded;

   bool hasBig = false;

   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      for (int i = 0; i < length; i++)
      {
         int c = __w[i];
         if (c > 127)
            *d++ = '?';
         else if (c == '+')
            *d++ = ' ';
         else if ((c == '%') && (i < (length - 2)))
         {
            int ch = ((hex(__w[i + 1]) << 4) | (hex(__w[i + 2])));
            if (ch>127)
               hasBig = true;
            *d++ = ch;
            i += 2;
         }
         else
             *d++ = c;
      }
   }
   else
   #endif
   {
      for (int i = 0; i < length; i++)
      {
         int c = __s[i];
         if (c > 127)
            *d++ = '?';
         else if (c == '+')
            *d++ = ' ';
         else if ((c == '%') && (i < (length - 2)))
         {
            int ch = ((hex(__s[i + 1]) << 4) | (hex(__s[i + 2])));
            #ifdef HX_SMART_STRINGS
            if (ch>127)
               hasBig = true;
            #endif
            *d++ = ch;
            i += 2;
         }
         else
             *d++ = c;
      }
   }

   #ifdef HX_SMART_STRINGS
   if (hasBig)
      return _hx_utf8_to_utf16((const unsigned char *)decoded, d-decoded,false);
   #endif

   return String( decoded, (d - decoded) );
}

::String &::String::dup()
{
   const char *s = __s;
   __s = 0;
   *this = create(s,length);
   return *this;
}

const ::String &::String::makePermanent() const
{
   if (!__s || (__s[HX_GC_CONST_ALLOC_MARK_OFFSET] & HX_GC_CONST_ALLOC_MARK_BIT) )
   {
      // Already permanent
   }
   else if (!length)
   {
      const_cast<String *>(this)->__s = emptyString.__s;
   }
   else if (length==1)
   {
      const_cast<String *>(this)->__s = String::fromCharCode(cca(0)).__s;
   }
   else
   {
      unsigned int myHash = hash();
      {
         while(! HxAtomicExchangeIf(0,1,&sPermanentStringSetMutex) )
            __hxcpp_gc_safe_point();
         TNonGcStringSet *element = sPermanentStringSet->find(myHash ,  *this);
         sPermanentStringSetMutex = 0;
         if (element)
         {
            const_cast<String *>(this)->__s = element->key.__s;
            return *this;
         }
      }

      #ifdef HX_SMART_STRINGS
      if (isUTF16Encoded())
      {
         char16_t *w = (char16_t *)InternalCreateConstBuffer(__s,(length+1)*2,true);
         ((unsigned int *)w)[-1] |= HX_GC_STRING_CHAR16_T;
         const_cast<String *>(this)->__w = w;
         fixHashPerm16(*this);
      }
      else
      #endif
      {
         char *s  = (char *)InternalCreateConstBuffer(__s,length+1,true);
         const_cast<String *>(this)->__s = s;
      }

      while(! HxAtomicExchangeIf(0,1,&sPermanentStringSetMutex) )
         __hxcpp_gc_safe_point();
      sPermanentStringSet->set(*this,null());
      sPermanentStringSetMutex = 0;
   }

   return *this;
}


String String::createPermanent(const char *inUtf8, int length)
{
   if (!inUtf8)
      return String();

   if (length<0)
      length = strlen(inUtf8);

   if (!length)
   {
      return emptyString;
   }
   else if (length==1)
   {
      return String::fromCharCode( *(unsigned char *)inUtf8  );
   }
   else
   {
      String temp = create(inUtf8,length);
      return temp.makePermanent();
   }
}




template<typename T>
static int TIndexOf(int s, const T *str, int strLen, const T *sought, int soughtLen)
{
   if (soughtLen==1)
   {
      T test = *sought;
      while(s<strLen)
      {
         if (str[s]==test)
            return s;
         ++s;
      }
   }
   else
   {
      while(s+soughtLen<=strLen)
      {
         if (!memcmp(str + s,sought,soughtLen*sizeof(T)))
            return s;
         s++;
      }
   }
   return -1;
}

static bool StrMatch(const char16_t *src0, const char *src1, int len)
{
   for(int i=0;i<len;i++)
      if (src0[i]!=src1[i])
         return false;
   return true;
}


int String::indexOf(const String &inValue, Dynamic inStart) const
{
   if (__s==0)
      return -1;
   int s = inStart==null() ? 0 : inStart->__ToInt();
   int l = inValue.length;

   if (l==0) {
      return s > length ? length : s;
   }

   #ifdef HX_SMART_STRINGS
   bool s016 =  isUTF16Encoded();
   bool s116 = inValue.isUTF16Encoded();
   if (s016 || s116)
   {
      if (s016 && s116)
         return TIndexOf(s, __w, length, inValue.__w, inValue.length);

      while(s+l<=length)
      {
         if (s016 ? StrMatch(__w+s, inValue.__s, l) : StrMatch(inValue.__w, __s+s, l) )
            return s;
         s++;
      }
      return -1;
   }
   #endif

   return TIndexOf(s, __s, length, inValue.__s, inValue.length);
}


template<typename T>
static int TLastIndexOf(int s, const T *str, int strLen, const T *sought, int soughtLen)
{
   if (soughtLen==1)
   {
      T test = *sought;
      while(s>=0)
      {
         if (str[s]==test)
            return s;
         s--;
      }
   }
   else
   {
      while(s>=0)
      {
         if (!memcmp(str + s,sought,soughtLen*sizeof(T)))
            return s;
         --s;
      }
   }
   return -1;
}



int String::lastIndexOf(const String &inValue, Dynamic inStart) const
{
   if (__s==0)
      return -1;
   int l = inValue.length;
   if (l>length) return -1;
   int s = inStart==null() ? length : inStart->__ToInt();
   if (s+l>length) s = length-l;

   #ifdef HX_SMART_STRINGS
   bool s016 = isUTF16Encoded();
   bool s116 = inValue.isUTF16Encoded();
   if (s016 || s116)
   {
      if (s016 && s116)
         return TLastIndexOf(s, __w, length, inValue.__w, inValue.length);

      while(s>=0)
      {
         if (s016 ? StrMatch(__w+s, inValue.__s, l) : StrMatch(inValue.__w, __s+s, l) )
            return s;
         s--;
      }
      return -1;
   }
   #endif
   return TLastIndexOf(s, __s, length, inValue.__s, inValue.length);
}





Dynamic String::charCodeAt(int inPos) const
{
   if (inPos<0 || inPos>=length)
      return null();

   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      return (int)__w[inPos];
   }
   #endif
   return (int)(((unsigned char *)__s)[inPos]);
}

String String::fromCharCode( int c )
{
   if (c<=255)
   {
      return sConstStrings[c];
   }
   else
   {
      #ifdef HX_SMART_STRINGS
      if (IsUtf16Surrogate(c)||c>=0x110000)
         c = 0xFFFD;
      #endif

      int group = c>>10;
      if (group>=1088)
         hx::Throw(HX_CSTRING("Invalid char code"));
      if (!sCharToString[group])
      {
         String *ptr = (String *)malloc( sizeof(String)*1024 );
         memset(ptr, 0, sizeof(String)*1024 );
         sCharToString[group] = ptr;
      }
      String *ptr = sCharToString[group];
      int cid = c & ((1<<10)-1);
      if (!ptr[cid].__s)
      {
         #ifdef HX_SMART_STRINGS
         int l = UTF16BytesCheck(c);
         char16_t *p = (char16_t *)InternalCreateConstBuffer(0,(l+1)*2,true);
         ((unsigned int *)p)[-1] |= HX_GC_STRING_CHAR16_T;
         if (c>=0x10000)
         {
            int over = (c-0x10000);
            p[0] = (over>>10) + 0xd800;
            p[1] = (over&0x3ff) + 0xdc00;
         }
         else
            p[0] = c;

         ptr[cid].length = l;
         ptr[cid].__w = p;
         fixHashPerm16(ptr[cid]);
         #else
         char buf[5];
         int  utf8Len = UTF8Bytes(c);
         char *p = buf;
         UTF8EncodeAdvance(p,c);
         buf[utf8Len] = '\0';
         const char *s = (char *)InternalCreateConstBuffer(buf,utf8Len+1,true);
         ptr[cid].length = utf8Len;
         ptr[cid].__s = s;
         #endif
      }
      return ptr[cid];
   }
}

String String::charAt( int at ) const
{
   if (at<0 || at>=length)
      return emptyString;

   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
      return fromCharCode(__w[at]);
   #endif
   return fromCharCode(__s[at]);
}

void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString)
{
   if (!inString.length)
      return;

   #ifdef HX_SMART_STRINGS
   if (inString.isUTF16Encoded())
   {
      const char16_t *src = inString.raw_wptr();
      const char16_t *end = src + inString.length;
      while(src<end)
      {
         int c = Char16Advance(src);

         if( c <= 0x7F )
            outBytes->push(c);
         else if( c <= 0x7FF )
         {
            outBytes->push( 0xC0 | (c >> 6) );
            outBytes->push( 0x80 | (c & 63) );
         }
         else if( c <= 0xFFFF )
         {
            outBytes->push( 0xE0 | (c >> 12) );
            outBytes->push( 0x80 | ((c >> 6) & 63) );
            outBytes->push( 0x80 | (c & 63) );
         }
         else
         {
            outBytes->push( 0xF0 | (c >> 18) );
            outBytes->push( 0x80 | ((c >> 12) & 63) );
            outBytes->push( 0x80 | ((c >> 6) & 63) );
            outBytes->push( 0x80 | (c & 63) );
         }
      }
   }
   else
   #endif
   {
      outBytes->__SetSize(inString.length);
      memcpy(outBytes->GetBase(), inString.raw_ptr(),inString.length);
   }
}

#ifdef HX_SMART_STRINGS
String _hx_utf8_to_utf16(const unsigned char *ptr, int inUtf8Len, bool addHash)
{
   unsigned int hash = 0;
   if (addHash)
      for(int i=0;i<inUtf8Len;i++)
         hash = hash*223 + ptr[i];

   int char16Count = 0;
   const unsigned char *u = ptr;
   const unsigned char *end = ptr + inUtf8Len;
   while(u<end)
   {
      int code = DecodeAdvanceUTF8(u,end);
      char16Count += UTF16BytesCheck(code);
   }

   int allocSize = 2*(char16Count+1);
   if (addHash)
      allocSize += sizeof(int);
   char16_t *str = (char16_t *)NewGCPrivate(0,allocSize);

   u = ptr;
   char16_t *o = str;
   while(u<end)
   {
      int code = DecodeAdvanceUTF8(u,end);
      Char16AdvanceSet(o,code);
   }
   if (addHash)
   {
      #ifdef EMSCRIPTEN
         *((emscripten_align1_int *)(str+char16Count+1) ) = hash;
      #else
         *((unsigned int *)(str+char16Count+1) ) = hash;
      #endif
      ((unsigned int *)(str))[-1] |= HX_GC_STRING_HASH | HX_GC_STRING_CHAR16_T;
   }
   else
      ((unsigned int *)(str))[-1] |= HX_GC_STRING_CHAR16_T;

   return String(str, char16Count);
}
#endif


void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len,bool inCopyPointer)
{
   if (inCopyPointer)
      outString = String( (const char *)inBytes->GetBase(), len);
   else if (len==0)
      outString = String::emptyString;
   else
   {
      const unsigned char *p0 = (const unsigned char *)inBytes->GetBase();
      #ifdef HX_SMART_STRINGS
      bool hasWChar = false;
      const unsigned char *p = p0 + pos;
      for(int i=0;i<len;i++)
         if (p[i]>127)
         {
            hasWChar = true;
            break;
         }
      if (hasWChar)
      {
         outString = _hx_utf8_to_utf16(p0+pos,len,true);
      }
      else
      #endif
      outString = String( GCStringDup((const char *)p0+pos, len, 0), len);
   }
}



const char * String::utf8_str(hx::IStringAlloc *inBuffer,bool throwInvalid) const
{
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
      return TConvertToUTF8(__w,0,inBuffer,throwInvalid);
   #endif
   return __s;
}

const char *String::ascii_substr(hx::IStringAlloc *inBuffer,int start, int length) const
{
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      const char16_t *p0 = __w + start;
      const char16_t *p = p0;
      const char16_t *limit = p+length;
      while(p<limit)
      {
         if (*p<=0 || *p>=127)
            break;
         p++;
      }
      int validLen = (int)(p-p0);
      char *result = (char *)inBuffer->allocBytes(validLen+1);
      for(int i=0;i<validLen;i++)
         result[i] = p0[i];
      result[validLen] = 0;
      return result;
   }
   #endif
   if (__s[start+length]=='\0')
      return __s+start;
   char *result = (char *)inBuffer->allocBytes(length+1);
   memcpy(result,__s+start,length);
   result[length] = '\0';

   return result;
}

#ifdef HX_SMART_STRINGS

bool String::eq(const ::String &inRHS) const
{
   if (length != inRHS.length)
      return false;

   bool s0IsWide = isUTF16Encoded();
   if (s0IsWide != inRHS.isUTF16Encoded() )
      return false;
   return !memcmp(__s, inRHS.__s, s0IsWide ? 2 * length : length );
}


int String::compare(const ::String &inRHS) const
{
   if (!__s)
      return inRHS.__s ? -1 : 0;
   if (!inRHS.__s)
      return 1;

   int cmp = 0;
   int minLen = length<inRHS.length ? length : inRHS.length;

   if (minLen>0)
   {
      bool s0IsWide = isUTF16Encoded();
      bool s1IsWide = inRHS.isUTF16Encoded();

      if (s0IsWide==s1IsWide)
      {
         if (!s0IsWide)
         {
            cmp = memcmp(__s,inRHS.__s,minLen);
         }
         else
         {
            for(int i=0;i<minLen;i++)
            {
               if (__w[i]!=inRHS.__w[i])
               {
                  cmp = __w[i] - inRHS.__w[i];
                  break;
               }
            }
         }
      }
      else
      {
         const unsigned char *s = (const unsigned char *)( s0IsWide ? inRHS.__s : __s );
         const char16_t *w = s0IsWide ? __w : inRHS.__w;
         for(int i=0;i<minLen;i++)
            if (s[i]!=w[i])
            {
               cmp =  s[i]<w[i] ? -1 : 1;
               if (s0IsWide)
                  cmp = - cmp;
               break;
            }
      }
   }
   return cmp ? cmp : length - inRHS.length;
}
#endif


namespace hx
{

wchar_t *ConvertToWChar(const char *inStr, int *ioLen)
{
   int len = ioLen ? *ioLen : strlen(inStr);

   wchar_t *result = (wchar_t *)NewGCPrivate(0,sizeof(wchar_t)*(len+1));
   int l = 0;

   unsigned char *b = (unsigned char *)inStr;
   for(int i=0;i<len;)
   {
      int c = b[i++];
      if (c==0) break;
      else if( c < 0x80 )
      {
        result[l++] = c;
      }
      else if( c < 0xE0 )
        result[l++] = ( ((c & 0x3F) << 6) | (b[i++] & 0x7F) );
      else if( c < 0xF0 )
      {
        int c2 = b[i++];
        result[l++] = ( ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( b[i++] & 0x7F) );
      }
      else
      {
        int c2 = b[i++];
        int c3 = b[i++];
        result[l++] = ( ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 << 6) & 0x7F) | (b[i++] & 0x7F) );
      }
   }
   result[l] = '\0';
   if (ioLen)
      *ioLen = l;
   return result;
}




}



const char16_t * String::wc_str(hx::IStringAlloc *inBuffer) const
{
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
      return __w;
   #endif

   int char16Count = 0;
   const unsigned char *ptr = (const unsigned char *)__s;
   const unsigned char *u = ptr;
   const unsigned char *end = u + length;
   while(u<end)
   {
      int code = DecodeAdvanceUTF8(u,end);
      char16Count += UTF16BytesCheck(code);
   }

   char16_t *str = inBuffer ? (char16_t *)inBuffer->allocBytes(2*(char16Count+1)) :
                              (char16_t *)NewGCPrivate(0,2*(char16Count+1));

   u = ptr;
   char16_t *o = str;
   while(u<end)
   {
      int code = DecodeAdvanceUTF8(u,end);
      Char16AdvanceSet(o,code);
   }
   *o = 0;

   return str;
}


const wchar_t * String::wchar_str(hx::IStringAlloc *inBuffer) const
{
   if (!__s)
      return 0;
   if (length==0)
      return L"";

   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      if (sizeof(wchar_t)==sizeof(char16_t))
          return (wchar_t *)__w;
   }

   wchar_t *result = 0;
   if (inBuffer)
   {
      result = (wchar_t *)inBuffer->allocBytes(sizeof(wchar_t)*(length+1) );
   }
   else
   {
      result = (wchar_t *)NewGCPrivate(0,sizeof(wchar_t)*(length+1) );
   }
   if (isUTF16Encoded())
      for(int i=0;i<length;i++)
         result[i] = __w[i];
   else
      for(int i=0;i<length;i++)
         result[i] = __s[i];
   result[length] = 0;
   return result;
   #else

   const unsigned char *ptr = (const unsigned char *)__s;
   const unsigned char *end = ptr + length;
   int idx = 0;
   while(ptr<end)
   {
      DecodeAdvanceUTF8(ptr,end);
      idx++;
   }

   wchar_t *result = 0;
   if (inBuffer)
   {
      result = (wchar_t *)inBuffer->allocBytes(sizeof(wchar_t)*(idx+1) );
   }
   else
   {
      result = (wchar_t *)NewGCPrivate(0,sizeof(wchar_t)*(idx+1) );
   }

   ptr = (const unsigned char *)__s;
   idx = 0;
   while(ptr<end)
      result[idx++] = DecodeAdvanceUTF8(ptr,end);
   result[idx] = 0;
   return result;
   #endif
}



#ifdef ANDROID
bool my_wstrneq(const wchar_t *s1, const wchar_t *s2, int len)
{
   for(int i=0;i<len;i++)
      if (s1[i]!=s2[i])
        return false;
   return true;
}
#endif

static bool isSame(const char16_t *a, const char *b, int count)
{
   for(int i=0;i<count;i++)
      if (a[i]!=b[i])
         return false;
   return true;
}

Array<String> String::split(const String &inDelimiter) const
{
   int len = inDelimiter.length;
   int pos = 0;
   int last = 0;
   if (len==0)
   {
      // Special case of splitting into characters - use efficient code...
      int chars = length;
      Array<String> result(0,chars);
      int idx = 0;
      #ifdef HX_SMART_STRINGS
      if (isUTF16Encoded())
      {
         const char16_t *p = __w;
         for(int i=0;i<length;i++)
            result[i] = String::fromCharCode(p[i]);
         /*
         const char16_t *end = p + length;
         while(p<end)
            result[idx++] = String::fromCharCode(Char16Advance(p));
         */
      }
      else
      {
         for(int i=0;i<chars;i++)
            result[i] = String::fromCharCode( __s[i] );
      }
      #else
      for(int i=0;i<chars; )
      {
         const unsigned char *start = (const unsigned char *)(__s + i);
         const unsigned char *ptr = start;
         DecodeAdvanceUTF8(ptr);
         int len =  ptr - start;
         result[idx++] = String::create( __s+i, len );
         i+=len;
      }
      #endif
      return result;
   }


   Array<String> result(0,1);
   #if HX_SMART_STRINGS
   bool s0 = isUTF16Encoded();
   bool s1 = inDelimiter.isUTF16Encoded();
   if (s0 || s1)
   {
      if (s0 && s1)
      {
         while(pos+len <=length )
            if (!memcmp(__w+pos,inDelimiter.__w,len*2))
            {
               result->push( substr(last,pos-last) );
               pos += len;
               last = pos;
            }
            else
               pos++;
      }
      else if (s0)
         while(pos+len <=length )
            if (isSame(__w+pos,inDelimiter.__s,len))
            {
               result->push( substr(last,pos-last) );
               pos += len;
               last = pos;
            }
            else
               pos++;
      else
         while(pos+len <=length )
            if (isSame(inDelimiter.__w,__s+pos,len))
            {
               result->push( substr(last,pos-last) );
               pos += len;
               last = pos;
            }
            else
               pos++;
   }
   else
   #endif
   {
      while(pos+len <=length )
         if (!strncmp(__s+pos,inDelimiter.__s,len))
         {
            result->push( substr(last,pos-last) );
            pos += len;
            last = pos;
         }
         else
            pos++;
   }

   result->push( substr(last,null()) );


   return result;
}

Dynamic CreateEmptyString() { return sConstEmptyString; }

Dynamic CreateString(DynamicArray inArgs)
{
   if (inArgs->__length()>0)
      return inArgs[0]->toString();
   return String();
}


String String::substr(int inFirst, Dynamic inLen) const
{
   int len = inLen == null() ? length : inLen->__ToInt();
   if (inFirst<0) inFirst += length;
   if (inFirst<0) inFirst = 0;
   if (len<0)
   {
      len += length;
      // This logic matches flash ....
      if (inFirst + len >=length)
         len = 0;
   }

   if (len<=0 || inFirst>=length)
      return String::emptyString;

   if ((len+inFirst > length) ) len = length - inFirst;
   if (len==0)
      return String::emptyString;


   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      if (len==1)
         return String::fromCharCode(__w[inFirst]);
      return String( GCStringDup(__w+inFirst, len, 0), len );
   }
   #endif

   if (len==1)
      return String::fromCharCode(__s[inFirst]);

   return String( GCStringDup(__s+inFirst, len, 0), len );
}

String String::substring(int startIndex, Dynamic inEndIndex) const
{
   int endIndex = inEndIndex == null() ? length : inEndIndex->__ToInt();
   if ( endIndex < 0 ) {
      endIndex = 0;
   } else if ( endIndex > length ) {
      endIndex = length;
   }

   if ( startIndex < 0 ) {
      startIndex = 0;
   } else if ( startIndex > length ) {
      startIndex = length;
   }

   if ( startIndex > endIndex ) {
      int tmp = startIndex;
      startIndex = endIndex;
      endIndex = tmp;
   }

   return substr( startIndex, endIndex - startIndex );
}

String String::operator+(const String &inRHS) const
{
   if (!__s) return HX_CSTRING("null") + inRHS;
   if (!length)
   {
      if (!inRHS.__s)
         return HX_CSTRING("null");
      return inRHS;
   }
   if (!inRHS.__s) return *this + HX_CSTRING("null");
   if (!inRHS.length) return *this;

   int l = length + inRHS.length;

   #ifdef HX_SMART_STRINGS
   bool ws0 = isUTF16Encoded();
   bool ws1 = inRHS.isUTF16Encoded();
   if (ws0 || ws1)
   {
      char16_t *result = String::allocChar16Ptr(l);

      if (ws0)
         memcpy(result,__w,length*2);
      else
         for(int i=0;i<length;i++)
            result[i] = __s[i];

      char16_t *r2 = result + length;
      if (ws1)
         memcpy(r2, inRHS.__w, inRHS.length*2);
      else
         for(int i=0;i<inRHS.length;i++)
            r2[i] = inRHS.__s[i];

      return String(result,l);
   }
   #endif


   char *result = hx::NewString(l);
   memcpy(result,__s,length*sizeof(char));
   memcpy(result+length,inRHS.__s,inRHS.length*sizeof(char));
   result[l] = '\0';
   return String(result,l);
}


String &String::operator+=(const String &inRHS)
{
   if (length==0)
   {
      *this = inRHS;
   }
   else if (inRHS.length>0)
   {
      int l = length + inRHS.length;
      #ifdef HX_SMART_STRINGS
      bool ws0 = isUTF16Encoded();
      bool ws1 = inRHS.isUTF16Encoded();
      if (ws0 || ws1)
      {
         char16_t *result = String::allocChar16Ptr(l);

         if (ws0)
            memcpy(result,__w,length*2);
         else
            for(int i=0;i<length;i++)
               result[i] = __s[i];

         char16_t *r2 = result + length;
         if (ws1)
            memcpy(r2, inRHS.__w, inRHS.length*2);
         else
            for(int i=0;i<inRHS.length;i++)
               r2[i] = inRHS.__s[i];

         __w = result;
      }
      else
      #endif
      {
      char *s = hx::NewString(l);
      memcpy(s,__s,length*sizeof(char));
      memcpy(s+length,inRHS.__s,inRHS.length*sizeof(char));
      __s = s;
      }

      length = l;
   }
   return *this;
}

#ifdef HXCPP_VISIT_ALLOCS
#define STRING_VISIT_FUNC \
    void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_STRING(mThis.raw_ref()); }
#else
#define STRING_VISIT_FUNC
#endif

#define DEFINE_STRING_FUNC(func,array_list,dynamic_arg_list,arg_list,ARG_C) \
struct __String_##func : public hx::Object \
{ \
   bool __IsFunction() const { return true; } \
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdClosure }; \
   String mThis; \
   __String_##func(const String &inThis) : mThis(inThis) { \
      HX_OBJ_WB_NEW_MARKED_OBJECT(this); \
   } \
   String toString() const{ return HX_CSTRING(#func); } \
   String __ToString() const{ return HX_CSTRING(#func); } \
   int __GetType() const { return vtFunction; } \
   void *__GetHandle() const { return const_cast<char *>(mThis.raw_ptr()); } \
   int __ArgCount() const { return ARG_C; } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      return mThis.func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      return mThis.func(arg_list); return Dynamic(); \
   } \
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_STRING(mThis.raw_ptr()); } \
   STRING_VISIT_FUNC \
   void  __SetThis(Dynamic inThis) { mThis = inThis; } \
}; \
Dynamic String::func##_dyn()  { return new __String_##func(*this);  }


#define DEFINE_STRING_FUNC0(func) DEFINE_STRING_FUNC(func,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0,0)
#define DEFINE_STRING_FUNC1(func) DEFINE_STRING_FUNC(func,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1,1)
#define DEFINE_STRING_FUNC2(func) DEFINE_STRING_FUNC(func,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2,2)

DEFINE_STRING_FUNC1(charAt);
DEFINE_STRING_FUNC1(charCodeAt);
DEFINE_STRING_FUNC2(indexOf);
DEFINE_STRING_FUNC2(lastIndexOf);
DEFINE_STRING_FUNC1(split);
DEFINE_STRING_FUNC2(substr);
DEFINE_STRING_FUNC2(substring);
DEFINE_STRING_FUNC0(toLowerCase);
DEFINE_STRING_FUNC0(toUpperCase);
DEFINE_STRING_FUNC0(toString);

hx::Val String::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (HX_FIELD_EQ(inString,"length")) return length;
   if (HX_FIELD_EQ(inString,"charAt")) return charAt_dyn();
   if (HX_FIELD_EQ(inString,"charCodeAt")) return charCodeAt_dyn();
   if (HX_FIELD_EQ(inString,"indexOf")) return indexOf_dyn();
   if (HX_FIELD_EQ(inString,"lastIndexOf")) return lastIndexOf_dyn();
   if (HX_FIELD_EQ(inString,"split")) return split_dyn();
   if (HX_FIELD_EQ(inString,"substr")) return substr_dyn();
   if (HX_FIELD_EQ(inString,"substring")) return substring_dyn();
   if (HX_FIELD_EQ(inString,"toLowerCase")) return toLowerCase_dyn();
   if (HX_FIELD_EQ(inString,"toUpperCase")) return toUpperCase_dyn();
   if (HX_FIELD_EQ(inString,"toString")) return toString_dyn();
   return null();
}


static String sStringStatics[] = {
   HX_CSTRING("fromCharCode"),
   String(null())
};
static String sStringFields[] = {
   HX_CSTRING("length"),
   HX_CSTRING("charAt"),
   HX_CSTRING("charCodeAt"),
   HX_CSTRING("indexOf"),
   HX_CSTRING("lastIndexOf"),
   HX_CSTRING("split"),
   HX_CSTRING("substr"),
   HX_CSTRING("substring"),
   HX_CSTRING("toLowerCase"),
   HX_CSTRING("toUpperCase"),
   HX_CSTRING("toString"),
   String(null())
};

STATIC_HX_DEFINE_DYNAMIC_FUNC1(String,fromCharCode,return )

namespace hx
{



#ifndef HX_WINDOWS
inline double _wtof(const wchar_t *inStr)
{
   #ifdef ANDROID
   char buf[101];
   int i;
   for(i=0;i<100 && inStr[i];i++)
      buf[i] = inStr[i];
   buf[i] = '\0';
   return strtod(buf, 0);
   #else
   return wcstod(inStr,0);
   #endif
}

#ifdef HX_ANDROID
int my_wtol(const wchar_t *inStr,wchar_t ** end, int inBase)
{
   char buf[101];
   int i;
   for(i=0;i<100 && inStr[i];i++)
      buf[i] = inStr[i];
   buf[i] = '\0';
   char *cend = buf;
   int result = strtol(buf,&cend,inBase);
   *end = (wchar_t *)inStr + (cend-buf);
   return result;
}
#define wcstol my_wtol
#endif

inline int _wtoi(const wchar_t *inStr)
{
   wchar_t *end = 0;
   if (!inStr) return 0;
   long result = 0;
   if (inStr[0]=='0' && (inStr[1]=='x' || inStr[1]=='X'))
      result = wcstol(inStr,&end,16);
   else
      result = wcstol(inStr,&end,10);
   return result;
}
#endif



class StringData : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdString };

   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=hx::NewObjContainer)
      { return hx::Object::operator new(inSize,inAlloc); }


   StringData(String inValue) : mValue(inValue) {
      HX_OBJ_WB_GET(this,mValue.raw_ref());
   };

   hx::Class __GetClass() const { return __StringClass; }
   #if (HXCPP_API_LEVEL<331)
   bool __Is(hx::Object *inClass) const { return dynamic_cast< StringData *>(inClass); }
   #endif

   virtual int __GetType() const { return vtString; }
   String __ToString() const { return mValue; }
   String toString() { return mValue; }
   double __ToDouble() const
   {
      if (!mValue.raw_ptr()) return 0;

      #ifdef HX_ANDROID
      return strtod(mValue.utf8_str(),0);
      #else
      return atof(mValue.utf8_str());
      #endif
   }
   int __length() const { return mValue.length; }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(mValue);
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(mValue);
   }
   #endif


   int __ToInt() const
   {
      if (!mValue.raw_ptr()) return 0;
      return atoi(mValue.utf8_str());
   }

   int __Compare(const hx::Object *inRHS) const
   {
      return mValue.compare( const_cast<hx::Object*>(inRHS)->toString() );
   }

   hx::Val __Field(const String &inString, hx::PropertyAccess inCallProp)
   {
      return mValue.__Field(inString, inCallProp);
   }


   String mValue;
};

hx::Class &GetStringClass() { return __StringClass; }

}

hx::Object *String::__ToObject() const
{
   if (!__s)
      return 0;

   if (length==0)
   {
      return sConstEmptyString.mPtr;
   }
   else if (length==1)
   {
      #ifdef HX_SMART_STRINGS
      int idx = isUTF16Encoded() ? __w[0] : ((unsigned char *)__s)[0];
      #else
      int idx = ((unsigned char *)__s)[0];
      #endif

      if (idx<=255)
      {
         if (sConstDynamicStrings[idx].mPtr)
            return  sConstDynamicStrings[idx].mPtr;

         return sConstDynamicStrings[idx].mPtr = new (hx::NewObjConst)StringData(fromCharCode(idx));
      }
   }

   bool isConst = __s[HX_GC_CONST_ALLOC_MARK_OFFSET] & HX_GC_CONST_ALLOC_MARK_BIT;
   NewObjectType type = isConst ?  NewObjAlloc : NewObjContainer;
   return new (type) StringData(*this);
}



void String::__boot()
{
   #ifdef HXCPP_COMBINE_STRINGS
   InitIdent();
   #endif

   sPermanentStringSet = new StringSet();
   GCAddRoot((hx::Object **)&sPermanentStringSet);

   for(int i=0;i<256;i++)
      safeChars[i] = i>32 && i<127;
   unsigned char dodgy[] = { 36, 38, 43, 44, 47, 58, 59, 61, 63, 64,
      34, 60, 62, 35, 37, 123, 125, 124, 92, 94, 126, 91, 93, 96 };
   for(int i=0;i<sizeof(dodgy);i++)
      safeChars[ dodgy[i] ] = 0;

   for(int c=0;c<256;c++)
   {
      #ifdef HX_SMART_STRINGS
      if (c>127)
      {
         char16_t buf[20];
         buf[0] = c;
         buf[1] = '\0';
         sConstStrings[c].length = 1;
         char16_t *w = (char16_t *)InternalCreateConstBuffer(buf,2*2,true);
         ((unsigned int *)w)[-1] |= HX_GC_STRING_CHAR16_T;
         sConstStrings[c].__w = w;
         fixHashPerm16(sConstStrings[c]);
      }
      else
      #endif
      {
         char buf[20];
         int  utf8Len = UTF8Bytes(c);
         char *p = buf;
         UTF8EncodeAdvance(p,c);
         buf[utf8Len] = '\0';
         sConstStrings[c].__s = (char *)InternalCreateConstBuffer(buf,utf8Len+1,true);
         sConstStrings[c].length = utf8Len;
      }
   }

   sConstEmptyString.mPtr = new (hx::NewObjConst)StringData(emptyString);

   Static(__StringClass) = hx::_hx_RegisterClass(HX_CSTRING("String"),TCanCast<StringData>,sStringStatics, sStringFields,
           &CreateEmptyString, &CreateString, 0, 0, 0
    );
}





