#ifndef HX_MATH
#define HX_MATH

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

#include <cmath>
#include <stdlib.h>

class HXCPP_EXTERN_CLASS_ATTRIBUTES Math_obj : public hx::Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdMath };

   typedef hx::Object super;
   typedef Math_obj OBJ_;
   HX_DO_RTTI;
   void __construct();
   static Dynamic __CreateEmpty();
   static void __boot();

   inline static int floor(double inX) { return __int__(::std::floor(inX)); }
   inline static int ceil(double inX) { return __int__(::std::ceil(inX)); }
   inline static int round(double inX) { return floor(inX+0.5); }
   inline static double ffloor(double inX) { return ::std::floor(inX); }
   inline static double fceil(double inX) { return ::std::ceil(inX); }
   inline static double fround(double inX) { return ::std::floor(inX+0.5); }
   inline static double random() { return __hxcpp_drand(); }
   inline static double sqrt(double inX) { return ::std::sqrt(inX); }
   inline static double cos(double inX) { return ::std::cos(inX); }
   inline static double sin(double inX) { return ::std::sin(inX); }
   inline static double tan(double inX) { return ::std::tan(inX); }
   inline static double atan2(double inY,double inX) { return ::std::atan2(inY,inX); }
   inline static double abs(double inX) { return ::std::fabs(inX); }
   inline static double pow(double inA,double inB) { return ::std::pow(inA,inB); }
   inline static double log(double inA) { return ::std::log(inA); }
   inline static double max(double inA,double inB) { return inA>inB ? inA:inA==inA?inB:inA; }
   inline static double min(double inA,double inB) { return inA<inB ? inA:inA==inA?inB:inA; }

   inline static double atan(double inA) { return ::std::atan(inA); }
   inline static double asin(double inA) { return ::std::asin(inA); }
   inline static double acos(double inA) { return ::std::acos(inA); }
   inline static double exp(double inA) { return ::std::exp(inA); }


   static bool isNaN(double inX);
   static bool isFinite(double inX);

#if (HXCPP_API_LEVEL>=500)
   static ::hx::Callable<int(double)> floor_dyn();
   static ::hx::Callable<int(double)> ceil_dyn();
   static ::hx::Callable<int(double)> round_dyn();
   static ::hx::Callable<double(double)> ffloor_dyn();
   static ::hx::Callable<double(double)> fceil_dyn();
   static ::hx::Callable<double(double)> fround_dyn();
   static ::hx::Callable<double()> random_dyn();
   static ::hx::Callable<double(double)> sqrt_dyn();
   static ::hx::Callable<double(double)> cos_dyn();
   static ::hx::Callable<double(double)> sin_dyn();
   static ::hx::Callable<double(double)> tan_dyn();
   static ::hx::Callable<double(double, double)> atan2_dyn();
   static ::hx::Callable<double(double)> abs_dyn();
   static ::hx::Callable<double(double, double) > pow_dyn();
   static ::hx::Callable<double(double)> log_dyn();
   static ::hx::Callable<double(double, double)> min_dyn();
   static ::hx::Callable<double(double, double)> max_dyn();
   static ::hx::Callable<double(double)> atan_dyn();
   static ::hx::Callable<double(double)> asin_dyn();
   static ::hx::Callable<double(double)> acos_dyn();
   static ::hx::Callable<double(double)> exp_dyn();
   static ::hx::Callable<bool(double)> isNaN_dyn();
   static ::hx::Callable<bool(double)> isFinite_dyn();
#else
   static Dynamic floor_dyn();
   static Dynamic ceil_dyn();
   static Dynamic round_dyn();
   static Dynamic ffloor_dyn();
   static Dynamic fceil_dyn();
   static Dynamic fround_dyn();
   static Dynamic random_dyn();
   static Dynamic sqrt_dyn();
   static Dynamic cos_dyn();
   static Dynamic sin_dyn();
   static Dynamic tan_dyn();
   static Dynamic atan2_dyn();
   static Dynamic abs_dyn();
   static Dynamic pow_dyn();
   static Dynamic log_dyn();
   static Dynamic min_dyn();
   static Dynamic max_dyn();
   static Dynamic atan_dyn();
   static Dynamic asin_dyn();
   static Dynamic acos_dyn();
   static Dynamic exp_dyn();
   static Dynamic isNaN_dyn();
   static Dynamic isFinite_dyn();
#endif

   static double NaN;
   static double PI;
   static double NEGATIVE_INFINITY;
   static double POSITIVE_INFINITY;
};

typedef hx::ObjectPtr<Math_obj> Math;


#endif
