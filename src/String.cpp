#include <hxcpp.h>
#include <ctype.h>

using namespace hx;

// -------- String ----------------------------------------

#ifdef _MSC_VER

#define WPRINTF wprintf

// vc 7...
#if _MSC_VER < 1400 

#define SPRINTF _snwprintf
#define SSCANF _snwscanf

#else // vc8+

#define SPRINTF _snwprintf_s
#define SSCANF _snwscanf_s

#endif

#else // not _MSC_VER ..
#define SPRINTF swprintf
#define SSCANF(str,len,fmt,ptr) swscanf(str,fmt,ptr)
#define WPRINTF printf
#endif


// --- GC helper

Class __StringClass;

static wchar_t *GCStringDup(const wchar_t *inStr,int &outLen)
{
   if (inStr==0)
   {
      outLen = 0;
      return 0;
   }

   #ifndef ANDROID
   outLen = wcslen(inStr);
   #else
   outLen = 0;
   while(inStr[outLen]) outLen++;
   #endif
   if (outLen==0)
      return 0;

   wchar_t *result = hx::NewString(outLen);
   memcpy(result,inStr,sizeof(wchar_t)*(outLen+1));
   return result;
}


static wchar_t *GCStringDup(const char *inStr,int &outLen)
{
   if (inStr==0)
   {
      outLen = 0;
      return 0;
   }

   outLen = strlen(inStr);
   if (outLen==0)
      return 0;

   wchar_t *result = hx::NewString(outLen);
   for(int i=0;i<=outLen;i++)
      result[i] = ((unsigned char *)inStr)[i];
   return result;
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


String::String(const int &inRHS)
{
#ifdef ANDROID
   char buf[100];
   snprintf(buf,100,"%d",inRHS);
#else
   wchar_t buf[100];
   SPRINTF(buf,100,L"%d",inRHS);
#endif
   buf[99]='\0';
   __s = GCStringDup(buf,length);
}


String::String(const cpp::CppInt32__ &inRHS)
{
   wchar_t buf[100];
   SPRINTF(buf,100,L"%d",inRHS.mValue);
   __s = GCStringDup(buf,length);
}

// Construct from utf8 string
String::String(const char *inPtr,int inLen)
{
   length = inLen<0 ? 0 : inLen;
   if (inPtr)
   {
      if (inLen<0)
         length = strlen(inPtr);
      __s = hx::ConvertToWChar(inPtr,&length);
   }
   else
   {
      __s = hx::NewString(length);
   }
}

String::String(const wchar_t *inStr)
{
   __s = GCStringDup(inStr,length);
}





String::String(const double &inRHS)
{
   #ifdef ANDROID
   char buf[100];
   snprintf(buf,100,"%.10g",inRHS);
   #else
   wchar_t buf[100];
   SPRINTF(buf,100,L"%.10g",inRHS);
   #endif
   buf[99]='\0';
   __s = GCStringDup(buf,length);
}

String::String(const bool &inRHS)
{
   if (inRHS)
   {
      *this = HX_STR(L"true");
   }
   else
   {
      *this = HX_STR(L"false");
   }
}

String String::__URLEncode() const
{
   Array<unsigned char> bytes(0,length);
   // utf8-encode
   __hxcpp_bytes_of_string(bytes,*this);

   int extra = 0;
   int spaces = 0;
   int utf8_chars = bytes->__length();
   for(int i=0;i<utf8_chars;i++)
      if ( !isalnum(bytes[i]) && bytes[i]!=' ' && bytes[i]!='-')
         extra++;
      else if (bytes[i]==' ')
         spaces++;
   if (extra==0 && spaces==0)
      return *this;

   int l = utf8_chars + extra*2 + spaces*2 /* *0 */;
   wchar_t *result = hx::NewString(l);
   wchar_t *ptr = result;
   bool has_plus = false;

   for(int i=0;i<utf8_chars;i++)
   {
      if ( bytes[i]==' ')
      {
         //*ptr++ = '+';
         *ptr++ = '%';
         *ptr++ = '2';
         *ptr++ = '0';
      }
      else if ( !isalnum(bytes[i]) && bytes[i]!='-' )
      {
         static wchar_t hex[] = L"0123456789ABCDEF";
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
   wchar_t *result = hx::NewString(length);
   for(int i=0;i<length;i++)
      result[i] = toupper( __s[i] );
   return String(result,length);
}

String String::toLowerCase() const
{
   wchar_t *result = hx::NewString(length);
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
   Array<unsigned char> bytes(0,length);
   for(int i=0;i<length;i++)
   {
      int c = __s[i];
      if (c>127)
         bytes->push('?');
      else if (c=='+')
         bytes->push(' ');
      else if (c=='%')
      {
         i++;
         int h0 = __s[i];
         if (h0!=0)
         {
            i++;
            int h1 = __s[i];
            bytes->push( (hex(h0)<<4) | hex(h1) );
         }
      }
      else
         bytes->push(c);
   }
   // utf8 -> unicode ...
   int len = bytes->__length();
   bytes->push(' ');
   return String( bytes->GetBase(), len );
}


String &String::dup()
{
   __s = GCStringDup(__s,length);
   return *this;
}

int String::indexOf(const String &inValue, Dynamic inStart) const
{
   if (__s==0)
      return -1;
   int s = inStart==null() ? 0 : inStart->__ToInt();
   int l = inValue.length;
   if (l==1)
   {
      wchar_t test = *inValue.__s;
      while(s<length)
      {
         if (__s[s]==test)
            return s;
         ++s;
      }
   }
   else
   {
      while(s<=length-l)
      {
         if (!memcmp(__s + s,inValue.__s,l*sizeof(wchar_t)))
            return s;
         s++;
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

   if (l==1)
   {
      wchar_t test = *inValue.__s;
      while(s>=0)
      {
         if (__s[s]==test)
            return s;
         --s;
      }
   }
   else
   {
      while(s>=0)
      {
         if (!memcmp(__s + s,inValue.__s,l*sizeof(wchar_t)))
            return s;
         --s;
      }
   }
   return -1;
}





Dynamic String::charCodeAt(int inPos) const
{
   if (inPos<0 || inPos>=length)
      return null();

   return (int)(__s[inPos]);
}



String String::fromCharCode( int c )
{
   wchar_t *result = hx::NewString(1);
   result[0] = c;
   result[1] = '\0';
   return String(result,1);
}

String String::charAt( int at ) const
{
   if (at<0 || at>=length) return HX_STR(L"");
   return fromCharCode(__s[at]);
}

void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString)
{
   for(int i=0;i<inString.length;i++)
   {
      int c = inString.__s[i];
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

void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len)
{
   const unsigned char *ptr = (unsigned char *)inBytes->GetBase() + pos;
   const unsigned char *last = ptr + len;
   wchar_t *result = hx::NewString(len);
   wchar_t *out = result;

   // utf8-encode
   while( ptr < last )
   {
      int c = *ptr++;
      if( c < 0x80 )
      {
         if( c == 0 ) break;
         *out++ = c;
      }
      else if( c < 0xE0 )
      {
         *out++ = ((c & 0x3F) << 6) | ((*ptr++) & 0x7F);
      }
      else if( c < 0xF0 )
      {
         int c2 = *ptr++;
         *out++ += ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( (*ptr++) & 0x7F);
      }
      else
      {
         int c2 = *ptr++;
         int c3 = *ptr++;
         *out++ +=((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 << 6) & 0x7F) | ((*ptr++) & 0x7F);
      }
   }
   int l = out - result;
   *out++ = '\0';

   outString = String(result,l);
}

char * String::__CStr() const
{
   Array<unsigned char> bytes(0,length+1);
   __hxcpp_bytes_of_string(bytes,*this);
	bytes.Add(0);
   char *result =  bytes->GetBase();
   if (result)
   {
      return  (char *)NewGCPrivate(result,bytes->length);
   }
   return (char *)"";
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

Array<String> String::split(const String &inDelimiter) const
{
   int len = inDelimiter.length;
   int pos = 0;
   int last = 0;
   if (len==0)
   {
      // Special case of splitting into characters - use efficient code...
      int chars = length;
      Array<String> result(chars,chars);
      for(int i=0;i<chars;i++)
      {
         wchar_t *ptr = hx::NewString(1);
         ptr[0] = __s[i];
         ptr[1] = '\0';
         result[i] = String(ptr,1);
      }
      return result;
   }


   Array<String> result(0,1);
   while(pos+len <=length )
   {
      #ifdef ANDROID
      if (my_wstrneq(__s+pos,inDelimiter.__s,len))
      #else
      if (!wcsncmp(__s+pos,inDelimiter.__s,len))
      #endif
      {
         result.Add( substr(last,pos-last) );
         pos += len;
         last = pos;
      }
      else
      {
         pos++;
      }
   }

   result.Add( substr(last,null()) );

   return result;
}

Dynamic CreateEmptyString() { return HX_STRING(L"",0); }

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
		return HX_STRING(L"",0);

   if ((len+inFirst > length) ) len = length - inFirst;
   if (len==0)
      return HX_STRING(L"",0);

   wchar_t *ptr = hx::NewString(len);
   memcpy(ptr,__s+inFirst,len*sizeof(wchar_t));
   ptr[len] = 0;
   return String(ptr,len);
}

String String::operator+(String inRHS) const
{
   if (!__s) return String(L"null",4) + inRHS;
   if (!length) return inRHS;
   if (!inRHS.__s) return *this + String(L"null",4);
   if (!inRHS.length) return *this;
   int l = length + inRHS.length;
   wchar_t *result = hx::NewString(l);
   memcpy(result,__s,length*sizeof(wchar_t));
   memcpy(result+length,inRHS.__s,inRHS.length*sizeof(wchar_t));
   result[l] = '\0';
   return String(result,l);
}


String &String::operator+=(String inRHS)
{
   if (length==0)
   {
      *this = inRHS;
   }
   else if (inRHS.length>0)
   {
      int l = length + inRHS.length;
      wchar_t *s = hx::NewString(l);
      memcpy(s,__s,length*sizeof(wchar_t));
      memcpy(s+length,inRHS.__s,inRHS.length*sizeof(wchar_t));
      s[l] = '\0';
      __s = s;
      length = l;
   }
   return *this;
}


#define DEFINE_STRING_FUNC(func,array_list,dynamic_arg_list,arg_list,ARG_C) \
struct __String_##func : public hx::Object \
{ \
   bool __IsFunction() const { return true; } \
   String mThis; \
   __String_##func(const String &inThis) : mThis(inThis) { } \
   String toString() const{ return L###func ; } \
   String __ToString() const{ return L###func ; } \
	int __GetType() const { return vtFunction; } \
	void *__GetHandle() const { return const_cast<wchar_t *>(mThis.__s); } \
	int __ArgCount() const { return ARG_C; } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      return mThis.func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      return mThis.func(arg_list); return Dynamic(); \
   } \
	void __Mark() { HX_MARK_STRING(mThis.__s); } \
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
DEFINE_STRING_FUNC0(toLowerCase);
DEFINE_STRING_FUNC0(toUpperCase);
DEFINE_STRING_FUNC0(toString);

Dynamic String::__Field(const String &inString)
{
   if (inString==HX_STR(L"length")) return length;
   if (inString==HX_STR(L"charAt")) return charAt_dyn();
   if (inString==HX_STR(L"charCodeAt")) return charCodeAt_dyn();
   if (inString==HX_STR(L"indexOf")) return indexOf_dyn();
   if (inString==HX_STR(L"lastIndexOf")) return lastIndexOf_dyn();
   if (inString==HX_STR(L"split")) return split_dyn();
   if (inString==HX_STR(L"substr")) return substr_dyn();
   if (inString==HX_STR(L"toLowerCase")) return toLowerCase_dyn();
   if (inString==HX_STR(L"toUpperCase")) return toUpperCase_dyn();
   if (inString==HX_STR(L"toString")) return toString_dyn();
   return null();
}


static String sStringStatics[] = {
   HX_STRING(L"fromCharCode",12),
   String(null())
};
static String sStringFields[] = {
   HX_STRING(L"length",6),
   HX_STRING(L"charAt",6),
   HX_STRING(L"charCodeAt",10),
   HX_STRING(L"indexOf",7),
   HX_STRING(L"lastIndexOf",11),
   HX_STRING(L"split",5),
   HX_STRING(L"substr",6),
   HX_STRING(L"toLowerCase",11),
   HX_STRING(L"toUpperCase",11),
   HX_STRING(L"toString",8),
   String(null())
};

STATIC_HX_DEFINE_DYNAMIC_FUNC1(String,fromCharCode,return )

namespace hx
{



#ifndef _WIN32
inline double _wtof(const wchar_t *inStr)
{
   #ifdef ANDROID
   char buf[101];
   int i;
   for(int i=0;i<100 && inStr[i];i++)
      buf[i] = inStr[i];
   buf[i] = '\0';
   return atof(buf);
   #else
   return wcstod(inStr,0);
   #endif
}

#ifdef ANDROID
int my_wtol(const wchar_t *inStr,wchar_t ** end, int inBase)
{
   char buf[101];
   int i;
   for(int i=0;i<100 && inStr[i];i++)
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
   StringData(String inValue) : mValue(inValue) {};

   Class __GetClass() const { return __StringClass; }
   bool __Is(hx::Object *inClass) const { return dynamic_cast< StringData *>(inClass); }

   virtual int __GetType() const { return vtString; }
   String __ToString() const { return mValue; }
   String toString() { return mValue; }
   double __ToDouble() const
   {
      if (!mValue.__s) return 0;
      return _wtof(mValue.__s);
   }
   int __length() const { return mValue.length; }

   void __Mark()
   {
      MarkMember(mValue);
   }

   int __ToInt() const
   {
      if (!mValue.__s) return 0;
      return _wtoi(mValue.__s);
   }

   int __Compare(const hx::Object *inRHS) const
   {
      return mValue.compare( const_cast<hx::Object*>(inRHS)->toString() );
   }

   Dynamic __Field(const String &inString)
   {
      return mValue.__Field(inString);
   }


   String mValue;
};

Class &GetStringClass() { return __StringClass; }

}

hx::Object *String::__ToObject() const { return new StringData(*this); }




void String::__boot()
{
   Static(__StringClass) = RegisterClass(HX_STRING(L"String",6),TCanCast<StringData>,sStringStatics, sStringFields,
           &CreateEmptyString, &CreateString, 0);
}





