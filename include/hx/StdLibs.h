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

HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_print_string(const String &inV);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_println_string(const String &inV);

template<typename T> inline void __hxcpp_println(T inV)
{
   Dynamic d(inV);
   __hxcpp_println_string(d);
}
// Specialization that does not need dynamic boxing
template<> inline void __hxcpp_println(String inV)
{
   __hxcpp_println_string(inV);
}

template<typename T> inline void __hxcpp_print(T inV)
{
   Dynamic d(inV);
   __hxcpp_print_string(d);
}
// Specialization that does not need dynamic boxing
template<> inline void __hxcpp_print(String inV)
{
   __hxcpp_print_string(inV);
}



HXCPP_EXTERN_CLASS_ATTRIBUTES void __trace(Dynamic inPtr, Dynamic inData);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_exit(int inExitCode);
void           __hxcpp_stdlibs_boot();

HXCPP_EXTERN_CLASS_ATTRIBUTES int hxcpp_alloc_kind();

// --- Maths ---------------------------------------------------------
double __hxcpp_drand();
HXCPP_EXTERN_CLASS_ATTRIBUTES int __hxcpp_irand(int inMax);

// --- Casting/Converting ---------------------------------------------------------
HXCPP_EXTERN_CLASS_ATTRIBUTES bool  __instanceof(const Dynamic &inValue, const Dynamic &inType);
HXCPP_EXTERN_CLASS_ATTRIBUTES int   __int__(double x);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool  __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __hxcpp_parse_int(const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES double __hxcpp_parse_float(const String &inString);
HXCPP_EXTERN_CLASS_ATTRIBUTES double __hxcpp_parse_substr_float(const String &inString, int start, int len);
HXCPP_EXTERN_CLASS_ATTRIBUTES int __hxcpp_parse_substr_int(const String &inString, int start=0, int len=-1);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic __hxcpp_create_var_args(Dynamic &inArrayFunc);
HXCPP_EXTERN_CLASS_ATTRIBUTES void __hxcpp_set_float_format(String inFormat);

inline int _hx_idiv(int inNum,int inDenom) { return inNum/inDenom; }
inline int _hx_imod(int inNum,int inDenom) { return inNum%inDenom; }
inline int _hx_cast_int(int inX) { return inX; }
inline int _hx_fast_floor(double inX) {
   union Cast
   {
      double d;
      long l;
   };
   Cast c;
   c.d = (inX-0.5) + 6755399441055744.0;
   return c.l;
}



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


#ifdef HXCPP_GC_GENERATIONAL
   #define HX_MAP_THIS this, h
   #define HX_MAP_THIS_ this,
   #define HX_MAP_THIS_ARG hx::Object *owner, Dynamic &ioHash
#else
   #define HX_MAP_THIS h
   #define HX_MAP_THIS_ 
   #define HX_MAP_THIS_ARG Dynamic &ioHash
#endif

// --- IntHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES inline hx::Object   *__int_hash_create() { return 0; }
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set(HX_MAP_THIS_ARG,int inKey,const Dynamic &value);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __int_hash_exists(Dynamic &hash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __int_hash_remove(Dynamic &hash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<int>    __int_hash_keys(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_values(Dynamic &hash);
// Typed IntHash access...
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set_int(HX_MAP_THIS_ARG,int inKey,int inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set_string(HX_MAP_THIS_ARG,int inKey,::String inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_set_float(HX_MAP_THIS_ARG,int inKey,Float inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __int_hash_to_string(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __int_hash_clear(Dynamic &hash);

HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __int_hash_get(Dynamic inHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __int_hash_get_int(Dynamic inHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __int_hash_get_string(Dynamic inHash,int inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float         __int_hash_get_float(Dynamic inHash,int inKey);
inline  bool   __int_hash_get_bool(Dynamic inHash,int inKey) { return __int_hash_get_int(inHash,inKey); }

// --- StringHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set(HX_MAP_THIS_ARG,String inKey,const Dynamic &value,bool inForceDynamic=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __string_hash_exists(Dynamic &hash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __string_hash_remove(Dynamic &hash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array< ::String> __string_hash_keys(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __string_hash_values(Dynamic &hash);
// Typed StringHash access...
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set_int(HX_MAP_THIS_ARG,String inKey,int inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set_string(HX_MAP_THIS_ARG,String inKey,::String inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_set_float(HX_MAP_THIS_ARG,String inKey,Float inValue);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __string_hash_map_substr(HX_MAP_THIS_ARG,String inKey,int inStart, int inLength);

HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __string_hash_to_string(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __string_hash_to_string_raw(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __string_hash_clear(Dynamic &hash);

HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __string_hash_get(Dynamic inHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __string_hash_get_int(Dynamic inHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __string_hash_get_string(Dynamic inHash,String inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float         __string_hash_get_float(Dynamic inHash,String inKey);
inline  bool __string_hash_get_bool(Dynamic inHash,String inKey) { return __string_hash_get_int(inHash,inKey); }

// --- ObjectHash ----------------------------------------------------------------------

HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set(HX_MAP_THIS_ARG,Dynamic inKey,const Dynamic &value,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __object_hash_exists(Dynamic &hash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool          __object_hash_remove(Dynamic &hash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array< ::Dynamic> __object_hash_keys(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __object_hash_values(Dynamic &hash);
// Typed ObjectHash access...
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set_int(HX_MAP_THIS_ARG,Dynamic inKey,int inValue,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set_string(HX_MAP_THIS_ARG,Dynamic inKey,::String inValue,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_set_float(HX_MAP_THIS_ARG,Dynamic inKey,Float inValue,bool inWeakKey=false);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __object_hash_to_string(Dynamic &hash);
HXCPP_EXTERN_CLASS_ATTRIBUTES void          __object_hash_clear(Dynamic &hash);


HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic       __object_hash_get(Dynamic inHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES int           __object_hash_get_int(Dynamic inHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES ::String      __object_hash_get_string(Dynamic inHash,Dynamic inKey);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float         __object_hash_get_float(Dynamic inHash,Dynamic inKey);
inline bool  __object_hash_get_bool(Dynamic inHash,Dynamic inKey) { return __object_hash_get_int(inHash,inKey); }

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

bool _hx_atomic_exchange_if(::cpp::Pointer<cpp::AtomicInt> inPtr, int test, int newVal  );
int _hx_atomic_inc(::cpp::Pointer<cpp::AtomicInt> inPtr );
int _hx_atomic_dec(::cpp::Pointer<cpp::AtomicInt> inPtr );

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
      const unsigned char *src = (const unsigned char *)&v;
      unsigned char *dest = base + addr;
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      dest[3] = src[3];
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
      unsigned char *dest = (unsigned char *)&buf;
      const unsigned char *src = base + addr;
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      dest[3] = src[3];
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
      unsigned char *dest = base + addr;
      const unsigned char *src = (const unsigned char *)&v;
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      dest[3] = src[3];
      dest[4] = src[4];
      dest[5] = src[5];
      dest[6] = src[6];
      dest[7] = src[7];
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
      unsigned char *dest = (unsigned char *)&buf;
      const unsigned char *src = base + addr;
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      dest[3] = src[3];
      dest[4] = src[4];
      dest[5] = src[5];
      dest[6] = src[6];
      dest[7] = src[7];
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

#ifdef __OBJC__
#ifdef HXCPP_OBJC

inline NSData *_hx_bytes_to_nsdata( ::Array<unsigned char> inBytes)
{
   if (!inBytes.mPtr)
     return nil;

   return [NSData dataWithBytes: inBytes->getBase() length:inBytes->length ];

}

inline ::Array<unsigned char> _hx_nsdata_to_bytes(NSData *inData)
{
   if (inData==nil)
      return null();

   return ::Array_obj<unsigned char>::fromData( (const unsigned char *)inData.bytes, inData.length );
}

#endif
#endif

HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_regexp_new_options(String s, String options);

// EReg.hx -> src/hx/libs/regexp/RegExp.cpp
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_regexp_new_options(String s, String options);
HXCPP_EXTERN_CLASS_ATTRIBUTES bool    _hx_regexp_match(Dynamic handle, String string, int pos, int len);
HXCPP_EXTERN_CLASS_ATTRIBUTES String  _hx_regexp_matched(Dynamic handle, int pos);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_regexp_matched_pos(Dynamic handle, int match);


// haxe.zip.(Un)Compress.hx -> src/hx/libs/zlib/ZLib.cpp
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_deflate_init(int level);
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_deflate_bound(Dynamic handle,int length);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_deflate_buffer(Dynamic handle, Array<unsigned char> src, int srcPos, Array<unsigned char> dest, int destPos);
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_deflate_end(Dynamic handle);

HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_inflate_init(Dynamic windowBits);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_inflate_buffer(Dynamic handle, Array<unsigned char> src, int srcPos, Array<unsigned char> dest, int destPos);
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_inflate_end(Dynamic handle);

HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_zip_set_flush_mode(Dynamic handle, String flushMode);

// sys.db.Mysql.hx -> src/hx/libs/regexp/RegExp.cpp
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_mysql_connect(Dynamic params);
HXCPP_EXTERN_CLASS_ATTRIBUTES void    _hx_mysql_select_db(Dynamic handle,String db);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_mysql_request(Dynamic handle,String req);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_mysql_close(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES String  _hx_mysql_escape(Dynamic handle,String str);
HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_mysql_result_get_length(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_mysql_result_get_nfields(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_mysql_result_next(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES String  _hx_mysql_result_get(Dynamic handle,int i);
HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_mysql_result_get_int(Dynamic handle,int i);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float   _hx_mysql_result_get_float(Dynamic handle,int i);
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<String> _hx_mysql_result_get_fields_names(Dynamic handle);

namespace cpp { template<typename T> class Function; }

HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_mysql_set_conversion(
      cpp::Function< Dynamic(Dynamic) > inCharsToBytes,
      cpp::Function< Dynamic(Float) > inTimeToDate );

// sys.db.Sqlite.hx -> src/hx/libs/sqlite/RegExp.cpp

HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_sqlite_connect(String filename);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_sqlite_request(Dynamic handle,String req);
HXCPP_EXTERN_CLASS_ATTRIBUTES void    _hx_sqlite_close(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_sqlite_last_insert_id(Dynamic handle);

HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_sqlite_result_get_length(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_sqlite_result_get_nfields(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_sqlite_result_next(Dynamic handle);
HXCPP_EXTERN_CLASS_ATTRIBUTES String  _hx_sqlite_result_get(Dynamic handle,int i);
HXCPP_EXTERN_CLASS_ATTRIBUTES int     _hx_sqlite_result_get_int(Dynamic handle,int i);
HXCPP_EXTERN_CLASS_ATTRIBUTES Float   _hx_sqlite_result_get_float(Dynamic handle,int i);

// src/hx/libs/std ..
// File
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_file_open( String fname, String r );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_file_close( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_file_write( Dynamic handle, Array<unsigned char> s, int p, int n );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_file_write_char( Dynamic handle, int c );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_file_read( Dynamic handle, Array<unsigned char> buf, int p, int n );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_file_read_char( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_file_seek( Dynamic handle, int pos, int kind );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_file_tell( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES bool _hx_std_file_eof( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_file_flush( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_file_contents_string( String name );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<unsigned char> _hx_std_file_contents_bytes( String name );
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_file_stdin();
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_file_stdout();
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_file_stderr();

// Process
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_process_run( String cmd, Array<String> vargs, int inShow= 1 /* SHOW_NORMAL */ );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_process_stdout_read( Dynamic handle, Array<unsigned char> buf, int pos, int len );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_process_stderr_read( Dynamic handle, Array<unsigned char> buf, int pos, int len );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_process_stdin_write( Dynamic handle, Array<unsigned char> buf, int pos, int len );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_process_stdin_close( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_process_exit( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_process_pid( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_process_kill( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_process_close( Dynamic handle );

// Random
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_random_new();
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_random_set_seed( Dynamic handle, int v );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_random_int( Dynamic handle, int max );
HXCPP_EXTERN_CLASS_ATTRIBUTES double _hx_std_random_float( Dynamic handle );

// Socket
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_init();
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_socket_new( bool udp, bool ipv6 = false );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_bind( Dynamic o, int host, int port );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_bind_ipv6( Dynamic o, Array<unsigned char> host, int port );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_close( Dynamic handle );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_send_char( Dynamic o, int c );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_socket_send( Dynamic o, Array<unsigned char> buf, int p, int l );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_socket_recv( Dynamic o, Array<unsigned char> buf, int p, int l );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_socket_recv_char( Dynamic o );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_write( Dynamic o, Array<unsigned char> buf );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<unsigned char> _hx_std_socket_read( Dynamic o );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_host_resolve( String host );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<unsigned char> _hx_std_host_resolve_ipv6( String host, bool dummy=true );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_host_to_string( int ip );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_host_to_string_ipv6( Array<unsigned char> ip );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_host_reverse( int host );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_host_reverse_ipv6( Array<unsigned char> host );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_host_local();
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_connect( Dynamic o, int host, int port );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_connect_ipv6( Dynamic o, Array<unsigned char> host, int port );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_listen( Dynamic o, int n );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<Dynamic> _hx_std_socket_select( Array<Dynamic> rs, Array<Dynamic> ws, Array<Dynamic> es, Dynamic timeout );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_fast_select( Array<Dynamic> rs, Array<Dynamic> ws, Array<Dynamic> es, Dynamic timeout );
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_socket_accept( Dynamic o );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<int> _hx_std_socket_peer( Dynamic o );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<int> _hx_std_socket_host( Dynamic o );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_set_timeout( Dynamic o, Dynamic t );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_shutdown( Dynamic o, bool r, bool w );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_set_blocking( Dynamic o, bool b );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_set_fast_send( Dynamic o, bool b );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_set_broadcast( Dynamic o, bool b );
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_socket_poll_alloc( int nsocks );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<Dynamic> _hx_std_socket_poll_prepare( Dynamic pdata, Array<Dynamic> rsocks, Array<Dynamic> wsocks );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_socket_poll_events( Dynamic pdata, double timeout );
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<Dynamic> _hx_std_socket_poll( Array<Dynamic> socks, Dynamic pdata, double timeout );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_socket_send_to( Dynamic o, Array<unsigned char> buf, int p, int l, Dynamic inAddr );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_socket_recv_from( Dynamic o, Array<unsigned char> buf, int p, int l, Dynamic outAddr);

// Sys
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_get_env( String v );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_put_env( String e, String v );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_sys_sleep( double f );
HXCPP_EXTERN_CLASS_ATTRIBUTES bool _hx_std_set_time_locale( String l );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_get_cwd();
HXCPP_EXTERN_CLASS_ATTRIBUTES bool _hx_std_set_cwd( String d );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_sys_string();
HXCPP_EXTERN_CLASS_ATTRIBUTES bool _hx_std_sys_is64();
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_sys_command( String cmd );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_sys_exit( int code );
HXCPP_EXTERN_CLASS_ATTRIBUTES bool _hx_std_sys_exists( String path );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_file_delete( String path );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_sys_rename( String path, String newname );
HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_std_sys_stat( String path );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_sys_file_type( String path );
HXCPP_EXTERN_CLASS_ATTRIBUTES bool _hx_std_sys_create_dir( String path, int mode );
HXCPP_EXTERN_CLASS_ATTRIBUTES void _hx_std_sys_remove_dir( String path );
HXCPP_EXTERN_CLASS_ATTRIBUTES double _hx_std_sys_time();
HXCPP_EXTERN_CLASS_ATTRIBUTES double _hx_std_sys_cpu_time();
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<String> _hx_std_sys_read_dir( String p);
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_file_full_path( String path );
HXCPP_EXTERN_CLASS_ATTRIBUTES String _hx_std_sys_exe_path();
HXCPP_EXTERN_CLASS_ATTRIBUTES Array<String> _hx_std_sys_env();
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_sys_getch( bool b );
HXCPP_EXTERN_CLASS_ATTRIBUTES int _hx_std_sys_get_pid();


// SSL
void _hx_ssl_init();
Dynamic _hx_ssl_new( Dynamic hconf );
void _hx_ssl_close( Dynamic hssl );
void _hx_ssl_debug_set (int i);
void _hx_ssl_handshake( Dynamic handle );
void _hx_ssl_set_socket( Dynamic hssl, Dynamic hsocket );
void _hx_ssl_set_hostname( Dynamic hssl, String hostname );
Dynamic _hx_ssl_get_peer_certificate( Dynamic hssl );
bool _hx_ssl_get_verify_result( Dynamic hssl );
void _hx_ssl_send_char( Dynamic hssl, int v );
int _hx_ssl_send( Dynamic hssl, Array<unsigned char> buf, int p, int l );
void _hx_ssl_write( Dynamic hssl, Array<unsigned char> buf );
int _hx_ssl_recv_char( Dynamic hssl );
int _hx_ssl_recv( Dynamic hssl, Array<unsigned char> buf, int p, int l );
Array<unsigned char> _hx_ssl_read( Dynamic hssl );
Dynamic _hx_ssl_conf_new( bool server );
void _hx_ssl_conf_close( Dynamic hconf );
void _hx_ssl_conf_set_ca( Dynamic hconf, Dynamic hcert );
void _hx_ssl_conf_set_verify( Dynamic hconf, int mode );
void _hx_ssl_conf_set_cert( Dynamic hconf, Dynamic hcert, Dynamic hpkey );
void _hx_ssl_conf_set_servername_callback( Dynamic hconf, Dynamic obj );
Dynamic _hx_ssl_cert_load_defaults();
Dynamic _hx_ssl_cert_load_file( String file );
Dynamic _hx_ssl_cert_load_path( String path );
String _hx_ssl_cert_get_subject( Dynamic hcert, String objname );
String _hx_ssl_cert_get_issuer( Dynamic hcert, String objname );
Array<String> _hx_ssl_cert_get_altnames( Dynamic hcert );
Array<int> _hx_ssl_cert_get_notbefore( Dynamic hcert );
Array<int> _hx_ssl_cert_get_notafter( Dynamic hcert );
Dynamic _hx_ssl_cert_get_next( Dynamic hcert );
Dynamic _hx_ssl_cert_add_pem( Dynamic hcert, String data );
Dynamic _hx_ssl_cert_add_der( Dynamic hcert, Array<unsigned char> buf );
Dynamic _hx_ssl_key_from_der( Array<unsigned char> buf, bool pub );
Dynamic _hx_ssl_key_from_pem( String data, bool pub, String pass );
Array<unsigned char> _hx_ssl_dgst_make( Array<unsigned char> buf, String alg );
Array<unsigned char> _hx_ssl_dgst_sign( Array<unsigned char> buf, Dynamic hpkey, String alg );
bool _hx_ssl_dgst_verify( Array<unsigned char> buf, Array<unsigned char> sign, Dynamic hpkey, String alg );


#endif
