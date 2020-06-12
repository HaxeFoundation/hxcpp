#ifndef HX_CFFI_NEKO_LOADER_H
#define HX_CFFI_NEKO_LOADER_H

//-------- NEKO Interface -----------------------------------------------------
namespace
{

#include <hx/NekoFunc.h>


void *sNekoDllHandle = 0;

void *LoadNekoFunc(const char *inName)
{
   #ifdef HX_WINRT
   return 0;
   #else
   static bool tried = false;
   if (tried && !sNekoDllHandle)
       return 0;
   tried = true;

   if (!sNekoDllHandle)
   {
      #ifdef HX_WINDOWS
      sNekoDllHandle = GetModuleHandleA("neko.dll");
      #else
      sNekoDllHandle = dlopen("libneko." NEKO_EXT, RTLD_NOW);
      // The debian package creates libneko.so.0 without libneko.so...
      // The fedora/openSUSE rpm packages create libneko.so.1...
      if (!sNekoDllHandle)
         sNekoDllHandle = dlopen("libneko." NEKO_EXT ".0", RTLD_NOW);
      if (!sNekoDllHandle)
         sNekoDllHandle = dlopen("libneko." NEKO_EXT ".1", RTLD_NOW);
      if (!sNekoDllHandle)
         sNekoDllHandle = dlopen("libneko." NEKO_EXT ".2", RTLD_NOW);
      #endif
  
      if (!sNekoDllHandle)
      {
         fprintf(stderr,"Could not link to neko.\n");
         return 0;
      }
   }


   #ifdef HX_WINDOWS
   void *result = (void *)GetProcAddress((HMODULE)sNekoDllHandle,inName);
   #else
   void *result = dlsym(sNekoDllHandle,inName);
   #endif

   //printf(" %s = %p\n", inName, result );
   return result;
   #endif // !HX_WINRT
}


static int __a_id = 0;
static int __s_id = 0;
static int b_id = 0;
static int length_id = 0;
static int push_id = 0;

neko_value *gNeko2HaxeString = 0;
neko_value *gNekoNewArray = 0;
neko_value gNekoNull = 0;
neko_value gNekoTrue = 0;
neko_value gNekoFalse = 0;


namespace
{
void CheckInitDynamicNekoLoader()
{
   if (!gNekoNull)
   {
      printf("Haxe code is missing a call to cpp.Prime.nekoInit().\n");
   }
}
}


/*


*/

void *DynamicNekoLoader(const char *inName);

typedef neko_value (*alloc_object_func)(neko_value);
typedef neko_value (*alloc_string_func)(const char *);
typedef neko_value (*alloc_abstract_func)(neko_vkind,void *);
typedef neko_value (*val_call1_func)(neko_value,neko_value);
typedef neko_value (*val_field_func)(neko_value,int);
typedef neko_value (*alloc_float_func)(double);
typedef void (*alloc_field_func)(neko_value,int,neko_value);
typedef neko_value *(*alloc_root_func)(int);
typedef char *(*alloc_private_func)(int);
typedef neko_value (*copy_string_func)(const char *,int);
typedef int (*val_id_func)(const char *);
typedef neko_buffer (*alloc_buffer_func)(const char *);
typedef neko_value (*val_buffer_func)(neko_buffer);
typedef void (*buffer_append_sub_func)(neko_buffer,const char *,int);
typedef void (*fail_func)(neko_value,const char *,int);
typedef neko_value (*alloc_array_func)(unsigned int);
typedef void (*val_gc_func)(neko_value,void *);
typedef void (*val_ocall1_func)(neko_value,int,neko_value);
typedef neko_value (*alloc_empty_string_func)(int);

static alloc_object_func dyn_alloc_object = 0;
static alloc_string_func dyn_alloc_string = 0;
static alloc_abstract_func dyn_alloc_abstract = 0;
static val_call1_func dyn_val_call1 = 0;
static val_field_func dyn_val_field = 0;
static alloc_field_func dyn_alloc_field = 0;
static alloc_float_func dyn_alloc_float = 0;
static alloc_root_func dyn_alloc_root = 0;
static alloc_private_func dyn_alloc_private = 0;
static alloc_private_func dyn_alloc = 0;
static copy_string_func dyn_copy_string = 0;
static val_id_func dyn_val_id = 0;
static alloc_buffer_func dyn_alloc_buffer = 0;
static val_buffer_func dyn_val_buffer = 0;
static fail_func dyn_fail = 0;
static buffer_append_sub_func dyn_buffer_append_sub = 0;
static alloc_array_func dyn_alloc_array = 0;
static val_gc_func dyn_val_gc = 0;
static val_ocall1_func dyn_val_ocall1 = 0;
static alloc_empty_string_func dyn_alloc_empty_string = 0;


neko_value api_alloc_string(const char *inString)
{
   CheckInitDynamicNekoLoader();
   neko_value neko_string = dyn_alloc_string(inString);
   if (gNeko2HaxeString)
      return dyn_val_call1(*gNeko2HaxeString,neko_string);
   return neko_string;
}


char *api_alloc_string_data(const char *inString,int inLength)
{
   CheckInitDynamicNekoLoader();
   char *result = (char *)dyn_alloc_private(inLength+1);
   memcpy(result,inString,inLength);
   result[inLength]='\0';
   return result;
}


neko_value api_alloc_raw_string(int inLength)
{
   CheckInitDynamicNekoLoader();
   return dyn_alloc_empty_string(inLength);
}


#define NEKO_NOT_IMPLEMENTED(func) dyn_fail(api_alloc_string("NOT Implemented:" func),__FILE__,__LINE__)

void * api_empty() { return 0; }

bool api_val_bool(neko_value  arg1) { return arg1==gNekoTrue; }
int api_val_int(neko_value  arg1) { return neko_val_int(arg1); }
double api_val_float(neko_value  arg1) { return *(double *)( ((char *)arg1) + 4 ); }
double api_val_number(neko_value  arg1) { return neko_val_is_int(arg1) ? neko_val_int(arg1) : api_val_float(arg1); }


neko_value api_alloc_bool(bool arg1) { CheckInitDynamicNekoLoader(); return arg1 ? gNekoTrue : gNekoFalse; }
neko_value api_alloc_int(int arg1) { return neko_alloc_int(arg1); }
neko_value api_alloc_empty_object()
{
   return dyn_alloc_object(gNekoNull);
}

neko_value api_buffer_to_string(neko_buffer arg1)
{
   neko_value neko_string = dyn_val_buffer(arg1);
   if (gNeko2HaxeString)
      return dyn_val_call1(*gNeko2HaxeString,neko_string);
   return neko_string;
}


const char * api_val_string(neko_value  arg1)
{
	if (neko_val_is_string(arg1))
	   return neko_val_string(arg1);

	if (neko_val_is_object(arg1))
   {
	   neko_value s = dyn_val_field(arg1,__s_id);
      if (neko_val_is_string(s))
	      return neko_val_string(s);
   }

   return 0;
}

void api_alloc_field_numeric(neko_value  arg1,int arg2, double arg3)
{
   dyn_alloc_field(arg1, arg2, dyn_alloc_float(arg3) );
}

double  api_val_field_numeric(neko_value  arg1,int arg2)
{
	neko_value field = dyn_val_field(arg1, arg2);
	if (neko_val_is_number(field))
		return api_val_number(field);
	if (field==gNekoTrue)
      return 1;
	return 0;
}




int api_val_strlen(neko_value  arg1)
{
	if (neko_val_is_string(arg1))
	   return neko_val_strlen(arg1);

	if (neko_val_is_object(arg1))
   {
      neko_value l =  dyn_val_field(arg1,length_id);
      if (neko_val_is_int(l))
         return api_val_int(l);
   }
	return 0;
}
void api_buffer_set_size(neko_buffer inBuffer,int inLen) { 
   NEKO_NOT_IMPLEMENTED("api_buffer_set_size");
}


void api_buffer_append_char(neko_buffer inBuffer,int inChar)
{
   NEKO_NOT_IMPLEMENTED("api_buffer_append_char");
}



// Byte arrays - use strings
neko_buffer api_val_to_buffer(neko_value  arg1)
{
   return (neko_buffer)api_val_string(arg1);
}
bool api_val_is_buffer(neko_value  arg1) { return neko_val_is_string(arg1); } 
int api_buffer_size(neko_buffer inBuffer) { return neko_val_strlen((neko_value)inBuffer); }
char * api_buffer_data(neko_buffer inBuffer) { return (char *)api_val_string((neko_value)inBuffer); }

char * api_val_dup_string(neko_value inVal)
{
	int len = api_val_strlen(inVal);
	const char *ptr = api_val_string(inVal);
	char *result = dyn_alloc_private(len+1);
	memcpy(result,ptr,len);
	result[len] = '\0';
	return result;
}

neko_value api_alloc_string_len(const char *inStr,int inLen)
{
	if (gNeko2HaxeString)
   {
      if (!inStr)
		   return dyn_val_call1(*gNeko2HaxeString,api_alloc_raw_string(inLen));
		return dyn_val_call1(*gNeko2HaxeString,dyn_copy_string(inStr,inLen));
   }
   if (!inStr)
		inStr = dyn_alloc_private(inLen);
   return dyn_copy_string(inStr,inLen);
}

neko_buffer api_alloc_buffer_len(int inLen)
{
	neko_value str=api_alloc_string_len(0,inLen+1);
	char *s=(char *)api_val_string(str);
	memset(s,0,inLen+1);
	return (neko_buffer)str;
}



neko_value api_alloc_wstring_len(const wchar_t *inStr,int inLen)
{
  int len = 0;
  const wchar_t *chars = inStr;
  for(int i=0;i<inLen;i++)
   {
      int c = chars[i];
      if( c <= 0x7F ) len++;
      else if( c <= 0x7FF ) len+=2;
      else if( c <= 0xFFFF ) len+=3;
      else len+= 4;
   }

   char *result = dyn_alloc_private(len);//+1?
   unsigned char *data =  (unsigned char *) &result[0];
   for(int i=0;i<inLen;i++)
   {
      int c = chars[i];
      if( c <= 0x7F )
         *data++ = c;
      else if( c <= 0x7FF )
      {
         *data++ = 0xC0 | (c >> 6);
         *data++ = 0x80 | (c & 63);
      }
      else if( c <= 0xFFFF )
      {
         *data++ = 0xE0 | (c >> 12);
         *data++ = 0x80 | ((c >> 6) & 63);
         *data++ = 0x80 | (c & 63);
      }
      else
      {
         *data++ = 0xF0 | (c >> 18);
         *data++ = 0x80 | ((c >> 12) & 63);
         *data++ = 0x80 | ((c >> 6) & 63);
         *data++ = 0x80 | (c & 63);
      }
   }
   //result[len] = 0;

   return api_alloc_string_len(result,len);
}



const wchar_t *api_val_wstring(neko_value  arg1)
{
	int len = api_val_strlen(arg1);

   unsigned char *b = (unsigned char *)api_val_string(arg1);
   wchar_t *result = (wchar_t *)dyn_alloc_private((len+1)*sizeof(wchar_t));
   int l = 0;

   for(int i=0;i<len;)
   {
       int c = b[i++];
       if (c==0) break;
       else if( c < 0x80 )
       {
           result[l++] = c;
       }
       else if( c < 0xE0 )
           result[l++] = ( ((c & 0x3F) << 6) | (b[i++] & 0x7F) );
       else if( c < 0xF0 )
       {
           int c2 = b[i++];
           result[l++] = ( ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( b[i++] & 0x7F) );
       }
       else
       {
           int c2 = b[i++];
           int c3 = b[i++];
           result[l++] = ( ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 << 6) & 0x7F) | (b[i++] & 0x7F) );
       }
   }
   result[l] = '\0';

   return result;
}


wchar_t * api_val_dup_wstring(neko_value inVal)
{
	return (wchar_t *)api_val_wstring(inVal);
}



int api_val_type(neko_value  arg1)
{
	int t=neko_val_type(arg1);

	if (t==VAL_OBJECT)
	{
		neko_value __a = dyn_val_field(arg1,__a_id);
		if (neko_val_is_array(__a))
			return valtArray;
		neko_value __s = dyn_val_field(arg1,__s_id);
		if (neko_val_is_string(__s))
			return valtString;
	}
	if (t<7)
		return (hxValueType)t;
	if (t==VAL_ABSTRACT)
		return valtAbstractBase;

	if (t==VAL_PRIMITIVE || t==VAL_JITFUN)
		return valtFunction;
	if (t==VAL_32_BITS || t==VAL_INT)
		return valtInt;
	return valtNull;
}

neko_value *api_alloc_root()
{
   return dyn_alloc_root(1);
}


void * api_val_to_kind(neko_value  arg1,neko_vkind arg2)
{
	neko_vkind k = (neko_vkind)neko_val_kind(arg1);
	if (k!=arg2)
		return 0;
	return neko_val_data(arg1);
}


int api_alloc_kind()
{
	static int id = 1;
	int result = id;
	id += 4;
	return result;
}
neko_value api_alloc_null()
{
   CheckInitDynamicNekoLoader();
   return gNekoNull;
}

neko_value api_create_abstract(neko_vkind inKind,int inSize,void *inFinalizer)
{
   void *data = dyn_alloc(inSize);
   neko_value val = dyn_alloc_abstract(inKind, data);
   dyn_val_gc(val, inFinalizer);
   return val;
}

void api_free_abstract(neko_value inAbstract)
{
   if (neko_val_is_abstract(inAbstract))
   {
      dyn_val_gc(inAbstract,0);
      neko_val_kind(inAbstract) = 0;
   }
}


neko_value api_buffer_val(neko_buffer arg1)
{
   return api_alloc_null();
}


void api_hx_error()
{
   dyn_fail(dyn_alloc_string("An unknown error has occurred."),"",1);
}

void * api_val_data(neko_value  arg1) { return neko_val_data(arg1); }

// Array access - generic
int api_val_array_size(neko_value  arg1)
{
	if (neko_val_is_array(arg1))
	   return neko_val_array_size(arg1);
	neko_value l = dyn_val_field(arg1,length_id);
	return neko_val_int(l);
}


neko_value  api_val_array_i(neko_value  arg1,int arg2)
{
	if (neko_val_is_array(arg1))
	   return neko_val_array_ptr(arg1)[arg2];
	return neko_val_array_ptr(dyn_val_field(arg1,__a_id))[arg2];
}

void api_val_array_set_i(neko_value  arg1,int arg2,neko_value inVal)
{
	if (!neko_val_is_array(arg1))
		arg1 = dyn_val_field(arg1,__a_id);
	neko_val_array_ptr(arg1)[arg2] = inVal;
}

void api_val_array_set_size(neko_value  arg1,int inLen)
{
	NEKO_NOT_IMPLEMENTED("api_val_array_set_size");
}

void api_val_array_push(neko_value  inArray,neko_value inValue)
{
   dyn_val_ocall1(inArray,push_id,inValue);
}


neko_value  api_alloc_array(int arg1)
{
   if (!gNekoNewArray)
	   return dyn_alloc_array(arg1);
	return dyn_val_call1(*gNekoNewArray,neko_alloc_int(arg1));
}


neko_value * api_val_array_value(neko_value  arg1)
{
	if (neko_val_is_array(arg1))
	   return neko_val_array_ptr(arg1);
	return neko_val_array_ptr(dyn_val_field(arg1,__a_id));
}

neko_value  api_val_call0_traceexcept(neko_value  arg1)
{
	NEKO_NOT_IMPLEMENTED("api_val_call0_traceexcept");
	return gNekoNull;
}


int  api_val_fun_nargs(neko_value arg1)
{
   if (!arg1 || !neko_val_is_function(arg1) )
      return faNotFunction;
   return neko_val_fun_nargs(arg1);
}



void api_val_gc(neko_value obj, void *finalizer)
{
   // Let neko deal with ints or abstracts ...
   if (neko_val_is_int(obj) || neko_val_is_abstract(obj))
   {
      dyn_val_gc(obj,finalizer);
   }
   else
   {
      // Hack type to abstract for the duration
      neko_val_type old_tag = neko_val_tag(obj);
      neko_val_tag(obj) = VAL_ABSTRACT;
      dyn_val_gc(obj,finalizer);
      neko_val_tag(obj) = old_tag;
   }
}

void api_gc_change_managed_memory(int,const char *)
{
   // Nothing to do here
}

bool api_gc_try_blocking() { return false; }
bool api_gc_try_unblocking() { return false; }

#define IMPLEMENT_HERE(x) if (!strcmp(inName,#x)) return (void *)api_##x;
#define IGNORE_API(x) if (!strcmp(inName,#x)) return (void *)api_empty;


void *DynamicNekoLoader(const char *inName)
{
   IMPLEMENT_HERE(alloc_kind)
   IMPLEMENT_HERE(alloc_null)
   IMPLEMENT_HERE(val_to_kind)
   if (!strcmp(inName,"hx_fail"))
      return LoadNekoFunc("_neko_failure");
   IMPLEMENT_HERE(val_type)
   IMPLEMENT_HERE(val_bool)
   IMPLEMENT_HERE(val_int)
   IMPLEMENT_HERE(val_float)
   IMPLEMENT_HERE(val_number)
   IMPLEMENT_HERE(val_field_numeric)
   IMPLEMENT_HERE(alloc_bool)
   IMPLEMENT_HERE(alloc_int)
   IMPLEMENT_HERE(alloc_empty_object)
   IMPLEMENT_HERE(alloc_root)
   IMPLEMENT_HERE(val_gc)
   IMPLEMENT_HERE(gc_try_blocking)
   IMPLEMENT_HERE(gc_try_unblocking)

   IMPLEMENT_HERE(create_abstract)
   IMPLEMENT_HERE(free_abstract)

   IGNORE_API(gc_enter_blocking)
   IGNORE_API(gc_exit_blocking)
   IGNORE_API(gc_safe_point)
   IGNORE_API(gc_add_root)
   IGNORE_API(gc_remove_root)
   IGNORE_API(gc_set_top_of_stack)
   IGNORE_API(gc_change_managed_memory)
   IGNORE_API(create_root)
   IGNORE_API(query_root)
   IGNORE_API(destroy_root)
   IGNORE_API(hx_register_prim)
   IGNORE_API(val_array_int)
   IGNORE_API(val_array_double)
   IGNORE_API(val_array_float)
   IGNORE_API(val_array_bool)

   if (!strcmp(inName,"hx_alloc"))
      return LoadNekoFunc("neko_alloc");

   IMPLEMENT_HERE(buffer_to_string)
   IMPLEMENT_HERE(buffer_val)

   if (!strcmp(inName,"val_iter_field_vals"))
      return LoadNekoFunc("neko_val_iter_fields");

   IMPLEMENT_HERE(val_strlen)
   IMPLEMENT_HERE(val_wstring)
   IMPLEMENT_HERE(val_string)
   IMPLEMENT_HERE(alloc_string)
   IMPLEMENT_HERE(alloc_raw_string)
   IMPLEMENT_HERE(alloc_string_data)
   IMPLEMENT_HERE(val_dup_wstring)
   IMPLEMENT_HERE(val_dup_string)
   IMPLEMENT_HERE(alloc_string_len)
   IMPLEMENT_HERE(alloc_wstring_len)

   IMPLEMENT_HERE(val_is_buffer)
   IMPLEMENT_HERE(val_to_buffer)
   IMPLEMENT_HERE(alloc_buffer_len)
   IMPLEMENT_HERE(buffer_size)
   IMPLEMENT_HERE(buffer_set_size)
   IMPLEMENT_HERE(buffer_append_char)
   IMPLEMENT_HERE(buffer_data)

   IMPLEMENT_HERE(hx_error)
   IMPLEMENT_HERE(val_array_i)
   IMPLEMENT_HERE(val_array_size)
   IMPLEMENT_HERE(val_data)
   IMPLEMENT_HERE(val_array_set_i)
   IMPLEMENT_HERE(val_array_set_size)
   IMPLEMENT_HERE(val_array_push)
   IMPLEMENT_HERE(alloc_array)
   IMPLEMENT_HERE(alloc_field_numeric)
   IMPLEMENT_HERE(val_array_value)

   IMPLEMENT_HERE(val_fun_nargs)

   IMPLEMENT_HERE(val_call0_traceexcept)


   char buffer[100];
   strcpy(buffer,"neko_");
   strcat(buffer,inName);
   void *result = LoadNekoFunc(buffer);
   if (result)
      return result;

	return 0;
}


ResolveProc InitDynamicNekoLoader()
{
   static bool init = false;
   if (!init)
   {
      dyn_alloc_private = (alloc_private_func)LoadNekoFunc("neko_alloc_private");
      dyn_alloc = (alloc_private_func)LoadNekoFunc("neko_alloc");
      dyn_alloc_object = (alloc_object_func)LoadNekoFunc("neko_alloc_object");
      dyn_alloc_string = (alloc_string_func)LoadNekoFunc("neko_alloc_string");
      dyn_alloc_abstract = (alloc_abstract_func)LoadNekoFunc("neko_alloc_abstract");
      dyn_val_call1 = (val_call1_func)LoadNekoFunc("neko_val_call1");
      dyn_val_field = (val_field_func)LoadNekoFunc("neko_val_field");
      dyn_alloc_field = (alloc_field_func)LoadNekoFunc("neko_alloc_field");
      dyn_alloc_float = (alloc_float_func)LoadNekoFunc("neko_alloc_float");
      dyn_alloc_root = (alloc_root_func)LoadNekoFunc("neko_alloc_root");
      dyn_copy_string = (copy_string_func)LoadNekoFunc("neko_copy_string");
      dyn_val_id = (val_id_func)LoadNekoFunc("neko_val_id");
      dyn_alloc_buffer = (alloc_buffer_func)LoadNekoFunc("neko_alloc_buffer");
      dyn_val_buffer = (val_buffer_func)LoadNekoFunc("neko_buffer_to_string");
      dyn_fail = (fail_func)LoadNekoFunc("_neko_failure");
      dyn_buffer_append_sub = (buffer_append_sub_func)LoadNekoFunc("neko_buffer_append_sub");
      dyn_alloc_array = (alloc_array_func)LoadNekoFunc("neko_alloc_array");
      dyn_val_gc = (val_gc_func)LoadNekoFunc("neko_val_gc");
      dyn_val_ocall1 = (val_ocall1_func)LoadNekoFunc("neko_val_ocall1");
      dyn_alloc_empty_string = (alloc_empty_string_func)LoadNekoFunc("neko_alloc_empty_string");
      init = true;
   }

   if (!dyn_val_id)
     return 0;


   __a_id = dyn_val_id("__a");
   __s_id = dyn_val_id("__s");
   b_id = dyn_val_id("b");
   length_id = dyn_val_id("length");
   push_id = dyn_val_id("push");

   return DynamicNekoLoader;
}


neko_value neko_init(neko_value inNewString,neko_value inNewArray,neko_value inNull, neko_value inTrue, neko_value inFalse)
{
   InitDynamicNekoLoader();

   gNekoNull = inNull;
   gNekoTrue = inTrue;
   gNekoFalse = inFalse;

   gNeko2HaxeString = dyn_alloc_root(1);
   *gNeko2HaxeString = inNewString;
   gNekoNewArray = dyn_alloc_root(1);
   *gNekoNewArray = inNewArray;

   return gNekoNull;
}



} // end anon namespace

#endif

