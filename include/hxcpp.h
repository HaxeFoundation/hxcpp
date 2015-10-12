#ifndef HXCPP_H
#define HXCPP_H

// Standard headers ....

// Windows hack
#define NOMINMAX

#ifndef HXCPP_API_LEVEL
   #define HXCPP_API_LEVEL 0
#endif

#ifdef _MSC_VER
   #include <typeinfo.h>
   namespace hx { typedef ::type_info type_info; }
#else
   #include <typeinfo>
   #include <stdint.h>
   namespace hx { typedef std::type_info type_info; }
   #ifndef EMSCRIPTEN
      using hx::type_info;
      #ifdef __MINGW32__
         #include <stdint.h>
      #else
         typedef int64_t  __int64;
      #endif
   #endif
#endif

#if defined(EMSCRIPTEN) || defined(IPHONE) || defined(APPLETV)
  #include <unistd.h>
  #include <cstdlib>
#endif

#ifdef __OBJC__
#ifdef HXCPP_OBJC
  #import <Foundation/Foundation.h>
#endif
#endif


#include <string.h>

#define HX_UTF8_STRINGS

#include <wchar.h>

#ifdef HX_LINUX
  #include <unistd.h>
  #include <cstdio>
  #include <stddef.h>
#endif

#ifdef EMSCRIPTEN
#define HXCPP_ALIGN_FLOAT
#endif




// Some compilers are over-enthusiastic about what they #define ...
//#ifdef NULL
//#undef NULL
//#endif

#ifdef assert
#undef assert
#endif

#define HXCPP_CLASS_ATTRIBUTES

#ifdef _MSC_VER
  #if defined(HXCPP_DLL_IMPORT)
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES __declspec(dllimport)
  #elif defined (HXCPP_DLL_EXPORT)
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES __declspec(dllexport)
  #else
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES
  #endif
#else
  #if defined(HXCPP_DLL_EXPORT)
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES __attribute__((visibility("default")))
  #else
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES
  #endif
#endif

typedef char HX_CHAR;



#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER)) && !defined HXCPP_CHECK_POINTER
#define HXCPP_CHECK_POINTER
#endif



#ifdef BIG_ENDIAN
#undef BIG_ENDIAN

  #ifndef HXCPP_BIG_ENDIAN
  #define HXCPP_BIG_ENDIAN
  #endif
#endif

#ifdef __BIG_ENDIAN__
  #ifndef HXCPP_BIG_ENDIAN
  #define HXCPP_BIG_ENDIAN
  #endif
#endif

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN

  #ifdef HXCPP_BIG_ENDIAN
  #undef HXCPP_BIG_ENDIAN
  #endif
#endif

#ifdef __LITTLE_ENDIAN__
  #ifdef HXCPP_BIG_ENDIAN
  #undef HXCPP_BIG_ENDIAN
  #endif
#endif

// HX_HCSTRING is for constant strings with built-in hashes
//     HX_GC_CONST_ALLOC_BIT
// HX_CSTRING is for constant strings without built-in hashes
//     HX_GC_CONST_ALLOC_BIT | HX_GC_NO_STRING_HASH

#ifdef HXCPP_BIG_ENDIAN
#define HX_HCSTRING(s,h0,h1,h2,h3) ::String( (const HX_CHAR *)((h3 h2 h1 h0 "\x80\x00\x00\x00" s )) + 8 , sizeof(s)/sizeof(HX_CHAR)-1)
#define HX_STRINGI(s,len) ::String( (const HX_CHAR *)(("\xc0\x00\x00\x00" s)) + 4 ,len)
#else

#define HX_HCSTRING(s,h0,h1,h2,h3) ::String( (const HX_CHAR *)((h0 h1 h2 h3 "\x00\x00\x00\x80" s )) + 8 , sizeof(s)/sizeof(HX_CHAR)-1)

#define HX_STRINGI(s,len) ::String( (const HX_CHAR *)(("\x00\x00\x0\xc0" s)) + 4 ,len)
#endif



#define HX_STRI(s) HX_STRINGI(s,sizeof(s)/sizeof(HX_CHAR)-1)
#define HX_CSTRING(x) HX_STRI(x)
#define HX_CSTRING2(wide,len,utf8) HX_STRI(utf8)
#define HX_FIELD_EQ(name,field) !::memcmp(name.__s, field, sizeof(field)/sizeof(char))



#pragma warning(disable:4251)
#pragma warning(disable:4800)

#if defined(_MSC_VER) && _MSC_VER < 1201
#error MSVC 7.1 does not support template specialization and is not supported by HXCPP
#endif


// HXCPP includes...

// Basic mapping from haxe -> c++

typedef int Int;
typedef bool Bool;

#ifdef HXCPP_FLOAT32
typedef float Float;
#else
typedef double Float;
#endif


// Extended mapping - cpp namespace
namespace cpp
{
   typedef signed char Int8;
   typedef unsigned char UInt8;
   typedef char Char;
   typedef signed short Int16;
   typedef unsigned short UInt16;
   typedef signed int Int32;
   typedef unsigned int UInt32;
   #ifdef _WIN32
   typedef __int64 Int64;
   typedef unsigned __int64 UInt64;
   // TODO - EMSCRIPTEN?
   #else
   typedef int64_t Int64;
   typedef uint64_t UInt64;
   #endif
   typedef float Float32;
   typedef double Float64;
};
// Extended mapping - old way
namespace haxe { namespace io { typedef unsigned char Unsigned_char__; } }

// --- Forward decalarations --------------------------------------------

namespace cpp { class CppInt32__; }
namespace hx { class Object; }
namespace hx { class FieldRef; }
namespace hx { class IndexRef; }
namespace hx { template<typename O> class ObjectPtr; }
namespace cpp { template<typename S,typename H> class Struct; }
template<typename ELEM_> class Array_obj;
template<typename ELEM_> class Array;
namespace hx {
   class Class_obj;
   typedef hx::ObjectPtr<hx::Class_obj> Class;
}

#if (HXCPP_API_LEVEL < 320) && !defined(__OBJC__)
typedef hx::Class Class;
typedef hx::Class_obj Class_obj;
#endif

class Dynamic;
class String;

// Use an external routine to throw to avoid sjlj overhead on iphone.
namespace hx { HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic Throw(Dynamic inDynamic); }
namespace hx { HXCPP_EXTERN_CLASS_ATTRIBUTES void CriticalError(const String &inError); }
namespace hx { HXCPP_EXTERN_CLASS_ATTRIBUTES void NullReference(const char *type, bool allowFixup); }
namespace hx { extern String sNone[]; }
void __hxcpp_check_overflow(int inVal);

namespace hx
{
class MarkContext;

class VisitContext
{
public:
   virtual void visitObject(hx::Object **ioPtr)=0;
   virtual void visitAlloc(void **ioPtr)=0;
};


#if (HXCPP_API_LEVEL >= 313)
enum PropertyAccessMode
{
   paccNever   = 0,
   paccDynamic = 1,
   paccAlways  = 2,
};
typedef PropertyAccessMode PropertyAccess;
#define HX_PROP_NEVER  hx::paccNever
#define HX_PROP_DYNAMIC hx::paccDynamic
#define HX_PROP_ALWAYS hx::paccAlways
#else
typedef bool PropertyAccess;
#define HX_PROP_NEVER  false
#define HX_PROP_DYNAMIC true
#define HX_PROP_ALWAYS true
#endif

} // end namespace hx




// The order of these includes has been chosen to minimize forward declarations.
// You should not include the individual files, just this one.
#include <hx/Macros.h>
#include <hx/ErrorCodes.h>
#include <hx/GC.h>
#include "null.h"
#include <hx/Object.h>
#include "hxString.h"
#include "Dynamic.h"
#include <cpp/CppInt32__.h>
// This needs to "see" other declarations ...
#include <hx/GcTypeInference.h>
#include <hx/FieldRef.h>
#include <hx/Anon.h>
#include "Array.h"
#include <hx/Class.h>
#include "Enum.h"
#include <hx/Interface.h>
#include <hx/Telemetry.h>
#include <hx/StdLibs.h>
#include <cpp/Pointer.h>
#include <hx/Operators.h>
#include <hx/Functions.h>
#include <hx/Debug.h>
#include <hx/Boot.h>
#include <hx/Undefine.h>

#endif

