#ifndef HX_CFFI_H
#define HX_CFFI_H

// 410 - adds gc_try_unblocking
#define HX_CFFI_API_VERSION 410

#ifdef HXCPP_JS_PRIME
#include <emscripten/bind.h>
using namespace emscripten;

typedef struct emscripten::val value;
typedef struct _vkind  *vkind;
typedef struct _buffer  *buffer;
#define HAVE_NEKO_TYPES 1
#endif

#include "OS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(BLACKBERRY)
using namespace std;
#endif
// --- Register functions (primitives) ----

#ifdef STATIC_LINK

#define DEFINE_PRIM_MULT(func) \
int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func)); \

#define DEFINE_PRIM(func,nargs) \
int __reg_##func = hx_register_prim(#func "__" #nargs,(void *)(&func)); \


#define DEFINE_PRIM_MULT_NATIVE(func,ext) \
int __reg_##func = hx_register_prim(#func "__MULT",(void *)(&func)) + \
                   hx_register_prim(#func "__"  #ext,(void *)(&func##_##ext)) ; 

#define DEFINE_PRIM_NATIVE(func,nargs,ext) \
int __reg_##func = hx_register_prim(#func "__" #nargs,(void *)(&func)) + \
                   hx_register_prim(#func "__" #ext,(void *)(&func##_##ext)) ; 


#define DEFINE_LIB_PRIM_MULT(lib,func) \
int __reg_##func = hx_register_prim(lib "_" #func "__MULT",(void *)(&func)); \

#define DEFINE_LIB_PRIM(lib,func,nargs) \
int __reg_##func = hx_register_prim(lib "_" #func "__" #nargs,(void *)(&func)); \


#elif defined(HXCPP_JS_PRIME)

//#define DEFINE_PRIM_MULT(func) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }
//TODO
#define DEFINE_PRIM_MULT(func)

#define DEFINE_PRIM(func,nargs) EMSCRIPTEN_BINDINGS(func) { function(#func, &func); }


#else


#define DEFINE_PRIM_MULT(func) extern "C" { \
  EXPORT void *func##__MULT() { return (void*)(&func); } \
}


#define DEFINE_PRIM(func,nargs) extern "C" { \
  EXPORT void *func##__##nargs() { return (void*)(&func); } \
}


#define DEFINE_PRIM_MULT_NATIVE(func,ext) extern "C" { \
  EXPORT void *func##__MULT() { return (void*)(&func); } \
  EXPORT void *func##__##ext() { return (void*)(&func##_##ext); } \
}


#define DEFINE_PRIM_NATIVE(func,nargs,ext) extern "C" { \
  EXPORT void *func##__##nargs() { return (void*)(&func); } \
  EXPORT void *func##__##ext() { return (void*)(&func##_##ext); } \
}


#define DEFINE_LIB_PRIM_MULT(lib,func) extern "C" { \
  EXPORT void *func##__MULT() { return (void*)(&func); } \
}


#define DEFINE_LIB_PRIM(lib,func,nargs) extern "C" { \
  EXPORT void *func##__##nargs() { return (void*)(&func); } \
}



#endif // !STATIC_LINK




 
#define DEFFUNC_0(ret,name) DEFFUNC(name,ret, (), ())
#define DEFFUNC_1(ret,name,t1) DEFFUNC(name,ret, (t1 a1), (a1))
#define DEFFUNC_2(ret,name,t1,t2) DEFFUNC(name,ret, (t1 a1, t2 a2), (a1,a2))
#define DEFFUNC_3(ret,name,t1,t2,t3) DEFFUNC(name,ret, (t1 a1, t2 a2, t3 a3), (a1,a2,a3))
#define DEFFUNC_4(ret,name,t1,t2,t3,t4) DEFFUNC(name,ret, (t1 a1, t2 a2, t3 a3, t4 a4), (a1,a2,a3,a4))
#define DEFFUNC_5(ret,name,t1,t2,t3,t4,t5) DEFFUNC(name,ret, (t1 a1, t2 a2, t3 a3, t4 a4,t5 a5), (a1,a2,a3,a4,a5))
 

enum hxValueType
{
   valtUnknown = -1,
   valtInt = 0xff,
   valtNull = 0,
   valtFloat = 1,
   valtBool = 2,
   valtString = 3,
   valtObject = 4,
   valtArray = 5,
   valtFunction = 6,
   valtEnum,
   valtClass,
   valtRoot = 0xff,
   valtAbstractBase = 0x100,
};

namespace hx
{
enum StringEncoding
{
   StringAscii,
   StringUtf8,
   StringUtf16
};
}

// val_fun_nargs may return a special value
enum { faNotFunction = -2, faVarArgs=-1, faArgs0=0 /* ... */ };

typedef int field;



#ifdef IMPLEMENT_API
#include "CFFILoader.h"
#endif


#if !defined(HAVE_NEKO_TYPES)
#ifdef HXCPP_NATIVE_CFFI_VALUE
namespace hx { class Object; }
typedef hx::Object _value;
#else
struct _value;
#endif
typedef _value *value;
typedef struct _vkind  *vkind;
typedef struct _buffer  *buffer;
#endif

typedef buffer cffiByteBuffer;

typedef struct _gcroot  *gcroot;

typedef void (*hxFinalizer)(value v);
typedef void (*hxPtrFinalizer)(void *v);

typedef void (__hx_field_iter)(value v,field f,void *);

#define hx_failure(msg)		hx_fail(msg,__FILE__,__LINE__)

#ifndef IGNORE_CFFI_API_H


#ifndef IMPLEMENT_API
 
#if defined(STATIC_LINK) || defined(HXCPP_JS_PRIME)

#define DEFFUNC(name,ret,def_args,call_args) \
extern "C" ret name def_args;


#else

#define DEFFUNC(name,ret,def_args,call_args) \
typedef ret (*FUNC_##name) def_args; \
extern FUNC_##name name;

#endif

#endif
 

#include "CFFIAPI.h"


#ifdef WANT_DYNALLOC_ALLOC_BYTES
void *DynAlloc::allocBytes(size_t n)
{
   return hx_alloc((int)n);
}
#endif



#define DEFINE_KIND(name) extern "C" { vkind name = 0; }

#ifdef STATIC_LINK
#	define DEFINE_ENTRY_POINT(name)
#else
#	define DEFINE_ENTRY_POINT(name) extern "C" {  void name(); EXPORT void *__neko_entry_point() { return (void *)&name; } }
#endif

#ifdef HEADER_IMPORTS
#	define H_EXTERN IMPORT
#else
#	define H_EXTERN EXPORT
#endif

#define DECLARE_PRIM(func,nargs) extern "C" {  H_EXTERN void *func##__##nargs(); }
#define DECLARE_KIND(name) extern "C" {  H_EXTERN extern vkind name; }



// --- Helpers ----------------------------------------------------------------

// Check type...
inline bool val_is_null(value inVal) { return val_type(inVal)==valtNull; }
inline bool val_is_int(value inVal) { return val_type(inVal)==valtInt; }
inline bool val_is_bool(value inVal) { return val_type(inVal)==valtBool; }
inline bool val_is_float(value inVal) { return val_type(inVal)==valtFloat; }
inline bool val_is_string(value inVal) { return val_type(inVal)==valtString; }
inline bool val_is_function(value inVal) { return val_type(inVal)==valtFunction; }
inline bool val_is_array(value inVal) { return val_type(inVal)==valtArray; }
inline bool val_is_abstract(value inVal) { return val_type(inVal)>=valtAbstractBase; }
inline bool val_is_kind(value inVal,vkind inKind) { return val_to_kind(inVal,inKind)!=0; }

inline bool val_is_number(value inVal)
{
	int t = val_type(inVal);
	return t==valtInt || t==valtFloat;
}
inline bool val_is_object(value inVal)
{
	int t = val_type(inVal);
	return t==valtObject || t==valtEnum ||t==valtClass;
}

class AutoGCBlocking
{
public:
	AutoGCBlocking() : mLocked( gc_try_blocking() ) {  }
	~AutoGCBlocking() { Close(); }
	void Close() { if (mLocked) gc_exit_blocking(); mLocked = false; }

	bool mLocked;
};

class AutoGCUnblocking
{
public:
	AutoGCUnblocking() : mUnlocked( gc_try_unblocking() ) {  }
	~AutoGCUnblocking() { Close(); }
	void Close() { if (mUnlocked) gc_enter_blocking(); mUnlocked = false; }

	bool mUnlocked;
};


class AutoGCRoot
{
public:
   AutoGCRoot(value inValue)
   {
		mRoot = 0;
		mPtr = alloc_root();
		if (mPtr)
			*mPtr = inValue;
		else
			mRoot = create_root(inValue);
   }

  ~AutoGCRoot()
   {
		if (mPtr)
			free_root(mPtr);
		else if (mRoot)
         destroy_root(mRoot);
   }
   value get()const { return mPtr ? *mPtr : query_root(mRoot); }
   void set(value inValue)
	{ 
		if (mPtr)
			*mPtr = inValue;
		else
		{
			if (mRoot) destroy_root(mRoot);
			mRoot = create_root(inValue);
		}
	}
   
private:
   value *mPtr;
   gcroot mRoot;
   AutoGCRoot(const AutoGCRoot &);
   void operator=(const AutoGCRoot &);
};

struct CffiBytes
{
   CffiBytes( unsigned char *inData=0, int inLength=0) : data(inData), length(inLength) {}

   unsigned char *data;
   int length;
};

inline CffiBytes getByteData(value inValue)
{
   if (val_is_object(inValue))
   {
      static field bField = 0;
      static field lengthField = 0;
      if (bField==0)
      {
         bField = val_id("b");
         lengthField = val_id("length");
      }
      value b = val_field(inValue, bField);
      value len = val_field(inValue, lengthField);
      if (val_is_string(b) && val_is_int(len))
         return CffiBytes( (unsigned char *)val_string(b), val_int(len) );
      if (val_is_buffer(b) && val_is_int(len))
         return CffiBytes( (unsigned char *)buffer_data(val_to_buffer(b)), val_int(len) );
   }
   return CffiBytes();
}

inline bool resizeByteData(value inValue, int inNewLen)
{
   if (!val_is_object(inValue))
      return false;

   static field bField = 0;
   static field lengthField = 0;
   if (bField==0)
   {
      bField = val_id("b");
      lengthField = val_id("length");
   }
   value len = val_field(inValue, lengthField);
   if (!val_is_int(len))
      return false;
   int oldLen = val_int(len);
   value b = val_field(inValue, bField);
   if (val_is_string(b))
   {
      if (inNewLen>oldLen)
      {
         value newString = alloc_raw_string(inNewLen);
         memcpy( (char *)val_string(newString), val_string(b), inNewLen);
         alloc_field(inValue, bField, newString );
      }
      alloc_field(inValue, lengthField, alloc_int(inNewLen) );
   }
   else if (val_is_buffer(b))
   {
      cffiByteBuffer buf = val_to_buffer(b);
      buffer_set_size(buf,inNewLen);
      alloc_field(inValue, lengthField, alloc_int(inNewLen) );
   }
   else
      return false;

   return true;
}


#define val_null alloc_null()

#define bfailure(x) val_throw(buffer_to_string(x))

#define copy_string(str,len) alloc_string_len(str,len)


// The "Check" macros throw an error if assumtion is false
#define val_check_kind(v,t)	if( !val_is_kind(v,t) ) hx_failure("invalid kind");
#define val_check_function(f,n) if( !val_is_function(f) || (val_fun_nargs(f) != (n) && val_fun_nargs(f) != faVarArgs) ) hx_failure("Bad function");
#define val_check(v,t)		if( !val_is_##t(v) ) hx_failure("type not " #t);

// The "Get" function will return or force an error
inline bool val_get_bool(value inVal) {  val_check(inVal,bool); return val_bool(inVal); }
inline int val_get_int(value inVal) {  val_check(inVal,int); return val_int(inVal); }
inline double val_get_double(value inVal) {  val_check(inVal,number); return val_number(inVal); }
inline const char *val_get_string(value inVal) {  val_check(inVal,string); return val_string(inVal); }
inline void *val_get_handle(value inVal,vkind inKind)
  {  val_check_kind(inVal,inKind); return val_to_kind(inVal,inKind); }


inline value alloc_string(const char *inStr)
{
   const char *end = inStr;
   while(*end) end++;
   return alloc_string_len(inStr,(int)(end-inStr));
}

inline value alloc_wstring(const wchar_t *inStr)
{
   const wchar_t *end = inStr;
   while(*end) end++;
   return alloc_wstring_len(inStr,(int)(end-inStr));
}

inline void hxcpp_unscramble(const unsigned char *bytes, int len, const char *key, unsigned char *dest)
{
   int keyLen = 0;
   while(key[keyLen])
      keyLen++;
   int state = 0;
   //state = ((state + key[i]) ^ ch) & 0xff);
   for(int i=0;i<len;i++)
   {
      dest[i] = ( (state + key[i%keyLen]) ^ bytes[i] ) & 0xff;
      state = bytes[i];
   }
}


//additional glue for easier neko modules compilation
#define val_true    alloc_bool(true)
#define val_false    alloc_bool(false)
inline void neko_error() { hx_error(); }


// Conservative marking within a buffer is not yet supported.
//inline void * alloc(int i) { return hx_alloc(i); }

// The bytes themselves will be GC'd, but not the pointers contained within.
inline void * alloc_private(int i) { return hx_alloc(i); }

// You should use alloc_buffer_len/buffer_data instead
//value alloc_empty_string(int len) { }


#endif


#endif
