#include <hxcpp.h>
#include <limits>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// These functions are inlined prior to android-ndk-platform-21, which means they
// are missing from the libc functions on those phones, and you will get link errors.

#if HXCPP_ANDROID_PLATFORM>=21
extern "C" {

int rand() { return lrand48(); }

void srand(unsigned int x) { srand48(x); }


double atof(const char *nptr)
{
    return (strtod(nptr, NULL));
}
extern __sighandler_t bsd_signal(int, __sighandler_t);


__sighandler_t signal(int s, __sighandler_t f)
{
    return bsd_signal(s,f);
}
}
#endif

