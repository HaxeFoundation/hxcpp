/*
 This bit of Macro magic is used to define extern function pointers
  in ndlls, define stub implementations that link back to the hxcpp dll
  and glue up the implementation in the hxcpp runtime.
*/

DEFFUNC_0(void,hx_error)
DEFFUNC_1(void,hx_throw,value)
DEFFUNC_3(void,hx_fail,char *,char *,int)

// Determine value type
DEFFUNC_1(ValueType,val_type,value)
DEFFUNC_1(vkind,val_kind,value)
DEFFUNC_2(void *,val_to_kind,value,vkind)
// don't check the 'kind' ...
DEFFUNC_1(void *,val_data,value)
DEFFUNC_1(int,val_fun_nargs,value)


// Extract value type
DEFFUNC_1(bool,val_bool,value)
DEFFUNC_1(int,val_int,value)
DEFFUNC_1(double,val_float,value)
DEFFUNC_1(double,val_number,value)

// Create value type

DEFFUNC_0(value,alloc_null)
DEFFUNC_1(value,alloc_bool,bool)
DEFFUNC_1(value,alloc_int,int)
DEFFUNC_1(value,alloc_float,double)
DEFFUNC_0(value,alloc_empty_object)
DEFFUNC_2(value,alloc_abstract,vkind,void *)
DEFFUNC_1(value,alloc_best_int,int)
DEFFUNC_1(value,alloc_int32,int)

// String access
DEFFUNC_1(int,val_strlen,value)
DEFFUNC_1(const wchar_t *,val_wstring,value)
DEFFUNC_1(const char *,val_string,value)
DEFFUNC_2(value,alloc_string_len,const char *,int)
DEFFUNC_2(value,alloc_wstring_len,const wchar_t *,int)

// Array access - generic
DEFFUNC_1(value,alloc_array,int)
DEFFUNC_1(int,val_array_size,value)
DEFFUNC_2(value,val_array_i,value,int)
DEFFUNC_3(value,val_array_set_i,value,int,value)
DEFFUNC_2(value,val_array_push,value,value)


// Array access - fast if possible - may return null
// Resizing the array may invalidate the pointer
DEFFUNC_1(bool *,val_array_bool,value)
DEFFUNC_1(int *,val_array_int,value)
DEFFUNC_1(double *,val_array_double,value)

// Byte arrays
// The byte array may be a string or a Array<bytes> depending on implementation
DEFFUNC_1(hx_byte_array,val_byte_array,value)
DEFFUNC_1(int,byte_array_len,hx_byte_array)
DEFFUNC_2(unsigned char,byte_array_i,hx_byte_array,int)
DEFFUNC_2(void,byte_array_push,hx_byte_array,unsigned char)
DEFFUNC_2(void,byte_array_resize,hx_byte_array,int)

// Call Function 
DEFFUNC_1(value,val_call0,value)
DEFFUNC_2(value,val_call1,value,value)
DEFFUNC_3(value,val_call2,value,value,value)
DEFFUNC_4(value,val_call3,value,value,value,value)
DEFFUNC_2(value,val_callN,value,value)
// Call object field
DEFFUNC_2(value,val_ocall0,value,int)
DEFFUNC_3(value,val_ocall1,value,int,value)
DEFFUNC_4(value,val_ocall2,value,int,value,value)
DEFFUNC_5(value,val_ocall3,value,int,value,value,value)
DEFFUNC_3(value,val_ocallN,value,int,value)

// Objects access
DEFFUNC_1(int,val_id,const char *)
DEFFUNC_3(void,alloc_field,value,int,value)
DEFFUNC_2(value,val_field,value,int)

// Abstract types
DEFFUNC_0(vkind,hx_alloc_kind)

// Garbage Collection
DEFFUNC_1(void *,hx_alloc,int)
DEFFUNC_1(void *,hx_alloc_private,int)
DEFFUNC_2(void, val_gc,value,hxFinalizer)

// Used for finding functions in static libraries
DEFFUNC_2(int, hx_register_prim, wchar_t *, void*)


