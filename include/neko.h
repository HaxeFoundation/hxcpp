/* ************************************************************************ */
/*																			*/
/*  Neko Virtual Machine													*/
/*  Copyright (c)2005 Motion-Twin											*/
/*																			*/
/* This library is free software; you can redistribute it and/or			*/
/* modify it under the terms of the GNU Lesser General Public				*/
/* License as published by the Free Software Foundation; either				*/
/* version 2.1 of the License, or (at your option) any later version.		*/
/*																			*/
/* This library is distributed in the hope that it will be useful,			*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of			*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU		*/
/* Lesser General Public License or the LICENSE file for more details.		*/
/*																			*/
/* ************************************************************************ */
#ifndef _HXCPP_H
#define _HXCPP_H

// OS FLAGS
#if defined(_WIN32)
#	define NEKO_WINDOWS
#endif

#if defined(__APPLE__) || defined(__MACH__) || defined(macintosh)
#	define NEKO_MAC
#endif

#if defined(linux) || defined(__linux__)
#	define NEKO_LINUX
#endif

#if defined(__FreeBSD_kernel__)
#	define NEKO_GNUKBSD
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define NEKO_BSD
#endif

// COMPILER/PROCESSOR FLAGS
#if defined(__GNUC__)
#	define NEKO_GCC
#endif

#if defined(_MSC_VER)
#	define NEKO_VCC
#endif

#if defined(__MINGW32__)
#	define NEKO_MINGW
#endif

#if defined(__i386__) || defined(_WIN32)
#	define NEKO_X86
#endif

#if defined(__ppc__)
#	define NEKO_PPC
#endif

#if defined(_64BITS)
#	define NEKO_64BITS
#endif

#if defined(NEKO_LINUX) || defined(NEKO_MAC) || defined(NEKO_BSD) || defined(NEKO_GNUKBSD)
#	define NEKO_POSIX
#endif

#if defined(NEKO_GCC)
#	define NEKO_THREADED
#	define NEKO_DIRECT_THREADED
#endif

#include <hxObject.h>
#include <stddef.h>
#ifndef NEKO_VCC
#	include <stdint.h>
#endif



#ifdef NEKO_POSIX
#	include <errno.h>
#	define POSIX_LABEL(name)	name:
#	define HANDLE_EINTR(label)	if( errno == EINTR ) goto label
#	define HANDLE_FINTR(f,label) if( ferror(f) && errno == EINTR ) goto label
#else
#	define POSIX_LABEL(name)
#	define HANDLE_EINTR(label)
#	define HANDLE_FINTR(f,label)
#endif


// Conditional compile
#define HXCPP


#define VAL_INT vtInt
#define VAL_NULL vtNull
#define VAL_FLOAT vtFloat
#define VAL_BOOL vtBool
#define VAL_STRING vtString
#define VAL_OBJECT vtObject
#define VAL_ARRAY vtArray
#define VAL_FUNCTION vtFunction
#define VAL_ENUM vtEnum
#define VAL_CLASS vtClass
//#define VAL_ABSTRACT - this is actually a range of constants in c++



typedef Dynamic value;

typedef int field;
typedef double tfloat;
typedef int int_val;

typedef void (*finalizer)(value v);

typedef String vstring;

typedef Array<Dynamic> varray;


typedef int vkind;

class Abstract_obj : public hxObject
{
public:
   Abstract_obj(int inType,void *inData)
   {
      mType = inType;
      mHandle = inData;
   }

   virtual int __GetType() const { return mType; }
   virtual hxObjectPtr<Class_obj> __GetClass() const { return 0; }
   virtual bool __IsClass(Class inClass ) const { return false; }

   virtual void *__GetHandle() const
   {
      return mHandle;
   }

   void *mHandle;
   int mType;
};

typedef hxObjectPtr<Abstract_obj> Abstract;

class ArrayDynamicAcess
{
public:
   ArrayDynamicAcess(const Dynamic &inArray) : mArray(inArray) { }
   ArrayDynamicAcess(hxObject *inArray) : mArray(inArray) { }
   Dynamic operator[](int inIndex) { return mArray->__GetItem(inIndex); }

private:
   Dynamic mArray;
};

typedef ArrayDynamicAcess array_ptr;

inline bool val_is_bytes(const Dynamic &v)
{
   Array<unsigned char> array = v.Cast<Array<unsigned char> >();
   return array != null();
}


inline char *val_bytes(const Dynamic &v)
{
   Array<unsigned char> array = v.Cast<Array<unsigned char> >();
   if (array==null()) return 0;
   return array->GetBase();
}
inline void val_set_size(Dynamic &v,int inLen) { v->__SetSize(inLen); }


// String buffer
struct hxStringBuffer : Array_obj<String>
{
   inline hxStringBuffer(const char *init) : Array_obj<String>(0,1) { push(init); }
   inline void Append(const char *inString) { push(inString); }
   inline void Append(const char *inString,int inLen) { push(String(inString,inLen)); }
   inline void Append(char inChar) { push(String(&inChar,1)); }
   inline void Append(const String &inS) { push(inS); }
   inline String ToString() { return join(String(L"",0)); }
};

typedef hxStringBuffer *buffer;

inline buffer alloc_buffer( const char *init ) { return new hxStringBuffer(init); }
inline void buffer_append( buffer b, const char *s ) { b->Append(s); }
inline void buffer_append_sub( buffer b, const char *s, int len ) { b->Append(s,len); }
inline void buffer_append_char( buffer b, char c ) { b->Append(c); }
inline String buffer_to_string( buffer b ) { return b->ToString(); }
inline void val_buffer( buffer b, value v ) { b->Append(v->__ToString()); }

inline void bfailure(buffer b) { throw Dynamic(b->ToString()); }
inline void *alloc(int inLen) { return hxNewGCBytes(0,inLen); }
inline void *alloc_private(int inLen) { return hxNewGCPrivate(0,inLen); }

inline int val_fun_nargs(Dynamic inD) { return inD->__ArgCount(); }
inline Dynamic val_call0(Dynamic f) { return f->__run(); }
inline Dynamic val_call1(Dynamic f,Dynamic a1) { return f->__run(a1); }
inline Dynamic val_call2(Dynamic f,Dynamic a1,Dynamic a2) { return f->__run(a1,a2); }
inline Dynamic val_call3(Dynamic f,Dynamic a1,Dynamic a2,Dynamic a3) { return f->__run(a1,a2,a3); }
inline Dynamic val_callN(Dynamic f,Dynamic args) { return f->__Run(args); }

inline Dynamic val_ocall0( Dynamic o, int field_id)
    { return o->__IField(field_id)->__run(); }
inline Dynamic val_ocall1( Dynamic o, int field_id, Dynamic a1 )
    { return o->__IField(field_id)->__run(a1); }
inline Dynamic val_ocall2( Dynamic o, int field_id, Dynamic a1, Dynamic a2 )
    { return o->__IField(field_id)->__run(a1,a2); }
inline Dynamic val_ocall3( Dynamic o, int field_id, Dynamic a1, Dynamic a2, Dynamic a3 )
    { return o->__IField(field_id)->__run(a1,a2,a3); }

inline const char *val_string(const Dynamic &v) { return v->__CStr(); }

#define copy_string(ptr,len) String(ptr,len).dup()

inline Dynamic alloc_array(int inLen) { return Array<Dynamic>(2,0); }
inline void val_throw(Dynamic inVal) { throw inVal; }
inline String alloc_empty_string( unsigned int size ) { return String((const char *)0,size); }


#define val_tag(v)			(v->__GetType())
#define val_is_null(v)		((v) == null())
#define val_is_int(v)		((v).GetPtr() && ((v)->__GetType()==vtInt))
#define val_is_bool(v)		((v).GetPtr() && ((v)->__GetType()==vtBool))
#define val_is_number(v)	((v).GetPtr() && (val_is_int(v) || (v)->__GetType()==vtFloat))
#define val_is_float(v)		((v).GetPtr() && ((v)->__GetType()==vtFloat))
#define val_is_string(v)	((v).GetPtr() && ((v)->__GetType()==vtString))
#define val_is_function(v)	((v).GetPtr() && ((v)->__GetType()==vtFunction))
#define val_is_object(v)	((v)!=null() && ((v)->__GetType()==vtObject || (v)->__GetType()==vtClass))
#define val_is_array(v)		((v)->__GetType()==vtArray)
#define val_is_abstract(v) ((v)->__GetType()>=vtAbstractBase)
#define val_is_kind(v,t)	((v)!=null() && ((v)->__GetType() == (t)))
#define val_check_kind(v,t)	if( !val_is_kind(v,t) ) neko_error();
#define val_check_function(f,n) if( !val_is_function(f) || (val_fun_nargs(f) != (n) && val_fun_nargs(f) != VAR_ARGS) ) neko_error();
#define val_check(v,t)		if( !val_is_##t(v) ) neko_error();
#define val_data(v)			((v)->__GetHandle())
#define val_kind(v)			((v)->__GetType())

#define val_type(v)			(val_tag(v))
#define val_int(v)			((v)->__ToInt())
#define val_float(v)		((v)->__ToDouble())
#define val_bool(v)			(val_int(v)!=0)
#define val_number(v)		(val_float(v))
//#define val_hdata(v)		((vhash*)val_data(v))
#define val_strlen(v)		((v)->__length())
//#define val_set_length(v,l) val_tag(v) = (val_tag(v)&7) | ((l) << 3)
//#define val_set_size		val_set_length

#define val_array_size(v)	((v)->__length())
#define val_array_ptr(v)	(ArrayDynamicAcess(v))
#define val_get_array_i(v,index)	((v)->__GetItem(index))
//#define val_fun_nargs(v)	((vfunction*)(v))->nargs
#define alloc_int(v)		((int)v)
#define alloc_bool(b)		((bool)(b))

#define max_array_size		((1 << 29) - 1)
#define max_string_size		((1 << 29) - 1)
#define invalid_comparison	0xFE

#undef EXTERN
#undef EXPORT
#undef IMPORT
#if defined(NEKO_VCC) || defined(NEKO_MINGW)
#	define INLINE __inline
#	define EXPORT __declspec( dllexport )
#	define IMPORT __declspec( dllimport )
#else
#	define INLINE inline
#	define EXPORT
#	define IMPORT
#endif

#if defined(NEKO_SOURCES)
#	define EXTERN EXPORT
#elif defined(NEKO_INSTALLER)
#	define EXTERN
#	undef EXPORT
#	undef IMPORT
#	define EXPORT
#	define IMPORT
#else
#	define EXTERN IMPORT
#endif

#define VEXTERN extern EXTERN

#define alloc_int32(i) (int(i))
#define alloc_best_int(i) ((int)(i))
#define val_int32(v) (val_int(v))
#define val_is_int32(v) (val_is_int(v))


#define neko_error()		return null()
#define failure(msg)		hxcpp_fail(msg,__FILE__,__LINE__)
//#define bfailure(buf)		_neko_failure(buffer_to_string(b),__FILE__,__LINE__)


#define VAR_ARGS (-1)
#define DEFINE_PRIM_MULT(func) extern "C" {  EXPORT void *func##__MULT() { return (void*)(&func); } }

#define DEFINE_PRIM(func,nargs) extern "C" {  EXPORT void *func##__##nargs() { return (void*)(&func); } }
#define DEFINE_KIND(name) vkind name = hxcpp_alloc_kind();

#ifdef NEKO_INSTALLER
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
#define DECLARE_KIND(name) extern "C" {  H_EXTERN extern int name; }


/*
#define alloc_float			neko_alloc_float
#define alloc_string		neko_alloc_string
#define alloc_empty_string	neko_alloc_empty_string
#define copy_string			neko_copy_string
#define val_this			neko_val_this
#define val_field			neko_val_field
#define alloc_object		neko_alloc_object
#define alloc_field			neko_alloc_field
#define alloc_array			neko_alloc_array
#define val_call0			neko_val_call0
#define val_call1			neko_val_call1
#define val_call2			neko_val_call2
#define val_call3			neko_val_call3
#define val_callN			neko_val_callN
#define val_ocall0			neko_val_ocall0
#define val_ocall1			neko_val_ocall1
#define val_ocall2			neko_val_ocall2
#define val_ocallN			neko_val_ocallN
#define val_callEx			neko_val_callEx
#define	alloc_root			neko_alloc_root
#define free_root			neko_free_root
#define alloc				neko_alloc
#define alloc_private		neko_alloc_private
#define alloc_abstract		neko_alloc_abstract
#define alloc_function		neko_alloc_function
#define alloc_buffer		neko_alloc_buffer
#define buffer_append		neko_buffer_append
#define buffer_append_sub	neko_buffer_append_sub
#define buffer_append_char	neko_buffer_append_char
#define buffer_to_string	neko_buffer_to_string
#define val_buffer			neko_val_buffer
#define val_compare			neko_val_compare
#define val_print			neko_val_print
#define val_gc				neko_val_gc
#define val_throw			neko_val_throw
#define val_rethrow			neko_val_rethrow
#define val_iter_fields		neko_val_iter_fields
#define val_field_name		neko_val_field_name
#define val_hash			neko_val_hash
#define k_int32				neko_k_int32
#define k_hash				neko_k_hash
#define kind_share			neko_kind_share
*/

#define val_true  true
#define val_false false
#define alloc_float(f) ((double)f)
inline Dynamic alloc_string(const char *str) { return String(str,(int)strlen(str)); }
#define val_id				__hxcpp_field_to_id
#define alloc_field			hxcpp_alloc_field
#define val_gc				hxcpp_val_gc
#define alloc_abstract(inKind,inData) Dynamic(new Abstract_obj(inKind,inData))
#define val_field			hxcpp_val_field
#define val_null			null()
//#define alloc_object(x) ( x==NULL ? (hxAnon_obj::Create()) : x )
#define empty_object (hxAnon_obj::Create())


inline Dynamic alloc_object(int x) { return empty_object; }
inline Dynamic alloc_object(const Dynamic &inObj) { return inObj; }

	VEXTERN vkind k_int32;
	VEXTERN vkind k_hash;

   EXTERN void hxcpp_fail(const char *inMsg,const char *inFile, int inLine);
   EXTERN int  hxcpp_alloc_kind();
   //EXTERN int  hxcpp_find_kind(const char *name);

	#define kind_share( k, name ) // Not sure ? *(k) = hxcpp_find_kind(name)

	EXTERN void  hxcpp_alloc_field( value obj, field f, value v );
	EXTERN void  hxcpp_val_gc( value v, finalizer f );

/*
	EXTERN value alloc_string( const char *str );
	EXTERN value alloc_empty_string( unsigned int size );
	EXTERN value copy_string( const char *str, int_val size );

	EXTERN value val_this();
	EXTERN field val_id( const char *str );
	EXTERN value val_field( value o, field f );
	EXTERN value alloc_object( value o );
	EXTERN void val_iter_fields( value obj, void f( value v, field f, void * ), void *p );
	EXTERN value val_field_name( field f );

	EXTERN value alloc_array( unsigned int n );
	EXTERN value alloc_abstract( vkind k, void *data );

	EXTERN value val_call0( value f );
	EXTERN value val_call1( value f, value arg );
	EXTERN value val_call2( value f, value arg1, value arg2 );
	EXTERN value val_call3( value f, value arg1, value arg2, value arg3 );
	EXTERN value val_callN( value f, value *args, int nargs );
	EXTERN value val_ocall0( value o, field f );
	EXTERN value val_ocall1( value o, field f, value arg );
	EXTERN value val_ocall2( value o, field f, value arg1, value arg2 );
	EXTERN value val_ocallN( value o, field f, value *args, int nargs );
	EXTERN value val_callEx( value vthis, value f, value *args, int nargs, value *exc );

	EXTERN value *alloc_root( unsigned int nvals );
	EXTERN void free_root( value *r );
	EXTERN char *alloc( unsigned int nbytes );
	EXTERN char *alloc_private( unsigned int nbytes );
	EXTERN value alloc_function( void *c_prim, unsigned int nargs, const char *name );

	EXTERN buffer alloc_buffer( const char *init );
	EXTERN void buffer_append( buffer b, const char *s );
	EXTERN void buffer_append_sub( buffer b, const char *s, int_val len );
	EXTERN void buffer_append_char( buffer b, char c );
	EXTERN value buffer_to_string( buffer b );
	EXTERN void val_buffer( buffer b, value v );

	EXTERN int val_compare( value a, value b );
	EXTERN void val_print( value s );
	EXTERN void val_gc( value v, finalizer f );
	EXTERN void val_throw( value v );
	EXTERN void val_rethrow( value v );
	EXTERN int val_hash( value v );

	EXTERN void kind_share( vkind *k, const char *name );
	EXTERN void _neko_failure( value msg, const char *file, int line );

*/


inline value hxcpp_val_field( value o, field f ) { return o->__IField( f ); }
#endif
/* ************************************************************************ */
