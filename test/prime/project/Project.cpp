#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
// Include neko glue....
#define NEKO_COMPATIBLE
#endif
#include <hx/CFFIPrime.h>


int addInts(int a, int b)
{
   return a+b;
}
DEFINE_PRIME2(addInts);

