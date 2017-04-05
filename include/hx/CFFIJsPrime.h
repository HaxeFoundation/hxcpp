
extern "C"
{

typedef std::map<std::string,int> IdMap;
static IdMap sIdMap;
static std::vector<val> sIdKeys;

int val_id(const char *inName)
{
   IdMap::iterator id = sIdMap.find(inName);
   if (id==sIdMap.end())
   {
      int result = sIdMap.size();
      sIdMap[inName] = result;
      sIdKeys.push_back(value(inName));
      return result;
   }
   return id->second;
}


double val_field_numeric(value inObject, int inFieldId)
{
   return inObject[sIdKeys[inFieldId]].as<double>();
}

int val_int(value inValue) { return inValue.as<int>(); }
bool val_bool(value inValue) { return inValue.as<bool>(); }
double val_number(value inValue) { return inValue.as<double>(); }
double val_float(value inValue) { return inValue.as<double>(); }

value alloc_null() { return emscripten::val::null(); }
value alloc_int(int inValue) { return value(inValue); }
value alloc_best_int(int inValue) { return value(inValue); }
value alloc_int32(int inValue) { return value(inValue); }
value alloc_bool(bool inValue) { return value(inValue); }
value alloc_float(double inValue) { return value(inValue); }

value val_field(value inObject, int inIndex) { return inObject[sIdKeys[inIndex]]; }
void alloc_field(value inObject, int inIndex, value inValue) { inObject.set(sIdKeys[inIndex],inValue); }



/*
DEFFUNC_1(void,val_throw,value)
DEFFUNC_0(void,hx_error)
DEFFUNC_3(void,hx_fail,const char *,const char *,int)

// Determine value type
DEFFUNC_1(int,val_type,value)
DEFFUNC_1(vkind,val_kind,value)
DEFFUNC_2(void *,val_to_kind,value,vkind)
// don't check the 'kind' ...
DEFFUNC_1(void *,val_data,value)
DEFFUNC_1(int,val_fun_nargs,value)



// Create value type

DEFFUNC_0(value,alloc_empty_object)
DEFFUNC_2(value,alloc_abstract,vkind,void *)
// Allocates conservative-collected memory
DEFFUNC_3(value,create_abstract,vkind,int,hxFinalizer)
DEFFUNC_1(void,free_abstract,value)

// String access
DEFFUNC_1(int,val_strlen,value)
DEFFUNC_1(const wchar_t *,val_wstring,value)
DEFFUNC_1(const char *,val_string,value)
DEFFUNC_1(wchar_t *,val_dup_wstring,value)
DEFFUNC_1(char *,val_dup_string,value)
DEFFUNC_2(char *,alloc_string_data,const char *,int)
DEFFUNC_2(value,alloc_string_len,const char *,int)
DEFFUNC_2(value,alloc_wstring_len,const wchar_t *,int)

// Array access - generic
DEFFUNC_1(value,alloc_array,int)
DEFFUNC_1(int,val_array_size,value)
DEFFUNC_2(void,val_array_set_size,value,int)
DEFFUNC_2(value,val_array_i,value,int)
DEFFUNC_3(void,val_array_set_i,value,int,value)
DEFFUNC_2(void,val_array_push,value,value)


// Array access - fast if possible - may return null
// Resizing the array may invalidate the pointer
DEFFUNC_1(bool *,val_array_bool,value)
DEFFUNC_1(int *,val_array_int,value)
DEFFUNC_1(double *,val_array_double,value)
DEFFUNC_1(float *,val_array_float,value)
DEFFUNC_1(value *,val_array_value,value)

// String Buffer
// A 'buffer' is a tool for joining strings together.
// The C++ implementation is haxe.io.BytesData
// The neko implementation is something else again, and can't be passes as a value, only copied to a string

// Create a buffer from string of an empty buffer of a given length
DEFFUNC_1(buffer,alloc_buffer,const char *)
DEFFUNC_1(buffer,alloc_buffer_len,int)

// Append a string representation of a value to the buffer
DEFFUNC_2(void,val_buffer,buffer,value)

// Append a c-string to a buffer
DEFFUNC_2(void,buffer_append,buffer,const char *)

// Append given number of bytes of a c-string to the buffer
DEFFUNC_3(void,buffer_append_sub,buffer,const char *,int)

// Append given character to string
DEFFUNC_2(void,buffer_append_char,buffer,int)

// Convert buffer back into string value
DEFFUNC_1(value,buffer_to_string,buffer)



// These routines are for direct access to the c++ BytesData structure
// Use getByteData and resizeByteData for more generic access to haxe.io.Bytes

// This will never return true on a neko host.
DEFFUNC_1(bool,val_is_buffer,value)

// These functions are only valid if val_is_buffer returns true
// Currently, cffiByteBuffer is the same struct as buffer, but the usage is quite different
DEFFUNC_1(cffiByteBuffer,val_to_buffer,value)

// Number of byes in the array
DEFFUNC_1(int,buffer_size,cffiByteBuffer)

// Pointer to the byte data - will become invalid if the array is resized
DEFFUNC_1(char *,buffer_data,cffiByteBuffer)

// Convert c++ ByteBuffer back to 'value' - no copy involved
DEFFUNC_1(value,buffer_val,cffiByteBuffer)

// Resize the array - will invalidate the data
DEFFUNC_2(void,buffer_set_size,cffiByteBuffer,int)

// This is used by resizeByteData for manipulating bytes directly on neko
DEFFUNC_1(value,alloc_raw_string,int)

// Call Function 
DEFFUNC_1(value,val_call0,value)
DEFFUNC_2(value,val_call1,value,value)
DEFFUNC_3(value,val_call2,value,value,value)
DEFFUNC_4(value,val_call3,value,value,value,value)
DEFFUNC_3(value,val_callN,value,value *,int)

// Call the function - catch and print any exceptions
DEFFUNC_1(value,val_call0_traceexcept,value)

// Call object field
DEFFUNC_2(value,val_ocall0,value,int)
DEFFUNC_3(value,val_ocall1,value,int,value)
DEFFUNC_4(value,val_ocall2,value,int,value,value)
DEFFUNC_4(value,val_ocallN,value,int,value *,int)

// Objects access
DEFFUNC_1(int,val_id,const char *)
DEFFUNC_3(void,alloc_field,value,int,value)
DEFFUNC_2(value,val_field,value,int)
DEFFUNC_2(double,val_field_numeric,value,int)

DEFFUNC_1(value,val_field_name,field)
DEFFUNC_3(void,val_iter_fields,value,__hx_field_iter,void *)
DEFFUNC_3(void,val_iter_field_vals,value,__hx_field_iter,void *)

// Abstract types
DEFFUNC_0(vkind,alloc_kind)
DEFFUNC_2(void,kind_share,vkind *,const char *)

// Garbage Collection
DEFFUNC_1(void *,hx_alloc,int)
DEFFUNC_2(void, val_gc,value,hxFinalizer)
DEFFUNC_2(void, val_gc_ptr,void *,hxPtrFinalizer)
DEFFUNC_0(value *, alloc_root)
DEFFUNC_1(void, free_root,value *)
DEFFUNC_2(void, gc_change_managed_memory,int,const char *)

// Only available on cpp target...
DEFFUNC_1(void, val_gc_add_root,value *)
DEFFUNC_1(void, val_gc_remove_root,value *)
// Only available on js target - use AutoGCRoot to assist
DEFFUNC_1(gcroot, create_root,value)
DEFFUNC_1(value, query_root,gcroot)
DEFFUNC_1(void, destroy_root,gcroot)

DEFFUNC_0(void, gc_enter_blocking)
DEFFUNC_0(void, gc_exit_blocking)
DEFFUNC_0(void, gc_safe_point)
DEFFUNC_2(void, gc_set_top_of_stack,int *,bool)

// Used for finding functions in static libraries
DEFFUNC_2(int, hx_register_prim, const char *, void*)
*/


}
