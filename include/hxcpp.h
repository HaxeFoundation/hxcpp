#ifndef HXCPP_H
#define HXCPP_H

// Standard headers ....

// Windows hack
#define NOMINMAX


#ifdef _MSC_VER
   #include <typeinfo.h>
   namespace hx { typedef ::type_info type_info; }
   #undef TRUE
   #undef FALSE
   #undef BOOLEAN
   #undef ERROR
   #undef NO_ERROR
   #undef DELETE
   #undef OPTIONS
   #undef IN
   #undef OUT
   #undef ALTERNATE
   #undef OPTIONAL
   #undef DOUBLE_CLICK
   #undef DIFFERENCE
   #undef POINT
   #undef RECT
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

#if defined(EMSCRIPTEN) || defined(IPHONE)
  #include <unistd.h>
  #include <cstdlib>
#endif


#include <string.h>

#define HX_UTF8_STRINGS

#include <wchar.h>

#ifdef HX_LINUX
  #include <unistd.h>
  #include <cstdio>
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
  #else
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES __declspec(dllexport)
  #endif
#else
  #if defined(HXCPP_DLL_EXPORT)
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES __attribute__((visibility("default")))
  #else
     #define HXCPP_EXTERN_CLASS_ATTRIBUTES
  #endif
#endif

typedef char HX_CHAR;


#define HX_STRINGI(s,len) ::String( (const HX_CHAR *)(("\xff\xff\xff\xff" s)) + 4 ,len)

#define HX_STRI(s) HX_STRINGI(s,sizeof(s)/sizeof(HX_CHAR)-1)

#define HX_CSTRING(x) HX_STRI(x)

#define HX_CSTRING2(wide,len,utf8) HX_STRI(utf8)

#define HX_FIELD_EQ(name,field) !::memcmp(name.__s, field, sizeof(field)/sizeof(char))


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


#ifdef HXCPP_BIG_ENDIAN
#define HX_HCSTRING(s,h0,h1,h2,h3) ::String( (const HX_CHAR *)((h3 h2 h1 h0 "\x80\x00\x00\x00" s )) + 8 , sizeof(s)/sizeof(HX_CHAR)-1)
#else
#define HX_HCSTRING(s,h0,h1,h2,h3) ::String( (const HX_CHAR *)((h0 h1 h2 h3 "\x00\x00\x00\x80" s )) + 8 , sizeof(s)/sizeof(HX_CHAR)-1)
#endif


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
   #elif !defined(EMSCRIPTEN)
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
template<typename ELEM_> class Array_obj;
template<typename ELEM_> class Array;
class Class_obj;
typedef hx::ObjectPtr<Class_obj> Class;
namespace hx { typedef ObjectPtr<Class_obj> Class; }
class Dynamic;
class String;

// Use an external routine to throw to avoid sjlj overhead on iphone.
namespace hx { HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic Throw(Dynamic inDynamic); }
namespace hx { extern void CriticalError(const String &inError); }
namespace hx { extern void NullReference(const char *type, bool allowFixup); }
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
#include <hx/GCTemplates.h>
#include <hx/FieldRef.h>
#include <hx/Anon.h>
#include "Array.h"
#include "Class.h"
#include "Enum.h"
#include <hx/Interface.h>
#include <hx/StdLibs.h>
#include <hx/Operators.h>
#include <hx/Functions.h>
#include <hx/Debug.h>
#include <hx/Boot.h>
#include <hx/Undefine.h>
#include <cpp/Pointer.h>

#endif

