#include <hxcpp.h>
#include <limits>
#include <hxMath.h>

// -------- Math ---------------------------------------

using namespace hx;


bool Math_obj::isNaN(double inX)
  { return inX!=inX; }
bool Math_obj::isFinite(double inX)
  { return !isNaN(inX) && inX!=NEGATIVE_INFINITY && inX!=POSITIVE_INFINITY; }

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
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,ceil,return);
STATIC_HX_DEFINE_DYNAMIC_FUNC1(Math_obj,round,return);
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

Dynamic Math_obj::__Field(const String &inString)
{
   if (inString==HX_STR(L"floor")) return floor_dyn();
   if (inString==HX_STR(L"ceil")) return ceil_dyn();
   if (inString==HX_STR(L"round")) return round_dyn();
   if (inString==HX_STR(L"random")) return random_dyn();
   if (inString==HX_STR(L"sqrt")) return sqrt_dyn();
   if (inString==HX_STR(L"cos")) return cos_dyn();
   if (inString==HX_STR(L"sin")) return sin_dyn();
   if (inString==HX_STR(L"tan")) return tan_dyn();
   if (inString==HX_STR(L"atan2")) return atan2_dyn();
   if (inString==HX_STR(L"abs")) return abs_dyn();
   if (inString==HX_STR(L"pow")) return pow_dyn();
   if (inString==HX_STR(L"log")) return log_dyn();
   if (inString==HX_STR(L"min")) return min_dyn();
   if (inString==HX_STR(L"max")) return max_dyn();
   if (inString==HX_STR(L"atan")) return max_dyn();
   if (inString==HX_STR(L"acos")) return max_dyn();
   if (inString==HX_STR(L"asin")) return max_dyn();
   if (inString==HX_STR(L"exp")) return max_dyn();
   if (inString==HX_STR(L"isNaN")) return isNaN_dyn();
   if (inString==HX_STR(L"isFinite")) return isFinite_dyn();
   return null();
}

Dynamic Math_obj::__IField(int inFieldID)
{
   return __Field( __hxcpp_field_from_id(inFieldID) );
}

void Math_obj::__GetFields(Array<String> &outFields) { }

static String sMathFields[] = {
   HX_STRING(L"floor",5),
   HX_STRING(L"ceil",4),
   HX_STRING(L"round",5),
   HX_STRING(L"random",6),
   HX_STRING(L"sqrt",4),
   HX_STRING(L"cos",3),
   HX_STRING(L"sin",3),
   HX_STRING(L"tan",3),
   HX_STRING(L"atan2",5),
   HX_STRING(L"abs",3),
   HX_STRING(L"pow",3),
   HX_STRING(L"atan",4),
   HX_STRING(L"acos",4),
   HX_STRING(L"asin",4),
   HX_STRING(L"exp",3),
   HX_STRING(L"isFinite",8),
   String(null()) };


Dynamic Math_obj::__SetField(const String &inString,const Dynamic &inValue) { return null(); }

Dynamic Math_obj::__CreateEmpty() { return new Math_obj; }

Class Math_obj::__mClass;

/*
Class &Math_obj::__SGetClass() { return __mClass; }
Class Math_obj::__GetClass() const { return __mClass; }
bool Math_obj::__Is(hxObject *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; } \
*/


void Math_obj::__boot()
{
   Static(Math_obj::__mClass) = RegisterClass(HX_STRING(L"Math",4),TCanCast<Math_obj>,sMathFields,sNone, &__CreateEmpty,0 , 0 );
}

namespace hx
{

double DoubleMod(double inLHS,double inRHS)
{
   if (inRHS==0) return 0;
   double divs = inLHS/inRHS;
   int lots = divs<0 ? (int)(-divs) : (int)divs;
   return inLHS - lots*inRHS;
}

}


