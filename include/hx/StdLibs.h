#ifndef HX_STDLIBS_H
#define HX_STDLIBS_H

// --- Resource -------------------------------------------------------------

namespace hx
{
struct Resource
{
   String        mName;
   int           mDataLength;
   unsigned char *mData;

   bool operator<(const Resource &inRHS) const { return mName < inRHS.mName; }
};

Resource *GetResources();

HXCPP_EXTERN_CLASS_ATTRIBUTES
void RegisterResources(hx::Resource *inResources);


struct AnyCast
{
   AnyCast(void *inPtr) : mPtr(inPtr) { }

   template<typename T>
   operator T*() const { return (T*)mPtr; }

   void *mPtr;
};

} // end namespace hx

Array<String>        __hxcpp_resource_names();
String               __hxcpp_resource_string(String inName);
Array<unsigned char> __hxcpp_resource_bytes(String inName);




// System access
Array<String>  __get_args();
double         __time_stamp();
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_print(Dynamic &inV);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_println(Dynamic &inV);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __trace(Dynamic inPtr, Dynamic inData);
void           __hxcpp_stdlibs_boot();

// --- Maths ---------------------------------------------------------
double __hxcpp_drand();
int __hxcpp_irand(int inMax);

// --- Casting/Converting ---------------------------------------------------------
HXCPP_EXTERN_CLASS_ATTRIBUTES bool  __instanceof(const Dynamic &inValue, const Dynamic &inType);
HXCPP_EXTERN_CLASS_ATTRIBUTES int   __int__(double x);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool  __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __hxcpp_parse_int(const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES double __hxcpp_parse_float(const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __hxcpp_create_var_args(Dynamic &inArrayFunc);

// --- CFFI helpers ------------------------------------------------------------------

// Used for accessing object fields by integer ID, rather than string ID.
// Used mainly for neko ndll interaction.
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __hxcpp_field_to_id( const char *inField );
HXCPP_EXTERN_CLASS_ATTRIBUTES const String &__hxcpp_field_from_id( int f );
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __hxcpp_register_prim(const HX_CHAR *inName,void *inFunc);

// Get function pointer from dll file
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __loadprim(String inLib, String inPrim,int inArgCount);
HXCPP_EXTERN_CLASS_ATTRIBUTES void *__hxcpp_get_proc_address(String inLib, String inPrim,bool inNdllProc=true);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_run_dll(String inLib, String inPrim);
// Can assign to function pointer without error
inline hx::AnyCast __hxcpp_cast_get_proc_address(String inLib, String inPrim)
{
   return hx::AnyCast(__hxcpp_get_proc_address(inLib,inPrim,false));
}

// Loading functions via name (dummy return value)



// --- haxe.io.BytesData ----------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len);
// UTF8 processing
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_char_array_to_utf8_string(Array<int> &inChars,int inFirst=0, int inLen=-1);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<int> __hxcpp_utf8_string_to_char_array(String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_char_bytes_to_utf8_string(String &inBytes);
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_utf8_string_to_char_bytes(String &inUTF8);


// --- IntHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES hx::Object   *__int_hash_create();
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set(Dynamic inHash,int inKey,const Dynamic &value);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_get(Dynamic inHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __int_hash_exists(Dynamic inHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __int_hash_remove(Dynamic inHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_keys(Dynamic inHash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_values(Dynamic inHash);


// --- Date --------------------------------------------------------------------------

double __hxcpp_new_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds);
double __hxcpp_utc_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds);
int    __hxcpp_get_hours(double inSeconds);
int    __hxcpp_get_minutes(double inSeconds);
int    __hxcpp_get_seconds(double inSeconds);
int    __hxcpp_get_year(double inSeconds);
int    __hxcpp_get_month(double inSeconds);
int    __hxcpp_get_date(double inSeconds);
int    __hxcpp_get_day(double inSeconds);
String __hxcpp_to_string(double inSeconds);
double __hxcpp_date_now();

// --- vm/threading --------------------------------------------------------------------

Dynamic __hxcpp_thread_create(Dynamic inFunc);
Dynamic __hxcpp_thread_current();
void    __hxcpp_thread_send(Dynamic inThread, Dynamic inMessage);
Dynamic __hxcpp_thread_read_message(bool inBlocked);
bool __hxcpp_is_current_thread(hx::Object *inThread);

Dynamic __hxcpp_mutex_create();
void    __hxcpp_mutex_acquire(Dynamic);
bool    __hxcpp_mutex_try(Dynamic);
void    __hxcpp_mutex_release(Dynamic);


Dynamic __hxcpp_lock_create();
bool    __hxcpp_lock_wait(Dynamic inlock,double inTime);
void    __hxcpp_lock_release(Dynamic inlock);

Dynamic __hxcpp_deque_create();
void    __hxcpp_deque_add(Dynamic q,Dynamic inVal);
void    __hxcpp_deque_push(Dynamic q,Dynamic inVal);
Dynamic __hxcpp_deque_pop(Dynamic q,bool block);

Dynamic __hxcpp_tls_get(int inID);
void    __hxcpp_tls_set(int inID,Dynamic inVal);



Array<String> __hxcpp_get_call_stack(bool inSkipLast);
Array<String> __hxcpp_get_exception_stack();

// --- Profile -------------------------------------------------------------------

void __hxcpp_start_profiler(::String inDumpFile);
void __hxcpp_stop_profiler();



// --- Memory --------------------------------------------------------------------------


// Threadsafe methods - takes buffer
inline int __hxcpp_memory_get_byte(Array<unsigned char> inBuffer ,int addr) { return inBuffer->GetBase()[addr]; }
inline double __hxcpp_memory_get_double(Array<unsigned char> inBuffer ,int addr) { return *(double *)(inBuffer->GetBase()+addr); }
inline double __hxcpp_memory_get_float(Array<unsigned char> inBuffer ,int addr) { return *(float *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_i16(Array<unsigned char> inBuffer ,int addr) { return *(short *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_i32(Array<unsigned char> inBuffer ,int addr) { return *(int *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_ui16(Array<unsigned char> inBuffer ,int addr) { return *(unsigned short *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_ui32(Array<unsigned char> inBuffer ,int addr) { return *(unsigned int *)(inBuffer->GetBase()+addr); }

inline void __hxcpp_memory_set_byte(Array<unsigned char> inBuffer ,int addr,int v) { inBuffer->GetBase()[addr] = v; }
inline void __hxcpp_memory_set_double(Array<unsigned char> inBuffer ,int addr,double v) { *(double *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_float(Array<unsigned char> inBuffer ,int addr,double v) { *(float *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_i16(Array<unsigned char> inBuffer ,int addr,int v) { *(short *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_i32(Array<unsigned char> inBuffer ,int addr,int v) { *(int *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_ui16(Array<unsigned char> inBuffer ,int addr,int v) { *(unsigned short *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_ui32(Array<unsigned char> inBuffer ,int addr,int v) { *(unsigned int *)(inBuffer->GetBase()+addr) = v; }


// Uses global pointer...
extern unsigned char *__hxcpp_memory;

inline void __hxcpp_memory_clear( ) { __hxcpp_memory = 0; }
inline void __hxcpp_memory_select( Array<unsigned char> inBuffer )
   { __hxcpp_memory= (unsigned char *)inBuffer->GetBase(); }

inline int __hxcpp_memory_get_byte(int addr) { return __hxcpp_memory[addr]; }
inline double __hxcpp_memory_get_double(int addr) { return *(double *)(__hxcpp_memory+addr); }
inline double __hxcpp_memory_get_float(int addr) { return *(float *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_i16(int addr) { return *(short *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_i32(int addr) { return *(int *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_ui16(int addr) { return *(unsigned short *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_ui32(int addr) { return *(unsigned int *)(__hxcpp_memory+addr); }

inline void __hxcpp_memory_set_byte(int addr,int v) { __hxcpp_memory[addr] = v; }
inline void __hxcpp_memory_set_double(int addr,double v) { *(double *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_float(int addr,double v) { *(float *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_i16(int addr,int v) { *(short *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_i32(int addr,int v) { *(int *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_ui16(int addr,int v) { *(unsigned short *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_ui32(int addr,int v) { *(unsigned int *)(__hxcpp_memory+addr) = v; }




#endif
