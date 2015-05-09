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
   template<typename T>
   explicit AnyCast(T* inPtr) : mPtr((void *)inPtr) { }

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
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_exit(int inExitCode);
void           __hxcpp_stdlibs_boot();

// --- Maths ---------------------------------------------------------
double __hxcpp_drand();
HXCPP_EXTERN_CLASS_ATTRIBUTES int __hxcpp_irand(int inMax);

// --- Casting/Converting ---------------------------------------------------------
HXCPP_EXTERN_CLASS_ATTRIBUTES bool  __instanceof(const Dynamic &inValue, const Dynamic &inType);
HXCPP_EXTERN_CLASS_ATTRIBUTES int   __int__(double x);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool  __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __hxcpp_parse_int(const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES double __hxcpp_parse_float(const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __hxcpp_create_var_args(Dynamic &inArrayFunc);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_set_float_format(String inFormat);

// --- CFFI helpers ------------------------------------------------------------------

// Used for accessing object fields by integer ID, rather than string ID.
// Used mainly for neko ndll interaction.
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __hxcpp_field_to_id( const char *inField );
HXCPP_EXTERN_CLASS_ATTRIBUTES const String &__hxcpp_field_from_id( int f );
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __hxcpp_register_prim(const HX_CHAR *inName,void *inFunc);

// Get function pointer from dll file
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __loadprim(String inLib, String inPrim,int inArgCount);
HXCPP_EXTERN_CLASS_ATTRIBUTES void *__hxcpp_get_proc_address(String inLib, String inPrim,bool inNdll, bool inQuietFail=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_run_dll(String inLib, String inPrim);
// Can assign to function pointer without error
inline hx::AnyCast __hxcpp_cast_get_proc_address(String inLib, String inPrim,bool inQuietFail=false)
{
   return hx::AnyCast(__hxcpp_get_proc_address(inLib,inPrim,false,inQuietFail));
}

HXCPP_EXTERN_CLASS_ATTRIBUTES int __hxcpp_unload_all_libraries();
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_push_dll_path(String inPath);
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_get_dll_extension();
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_get_bin_dir();

HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_get_kind(Dynamic inObject);


// Loading functions via name (dummy return value)



// --- haxe.io.BytesData ----------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len,bool inCopyPointer=false);
// UTF8 processing
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_char_array_to_utf8_string(Array<int> &inChars,int inFirst=0, int inLen=-1);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<int> __hxcpp_utf8_string_to_char_array(String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_char_bytes_to_utf8_string(String &inBytes);
HXCPP_EXTERN_CLASS_ATTRIBUTES String __hxcpp_utf8_string_to_char_bytes(String &inUTF8);


// --- IntHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES inline hx::Object   *__int_hash_create() { return 0; }
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set(Dynamic &ioHash,int inKey,const Dynamic &value);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_get(Dynamic &ioHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __int_hash_exists(Dynamic &ioHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __int_hash_remove(Dynamic &ioHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<Int>    __int_hash_keys(Dynamic &ioHash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_values(Dynamic &ioHash);
// Typed IntHash access...
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set_int(Dynamic &ioHash,int inKey,int inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set_string(Dynamic &ioHash,int inKey,::String inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set_float(Dynamic &ioHash,int inKey,Float inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __int_hash_get_int(Dynamic &ioHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __int_hash_get_string(Dynamic &ioHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float         __int_hash_get_float(Dynamic &ioHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __int_hash_to_string(Dynamic &ioHash);


// --- StringHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set(Dynamic &ioHash,String inKey,const Dynamic &value,bool inForceDynamic=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __string_hash_get(Dynamic &ioHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __string_hash_exists(Dynamic &ioHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __string_hash_remove(Dynamic &ioHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array< ::String> __string_hash_keys(Dynamic &ioHash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __string_hash_values(Dynamic &ioHash);
// Typed StringHash access...
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set_int(Dynamic &ioHash,String inKey,int inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set_string(Dynamic &ioHash,String inKey,::String inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set_float(Dynamic &ioHash,String inKey,Float inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __string_hash_get_int(Dynamic &ioHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __string_hash_get_string(Dynamic &ioHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float         __string_hash_get_float(Dynamic &ioHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __string_hash_to_string(Dynamic &ioHash);


// --- ObjectHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set(Dynamic &ioHash,Dynamic inKey,const Dynamic &value,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __object_hash_get(Dynamic &ioHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __object_hash_exists(Dynamic &ioHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __object_hash_remove(Dynamic &ioHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array< ::Dynamic> __object_hash_keys(Dynamic &ioHash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __object_hash_values(Dynamic &ioHash);
// Typed ObjectHash access...
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set_int(Dynamic &ioHash,Dynamic inKey,int inValue,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set_string(Dynamic &ioHash,Dynamic inKey,::String inValue,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set_float(Dynamic &ioHash,Dynamic inKey,Float inValue,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __object_hash_get_int(Dynamic &ioHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __object_hash_get_string(Dynamic &ioHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float         __object_hash_get_float(Dynamic &ioHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __object_hash_to_string(Dynamic &ioHash);


// --- Date --------------------------------------------------------------------------

// returns Epoch UTC timestamp (in seconds); assumes that input date parts are considered to be in local timezone date/time representation
double __hxcpp_new_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds,int inMilliseconds = 0);

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


int    __hxcpp_get_utc_hours(double inSeconds); /* returns hour part of UTC date/time representation of input time (Epoch, in seconds), 0-23 */
int    __hxcpp_get_utc_minutes(double inSeconds); /* returns minutes part of UTC date/time representation of input time (Epoch, in seconds), 0-59 */
int    __hxcpp_get_utc_seconds(double inSeconds); /* returns seconds part of UTC date/time representation of input time (Epoch, in seconds), 0-59 */
int    __hxcpp_get_utc_year(double inSeconds); /* returns year part of UTC date/time representation of input time (Epoch, in seconds) */
int    __hxcpp_get_utc_month(double inSeconds); /* returns month part of UTC date/time representation of input time (Epoch, in seconds), 0-January...11-December */
int    __hxcpp_get_utc_date(double inSeconds); /* returns day of the month part of UTC date/time representation of input time (Epoch, in seconds), 1-31 */
int    __hxcpp_get_utc_day(double inSeconds); /* returns day of the week part of UTC date/time representation of input time (Epoch, in seconds), 0-Sunday...6-Saturday */
String __hxcpp_to_utc_string(double inSeconds); /* same as __hxcpp_to_string but in corresponding UTC format */

int    __hxcpp_is_dst(double inSeconds); /* is input time (Epoch UTC timestamp, in seconds)'s local time in DST ? 1 for true, 0 for false */
double __hxcpp_timezone_offset(double inSeconds); /* input time (Epoch UTC timestamp, in seconds)'s local time zone offset from UTC, in seconds */
double __hxcpp_from_utc(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds, int inMilliSeconds); /* returns Epoch timestamp (in seconds); assumes that input date parts are considered to be in UTC date/time representation */ 



double __hxcpp_time_stamp();

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
#define HXCPP_HAS_CLASSLIST
Array<String> __hxcpp_get_class_list();

// --- Profile -------------------------------------------------------------------

void __hxcpp_start_profiler(::String inDumpFile);
void __hxcpp_stop_profiler();



// --- Memory --------------------------------------------------------------------------

inline void __hxcpp_align_set_float32( unsigned char *base, int addr, float v)
{
   #ifdef HXCPP_ALIGN_FLOAT
   if (addr & 3)
   {
      unsigned char *fBuf = (unsigned char *)&v;
      base += addr;
      base[0] = fBuf[0];
      base[1] = fBuf[1];
      base[2] = fBuf[2];
      base[3] = fBuf[3];
   }
   else
   #endif
   *(float *)(base+addr) = v;
}


inline float __hxcpp_align_get_float32( unsigned char *base, int addr)
{
   #ifdef HXCPP_ALIGN_FLOAT
   if (addr & 3)
   {
      float buf;
      unsigned char *fBuf = (unsigned char *)&buf;
      base += addr;
      fBuf[0] = base[0];
      fBuf[1] = base[1];
      fBuf[2] = base[2];
      fBuf[3] = base[3];
      return buf;
   }
   #endif
   return *(float *)(base+addr);
}


inline void __hxcpp_align_set_float64( unsigned char *base, int addr, double v)
{
   #ifdef HXCPP_ALIGN_FLOAT
   if (addr & 3)
   {
      unsigned char *dBuf = (unsigned char *)&v;
      base += addr;
      base[0] = dBuf[0];
      base[1] = dBuf[1];
      base[2] = dBuf[2];
      base[3] = dBuf[3];
      base[4] = dBuf[4];
      base[5] = dBuf[5];
      base[6] = dBuf[6];
      base[7] = dBuf[7];
   }
   else
   #endif
   *(double *)(base + addr) = v;
}


inline double __hxcpp_align_get_float64( unsigned char *base, int addr)
{
   #ifdef HXCPP_ALIGN_FLOAT
   if (addr & 3)
   {
      double buf;
      unsigned char *dBuf = (unsigned char *)&buf;
      base += addr;
      dBuf[0] = base[0];
      dBuf[1] = base[1];
      dBuf[2] = base[2];
      dBuf[3] = base[3];
      dBuf[4] = base[4];
      dBuf[5] = base[5];
      dBuf[6] = base[6];
      dBuf[7] = base[7];
      return buf;
   }
   #endif
   return *(double *)(base+addr);
}




// Threadsafe methods - takes buffer
HXCPP_EXTERN_CLASS_ATTRIBUTES void  __hxcpp_memory_memset(Array<unsigned char> &inBuffer ,int pos, int len, int value);

inline int __hxcpp_memory_get_byte(Array<unsigned char> inBuffer ,int addr) { return inBuffer->GetBase()[addr]; }
inline double __hxcpp_memory_get_double(Array<unsigned char> inBuffer ,int addr) {
   return __hxcpp_align_get_float64((unsigned char *)inBuffer->GetBase(), addr);
}
inline float __hxcpp_memory_get_float(Array<unsigned char> inBuffer ,int addr) {
   return __hxcpp_align_get_float32((unsigned char *)inBuffer->GetBase(), addr);
}
inline int __hxcpp_memory_get_i16(Array<unsigned char> inBuffer ,int addr) { return *(short *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_i32(Array<unsigned char> inBuffer ,int addr) { return *(int *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_ui16(Array<unsigned char> inBuffer ,int addr) { return *(unsigned short *)(inBuffer->GetBase()+addr); }
inline int __hxcpp_memory_get_ui32(Array<unsigned char> inBuffer ,int addr) { return *(unsigned int *)(inBuffer->GetBase()+addr); }
inline float __hxcpp_memory_get_f32(Array<unsigned char> inBuffer ,int addr) {
   return __hxcpp_align_get_float32((unsigned char *)inBuffer->GetBase(), addr);
}

inline void __hxcpp_memory_set_byte(Array<unsigned char> inBuffer ,int addr,int v) { inBuffer->GetBase()[addr] = v; }
inline void __hxcpp_memory_set_double(Array<unsigned char> inBuffer ,int addr,double v) {
   return __hxcpp_align_set_float64((unsigned char *)inBuffer->GetBase(), addr,v);
}
inline void __hxcpp_memory_set_float(Array<unsigned char> inBuffer ,int addr,float v) {
   return __hxcpp_align_set_float32((unsigned char *)inBuffer->GetBase(), addr,v);
}
inline void __hxcpp_memory_set_i16(Array<unsigned char> inBuffer ,int addr,int v) { *(short *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_i32(Array<unsigned char> inBuffer ,int addr,int v) { *(int *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_ui16(Array<unsigned char> inBuffer ,int addr,int v) { *(unsigned short *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_ui32(Array<unsigned char> inBuffer ,int addr,int v) { *(unsigned int *)(inBuffer->GetBase()+addr) = v; }
inline void __hxcpp_memory_set_f32(Array<unsigned char> inBuffer ,int addr,float v) {
   return __hxcpp_align_set_float32((unsigned char *)inBuffer->GetBase(), addr, v);
}


// Uses global pointer...
extern unsigned char *__hxcpp_memory;

inline void __hxcpp_memory_clear( ) { __hxcpp_memory = 0; }
inline void __hxcpp_memory_select( Array<unsigned char> inBuffer )
   { __hxcpp_memory= (unsigned char *)inBuffer->GetBase(); }

inline int __hxcpp_memory_get_byte(int addr) { return __hxcpp_memory[addr]; }
inline double __hxcpp_memory_get_double(int addr) { return __hxcpp_align_get_float64(__hxcpp_memory,addr); }
inline float __hxcpp_memory_get_float(int addr) { return __hxcpp_align_get_float32(__hxcpp_memory,addr); }
inline int __hxcpp_memory_get_i16(int addr) { return *(short *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_i32(int addr) { return *(int *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_ui16(int addr) { return *(unsigned short *)(__hxcpp_memory+addr); }
inline int __hxcpp_memory_get_ui32(int addr) { return *(unsigned int *)(__hxcpp_memory+addr); }
inline float __hxcpp_memory_get_f32(int addr) { return __hxcpp_align_get_float32(__hxcpp_memory,addr); }

inline void __hxcpp_memory_set_byte(int addr,int v) { __hxcpp_memory[addr] = v; }
inline void __hxcpp_memory_set_double(int addr,double v) { __hxcpp_align_set_float64(__hxcpp_memory,addr,v); }
inline void __hxcpp_memory_set_float(int addr,float v) { __hxcpp_align_set_float32(__hxcpp_memory,addr,v); }
inline void __hxcpp_memory_set_i16(int addr,int v) { *(short *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_i32(int addr,int v) { *(int *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_ui16(int addr,int v) { *(unsigned short *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_ui32(int addr,int v) { *(unsigned int *)(__hxcpp_memory+addr) = v; }
inline void __hxcpp_memory_set_f32(int addr,float v) { __hxcpp_align_set_float32(__hxcpp_memory,addr,v); }

// FPHelper conversion

inline void __hxcpp_reverse_endian(int &ioData)
{
   ioData =   (((ioData>>24) & 0xff )    )|
              (((ioData>>16) & 0xff )<<8 )|
              (((ioData>>8 ) & 0xff )<<16 )|
              (((ioData    ) & 0xff )<<24  );
}


inline float __hxcpp_reinterpret_le_int32_as_float32(int inInt)
{
   #ifdef HXCPP_BIG_ENDIAN
   __hxcpp_reverse_endian(inInt);
   #endif
   return *(float*)(&inInt);
}


inline int __hxcpp_reinterpret_float32_as_le_int32(float inFloat)
{
   #ifdef HXCPP_BIG_ENDIAN
   __hxcpp_reverse_endian(*(int *)&inFloat);
   #endif
   return *(int*)(&inFloat);
}


inline double __hxcpp_reinterpret_le_int32s_as_float64(int inLow, int inHigh)
{
   int vals[2] = {inLow, inHigh};
   #ifdef HXCPP_BIG_ENDIAN
   __hxcpp_reverse_endian(vals[0]);
   __hxcpp_reverse_endian(vals[1]);
   #endif
   return *(double*)(vals);
}


inline int __hxcpp_reinterpret_float64_as_le_int32_low(double inValue)
{
   int *asInts = (int *)&inValue;
   #ifdef HXCPP_BIG_ENDIAN
   __hxcpp_reverse_endian(asInts[0]);
   #endif
   return asInts[0];
}


inline int __hxcpp_reinterpret_float64_as_le_int32_high(double inValue)
{
   int *asInts = (int *)&inValue;
   #ifdef HXCPP_BIG_ENDIAN
   __hxcpp_reverse_endian(asInts[1]);
   #endif
   return asInts[1];
}

#endif
