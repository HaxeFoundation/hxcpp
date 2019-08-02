#include <hxcpp.h>

#include <time.h>

#ifdef HX_WINDOWS
   #include <windows.h>
#else
   #include <stdint.h>
   #if defined(__unix__) || defined(__APPLE__)
      #include <unistd.h>
      #include <stdio.h>
      #if (_POSIX_VERSION >= 1)
         #define USE_TIME_R
      #endif
      #if (_POSIX_VERSION >= 199309L)
         #include <sys/time.h>
         #define USE_CLOCK_GETTIME
         #define USE_GETTIMEOFDAY
      #endif
   #endif
   #if defined(__ORBIS__)
      // fill in for a missing localtime_r with localtime_s
      #define localtime_r localtime_s
      #define gmtime_r gmtime_s
   #endif
#endif

#ifdef HX_MACOS
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <CoreServices/CoreServices.h>
#endif

#if defined(IPHONE) || defined(APPLETV)
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
   #if defined(IPHONE) || defined(APPLETV)
      double t = CACurrentMediaTime();
   #elif defined(USE_GETTIMEOFDAY)
      struct timeval tv;
      if( gettimeofday(&tv,NULL) )
         return 0;
      double t =  ( tv.tv_sec + ((double)tv.tv_usec) / 1000000.0 );
   #elif defined(USE_CLOCK_GETTIME)
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      double t =  ( ts.tv_sec + ((double)ts.tv_nsec)*1e-9  );
   #else
      double t = (double)clock() * (1.0 / CLOCKS_PER_SEC);
   #endif
   if (t0==0) t0 = t;
   return t-t0;
#endif
}

/*
 * for the provided Epoch time, fills the passed struct tm with date_time representation in local time zone
 */
void __internal_localtime(double inSeconds, struct tm* time)
{
   time_t t = (time_t) inSeconds;
   #ifdef USE_TIME_R
   localtime_r(&t, time);
   #else
   struct tm *result = localtime(&t);
   if (result)
      *time = *result;
   else
      memset(time, 0, sizeof(*time) );
   #endif
}

/*
 * for the provided Epoch time, fills the passed struct tm with with date_time representation in UTC
 */
void __internal_gmtime(double inSeconds, struct tm* time)
{
   time_t t = (time_t) inSeconds;
   #ifdef USE_TIME_R
   gmtime_r(&t, time);
   #else
   *time = *gmtime(&t);
   #endif
}

/*
 * input: takes Y-M-D h:m:s.ms (considers that date parts are in local date_time representation)
 * output: returns UTC time stamp (Epoch), in seconds
 */
double __hxcpp_new_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds, int inMilliseconds)
{
   struct tm time;

   time.tm_isdst = -1;
   time.tm_year = inYear - 1900;
   time.tm_mon = inMonth;
   time.tm_mday = inDay;
   time.tm_hour = inHour;
   time.tm_min = inMin;
   time.tm_sec = inSeconds;

   return (mktime(&time) + ((double) inMilliseconds * 0.001));
}

// Used by DateTools.makeUtc
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

/*
 * returns hh value (in hh:mm:ss) of date_time representation in local time zone
 */
int __hxcpp_get_hours(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_hour;
}

/*
 * returns mm value (in hh:mm:ss) of date_time representation in local time zone
 */
int __hxcpp_get_minutes(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_min;
}

/*
 * returns ss value (in hh:mm:ss) of date_time representation in local time zone
 */
int __hxcpp_get_seconds(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_sec;
}

/*
 * returns YYYY value (in YYYY-MM-DD) of date_time representation in local time zone
 */
int __hxcpp_get_year(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return (time.tm_year + 1900);
}

/*
 * returns MM value (in YYYY-MM-DD) of date_time representation in local time zone
 */
int __hxcpp_get_month(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_mon;
}

/*
 * returns DD value (in YYYY-MM-DD) of date_time representation in local time zone
 */
int __hxcpp_get_date(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_mday;
}

/*
 * returns week day (as int, Sun=0...Sat=6) of date_time representation in local time zone
 */
int __hxcpp_get_day(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_wday;
}

/*
 * returns hh value (in hh:mm:ss) of date_time representation in UTC
 */
int __hxcpp_get_utc_hours(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return time.tm_hour;
}

/*
 * returns mm value (in hh:mm:ss) of date_time representation in UTC
 */
int __hxcpp_get_utc_minutes(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return time.tm_min;
}

/*
 * returns ss value (in hh:mm:ss) of date_time representation in UTC
 */
int __hxcpp_get_utc_seconds(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return time.tm_sec;
}

/*
 * returns YYYY value (in YYYY-MM-DD) of date_time representation in UTC
 */
int __hxcpp_get_utc_year(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return (time.tm_year + 1900);
}

/*
 * returns MM value (in YYYY-MM-DD) of date_time representation in UTC
 */
int __hxcpp_get_utc_month(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return time.tm_mon;
}

/*
 * returns DD value (in YYYY-MM-DD) of date_time representation in UTC
 */
int __hxcpp_get_utc_date(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return time.tm_mday;
}

/*
 * returns week day (as int, Sun=0...Sat=6) of date_time representation in UTC
 */
int __hxcpp_get_utc_day(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return time.tm_wday;
}

/*
 * similar to __hxcpp_new_date but, takes no date parts as input because, it assumes NOW date_time
 */
double __hxcpp_date_now()
{
   #ifdef HX_WINDOWS
   typedef unsigned __int64 uint64_t;
   static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

   SYSTEMTIME  system_time;
   FILETIME    file_time;
   ULARGE_INTEGER ularge;

   GetSystemTime( &system_time );
   SystemTimeToFileTime( &system_time, &file_time );

   ularge.LowPart = file_time.dwLowDateTime;
   ularge.HighPart = file_time.dwHighDateTime;

   return (double)( (long) ((ularge.QuadPart - EPOCH) / 10000000L) ) +
          system_time.wMilliseconds*0.001;
   #elif defined(USE_GETTIMEOFDAY)
   struct timeval tv;
   gettimeofday(&tv, 0);
   return (tv.tv_sec + (((double) tv.tv_usec) / (1000 * 1000)));
   #else
   // per-second time resolution. not ideal, but OK given the docs for Date.now
   time_t t;
   struct tm ti;
   time(&t);
   __internal_localtime((double)t, &ti);
   return mktime(&ti);
   #endif
}

/*
 * for the input Epoch time, returns whether the corresponding local time would be in DST
 * 1 : yes, in DST ; 0 : no, not in DST
 */
int __hxcpp_is_dst(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return time.tm_isdst;
}

/*
 * for the input Epoch time, returns the correct timezone offset of local time zone;
 * return value is in seconds e.g. -28800 (that would be -8 hrs)
 */
double __hxcpp_timezone_offset(double inSeconds)
{
   struct tm localTime;
   __internal_localtime( inSeconds, &localTime);

   #if defined(HX_WINDOWS) || defined(__SNC__) || defined(__ORBIS__)
   struct tm gmTime;
   __internal_gmtime(inSeconds, &gmTime );

   return mktime(&localTime) - mktime(&gmTime);
   #else
   return localTime.tm_gmtoff;
   #endif
}

String __internal_to_string(struct tm time)
{
   // YYYY-MM-DD hh:mm:ss

   char buf[100];
   strftime(buf,100, "%Y-%m-%d %H:%M:%S", &time);
   return String::create(buf);
}

/*
 * string form of a given Epoch time, without milliseconds,
 * as in format [YYYY-MM-DD hh:mm:ss +hhmm] ex: [1997-07-16 19:20:30 +0100].
 */
String __hxcpp_to_utc_string(double inSeconds)
{
   struct tm time;
   __internal_gmtime( inSeconds, &time);
   return __internal_to_string( time);
}

/*
 * string form of a given Epoch time, without milliseconds and timezone offset,
 * as in  format [YYYY-MM-DD hh:mm:ss] ex: [1997-07-16 19:20:30].
 */
String __hxcpp_to_string(double inSeconds)
{
   struct tm time;
   __internal_localtime( inSeconds, &time);
   return __internal_to_string( time);
}

/*
 * input: takes Y-M-D h:m:s.ms (considers that date parts are in UTC date_time representation)
 * output: returns UTC time stamp (Epoch), in seconds
 */
double __hxcpp_from_utc(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds, int inMilliseconds)
{
   struct tm time;

   time.tm_isdst = -1;
   time.tm_year  = inYear - 1900;
   time.tm_mon   = inMonth;
   time.tm_mday  = inDay;
   time.tm_hour  = inHour;
   time.tm_min   = inMin;
   time.tm_sec   = inSeconds;

   time_t z = mktime(&time);
   time_t t = z + __hxcpp_timezone_offset(z);

   struct tm local_tm;
   __internal_localtime( t, &local_tm);

   return (mktime(&local_tm) + ((double) inMilliseconds * 0.001));
}

