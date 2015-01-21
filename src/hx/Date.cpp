#include <hxcpp.h>

#include <time.h>

#ifdef HX_WINDOWS
   #include <windows.h>
   #include <Shlobj.h>
#else
   #ifdef EPPC
      #include <time.h>
   #else
      #include <sys/time.h>
   #endif
   #include <stdint.h>
   #ifdef HX_LINUX
      #include <unistd.h>
      #include <stdio.h>
   #endif
#endif

#ifdef HX_MACOS
#include <mach/mach_time.h>  
#include <mach-o/dyld.h>
#include <CoreServices/CoreServices.h>
#endif

#ifdef IPHONE
#include <QuartzCore/QuartzCore.h>
#endif


//#include <hxMacros.h>

static double t0 = 0;
double __hxcpp_time_stamp()
{
#ifdef HX_WINDOWS
   static __int64 t0=0;
   static double period=0;
   __int64 now;

   if (QueryPerformanceCounter((LARGE_INTEGER*)&now))
   {
      if (t0==0)
      {
         t0 = now;
         __int64 freq;
         QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
         if (freq!=0)
            period = 1.0/freq;
      }
      if (period!=0)
         return (now-t0)*period;
   }

   return (double)clock() / ( (double)CLOCKS_PER_SEC);
#elif defined(HX_MACOS)
   static double time_scale = 0.0;
   if (time_scale==0.0)
   {
      mach_timebase_info_data_t info;
      mach_timebase_info(&info);  
      time_scale = 1e-9 * (double)info.numer / info.denom;
   }
   double r =  mach_absolute_time() * time_scale;  
   return mach_absolute_time() * time_scale;  
#else
   #if defined(IPHONE)
      double t = CACurrentMediaTime(); 
   #elif defined(GPH) || defined(HX_LINUX) || defined(EMSCRIPTEN)
      struct timeval tv;
      if( gettimeofday(&tv,NULL) )
         return 0;
      double t =  ( tv.tv_sec + ((double)tv.tv_usec) / 1000000.0 );
   #elif defined(EPPC)
      time_t tod;
      time(&tod);
      double t = (double)tod;
   #else
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      double t =  ( ts.tv_sec + ((double)ts.tv_nsec)*1e-9  );
   #endif
   if (t0==0) t0 = t;
   return t-t0;
#endif
}




double __hxcpp_new_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds)
{
   struct tm time;
   time.tm_year = inYear;

   time.tm_isdst = -1;
   time.tm_year = inYear - 1900;
   time.tm_mon = inMonth;
   time.tm_mday = inDay;
   time.tm_hour = inHour;
   time.tm_min = inMin;
   time.tm_sec = inSeconds;

   return mktime(&time);
}

double __hxcpp_utc_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds)
{
   int diff;
   time_t a, b;
   struct tm time, *temp;
 
   time.tm_year = inYear;
   time.tm_isdst = -1;
   time.tm_year = inYear - 1900;
   time.tm_mon = inMonth;
   time.tm_mday = inDay;
   time.tm_hour = inHour;
   time.tm_min = inMin;
   time.tm_sec = inSeconds;
   
   a =  mktime(&time); //timestamp based on local interpretation of date parts
   temp = gmtime(&a); //get utc date parts corresponding to this timestamp
   temp->tm_isdst=-1; //reset dst flag for use in mktime
   b = mktime(temp); //get timestamp for local interpretation of utc date parts
   diff= a - b; //find difference in timestamp values .
   
   return a+diff; 

}

int __hxcpp_get_hours(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_hour;
}
int __hxcpp_get_minutes(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_min;
}
int __hxcpp_get_seconds(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_sec;
}
int __hxcpp_get_year(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_year + 1900;
}
int __hxcpp_get_month(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_mon;
}
int __hxcpp_get_date(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_mday;
}
int __hxcpp_get_day(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
   return time->tm_wday;
}
double __hxcpp_date_now()
{
   time_t t = time(0);
   return t;
}
String __hxcpp_to_string(double inSeconds)
{
   time_t t = (time_t)inSeconds;
   struct tm *time = localtime(&t);
#ifndef HX_UTF8_STRINGS
   wchar_t buf[100];
   wcsftime(buf,100,L"%Y-%m-%d %H:%M:%S", time);
#else
   char buf[100];
   strftime(buf,100,"%Y-%m-%d %H:%M:%S", time);
#endif
   return String(buf).dup();
}

