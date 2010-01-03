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

void RegisterResources(hx::Resource *inResources);
} // end namespace hx

Array<String>        __hxcpp_resource_names();
String               __hxcpp_resource_string(String inName);
Array<unsigned char> __hxcpp_resource_bytes(String inName);



namespace hx
{
// Throw must return a value ...
inline Dynamic Throw(Dynamic inError) { throw inError; return Dynamic(); }
}

// System access
Array<String>  __get_args();
double         __time_stamp();
void           __hxcpp_print(Dynamic &inV);
void           __hxcpp_println(Dynamic &inV);
void           __trace(Dynamic inPtr, Dynamic inData);


// --- Casting/Converting ---------------------------------------------------------
bool  __instanceof(const Dynamic &inValue, const Dynamic &inType);
int   __int__(double x);
bool  __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2);
Dynamic __hxcpp_parse_int(const String &inString);
double __hxcpp_parse_float(const String &inString);

// --- CFFI helpers ------------------------------------------------------------------

// Used for accessing object fields by integer ID, rather than string ID.
// Used mainly for neko ndll interaction.
int           __hxcpp_field_to_id( const char *inField );
const String &__hxcpp_field_from_id( int f );
int           __hxcpp_register_prim(wchar_t *inName,void *inFunc);

// Get function pointer from dll file
Dynamic __loadprim(String inLib, String inPrim,int inArgCount);
// Loading functions via name (dummy return value)



// --- haxe.io.BytesData ----------------------------------------------------------------

void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString);
void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len);


// --- IntHash ----------------------------------------------------------------------

hx::Object   *__int_hash_create();
void          __int_hash_set(Dynamic &inHash,int inKey,const Dynamic &value);
Dynamic       __int_hash_get(Dynamic &inHash,int inKey);
bool          __int_hash_exists(Dynamic &inHash,int inKey);
bool          __int_hash_remove(Dynamic &inHash,int inKey);
Dynamic       __int_hash_keys(Dynamic &inHash);
Dynamic       __int_hash_values(Dynamic &inHash);


// --- Date --------------------------------------------------------------------------

double __hxcpp_new_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds);
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


#endif
