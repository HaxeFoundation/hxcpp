#ifndef HXCPP_H
#define HXCPP_H

// Standard headers ....

#include <string.h>
#include <wchar.h>

#ifdef _MSC_VER
#include <typeinfo.h>
#else
#include <typeinfo>
using std::type_info;
#endif


#ifdef HX_LINUX
#include <unistd.h>
#include <stdint.h>
#include <cstdio>
#endif


// Some compilers are over-enthusiastic about what they #define ...
#ifdef NULL
#undef NULL
#endif

#ifdef assert
#undef assert
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
typedef double Float;
typedef bool Bool;

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
namespace hx { extern Dynamic Throw(Dynamic inDynamic); }
namespace hx { extern void CriticalError(const wchar_t *inError); }
namespace hx { extern String sNone[]; }
void __hxcpp_check_overflow(int inVal);


// HX_INTERNAL_GC is now the default, unless HXCPP_BOEHM_GC is defined...

#ifndef HXCPP_BOEHM_GC
#ifndef HX_INTERNAL_GC
  #define HX_INTERNAL_GC
#endif
#endif

#ifdef HX_INTERNAL_GC
  #define GC_CLEARS_ALL
#else
  #ifndef HXCPP_MULTI_THREADED
     #define HXCPP_MULTI_THREADED
  #endif
#endif





// The order of these includes has been chosen to minimize forward declarations.
// You should not include the individual files, just this one.
#include <hx/Macros.h>
#include <hx/ErrorCodes.h>
#include <hx/GC.h>
#include "null.h"
#include <hx/Object.h>
#include <cpp/CppInt32__.h>
#include "hxString.h"
#include "Dynamic.h"
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

#include <hx/Boot.h>

#endif
