#include <hxcpp.h>
#include <hxMath.h>

#ifdef HX_WINDOWS
#include <windows.h>
#include <stdio.h>
#include <io.h>
#else
#include <sys/time.h>
#include <stdio.h>
#ifndef EMSCRIPTEN
typedef int64_t __int64;
#endif
#endif

#ifdef ANDROID
#include <android/log.h>
#endif
#ifdef WEBOS
#include <syslog.h>
#endif
#ifdef TIZEN
extern "C" EXPORT_EXTRA void AppLogInternal(const char* pFunction, int lineNumber, const char* pFormat, ...);
#endif
#if defined(BLACKBERRY) || defined(GCW0)
#include <unistd.h>
#endif
#include <string>
#include <vector>
#include <map>
#include <time.h>


#ifdef HX_ANDROID
#define rand() lrand48()
#define srand(x) srand48(x)
#endif

void __hx_stack_set_last_exception();

namespace hx
{

Dynamic Throw(Dynamic inDynamic)
{
   #ifdef HXCPP_STACK_TRACE
   __hx_stack_set_last_exception();
   #endif
   throw inDynamic;
   return null();
}

null NullArithmetic(const HX_CHAR *inErr)
{
	Throw( String(inErr).dup() );
	return null();
}

}

// -------- Resources ---------------------------------------

namespace hx
{

//typedef std::map<std::wstring,Resource> ResourceSet;
//static ResourceSet sgResources;

Resource *sgResources = 0;
Resource *sgSecondResources = 0;

void RegisterResources(Resource *inResources)
{
   if (sgResources)
      sgSecondResources = inResources;
   else
      sgResources = inResources;
}

}

using namespace hx;

Array<String> __hxcpp_resource_names()
{
   Array<String> result(0,0);

   if (sgResources)
      for(Resource *reso  = sgResources; reso->mData; reso++)
         result->push( reso->mName );

   if (sgSecondResources)
      for(Resource *reso  = sgSecondResources; reso->mData; reso++)
         result->push( reso->mName );

   return result;
}

String __hxcpp_resource_string(String inName)
{
   if (sgResources)
      for(Resource *reso  = sgResources; reso->mData; reso++)
      {
         if (reso->mName == inName)
         #if (HXCPP_API_LEVEL > 0)
             return String((const char *) reso->mData, reso->mDataLength );
         #else
             return String((const char *) reso->mData, reso->mDataLength ).dup();
         #endif
      }

   if (sgSecondResources)
   {
      for(Resource *reso  = sgSecondResources; reso->mData; reso++)
         if (reso->mName == inName)
            return String((const char *) reso->mData, reso->mDataLength );
   }
   return null();
}

Array<unsigned char> __hxcpp_resource_bytes(String inName)
{
   if (sgResources)
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
   if (sgSecondResources)
      for(Resource *reso  = sgSecondResources; reso->mData; reso++)
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

// --- Maths ---------------------------------------------------------
static double rand_scale = 1.0 / (1<<16) / (1<<16);
double __hxcpp_drand()
{
   unsigned int lo = rand() & 0xfff;
   unsigned int mid = rand() & 0xfff;
   unsigned int hi = rand() & 0xff;
   double result = (lo | (mid<<12) | (hi<<24) ) * rand_scale;
   return result;
}

int __hxcpp_irand(int inMax)
{
   unsigned int lo = rand() & 0xfff;
   unsigned int mid = rand() & 0xfff;
   unsigned int hi = rand() & 0xff;
   return (lo | (mid<<12) | (hi<<24) ) % inMax;
}

void __hxcpp_stdlibs_boot()
{
   #if defined(HX_WINDOWS) && !defined(HX_WINRT)
   HMODULE kernel32 = LoadLibraryA("kernel32");
   if (kernel32)
   {
      typedef BOOL (WINAPI *AttachConsoleFunc)(DWORD);
      typedef HWND (WINAPI *GetConsoleWindowFunc)(void);
      AttachConsoleFunc attach = (AttachConsoleFunc)GetProcAddress(kernel32,"AttachConsole");
      GetConsoleWindowFunc getConsole = (GetConsoleWindowFunc)GetProcAddress(kernel32,"GetConsoleWindow");
      if (attach && getConsole)
      {
         attach( /*ATTACH_PARENT_PROCESS*/ (DWORD)-1 );

         if (getConsole())
         {
            if (_fileno(stdout) < 0 || _get_osfhandle(fileno(stdout)) < 0)
               freopen("CONOUT$", "w", stdout);
            if (_fileno(stderr) < 0 || _get_osfhandle(fileno(stderr)) < 0)
               freopen("CONOUT$", "w", stderr);
            if (_fileno(stdin) < 0 || _get_osfhandle(fileno(stdin)) < 0)
               freopen("CONIN$", "r", stdin);
         }
      }
   }
   #endif
   
   setbuf(stdin, 0);
   setbuf(stdout, 0);
   setbuf(stderr, 0);
}

void __trace(Dynamic inObj, Dynamic inData)
{
#ifdef TIZEN
   AppLogInternal(inData==null() ? "?" : inData->__Field( HX_CSTRING("fileName") , HX_PROP_DYNAMIC) ->toString().__s,
      inData==null() ? 0 : inData->__Field( HX_CSTRING("lineNumber") , HX_PROP_DYNAMIC)->__ToInt(),
      "%s\n", inObj.GetPtr() ? inObj->toString().__s : "null" );
#else
#ifdef HX_UTF8_STRINGS
   #if defined(HX_ANDROID) && !defined(HXCPP_EXE_LINK)
   __android_log_print(ANDROID_LOG_INFO, "trace","%s:%d: %s",
   #elif defined(WEBOS)
   syslog(LOG_INFO, "%s:%d: %s",
   #else
   printf("%s:%d: %s\n",
   #endif
               inData==null() ? "?" : inData->__Field( HX_CSTRING("fileName") , HX_PROP_DYNAMIC) ->toString().__s,
               inData==null() ? 0 : inData->__Field( HX_CSTRING("lineNumber") , HX_PROP_DYNAMIC)->__ToInt(),
               inObj.GetPtr() ? inObj->toString().__s : "null" );
#else
   printf( "%S:%d: %S\n",
               inData->__Field( HX_CSTRING("fileName") , HX_PROP_DYNAMIC)->__ToString().__s,
               inData->__Field( HX_CSTRING("lineNumber") , HX_PROP_DYNAMIC)->__ToInt(),
               inObj.GetPtr() ? inObj->toString().__s : L"null" );
#endif
#endif
}

void __hxcpp_exit(int inExitCode)
{
   exit(inExitCode);
}

static double t0 = 0;
double  __time_stamp()
{
#ifdef HX_WINDOWS
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
   if( gettimeofday(&tv,0) )
      throw Dynamic("Could not get time");
   double t =  ( tv.tv_sec + ((double)tv.tv_usec) / 1000000.0 );
   if (t0==0) t0 = t;
   return t-t0;
#endif
}

#if defined(HX_WINDOWS)

/*
ISWHITE and ParseCommandLine are based on the implementation of the 
.NET Core runtime, CoreCLR, which is licensed under the MIT license:
Copyright (c) Microsoft. All rights reserved.
See LICENSE file in the CoreCLR project root for full license information.

The original source code of ParseCommandLine can be found in
https://github.com/dotnet/coreclr/blob/master/src/vm/util.cpp
*/

#define ISWHITE(x) ((x)==(' ') || (x)==('\t') || (x)==('\n') || (x)==('\r') )

static void ParseCommandLine(LPTSTR psrc, Array<String> &out)
{
    unsigned int argcount = 1;       // discovery of arg0 is unconditional, below

    bool    fInQuotes;
    int     iSlash;

    /* A quoted program name is handled here. The handling is much
       simpler than for other arguments. Basically, whatever lies
       between the leading double-quote and next one, or a terminal null
       character is simply accepted. Fancier handling is not required
       because the program name must be a legal NTFS/HPFS file name.
       Note that the double-quote characters are not copied, nor do they
       contribute to numchars.
         
       This "simplification" is necessary for compatibility reasons even
       though it leads to mishandling of certain cases.  For example,
       "c:\tests\"test.exe will result in an arg0 of c:\tests\ and an
       arg1 of test.exe.  In any rational world this is incorrect, but
       we need to preserve compatibility.
    */

    LPTSTR pStart = psrc;
    bool skipQuote = false;

    if (*psrc == '\"')
    {
        // scan from just past the first double-quote through the next
        // double-quote, or up to a null, whichever comes first
        while ((*(++psrc) != '\"') && (*psrc != '\0'))
            continue;

        skipQuote = true;
    }
    else
    {
        /* Not a quoted program name */

        while (!ISWHITE(*psrc) && *psrc != '\0')
            psrc++;
    }

    // We have now identified arg0 as pStart (or pStart+1 if we have a leading
    // quote) through psrc-1 inclusive
    if (skipQuote)
        pStart++;
    String arg0("");
    while (pStart < psrc)
    {
        arg0 += String::fromCharCode(*pStart);
        pStart++;
    }
    // out.Add(arg0); // the command isn't part of Sys.args()

    // if we stopped on a double-quote when arg0 is quoted, skip over it
    if (skipQuote && *psrc == '\"')
        psrc++;

    while ( *psrc != '\0')
    {
LEADINGWHITE:

        // The outofarg state.
        while (ISWHITE(*psrc))
            psrc++;

        if (*psrc == '\0')
            break;
        else
        if (*psrc == '#')
        {
            while (*psrc != '\0' && *psrc != '\n')
                psrc++;     // skip to end of line

            goto LEADINGWHITE;
        }

        argcount++;
        fInQuotes = FALSE;

        String arg("");

        while ((!ISWHITE(*psrc) || fInQuotes) && *psrc != '\0')
        {
            switch (*psrc)
            {
            case '\\':
                iSlash = 0;
                while (*psrc == '\\')
                {
                    iSlash++;
                    psrc++;
                }

                if (*psrc == '\"')
                {
                    for ( ; iSlash >= 2; iSlash -= 2)
                    {
                        arg += String("\\");
                    }

                    if (iSlash & 1)
                    {
                        arg += String::fromCharCode(*psrc);
                        psrc++;
                    }
                    else
                    {
                        fInQuotes = !fInQuotes;
                        psrc++;
                    }
                }
                else
                    for ( ; iSlash > 0; iSlash--)
                    {
                        arg += String("\\");
                    }

                break;

            case '\"':
                fInQuotes = !fInQuotes;
                psrc++;
                break;

            default:
                arg += String::fromCharCode(*psrc);
                psrc++;
            }
        }

        out.Add(arg);
        arg = String("");
    }
}
#endif


#ifdef __APPLE__
 #if !defined(IPHONE) && !defined(APPLETV)
   extern "C" {
   extern int *_NSGetArgc(void);
   extern char ***_NSGetArgv(void);
   }
 #endif
#endif
Array<String> __get_args()
{
   Array<String> result(0,0);

   #ifdef HX_WINRT
   // Do nothing
   #elif defined(HX_WINDOWS)
   LPTSTR str =  GetCommandLine();
   ParseCommandLine(str, result);
   #else
   #ifdef __APPLE__

   #if !defined(IPHONE) && !defined(APPLETV)
   int argc = *_NSGetArgc();
   char **argv = *_NSGetArgv();
   for(int i=1;i<argc;i++)
      result->push( String(argv[i],strlen(argv[i])).dup() );
   #endif

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
      String arg("");
      buf[0] = '\0';
      while (fread(buf, 1, 1, cmd))
      {
         if ((unsigned char)buf[0] == 0) // line terminator
         {
            if (real_arg)
               result->push(arg);
            real_arg = true;
            arg = String("");
         }
         else
            arg += String::fromCharCode(buf[0]);
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
   #ifdef HX_WINRT
   WINRT_PRINTF("%s",inV->toString().__s);
   #else
   #ifdef HX_UTF8_STRINGS
   printf("%s",inV->toString().__s);
   #else
   printf("%S",inV->toString().__s);
   #endif
   #endif
}

void __hxcpp_println(Dynamic &inV)
{
   #ifdef HX_WINRT
   WINRT_PRINTF("%s\n",inV->toString().__s);
   #else
   #ifdef HX_UTF8_STRINGS
   printf("%s\n",inV->toString().__s);
   #else
   printf("%S\n",inV->toString().__s);
   #endif
   #endif
}


// --- Casting/Converting ---------------------------------------------------------


bool __instanceof(const Dynamic &inValue, const Dynamic &inType)
{
   if (inType==hx::Object::__SGetClass())
      return true;
   if (inValue==null())
      return false;
   hx::Class c = inType;
   if (c==null())
      return false;
   return c->CanCast(inValue.GetPtr());
}


int __int__(double x)
{
   #ifndef EMSCRIPTEN
   if (x < -0x7fffffff || x>0x7fffffff )
   {
      __int64 big_int = (__int64)(x);
      return big_int & 0xffffffff;
   }
   else
   #endif
      return (int)x;
}


Dynamic __hxcpp_parse_int(const String &inString)
{
   if (!inString.__s)
      return null();
   long result;
   const HX_CHAR *str = inString.__s;
   bool hex =  (str[0]=='0' && (str[1]=='x' || str[1]=='X'));
   HX_CHAR *end = 0;

   #ifdef HX_UTF8_STRINGS
   if (hex)
      result = (long)strtoul(str+2,&end,16);
   else
      result = strtol(str,&end,10);
   #else
   if (hex)
      result = (long)wcstoul(str+2,&end,16);
   else
      result = wcstol(str,&end,10);
   #endif
   if (str==end)
      return null();
   return (int)result;
}

double __hxcpp_parse_float(const String &inString)
{
   const HX_CHAR *str = inString.__s;
   HX_CHAR *end = (HX_CHAR *)str;
   #ifdef HX_UTF8_STRINGS
   double result = str ? strtod(str,&end) : 0;
   #else
   double result =  str ? wcstod(str,&end) : 0;
   #endif

   if (end==str)
      return Math_obj::NaN;

   return result;
}


bool __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2)
{
   hx::Object *p1 = inF1.GetPtr();
   hx::Object *p2 = inF2.GetPtr();
   if (p1==0 || p2==0)
      return false;
   if ( (p1->__GetHandle() != p2->__GetHandle()))
      return false;
   return p1->__Compare(p2)==0;
}

namespace hx
{

struct VarArgFunc : public hx::Object
{
   VarArgFunc(Dynamic &inFunc) : mRealFunc(inFunc) { }

   int __GetType() const { return vtFunction; }
   ::String __ToString() const { return mRealFunc->__ToString() ; }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(mRealFunc); }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mRealFunc); }
   #endif

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
   str = String((HX_CHAR *)hx::InternalCreateConstBuffer(str.__s,(str.length+1) * sizeof(HX_CHAR),true), str.length );

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
      throw Dynamic(HX_CSTRING("Overflow ")+x);
}

// --- Memory ---------------------------------------------------------------------

unsigned char *__hxcpp_memory = 0;

void  __hxcpp_memory_memset(Array<unsigned char> &inBuffer ,int pos, int len, int value)
{
   if (pos<inBuffer->length)
   {
      if (pos+len>inBuffer->length)
         len = inBuffer->length - pos;
      if (len>0)
         memset( inBuffer->Pointer() + pos, value, len);
   }
}

