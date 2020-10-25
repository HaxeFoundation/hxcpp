#ifndef CPP_INT64_INCLUDED
#define CPP_INT64_INCLUDED

inline cpp::Int64 _hx_int64_make(int a, int b) { return (((cpp::Int64)(unsigned int)a)<<32) | (unsigned int)b; }
inline int _hx_int64_compare(cpp::Int64 a, cpp::Int64 b)
{
   return a==b ? 0 : a<b ? -1 : 1;
}
inline int _hx_int64_ucompare(cpp::Int64 a, cpp::Int64 b)
{
   return a==b ? 0 : ( ::cpp::UInt64)a<( ::cpp::UInt64)b ? -1 : 1;
}
inline cpp::Int64 _hx_int64_complement(cpp::Int64 a) { return ~a; }
inline cpp::Int64 _hx_int64_div(cpp::Int64 a, cpp::Int64 b) { return a/b; }
inline cpp::Int64 _hx_int64_mod(cpp::Int64 a, cpp::Int64 b) { return a%b; }
inline cpp::Int64 _hx_int64_and(cpp::Int64 a, cpp::Int64 b) { return a&b; }
inline cpp::Int64 _hx_int64_or(cpp::Int64 a, cpp::Int64 b) { return a|b; }
inline cpp::Int64 _hx_int64_xor(cpp::Int64 a, cpp::Int64 b) { return a^b; }
inline cpp::Int64 _hx_int64_shl(cpp::Int64 a, int b) { return a<<(b&63); }
inline cpp::Int64 _hx_int64_shr(cpp::Int64 a, int b) { return a>>(b&63); }
inline cpp::Int64 _hx_int64_ushr(cpp::Int64 a, int b) { return ((cpp::UInt64)a)>>(b&63); }
inline int _hx_int64_high(cpp::Int64 a) { return (int)( a >>32 ); }
inline int _hx_int64_low(cpp::Int64 a) { return (int)( a & 0xffffffff ); }



#endif
