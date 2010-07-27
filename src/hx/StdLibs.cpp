#include <hxcpp.h>

#ifdef HX_WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
typedef uint64_t __int64;
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <limits>


namespace hx
{

Dynamic Throw(Dynamic inDynamic)
{
   throw inDynamic;
   return null();
}

null NullArithmetic(const wchar_t *inErr)
{
	Throw( String(inErr) );
	return null();
}

}

// -------- Resources ---------------------------------------

namespace hx
{

//typedef std::map<std::wstring,Resource> ResourceSet;
//static ResourceSet sgResources;

Resource *sgResources;

void RegisterResources(Resource *inResources)
{
   sgResources = inResources;
   //while(inResources->mData)
   //{
      //sgResources[inResources->mName.__s] = *inResources;
      //inResources++;
   //}
}

}

using namespace hx;

Array<String> __hxcpp_resource_names()
{
   Array<String> result(0,0);

   for(Resource *reso  = sgResources; reso->mData; reso++)
      result->push( reso->mName );

   return result;
}

String __hxcpp_resource_string(String inName)
{
   for(Resource *reso  = sgResources; reso->mData; reso++)
   {
      if (reso->mName == inName)
          return String((const char *) reso->mData, reso->mDataLength );
   }
   return null();
}

Array<unsigned char> __hxcpp_resource_bytes(String inName)
{
   for(Resource *reso  = sgResources; reso->mData; reso++)
   {
      if (reso->mName == inName)
      {
         int len = reso->mDataLength;
         Array<unsigned char> result( len, 0);
         memcpy( result->GetBase() , reso->mData, len );
         return result;
      }
   }
   return null();
}



// --- System ---------------------------------------------------------------------


void __trace(Dynamic inObj, Dynamic inData)
{
#ifdef ANDROID
   __android_log_print(ANDROID_LOG_INFO, "trace", "%s:%d: %s",
               inData==null() ? "?" : inData->__Field(L"fileName")->toString().__CStr(),
               inData==null() ? 0 : inData->__Field(L"lineNumber")->__ToInt(),
               inObj.GetPtr() ? inObj->toString().__CStr() : "null" );
#else
   printf( "%S:%d: %S\n",
               inData->__Field(L"fileName")->__ToString().__s,
               inData->__Field(L"lineNumber")->__ToInt(),
               inObj.GetPtr() ? inObj->toString().__s : L"null" );
#endif
}

static double t0 = 0;
double  __time_stamp()
{
#ifdef _WIN32
   static __int64 t0=0;
   static double period=0;
   __int64 now;

   if (QueryPerformanceCounter((LARGE_INTEGER*)&now))
   {
      if (t0==0)
      {
         t0 = now;
         __int64 freq;
         QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
         if (freq!=0)
            period = 1.0/freq;
      }
      if (period!=0)
         return (now-t0)*period;
   }

   return (double)clock() / ( (double)CLOCKS_PER_SEC);
#else
   struct timeval tv;
   if( gettimeofday(&tv,NULL) )
      throw Dynamic("Could not get time");
   double t =  ( tv.tv_sec + ((double)tv.tv_usec) / 1000000.0 );
   if (t0==0) t0 = t;
   return t-t0;
#endif
}


#ifdef __APPLE__
   extern "C" {
   extern int *_NSGetArgc(void);
   extern char ***_NSGetArgv(void);
   }
#endif
Array<String> __get_args()
{
   Array<String> result(0,0);

   #ifdef _WIN32
   LPTSTR str =  GetCommandLine();
   bool skip_first = true;
   while(*str != '\0')
   {
      bool in_quote = false;
      LPTSTR end = str;
      String arg;
      while(*end!=0)
      {
         if (*end=='\0') break;
         if (!in_quote && *end==' ') break;
         if (*end=='"')
            in_quote = !in_quote;
         else
            arg += String::fromCharCode(*end);
         ++end;
      }

      if (!skip_first)
         result.Add( arg );
         skip_first = false;

      while(*end==' ') end++;
      str = end;
   }
   #else
   #ifdef __APPLE__

   int argc = *_NSGetArgc();
   char **argv = *_NSGetArgv();
   for(int i=1;i<argc;i++)
      result->push( String(argv[i],strlen(argv[i])) );


   #else
   #ifdef ANDROID
   // TODO: Get from java
   #else // linux

   char buf[80];
   sprintf(buf, "/proc/%d/cmdline", getpid());
   FILE *cmd = fopen(buf,"rb");
   bool real_arg = 0;
   if (cmd)
   {
      String arg;
      buf[1] = '\0';
      while (fread(buf, 1, 1, cmd))
      {
         if ((unsigned char)buf[0]<32) // line terminator
         {
            if (real_arg)
               result->push(arg);
            real_arg = true;
            arg = String();
         }
         else
            arg+=String(buf,1);
      }
      fclose(cmd);
   }
   #endif

   #endif
   #endif
   return result;
}


void __hxcpp_print(Dynamic &inV)
{
   printf("%S",inV->toString().c_str());
}

void __hxcpp_println(Dynamic &inV)
{
   printf("%S\n",inV->toString().c_str());
}


// --- Casting/Converting ---------------------------------------------------------


bool __instanceof(const Dynamic &inValue, const Dynamic &inType)
{
   if (inType==hx::Object::__SGetClass()) return true;
   if (inValue==null()) return false;
   Class c = inType;
   if (c==null()) return false;
   return c->CanCast(inValue.GetPtr());
}


int __int__(double x)
{
   if (x < -0x7fffffff || x>0x80000000 )
   {
      __int64 big_int = (__int64)x;
      return big_int & 0xffffffff;
   }
   else
      return (int)x;
}


Dynamic __hxcpp_parse_int(const String &inString)
{
   if (!inString.__s)
      return null();
   long result;
   const wchar_t *str = inString.__s;
   bool hex =  (str[0]=='0' && (str[1]=='x' || str[1]=='X'));

   #ifdef ANDROID
   char buf[100];
   int i;
   for(i=0;i<99 && i<inString.length;i++)
      buf[i] = str[i];
   buf[i] = '\0';
 
   char *end = 0;
   if (hex)
      result = strtol(buf+2,&end,16);
   else
      result = strtol(buf,&end,10);
   if (buf==end)
   #else
   wchar_t *end = 0;
   if (hex)
      result = wcstol(str+2,&end,16);
   else
      result = wcstol(str,&end,10);
   if (inString.__s==end)
   #endif
      return null();
   return (int)result;
}

double __hxcpp_parse_float(const String &inString)
{
   const wchar_t *str = inString.__s;
   #ifdef ANDROID
   char buf[100];
   int i;
   for(i=0;i<99 && i<inString.length;i++)
      buf[i] = str[i];
   buf[i] = '\0';
   char *end;
   double result = strtod(buf,&end);
   if (end==buf)
   #else
   wchar_t *end;
   double result =  wcstod(inString.__s,&end);
 
   if (end==inString.__s)
   #endif
   {
      if (std::numeric_limits<double>::has_quiet_NaN)
         return std::numeric_limits<double>::quiet_NaN();
      else
         return std::numeric_limits<double>::infinity();
   }
   return result;
}


bool __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2)
{
   hx::Object *p1 = inF1.GetPtr();
   hx::Object *p2 = inF2.GetPtr();
   if (p1==0 || p2==0)
      return false;
   if (p1->__GetHandle() != p2->__GetHandle())
      return false;
   return typeid(*p1) == typeid(*p2);
}

namespace hx
{

struct VarArgFunc : public hx::Object
{
   VarArgFunc(Dynamic &inFunc) : mRealFunc(inFunc) { }

   int __GetType() const { return vtFunction; }
   ::String __ToString() const { return mRealFunc->__ToString() ; }

   void __Mark() { MarkMember(mRealFunc); }

   void *__GetHandle() const { return mRealFunc.GetPtr(); }
   Dynamic __Run(const Array<Dynamic> &inArgs)
   {
      return mRealFunc->__run(inArgs);
   }

   Dynamic mRealFunc;
};

}

Dynamic __hxcpp_create_var_args(Dynamic &inArrayFunc)
{
   return Dynamic(new hx::VarArgFunc(inArrayFunc));
}

// --- CFFI helpers ------------------------------------------------------------------


// Field name management




typedef std::map<std::string,int> StringToField;

// These need to be pointers because of the unknown order of static object construction.
String *sgFieldToString=0;
int    sgFieldToStringSize=0;
int    sgFieldToStringAlloc=0;
StringToField *sgStringToField=0;

static String sgNullString;


const String &__hxcpp_field_from_id( int f )
{
   if (!sgFieldToString)
      return sgNullString;

   return sgFieldToString[f];
}


int  __hxcpp_field_to_id( const char *inFieldName )
{
   if (!sgFieldToStringAlloc)
   {
      sgFieldToStringAlloc = 100;
      sgFieldToString = (String *)malloc(sgFieldToStringAlloc * sizeof(String));

      sgStringToField = new StringToField;
   }

   std::string f(inFieldName);
   StringToField::iterator i = sgStringToField->find(f);
   if (i!=sgStringToField->end())
      return i->second;

   int result = sgFieldToStringSize;
   (*sgStringToField)[f] = result;
   String str(inFieldName,strlen(inFieldName));

   // Make into "const" string that will not get collected...
   #ifdef HX_INTERNAL_GC
   str = String((wchar_t *)hx::InternalCreateConstBuffer(str.__s,(str.length+1) * sizeof(wchar_t)), str.length );
   #else
   wchar_t *w = (wchar_t *)malloc((str.length+1) * sizeof(wchar_t));
   memcpy(w,str.__s,(str.length+1) * sizeof(wchar_t));
   str = String(w, str.length );
   #endif

   if (sgFieldToStringAlloc<=sgFieldToStringSize+1)
   {
      sgFieldToStringAlloc *= 2;
      sgFieldToString = (String *)realloc(sgFieldToString, sgFieldToStringAlloc*sizeof(String));
   }
   sgFieldToString[sgFieldToStringSize++] = str;
   return result;
}

// --- haxe.Int32 ---------------------------------------------------------------------
void __hxcpp_check_overflow(int x)
{
   if( (((x) >> 30) & 1) != ((unsigned int)(x) >> 31) )
      throw Dynamic(HX_STRING(L"Overflow ",9)+x);
}

namespace cpp
{

STATIC_HX_DEFINE_DYNAMIC_FUNC2(CppInt32___obj,make,return )

}




