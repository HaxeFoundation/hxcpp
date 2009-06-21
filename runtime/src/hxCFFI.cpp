#include <hxObject.h>
// Get headers etc.
#define IGNORE_CFFI_API_H
#include <hxCFFI.h>


// Class for boxing external handles

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

vkind k_int32 = (vkind)vtAbstractBase;
vkind k_hash = (vkind)(vtAbstractBase + 1);
static int sgKinds = (int)(vtAbstractBase + 2);


SHARED int hxcpp_alloc_kind()
{
   return ++sgKinds;
}




/*
 This bit of Macro magic is used to define extern function pointers
  in ndlls, define stub implementations that link back to the hxcpp dll
  and glue up the implementation in the hxcpp runtime.
*/

void hx_error()
{
	throw Dynamic( STRING(L"ERROR",6) );
}


void hx_throw(hxObject * arg1)
{
	if (arg1==0)
	   throw Dynamic(null());
	throw Dynamic(arg1);
}


void hx_fail(char * arg1,char * arg2,int arg3)
{
   fprintf(stderr,"Terminal error %s, File %s, line %d\n", arg1,arg2,arg3);
   exit(1);
}



// Determine hxObject * type
int val_type(hxObject * arg1)
{
	if (arg1==0)
		return valtNull;
	return arg1->__GetType();
}


vkind val_kind(hxObject * arg1)
{
	if (arg1==0)
	   throw Dynamic( STRING(L"Value has no 'kind'",19) );
	int type = arg1->__GetType();
	if (type<valtAbstractBase)
	   throw Dynamic( STRING(L"Value has no 'kind'",19) );
	return (vkind)(type);
}


void * val_to_kind(hxObject * arg1,vkind arg2)
{
	if (arg1==0)
		return 0;
	if ((int)arg2 == arg1->__GetType())
		return arg1->__GetHandle();
	return 0;
}


// don't check the 'kind' ...
void * val_data(hxObject * arg1)
{
	if (arg1==0)
		return 0;
	return arg1->__GetHandle();
}


int val_fun_nargs(hxObject * arg1)
{
	if (arg1==0)
		return faNotFunction;
	return arg1->__ArgCount();
}




// Extract hxObject * type
bool val_bool(hxObject * arg1)
{
	if (arg1==0) return false;
	return arg1->__ToInt()!=0;
}


int val_int(hxObject * arg1)
{
	if (arg1==0) return 0;
	return arg1->__ToInt();
}


double val_float(hxObject * arg1)
{
	if (arg1==0) return 0.0;
	return arg1->__ToDouble();
}


double val_number(hxObject * arg1)
{
	if (arg1==0) return 0.0;
	return arg1->__ToDouble();
}



// Create hxObject * type

hxObject * alloc_null() { return 0; }
hxObject * alloc_bool(bool arg1) { return Dynamic(arg1).GetPtr(); }
hxObject * alloc_int(int arg1) { return Dynamic(arg1).GetPtr(); }
hxObject * alloc_float(double arg1) { return Dynamic(arg1).GetPtr(); }
hxObject * alloc_empty_object() { return new hxAnon_obj(); }


hxObject * alloc_abstract(vkind arg1,void * arg2)
{
	int type = (int)arg1;
	return new Abstract_obj(type,arg2);
}

hxObject * alloc_best_int(int arg1) { return Dynamic(arg1).GetPtr(); }
hxObject * alloc_int32(int arg1) { return Dynamic(arg1).GetPtr(); }



// String access
int val_strlen(hxObject * arg1)
{
	if (arg1==0) return 0;
	return arg1->toString().length;
}


const wchar_t * val_wstring(hxObject * arg1)
{
	if (arg1==0) return L"";
	return arg1->toString().__s;
}


const char * val_string(hxObject * arg1)
{
	if (arg1==0) return "";
	return arg1->__CStr();
}


hxObject * alloc_string(const char * arg1)
{
	return Dynamic( String(arg1,strlen(arg1)) ).GetPtr();
}



// Array access - generic
int val_array_size(hxObject * arg1)
{
	if (arg1==0) return 0;
	return arg1->__length();
}


hxObject * val_array_i(hxObject * arg1,int arg2)
{
	if (arg1==0) return 0;
	return arg1->__GetItem(arg2).GetPtr();
}


hxObject * val_alloc_array(int arg1)
{
	Array<Dynamic> array(arg1,arg1);
	return array.GetPtr();
}



// Array access - fast if possible - may return null
// Resizing the array may invalidate the pointer
bool * val_array_bool(hxObject * arg1)
{
	Array_obj<bool> *a = dynamic_cast< Array_obj<bool> * >(arg1);
	if (a==0)
		return 0;
	return (bool *)a->GetBase();
}


int * val_array_int(hxObject * arg1)
{
	Array_obj<int> *a = dynamic_cast< Array_obj<int> * >(arg1);
	if (a==0)
		return 0;
	return (int *)a->GetBase();
}


double * val_array_double(hxObject * arg1)
{
	Array_obj<double> *a = dynamic_cast< Array_obj<double> * >(arg1);
	if (a==0)
		return 0;
	return (double *)a->GetBase();
}



// Byte arrays
// The byte array may be a string or a Array<bytes> depending on implementation
hx_byte_array val_byte_array(hxObject * arg1)
{
	Array_obj<unsigned char> *b = dynamic_cast< Array_obj<unsigned char> * >(arg1);
	return (hx_byte_array)b;
}


int byte_array_len(hx_byte_array arg1)
{
	Array_obj<unsigned char> *b = (Array_obj<unsigned char> *)(arg1);
	return b->length;
}


unsigned char byte_array_i(hx_byte_array arg1,int arg2)
{
	Array_obj<unsigned char> *b = (Array_obj<unsigned char> *)(arg1);
	return b->QuickItem(arg2);
}


void byte_array_push(hx_byte_array arg1,unsigned char arg2)
{
	Array_obj<unsigned char> *b = (Array_obj<unsigned char> *)(arg1);
	b->push(arg2);
}


void byte_array_resize(hx_byte_array arg1,int arg2)
{
	Array_obj<unsigned char> *b = (Array_obj<unsigned char> *)(arg1);
	b->__SetSize(arg2);
}



// Call Function 
hxObject * val_call0(hxObject * arg1)
{
	if (!arg1) throw NULL_FUNCTION_POINTER;
	return arg1->__run().GetPtr();
}


hxObject * val_call1(hxObject * arg1,hxObject * arg2)
{
	if (!arg1) throw NULL_FUNCTION_POINTER;
	return arg1->__run(arg2).GetPtr();
}


hxObject * val_call2(hxObject * arg1,hxObject * arg2,hxObject * arg3)
{
	if (!arg1) throw NULL_FUNCTION_POINTER;
	return arg1->__run(arg2,arg3).GetPtr();
}


hxObject * val_call3(hxObject * arg1,hxObject * arg2,hxObject * arg3,hxObject * arg4)
{
	if (!arg1) throw NULL_FUNCTION_POINTER;
	return arg1->__run(arg2,arg3,arg4).GetPtr();
}


hxObject * val_callN(hxObject * arg1,hxObject * arg2)
{
	if (!arg1) throw NULL_FUNCTION_POINTER;
	return arg1->__Run( Dynamic(arg2) ).GetPtr();
}


// Call object field
hxObject * val_ocall0(hxObject * arg1,int arg2)
{
	if (!arg1) throw INVALID_OBJECT;
	return arg1->__IField(arg2)->__run().GetPtr();
}


hxObject * val_ocall1(hxObject * arg1,int arg2,hxObject * arg3)
{
	if (!arg1) throw INVALID_OBJECT;
	return arg1->__IField(arg2)->__run(arg3).GetPtr();
}


hxObject * val_ocall2(hxObject * arg1,int arg2,hxObject * arg3,hxObject * arg4)
{
	if (!arg1) throw INVALID_OBJECT;
	return arg1->__IField(arg2)->__run(arg3,arg4).GetPtr();
}


hxObject * val_ocall3(hxObject * arg1,int arg2,hxObject * arg3,hxObject * arg4,hxObject * arg5)
{
	if (!arg1) throw INVALID_OBJECT;
	return arg1->__IField(arg2)->__run(arg3,arg4,arg5).GetPtr();
}


hxObject * val_ocallN(hxObject * arg1,int arg2,hxObject * arg3)
{
	if (!arg1) throw INVALID_OBJECT;
	return arg1->__IField(arg2)->__run(Dynamic(arg3)).GetPtr();
}



// Objects access
int val_id(const char * arg1)
{
	return __hxcpp_field_to_id(arg1);
}


SHARED void alloc_field(hxObject * arg1,int arg2,hxObject * arg3)
{
	if (!arg1) throw INVALID_OBJECT;
	arg1->__SetField(__hxcpp_field_from_id(arg2),arg3);
}
SHARED void hxcpp_alloc_field(hxObject * arg1,int arg2,hxObject * arg3)
{
	return alloc_field(arg1,arg2,arg3);
}


hxObject * val_field(hxObject * arg1,int arg2)
{
	if (!arg1) throw INVALID_OBJECT;
	return arg1->__IField(arg2).GetPtr();
}

	// Abstract types
vkind hx_alloc_kind()
{
	return (vkind)hxcpp_alloc_kind();
}



// Garbage Collection
void * hx_alloc(int arg1)
{
	return hxNewGCBytes(0,arg1);
}


void * hx_alloc_private(int arg1)
{
   return hxNewGCPrivate(0,arg1);
}


void  val_gc(hxObject * arg1,finalizer arg2)
{
	 hxGCAddFinalizer( arg1, arg2 );
}



// Used for finding functions in static libraries
int hx_register_prim( wchar_t * arg1, void* arg2)
{
	__hxcpp_register_prim(arg1,arg2);
	return 0;
}


extern "C" {

SHARED void * hx_cffi(const char *inName)
{

	#define DEFFUNC(name,r,b,c) if ( !strcmp(inName,#name) ) return (void *)name;

	#include <hxCFFIAPI.h>

	return 0;
}

}
