#include <hxcpp.h>
#include <limits>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <android/log.h>

#if (HXCPP_ANDROID_PLATFORM>=21) && !defined(HXCPP_M64)
extern "C" {

// These functions are inlined prior to android-ndk-platform-21, which means they
// are missing from the libc functions on those older versions.
// If platform 21+ binaries are included in a build targetting an older platform,
// this file adds those missing symbols so the platform 21+ objects link without errors.

char * stpcpy(char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;
  do
    *d++ = *s;
  while (*s++ != '\0');
  return d - 1;
}


int rand() { return lrand48(); }

void srand(unsigned int x) { srand48(x); }


double atof(const char *nptr)
{
    return (strtod(nptr, 0));
}

// For bsd_signal, the problem was that it was missing from libc versions for platform 21
// and above, until it was restored in ndk r13 for compatibilty.

#ifdef HXCPP_ANDROID_COMPAT_BSD_SIGNAL

typedef __sighandler_t (*bsd_signal_func_t)(int, __sighandler_t);
bsd_signal_func_t bsd_signal_func = 0;

__sighandler_t bsd_signal(int s, __sighandler_t f)
{
 if (bsd_signal_func == 0)
 {
   // For now (up to Android 7.0) this is always available
   bsd_signal_func = (bsd_signal_func_t) dlsym(RTLD_DEFAULT, "bsd_signal");

   if (bsd_signal_func == 0)
   {
     // You may try dlsym(RTLD_DEFAULT, "signal") or dlsym(RTLD_NEXT, "signal") here
     // Make sure you add a comment here in StackOverflow
     // if you find a device that doesn't have "bsd_signal" in its libc.so!!!

     __android_log_assert("", "bsd_signal_wrapper", "bsd_signal symbol not found!");
   }
 }

 return bsd_signal_func(s, f);
}

__sighandler_t signal(int s, __sighandler_t f)
{
    return bsd_signal(s,f);
}

}

#endif

#endif

