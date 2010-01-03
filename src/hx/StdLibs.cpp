#include <hxcpp.h>

#ifdef HX_WINDOWS
#include <windows.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <limits>

// -------- Resources ---------------------------------------

namespace hx
{

typedef std::map<String,Resource> ResourceSet;
static ResourceSet sgResources;

void RegisterResources(Resource *inResources)
{
   while(inResources->mData)
   {
      sgResources[inResources->mName] = *inResources;
      inResources++;
   }
}

}

using namespace hx;

Array<String> __hxcpp_resource_names()
{
   Array<String> result(0,0);
   for(ResourceSet::iterator i=sgResources.begin(); i!=sgResources.end();++i)
      result->push( i->first );
   return result;
}

String __hxcpp_resource_string(String inName)
{
   ResourceSet::iterator i=sgResources.find(inName);
   if (i==sgResources.end())
      return null();
   return String((const char *) i->second.mData, i->second.mDataLength );
}

Array<unsigned char> __hxcpp_resource_bytes(String inName)
{
   ResourceSet::iterator i=sgResources.find(inName);
   if (i==sgResources.end())
      return null();
   int len = i->second.mDataLength;
   Array<unsigned char> result( len, 0);
   memcpy( result->GetBase() , i->second.mData, len );
   return result;
}





// --- System ---------------------------------------------------------------------



void __trace(Dynamic inObj, Dynamic inData)
{
   printf( "%S:%d: %S\n",
               inData->__Field(L"fileName")->__ToString().__s,
               inData->__Field(L"lineNumber")->__ToInt(),
               inObj.GetPtr() ? inObj->toString().__s : L"null" );
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
	wchar_t *end = 0;
	if (!inString.__s)
		return null();
	long result;
	if (inString.__s[0]=='0' && (inString.__s[1]=='x' || inString.__s[1]=='X'))
		result = wcstol(inString.__s+2,&end,16);
	else
		result = wcstol(inString.__s,&end,10);
	if (inString.__s==end)
		return null();
	return result;
}

double __hxcpp_parse_float(const String &inString)
{
	wchar_t *end;
   double result =  wcstod(inString.__s,&end);
	if (end==inString.__s)
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

// --- CFFI helpers ------------------------------------------------------------------


// Field name management



#ifdef HX_INTERNAL_GC
typedef std::vector<String> FieldToString;
#else
typedef Array<String> FieldToString;
#endif

typedef std::map<std::string,int> StringToField;

// These need to be pointers because of the unknown order of static object construction.
FieldToString *sgFieldToString=0;
StringToField *sgStringToField=0;

static String sgNullString;


const String &__hxcpp_field_from_id( int f )
{
   if (!sgFieldToString)
      return sgNullString;

   return (*sgFieldToString)[f];
}


int  __hxcpp_field_to_id( const char *inFieldName )
{
   if (!sgFieldToString)
   {
      sgFieldToString = new FieldToString;

      #ifndef HX_INTERNAL_GC
      __RegisterStatic(sgFieldToString,sizeof(*sgFieldToString));
      (*sgFieldToString) = Array<String>(0,100);
      #endif
      sgStringToField = new StringToField;
   }

   std::string f(inFieldName);
   StringToField::iterator i = sgStringToField->find(f);
   if (i!=sgStringToField->end())
      return i->second;

   #ifdef HX_INTERNAL_GC
   int result = sgFieldToString->size();
   (*sgStringToField)[f] = result;
   String str(inFieldName,strlen(inFieldName));
   String cstr((wchar_t *)hx::InternalCreateConstBuffer(str.__s,(str.length+1) * sizeof(wchar_t)), str.length );
   sgFieldToString->push_back( cstr );
   #else
   int result = (*sgFieldToString)->size();
   (*sgStringToField)[f] = result;
   String str(inFieldName,strlen(inFieldName));
   (*sgFieldToString)->push(str);
   #endif
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





