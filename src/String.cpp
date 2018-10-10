#include <hxcpp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <string>
#include <hx/Unordered.h>
#include "hx/Hash.h"
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

#ifdef HX_UTF8_STRINGS
#define SPRINTF _snprintf
#else
#define SPRINTF _snwprintf
#endif

#else // vc8+

#ifdef HX_UTF8_STRINGS
#define SPRINTF _snprintf_s
#else
#define SPRINTF _snwprintf_s
#endif

#endif

#else // not windows ..

#ifdef HX_UTF8_STRINGS
#define SPRINTF snprintf
#else
#define SPRINTF swmprintf
#endif

#endif

namespace hx
{
#ifdef HX_UTF8_STRINGS
char HX_DOUBLE_PATTERN[20] = "%.15g";
#define HX_INT_PATTERN "%d"
#define HX_UINT_PATTERN "%ud"
#else
wchar_t HX_DOUBLE_PATTERN[20] =  L"%.15g";
#define HX_INT_PATTERN L"%d"
#endif
}

void __hxcpp_set_float_format(String inFormat)
{
  int last = inFormat.length < 19 ? inFormat.length : 19;
  memcpy(HX_DOUBLE_PATTERN, inFormat.__s, last*sizeof(char) );
  HX_DOUBLE_PATTERN[last] = '\0';
}

// --- GC helper

hx::Class __StringClass;

String  sEmptyString = HX_CSTRING("");
Dynamic sConstEmptyString;
String  sConstStrings[256];
Dynamic sConstDynamicStrings[256];

typedef std::set<String> ConstStringSet;
ConstStringSet sConstStringSet;
typedef hx::UnorderedMap<std::string,String> ConstWStringMap;
ConstWStringMap sConstWStringMap;

#ifdef HXCPP_COMBINE_STRINGS
static bool sIsIdent[256];
static bool InitIdent()
{
   for(int i=0;i<256;i++)
      sIsIdent[i]= (i>='a' && i<='z') || (i>='A' && i<='Z') || (i>='0' && i<='9') || (i=='_');
   return true;
}
#endif

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

int _hx_utf8_decode_advance(char *&ioPtr)
{
   return DecodeAdvanceUTF8( (const unsigned char * &) ioPtr );
}


inline int Char16Advance(const char16_t *&ioStr,bool throwOnErr=true)
{
   int ch = *ioStr++;
   if (ch>=0xd800)
   {
      int peek = *ioStr;
      if (peek<0xdc00)
      {
         if (throwOnErr)
            hx::Throw(HX_CSTRING("Invalid UTF16"));
         else
            return 0x10000 | ((ch-0xd800)  << 10);
      }

      ioStr++;
      ch = 0x10000 | ((ch-0xd800)  << 10) | (peek-0xdc00);
   }
   return ch;
}

void Char16AdvanceSet(char16_t *&ioStr,int inChar)
{
   if (inChar>=0x10000)
   {
      int over = (inChar-0x10000);
      *ioStr++ = (over>>10) + 0xd800;
      *ioStr++ = (over&0x3ff) + 0xdc00;
   }
   else
      *ioStr++ = inChar;
}




template<typename T>
char *TConvertToUTF8(const T *inStr, int *ioLen)
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

   char *buf = (char *)NewGCPrivate(0,chars+1);
   char *ptr = buf;
   for(int i=0;i<len;i++)
       UTF8EncodeAdvance(ptr,inStr[i]);
   *ptr = 0;
   if (ioLen)
      *ioLen = chars;

   return buf;
}


template<>
char *TConvertToUTF8(const char16_t *inStr, int *ioLen)
{
   int len = 0;
   if (ioLen==0 || *ioLen==0)
   {
      const char16_t *s = inStr;
      while( Char16Advance(s) ) { }
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
      chars += UTF8Bytes( Char16Advance( s ) );

   char *buf = (char *)NewGCPrivate(0,chars+1);
   char *ptr = buf;
   s = inStr;
   while(s<end)
      UTF8EncodeAdvance(ptr,Char16Advance(s));

   *ptr = 0;
   if (ioLen)
      *ioLen = chars;

   return buf;
}



String::String(const wchar_t *inString)
{
   length = 0;
   if (!inString)
   {
      __s = 0;
   }
   else
   {
      length = 0;
      __s = TConvertToUTF8(inString, &length);
   }
}

char16_t *String::allocChar16Ptr(int len)
{
   char16_t *result = (char16_t *)hx::InternalNew( (len+1)*2, false );
   ((unsigned int *)result)[-1] |= HX_GC_STRING_CHAR16_T;
   result[len] = 0;
   return result;
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
      return HX_CSTRING("");

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
      return String(ptr, inLen, true);
   }
   #endif
   char *result = TConvertToUTF8(base+inFirst,&len);
   return String(result,len);
}

Array<int> __hxcpp_utf8_string_to_char_array(String &inString)
{
   #ifdef HX_SMART_STRINGS
   Array<int> result = Array_obj<int>::__new(inString.length);
   if (inString.isUTF16Encoded())
      for(int i=0;i<inString.length;i++)
         result[i] = inString.__w[i];
   else
      for(int i=0;i<inString.length;i++)
         result[i] = inString.__s[i];
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
   return inBytes;
   #else
   int len = inBytes.length;
   char *result = TConvertToUTF8((unsigned char *)inBytes.__s,&len);
   return String(result,len);
   #endif
}


String __hxcpp_utf8_string_to_char_bytes(String &inUTF8)
{
   #ifdef HX_SMART_STRINGS
   return inUTF8;
   #else
    const unsigned char *src = (unsigned char *)inUTF8.__s;
    const unsigned char *end = src + inUTF8.length;
    int char_count = 0;
    while(src<end)
    {
        int c = DecodeAdvanceUTF8(src);
        char_count++;
        if( c == 8364 ) // euro symbol
            c = 164;
         else if( c == 0xFEFF ) // BOM
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
        if( c != 0xFEFF ) // BOM
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
         inIter( (int)inString.__w[i] );
   else
      for(int i=0;i<inString.length;i++)
         inIter( (int)inString.__s[i] );
   #else
   const unsigned char *src = (const unsigned char *)inString.__s;
   const unsigned char *end = src + inString.length;

   while(src<end)
      inIter(DecodeAdvanceUTF8(src));

   if (src>end)
      hx::Throw(HX_CSTRING("Invalid UTF8"));
   #endif
}

int _hx_utf8_char_code_at(String inString, int inIndex)
{
   #ifdef HX_SMART_STRINGS
   if (!inString.__s || inIndex>=inString.length)
      return 0;
   if (inString.isUTF16Encoded())
      return inString.__w[inIndex];
   else
      return inString.__s[inIndex];
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
   return DecodeAdvanceUTF8(src);
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
         return HX_CSTRING("");
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
   return String((const char *)start, src-start).dup();
   #endif
}




template<typename T>
static const T *GCStringDup(const T *inStr,int inLen, int *outLen=0)
{
   if (inStr==0 && inLen<=0)
   {
      if (outLen)
         outLen = 0;
      return (const T *)sEmptyString.__s;
   }

   int len = inLen;
   if (len==-1)
   {
      len=0;
      while(inStr[len])
         len++;
   }

   if (outLen)
      *outLen = len;

   if (len==1)
      return (const T *)String::fromCharCode(inStr[0]).__s;

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

   if (sizeof(T)==1)
   {
      char *result = hx::NewString( len );
      memcpy(result,inStr,sizeof(char)*(len));
      return (T*)result;
   }

   char16_t *result = String::allocChar16Ptr(len);
   memcpy(result,inStr,2*len);
   return (T *)result;
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

// Construct from wchar_t string - copy data
String::String(const wchar_t *inPtr,int inLen)
{
   if (!inPtr)
   {
       __s = 0;
       length = 0;
   }
   else if (inLen==0)
   {
       __s = 0;
       length = 0;
       *this = HX_CSTRING("");
   }
   else
   {
      length = inLen;
      __s = TConvertToUTF8(inPtr,&length);
   }
}

String::String(const char *inStr)
{
   if (inStr)
      __s = GCStringDup(inStr,-1,&length);
   else
   {
      __s = 0;
      length = 0;
   }
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


unsigned int String::calcHash() const
{
   unsigned int result = 0;
   #ifdef HX_SMART_STRINGS
   #define ADD_HASH(X) \
       result = result*223 + (int)(X)
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


static unsigned char safeChars[256];
static bool safeCharsInit = false;

String String::__URLEncode() const
{
   if (!safeCharsInit)
   {
      safeCharsInit = true;
      for(int i=0;i<256;i++)
         safeChars[i] = i>32 && i<127;
      unsigned char dodgy[] = { 36, 38, 43, 44, 47, 58, 59, 61, 63, 64,
         34, 60, 62, 35, 37, 123, 125, 124, 92, 94, 126, 91, 93, 96 };
      for(int i=0;i<sizeof(dodgy);i++)
         safeChars[ dodgy[i] ] = 0;
   }

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
         result[i] = __w[i]<256 ? toupper( __w[i] ) : __w[i];
      return String(result,length,true);
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
         result[i] = __w[i] < 256 ? tolower( __w[i] ) : __w[i];
      return String(result,length,true);
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


String &String::dup()
{
   if (length==0)
   {
      *this = HX_CSTRING("");
   }
   else
   {
      // Take copy incase GCStringDup generates GC event
      const char *oldString = __s;
      __s = 0;
      __s = GCStringDup(oldString,length,&length);
   }
   return *this;
}


String &String::dupConst()
{
   ConstStringSet::iterator sit = sConstStringSet.find(*this);
   if (sit!=sConstStringSet.end())
   {
      __s = sit->__s;
   }
   else if (length==1)
   {
      __s = String::fromCharCode(__s[0]);
   }
   else
   {
      char *ch  = (char *)InternalCreateConstBuffer(__s,length+1,true);
      ch[length] = '\0';
      __s = ch;
      sConstStringSet.insert(*this);
   }

   return *this;
}

::String String::makeConstString(const char *inStr)
{
   String unsafe(inStr, strlen(inStr) );
   ConstStringSet::iterator sit = sConstStringSet.find(unsafe);
   if (sit!=sConstStringSet.end())
      return *sit;
   return unsafe.dupConst();
}

::String String::makeConstChar16String(const char *inUtf8, int inLen)
{
   std::string key(inUtf8,inLen);
   ConstWStringMap::iterator sit = sConstWStringMap.find(key);
   if (sit!=sConstWStringMap.end())
      return sit->second;

   const unsigned char *ptr = (const unsigned char *)inUtf8;
   const unsigned char *end = ptr + inLen;

   int count = 0;
   while(ptr<end)
   {
      int code = DecodeAdvanceUTF8(ptr);
      count += code>=0x10000 ? 2 : 1;
   }

   char16_t *buffer  = (char16_t *)InternalCreateConstBuffer(0,(count+1)*2,true);
   ((unsigned int *)buffer)[-1] |= HX_GC_STRING_CHAR16_T;

   char16_t *b = buffer;
   ptr = (const unsigned char *)inUtf8;
   while(ptr<end)
   {
      int code = DecodeAdvanceUTF8(ptr);
      Char16AdvanceSet(b,code);
   }

   String result(buffer, count, true);
   sConstWStringMap[key] = result;
   return result;
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

      while(s>0)
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
   if (c<0) c+= 256;
   if (c<0)
       return null();

   if (c>255)
   {
      #ifdef HX_SMART_STRINGS
      int l = c>0x10000 ? 2 : 1;
      char16_t *p = String::allocChar16Ptr(l);

      if (c>=0x10000)
      {
         int over = (c-0x10000);
         p[0] = (over>>10) + 0xd800;
         p[1] = (over&0x3ff) + 0xdc00;
      }
      else
         p[0] = c;

      return String(p,l,true);
      #else
      return null();
      #endif
   }


   if (!sConstStrings[c].__s)
   {
      #ifdef HX_SMART_STRINGS
      if (c>127)
      {
         char16_t buf[2];
         buf[0] = c;
         buf[1] = '\0';
         sConstStrings[c].length = 1;
         char16_t *w = (char16_t *)InternalCreateConstBuffer(buf,2*2,true);
         ((unsigned int *)w)[-1] |= HX_GC_STRING_CHAR16_T;
         sConstStrings[c].__w = w;
      }
      else
      #endif
      {
         char buf[2];
         buf[0] = c;
         buf[1] = '\0';
         sConstStrings[c].__s = (char *)InternalCreateConstBuffer(buf,2,true);
         sConstStrings[c].length = 1;
      }
   }

   return sConstStrings[c];
}

String String::charAt( int at ) const
{
   if (at<0 || at>=length)
      return HX_CSTRING("");

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
      const char16_t *src = inString.__w;
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
      memcpy(outBytes->GetBase(), inString.__s,inString.length);
   }
}

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
      int code = DecodeAdvanceUTF8(u);
      char16Count+= code>=0x10000 ? 2 : 1;
   }

   int allocSize = 2*(char16Count+1);
   if (addHash)
      allocSize += sizeof(int);
   char16_t *str = (char16_t *)NewGCPrivate(0,allocSize);

   u = ptr;
   char16_t *o = str;
   while(u<end)
   {
      int code = DecodeAdvanceUTF8(u);
      Char16AdvanceSet(o,code);
   }
   if (addHash)
   {
      #ifdef EMSCRIPTEN
         *((emscripten_align1_int *)(str+char16Count+1) );
      #else
         *((unsigned int *)(str+char16Count+1) );
      #endif
         ((unsigned int *)(str))[-1] |= HX_GC_STRING_HASH | HX_GC_STRING_CHAR16_T;
   }
   else
      ((unsigned int *)(str))[-1] |= HX_GC_STRING_CHAR16_T;

   return String(str, char16Count, true);
}


void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len,bool inCopyPointer)
{
   if (inCopyPointer)
      outString = String( (const char *)inBytes->GetBase(), len);
   else if (len==0)
      outString = HX_CSTRING("");
   else if (len==1)
      outString = String::fromCharCode( inBytes[pos] );
   else
   {
      const unsigned char *p0 = (const unsigned char *)inBytes->GetBase();
      #ifdef HX_SMART_STRINGS
      bool hasWChar = false;
      const unsigned char *p = p0 + pos;
      for(int i=0;i<len;i++)
         if (p[i]>=127)
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



const char * String::__CStr() const
{
   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
      return TConvertToUTF8(__w,0);
   #endif
   return __s;
}

#ifdef HX_SMART_STRINGS
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
      bool s0IsWide = ((unsigned int *)__s)[-1] & HX_GC_STRING_CHAR16_T;
      bool s1IsWide = ((unsigned int *)inRHS.__s)[-1] & HX_GC_STRING_CHAR16_T;

      if (s0IsWide==s1IsWide)
      {
         cmp = memcmp(__s,inRHS.__s,s0IsWide ? minLen*2 : minLen);
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



const char16_t * String::wc_str() const
{
   #ifndef HX_SMART_STRINGS
   if (isUTF16Encoded())
      return __w;
   #endif

   String s = _hx_utf8_to_utf16((const unsigned char *)__s, length, false);
   return s.__w;
}


const wchar_t * String::__WCStr() const
{
   #ifndef HX_UTF8_STRINGS
   return __s ? __s : L"";
   #else
   
   const unsigned char *ptr = (const unsigned char *)__s;
   const unsigned char *end = ptr + length;
   int idx = 0;
   while(ptr<end)
   {
      DecodeAdvanceUTF8(ptr);
      idx++;
   }

   wchar_t *result = (wchar_t *)NewGCPrivate(0,sizeof(wchar_t) * (idx+1) );
   ptr = (const unsigned char *)__s;
   idx = 0;
   while(ptr<end)
      result[idx++] = DecodeAdvanceUTF8(ptr);
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
         /*
         const char16_t *p = __w;
         const char16_t *end = p + length;
         while(p<end)
            result[idx++] = String::fromCharCode(Char16Advance(p));
         */
         for(int i=0;i<length;i++)
            result[i] = String::fromCharCode(__w[i]);
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
         result[idx++] = String( __s+i, len ).dup();
         i+=len;
      }
      #endif
      return result;
   }


   Array<String> result(0,1);
   #if HX_SMART_STRINGS
   bool s0 = isUTF16Encoded();
   bool s1 = isUTF16Encoded();
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

Dynamic CreateEmptyString() { return HX_CSTRING(""); }

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
      return HX_CSTRING("");

   if ((len+inFirst > length) ) len = length - inFirst;
   if (len==0)
      return HX_CSTRING("");


   #ifdef HX_SMART_STRINGS
   if (isUTF16Encoded())
   {
      if (len==1)
         return String::fromCharCode(__w[inFirst]);
      return String( GCStringDup(__w+inFirst, len, 0), len, true );
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

      return String(result,l,true);
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
      char *s = hx::NewString(l);
      memcpy(s,__s,length*sizeof(char));
      memcpy(s+length,inRHS.__s,inRHS.length*sizeof(char));
      s[l] = '\0';
      __s = s;
      length = l;
   }
   return *this;
}

#ifdef HXCPP_VISIT_ALLOCS
#define STRING_VISIT_FUNC \
    void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_STRING(mThis.__s); }
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
   void *__GetHandle() const { return const_cast<char *>(mThis.__s); } \
   int __ArgCount() const { return ARG_C; } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      return mThis.func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      return mThis.func(arg_list); return Dynamic(); \
   } \
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_STRING(mThis.__s); } \
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
      HX_OBJ_WB_GET(this,mValue.__s);
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
      if (!mValue.__s) return 0;

      #ifdef HX_ANDROID
      return strtod(mValue.__s,0);
      #else
      return atof(mValue.__s);
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
      if (!mValue.__s) return 0;
      return atoi(mValue.__s);
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
      if (!sConstEmptyString.mPtr)
         sConstEmptyString.mPtr = new (hx::NewObjConst)StringData(sEmptyString);
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

   Static(__StringClass) = hx::_hx_RegisterClass(HX_CSTRING("String"),TCanCast<StringData>,sStringStatics, sStringFields,
           &CreateEmptyString, &CreateString, 0, 0, 0
    );
}





