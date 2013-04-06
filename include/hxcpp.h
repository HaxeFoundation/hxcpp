#ifndef HXCPP_H
#define HXCPP_H

// Standard headers ....

// Windows hack
#define NOMINMAX


#ifdef _MSC_VER
#include <typeinfo.h>
namespace hx { typedef ::type_info type_info; }
#else
#include <typeinfo>
#include <stdint.h>
namespace hx { typedef std::type_info type_info; }
#ifndef EMSCRIPTEN
using hx::type_info;
typedef  int64_t  __int64;
#endif
#endif

#ifdef EMSCRIPTEN
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
#ifdef NULL
#undef NULL
#endif

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

#define HX_FIELD_EQ(name,field) !memcmp(name.__s, field, sizeof(field)/sizeof(char))


#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER)) && !defined HXCPP_CHECK_POINTER
#define HXCPP_CHECK_POINTER
#endif



#ifdef BIG_ENDIAN
#undef BIG_ENDIAN

  #ifndef HX_LITTLE_ENDIAN
  #define HX_LITTLE_ENDIAN 0
  #endif
#endif

#ifdef __BIG_ENDIAN__
  #ifndef HX_LITTLE_ENDIAN
  #define HX_LITTLE_ENDIAN 0
  #endif
#endif

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN

  #ifndef HX_LITTLE_ENDIAN
  #define HX_LITTLE_ENDIAN 1
  #endif
#endif

#ifdef __LITTLE_ENDIAN__
  #ifndef HX_LITTLE_ENDIAN
  #define HX_LITTLE_ENDIAN 1
  #endif
#endif

#ifndef HX_LITTLE_ENDIAN
#define HX_LITTLE_ENDIAN 1
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

// --- Forward decalarations --------------------------------------------

namespace haxe { namespace io { typedef unsigned char Unsigned_char__; } }
namespace cpp { class CppInt32__; }
namespace hx { class Object; }
namespace hx { class FieldMap; }
namespace hx { class FieldRef; }
namespace hx { class IndexRef; }
namespace hx { template<typename O> class ObjectPtr; }
template<typename ELEM_> class Array_obj;
template<typename ELEM_> class Array;
class Class_obj;
typedef hx::ObjectPtr<Class_obj> Class;
class Dynamic;
class String;

// Use an external routine to throw to avoid sjlj overhead on iphone.
namespace hx { HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic Throw(Dynamic inDynamic); }
namespace hx { extern void CriticalError(const String &inError); }
namespace hx { extern void NullObjectReference(); }
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

#endif

