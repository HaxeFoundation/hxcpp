#ifndef HX_OPERATORS_H
#define HX_OPERATORS_H


template<typename T> bool null::operator == (const hx::ObjectPtr<T> &O) { return !O.mPtr; }
template<typename T> inline bool null::operator != (const hx::ObjectPtr<T> &O) { return O.mPtr; }

template<typename T> inline bool null::operator == (const Array<T> &O) { return !O.mPtr; }
template<typename T> inline bool null::operator != (const Array<T> &O) { return O.mPtr; }

inline bool null::operator == (const Dynamic &O) { return !O.mPtr; }
inline bool null::operator != (const Dynamic &O) { return O.mPtr; }

inline bool null::operator == (const String &O) { return !O.__s; }
inline bool null::operator != (const String &O) { return O.__s; }

// Operators for mixing various types ....


inline String operator+(const Int &i,const String &s) { return String(i) + s; }
inline String operator+(const double &d,const String &s) { return String(d) + s; }
inline String operator+(const bool &b,const String &s) { return String(b) + s; }
inline String operator+(const wchar_t *c,const String &s) { return String(c) + s; }
inline String operator+(const null &n,const String &s) { return String(n) + s; }
inline String operator+(const cpp::CppInt32__ &i,const String &s) { return String(i) + s; }

template<typename T_>
   inline String operator+(const hx::ObjectPtr<T_> &inLHS,const String &s)
   { return (inLHS.mPtr ? const_cast<hx::ObjectPtr<T_> & >(inLHS)->toString() : STRING(L"null",4) ) + s; }

template<typename RHS_>
   inline Dynamic operator+(const hx::FieldRef &inField,RHS_ &inRHS)
   { return inField.operator Dynamic() + inRHS; }



// += -= *= /= %= &= |= ^= <<= >>= >>>=

namespace hx
{

inline int UShr(int inData,int inShift)
{
   return ((unsigned int)inData) >> inShift;
}


double DoubleMod(double inLHS,double inRHS);

template<typename TL,typename TR>
double Mod(TL inLHS,TR inRHS) { return hx::DoubleMod(inLHS,inRHS); }

#if !defined(_MSC_VER) || _MSC_VER > 1399
inline int Mod(int inLHS,int inRHS) { return inLHS % inRHS; }
#endif


template<typename L, typename R>
inline L& AddEq(L &inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename L, typename R>
inline L& MultEq(L &inLHS, R inRHS) { inLHS = (double)inLHS * (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& DivEq(L &inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& SubEq(L &inLHS, R inRHS) { inLHS = (double)inLHS - (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& AndEq(L &inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& OrEq(L &inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& XorEq(L &inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& ShlEq(L &inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& ShrEq(L &inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& UShrEq(L &inLHS, R inRHS) { inLHS = hx::UShr(inLHS,inRHS); return inLHS; }
template<typename L, typename R>
inline L& ModEq(L &inLHS, R inRHS) { inLHS = (int)inLHS % (int)inRHS; return inLHS; }

#ifdef __GNUC__
template<typename R>
inline hx::FieldRef AddEq(hx::FieldRef inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef MultEq(hx::FieldRef inLHS, R inRHS) { inLHS = (double)inLHS * (double)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef DivEq(hx::FieldRef inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef SubEq(hx::FieldRef inLHS, R inRHS) { inLHS = (double)inLHS - (double)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef AndEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef OrEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef XorEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef ShlEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef ShrEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename R>
inline hx::FieldRef UShrEq(hx::FieldRef inLHS, R inRHS) { inLHS = hx::UShr(inLHS,inRHS); return inLHS; }
template<typename R>
inline hx::FieldRef ModEq(hx::FieldRef inLHS, R inRHS) { inLHS = (int)inLHS % (int)inRHS; return inLHS; }

#endif // __GNUC__

}




#endif
