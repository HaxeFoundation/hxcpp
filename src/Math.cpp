#include <hxcpp.h>
#include <limits>
#include <hxMath.h>


#include <stdlib.h>
#include <time.h>
#ifndef HX_WINDOWS
#include <unistd.h>
#include <sys/time.h>
#else
#include <process.h>
#endif

#ifdef HX_ANDROID
#define rand() lrand48()
#define srand(x) srand48(x)
#endif

// -------- Math ---------------------------------------

using namespace hx;


bool Math_obj::isNaN(double inX)
  { return inX!=inX; }

bool Math_obj::isFinite(double inX)
  { return inX==inX && inX!=NEGATIVE_INFINITY && inX!=POSITIVE_INFINITY; }

double Math_obj::NaN = std::numeric_limits<double>::quiet_NaN();
double Math_obj::NEGATIVE_INFINITY = -std::numeric_limits<double>::infinity();
double Math_obj::PI = 3.1415926535897932385;
double Math_obj::POSITIVE_INFINITY = std::numeric_limits<double>::infinity();

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,floor,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,ffloor,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,ceil,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,fceil,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,round,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,fround,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC0(Math_obj,random,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,sqrt,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,cos,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,sin,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,tan,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC2(Math_obj,atan2,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,abs,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC2(Math_obj,pow,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,log,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC2(Math_obj,min,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC2(Math_obj,max,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,atan,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,asin,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,acos,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,exp,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,isNaN,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,isFinite,return);

Dynamic Math_obj::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString=="floor") return floor_dyn();
   if (inString=="ffloor") return ffloor_dyn();
   if (inString=="ceil") return ceil_dyn();
   if (inString=="fceil") return fceil_dyn();
   if (inString=="round") return round_dyn();
   if (inString=="fround") return fround_dyn();
   if (inString=="random") return random_dyn();
   if (inString=="sqrt") return sqrt_dyn();
   if (inString=="cos") return cos_dyn();
   if (inString=="sin") return sin_dyn();
   if (inString=="tan") return tan_dyn();
   if (inString=="atan2") return atan2_dyn();
   if (inString=="abs") return abs_dyn();
   if (inString=="pow") return pow_dyn();
   if (inString=="log") return log_dyn();
   if (inString=="min") return min_dyn();
   if (inString=="max") return max_dyn();
   if (inString=="atan") return atan_dyn();
   if (inString=="acos") return acos_dyn();
   if (inString=="asin") return asin_dyn();
   if (inString=="exp") return exp_dyn();
   if (inString=="isNaN") return isNaN_dyn();
   if (inString=="isFinite") return isFinite_dyn();

   if (inString=="NEGATIVE_INFINITY") return NEGATIVE_INFINITY;
   if (inString=="POSITIVE_INFINITY") return POSITIVE_INFINITY;
   if (inString=="PI") return PI;
   if (inString=="NaN") return NaN;
   return null();
}

void Math_obj::__GetFields(Array<String> &outFields) { }

static String sMathFields[] = {
   HX_CSTRING("floor"),
   HX_CSTRING("ceil"),
   HX_CSTRING("round"),
   HX_CSTRING("random"),
   HX_CSTRING("sqrt"),
   HX_CSTRING("cos"),
   HX_CSTRING("sin"),
   HX_CSTRING("tan"),
   HX_CSTRING("atan2"),
   HX_CSTRING("abs"),
   HX_CSTRING("pow"),
   HX_CSTRING("atan"),
   HX_CSTRING("acos"),
   HX_CSTRING("asin"),
   HX_CSTRING("exp"),
   HX_CSTRING("isFinite"),
   String(null()) };


Dynamic Math_obj::__SetField(const String &inString,const Dynamic &inValue, hx::PropertyAccess inCallProp) { return null(); }

Dynamic Math_obj::__CreateEmpty() { return new Math_obj; }

hx::Class Math_obj::__mClass;

/*
Class &Math_obj::__SGetClass() { return __mClass; }
Class Math_obj::__GetClass() const { return __mClass; }
bool Math_obj::__Is(hxObject *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; } \
*/


void Math_obj::__boot()
{
   Static(Math_obj::__mClass) = hx::RegisterClass(HX_CSTRING("Math"),TCanCast<Math_obj>,sMathFields,sNone, &__CreateEmpty,0 , 0 );

	unsigned int t;
#ifdef HX_WINDOWS
	t = clock();
   #ifdef HX_WINRT
	int pid = Windows::Security::Cryptography::CryptographicBuffer::GenerateRandomNumber();
   #else
	int pid = _getpid();
   #endif
#else
	int pid = getpid();
	struct timeval tv;
	gettimeofday(&tv,0);
	t = tv.tv_sec * 1000000 + tv.tv_usec;
#endif	

  srand(t ^ (pid | (pid << 16)));
  rand();
}

namespace hx
{

double DoubleMod(double inLHS,double inRHS)
{
   return fmod(inLHS,inRHS);
}

}


