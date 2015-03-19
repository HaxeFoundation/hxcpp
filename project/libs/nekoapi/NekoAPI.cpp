// Get headers etc.
//

//#define HAVE_NEKO_TYPES

typedef struct _vkind *vkind;
typedef struct _value *value;
typedef struct _buffer *buffer;


#define IGNORE_CFFI_API_H
#include <hx/CFFI.h>
#include "string.h"


extern "C" {

// Proto-type the functions
#define DEFFUNC(name,r,b,c) r api_##name b;
#include <hx/CFFIAPI.h>
#undef DEFFUNC



EXPORT
void * hx_cffi(const char *inName)
{
	#define DEFFUNC(name,r,b,c) if ( !strcmp(inName,#name) ) return (void *)api_##name;

	#include <hx/CFFIAPI.h>
	return 0;
}

}


#ifndef neko_v1
#include "neko2.h"
#else
#include "neko.h"
#endif

static int __a_id = val_id("__a");
static int __s_id = val_id("__s");
static int b_id = val_id("b");
static int length_id = val_id("length");

value *gNeko2HaxeString = 0;
value *gNekoNewArray = 0;

value haxe_alloc_string(const char *inString)
{
   value neko_string = alloc_string(inString);
   if (gNeko2HaxeString)
      return val_call1(*gNeko2HaxeString,neko_string);
   return neko_string;
}
 
void api_hx_error()
{
	failure("error");
}


void api_val_throw(value arg1)
{
	val_throw(arg1);
}


void api_hx_fail(const char * arg1,const char * arg2,int arg3)
{
	_neko_failure( haxe_alloc_string(arg1), arg2, arg3 );
}
#define NOT_IMPLEMNETED(func) api_hx_fail("NOT Implemented:" func,__FILE__,__LINE__)


// Determine value type
int api_val_type(value  arg1)
{
	int t=val_type(arg1);
	if (t==VAL_OBJECT)
	{
		value __a = val_field(arg1,__a_id);
		if (val_is_array(__a))
			return valtArray;
		value __s = val_field(arg1,__s_id);
		if (val_is_string(__s))
			return valtString;
	}
	if (t<7)
		return (ValueType)t;
	if (t==VAL_ABSTRACT)
		return valtAbstractBase;

	if (t==VAL_PRIMITIVE || t==VAL_JITFUN)
		return valtFunction;
	if (t==VAL_32_BITS || t==VAL_INT)
		return valtInt;
	return valtNull;
}


vkind api_val_kind(value  arg1)
{
	return (vkind)val_kind(arg1);
}


void * api_val_to_kind(value  arg1,vkind arg2)
{
	vkind k = (vkind)val_kind(arg1);
	if (k!=arg2)
		return 0;
	return val_data(arg1);
}


// don't check the 'kind' ...
void * api_val_data(value  arg1)
{
	return val_data(arg1);
}


int api_val_fun_nargs(value  arg1)
{
	return val_fun_nargs(arg1);
}




// Extract value  type
bool api_val_bool(value  arg1)
{
	return val_bool(arg1);
}


int api_val_int(value  arg1)
{
	return val_int32(arg1);
}


double api_val_float(value  arg1)
{
	return val_float(arg1);
}


double api_val_number(value  arg1)
{
	return val_number(arg1);
}


// Create value  type

value  api_alloc_null() { return val_null; }
value  api_alloc_bool(bool arg1) { return alloc_bool(arg1); }
value  api_alloc_int(int arg1)
{
   // TODO int32 ?
	return alloc_int(arg1);
}
value  api_alloc_float(double arg1) { return alloc_float(arg1); }

value  api_alloc_empty_object() { return alloc_object(0); }


value  api_alloc_abstract(vkind arg1,void * arg2)
{
	return alloc_abstract(arg1,arg2);
}

void api_free_abstract(value inAbstract)
{
   val_gc(inAbstract,0);
   val_kind(inAbstract) = 0;
}

value api_create_abstract(vkind inKind,int inMemSize, finalizer inFree )
{
   void *mem = alloc(inMemSize);
   value result = alloc_abstract(inKind, mem );
   val_gc(result, inFree);
   return result;
}


value  api_alloc_best_int(int arg1) { return alloc_best_int(arg1); }
value  api_alloc_int32(int arg1) { return  alloc_int32(arg1); }



// String access
int api_val_strlen(value  arg1)
{
	if (val_is_string(arg1))
	   return val_strlen(arg1);
	value l =  val_field(arg1,length_id);
	if (val_is_int(l))
		return val_int(l);
	return 0;
}


const wchar_t * api_val_wstring(value  arg1)
{
	int len = api_val_strlen(arg1);
	unsigned char *ptr = (unsigned char *)api_val_string(arg1);
	wchar_t *result = (wchar_t *)alloc_private((len+1)*sizeof(wchar_t));
	for(int i=0;i<len;i++)
		result[i] = ptr[i];
	result[len] = 0;
	return result;
}


const char * api_val_string(value  arg1)
{
	if (val_is_string(arg1))
	   return val_string(arg1);
	value s = val_field(arg1,__s_id);
	return val_string(s);
}


value  api_alloc_string(const char * arg1)
{
	return haxe_alloc_string(arg1);
}

wchar_t * api_val_dup_wstring(value inVal)
{
	return (wchar_t *)api_val_wstring(inVal);
}

char * api_val_dup_string(value inVal)
{
	int len = api_val_strlen(inVal);
	const char *ptr = api_val_string(inVal);
	char *result = alloc_private(len+1);
	memcpy(result,ptr,len);
	result[len] = '\0';
	return result;
}

value api_alloc_string_len(const char *inStr,int inLen)
{
	if (gNeko2HaxeString)
		return val_call1(*gNeko2HaxeString,copy_string(inStr,inLen));
   return copy_string(inStr,inLen);
}


value api_alloc_wstring_len(const wchar_t *inStr,int inLen)
{
	char *result = alloc_private(inLen+1);
	for(int i=0;i<inLen;i++)
		result[i] = inStr[i];
	result[inLen] = 0;
   return api_alloc_string_len(result,inLen);
}

// Array access - generic
int api_val_array_size(value  arg1)
{
	if (val_is_array(arg1))
	   return val_array_size(arg1);
	value l = val_field(arg1,length_id);
	return val_int(l);
}


value  api_val_array_i(value  arg1,int arg2)
{
	if (val_is_array(arg1))
	   return val_array_ptr(arg1)[arg2];
	return val_array_ptr(val_field(arg1,__a_id))[arg2];
}

void api_val_array_set_i(value  arg1,int arg2,value inVal)
{
	if (!val_is_array(arg1))
		arg1 = val_field(arg1,__a_id);
	val_array_ptr(arg1)[arg2] = inVal;
}

void api_val_array_set_size(value  arg1,int inLen)
{
	NOT_IMPLEMNETED("api_val_array_set_size");
}

void api_val_array_push(value  arg1,value inValue)
{
	NOT_IMPLEMNETED("api_val_array_push");
}


value  api_alloc_array(int arg1)
{
   if (!gNekoNewArray)
	   return alloc_array(arg1);
	return val_call1(*gNekoNewArray,alloc_int(arg1));
}



// Array access - fast if possible - may return null
// Resizing the array may invalidate the pointer
bool * api_val_array_bool(value  arg1)
{
	return 0;
}


int * api_val_array_int(value  arg1)
{
	return 0;
}


double * api_val_array_double(value  arg1)
{
	return 0;
}

float * api_val_array_float(value  arg1)
{
	return 0;
}


value * api_val_array_value(value  arg1)
{
	if (val_is_array(arg1))
	   return val_array_ptr(arg1);
	return val_array_ptr(val_field(arg1,__a_id));
}


// Byte arrays
buffer api_val_to_buffer(value  arg1)
{
	return alloc_buffer(api_val_string(arg1));
}

buffer api_alloc_buffer(const char *inStr)
{
	return alloc_buffer(inStr);
}


buffer api_alloc_buffer_len(int inLen)
{
	char *s=alloc_private(inLen+1);
	memset(s,' ',inLen);
	s[inLen] = 0;
	buffer b = alloc_buffer(s);
	return b;
}


value api_buffer_val(buffer b)
{
	return buffer_to_string(b);
}


value api_buffer_to_string(buffer inBuffer)
{
	return buffer_to_string(inBuffer);
}


void api_buffer_append(buffer inBuffer,const char *inStr)
{
	buffer_append(inBuffer,inStr);
}


int api_buffer_size(buffer inBuffer)
{
	return val_int(val_field((value)inBuffer,length_id));
}

void api_buffer_set_size(buffer inBuffer,int inLen)
{
	NOT_IMPLEMNETED("api_buffer_set_size");
}

bool api_val_is_buffer(value inVal)
{
   return false;
}

void api_buffer_append_sub(buffer inBuffer,const char *inStr,int inLen)
{
	buffer_append_sub(inBuffer,inStr,inLen);
}


void api_buffer_append_char(buffer inBuffer,int inChar)
{
	char buf[2] = { inChar, '\0' };
	buffer_append_sub(inBuffer,buf,1);
}


char * api_buffer_data(buffer inBuffer)
{
	return (char *)val_string(val_field((value)inBuffer,b_id));
}


// Append value to buffer
void api_val_buffer(buffer inBuffer,value inValue)
{
	val_buffer(inBuffer,inValue);
}






// Call Function 
value  api_val_call0(value  arg1)
{
	return val_call0(arg1);
}

value  api_val_call0_traceexcept(value  arg1)
{
	NOT_IMPLEMNETED("api_val_call0_traceexcept");
	return val_null;
}


value  api_val_call1(value  arg1,value  arg2)
{
	return val_call1(arg1,arg2);
}


value  api_val_call2(value  arg1,value  arg2,value  arg3)
{
	return val_call2(arg1,arg2,arg3);
}


value  api_val_call3(value  arg1,value  arg2,value  arg3,value  arg4)
{
	return val_call3(arg1,arg2,arg3,arg4);
}


value  api_val_callN(value  arg1,value  *arg2,int nargs)
{
	return val_callN(arg1,arg2,nargs);
}


// Call object field
value  api_val_ocall0(value  arg1,int arg2)
{
	return val_ocall0(arg1,arg2);
}


value  api_val_ocall1(value  arg1,int arg2,value  arg3)
{
	return val_ocall1(arg1,arg2,arg3);
}


value  api_val_ocall2(value  arg1,int arg2,value  arg3,value  arg4)
{
	return val_ocall2(arg1,arg2,arg3,arg4);
}



value  api_val_ocallN(value  obj,int fid,value *arg3,int inN)
{
	return val_ocallN(obj,fid,arg3,inN);
}


// Objects access
int api_val_id(const char * arg1)
{
	return val_id(arg1);
}

value api_val_field_name(field inField)
{
   return val_field_name(inField);
}


void api_val_iter_fields(value inObj, __hx_field_iter inFunc ,void *inCookie)
{
   val_iter_fields(inObj,inFunc,inCookie);
}




void api_alloc_field(value  arg1,int arg2,value  arg3)
{
	return alloc_field(arg1,arg2,arg3);
}

value  api_val_field(value  arg1,int arg2)
{
	return val_field(arg1,arg2);
}

double  api_val_field_numeric(value  arg1,int arg2)
{
	value field = val_field(arg1, arg2);
	if (val_is_number(field))
		return val_number(field);
	if (val_is_bool(field))
		return val_bool(field);
	return 0;
}

// Abstract types
vkind api_alloc_kind()
{
	static int id = 1;
	vkind result = (vkind)(size_t)id;
	id += 4;
	return result;
}

void api_kind_share(vkind *inKind,const char *inName)
{
	kind_share(inKind,inName);
}



// Garbage Collection
void * api_hx_alloc(int arg1)
{
	return alloc(arg1);
}


void * api_alloc_private(int arg1)
{
	return alloc_private(arg1);
}

void api_gc_change_managed_memory(int,const char *)
{
   // Nothing to do here
}


void  api_val_gc(value  arg1,finalizer arg2)
{
	 val_gc( arg1, arg2 );
}

void  api_val_gc_ptr(void * arg1,hxPtrFinalizer arg2)
{
	 val_gc( (value)arg1, (finalizer)arg2 );
}



// Used for finding functions in static libraries
int api_hx_register_prim( const char * arg1, void* arg2)
{
	// Not used - but return something anyhow.
	return 0;
}

void api_val_gc_add_root(value *inVal)
{
	// TODO
}

void api_val_gc_remove_root(value *inVal)
{
	// TODO
}

void api_gc_set_top_of_stack(int *,bool)
{
}



value *api_alloc_root()
{
   return alloc_root(1);
}

void api_free_root(value *inVal)
{
   return free_root(inVal);
}

// No need to do anything here...
void api_gc_enter_blocking() { }
void api_gc_exit_blocking() { }
void api_gc_safe_point() { }

// For v8 target
gcroot api_create_root(value) { return 0; }
value api_query_root(gcroot) { return 0; }
void api_destroy_root(gcroot) { }


#undef EXPORT
#if defined(NEKO_VCC) || defined(NEKO_MINGW)
#	define EXPORT __declspec( dllexport )
#else
#	define EXPORT __attribute__ ((visibility("default")))
#endif

value neko_api_init(value inCallback)
{
   gNeko2HaxeString = api_alloc_root();
   *gNeko2HaxeString = inCallback;
   return val_null;
}
NEKO_DEFINE_PRIM(neko_api_init,1)


value neko_api_init2(value inNewString,value inNewArray)
{
   gNeko2HaxeString = api_alloc_root();
   *gNeko2HaxeString = inNewString;
   gNekoNewArray = api_alloc_root();
   *gNekoNewArray = inNewArray;
   return val_null;
}
NEKO_DEFINE_PRIM(neko_api_init2,2)




