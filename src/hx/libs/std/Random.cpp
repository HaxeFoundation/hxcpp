#include <hxcpp.h>
#include <hx/OS.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <string.h>
#ifdef HX_WINDOWS
#   include <windows.h>
#   include <process.h>
#elif defined(EPPC)
#   include <time.h>
#else
#   include <sys/time.h>
#   include <sys/types.h>
#   include <unistd.h>
#endif


/**
   <doc>
   <h1>Random</h1>
   <p>A seeded pseudo-random generator</p>
   </doc>
**/



#define NSEEDS   25
#define MAX      7

namespace
{

struct rnd  : public hx::Object
{
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdRandom };

   unsigned long seeds[NSEEDS];
   unsigned long cur;

   String toString() { return HX_CSTRING("rand"); }
};

static unsigned long mag01[2]={ 
   0x0, 0x8ebfd028 // magic, don't change
};

static const unsigned long init_seeds[] = {
   0x95f24dab, 0x0b685215, 0xe76ccae7, 0xaf3ec239, 0x715fad23,
   0x24a590ad, 0x69e4b5ef, 0xbf456141, 0x96bc1b7b, 0xa7bdf825,
   0xc1de75b7, 0x8858a9c9, 0x2da87693, 0xb657f9dd, 0xffdc8a9f,
   0x8121da71, 0x8b823ecb, 0x885d05f5, 0x4e20cd47, 0x5a9ad5d9,
   0x512c0c03, 0xea857ccd, 0x4cc1d30f, 0x8891a8a1, 0xa6b7aadb
};



static void rnd_set_seed( rnd *r, int s )
{
   int i;
   r->cur = 0;
   memcpy(r->seeds,init_seeds,sizeof(init_seeds));
   for(i=0;i<NSEEDS;i++)
      r->seeds[i] ^= s;
}

rnd *getRnd(Dynamic handle)
{
   rnd *r = dynamic_cast<rnd *>(handle.mPtr);
   if (!r)
      hx::Throw(HX_CSTRING("Invalid random handle"));
   return r;
}

} // end anon namespace

Dynamic _hx_std_random_new()
{
   rnd *r = new rnd();

#if defined(NEKO_WINDOWS)
  #if defined(HX_WINRT) && defined(__cplusplus_winrt)
   int pid = Windows::Security::Cryptography::CryptographicBuffer::GenerateRandomNumber();
  #else
   int pid = GetCurrentProcessId();
  #endif
#elif defined(EPPC)
   int pid = 1;
#else
   int pid = getpid();
#endif

   unsigned int t;
#ifdef HX_WINRT
   t = (unsigned int)GetTickCount64();
#elif defined(NEKO_WINDOWS)
   t = GetTickCount();
#elif defined(EPPC)
   time_t tod;
   time(&tod);
   t = (double)tod;
#else
   struct timeval tv;
   gettimeofday(&tv,NULL);
   t = tv.tv_sec * 1000000 + tv.tv_usec;
#endif   


   rnd_set_seed(r,t ^ (pid | (pid << 16)));
   return r;
}

static unsigned int rnd_int( rnd *r )
{
   unsigned int y;
   int pos = r->cur++;
    if( pos >= NSEEDS ) {
      int kk;
      for(kk=0;kk<NSEEDS-MAX;kk++)
         r->seeds[kk] = r->seeds[kk+MAX] ^ (r->seeds[kk] >> 1) ^ mag01[r->seeds[kk] % 2];      
      for(;kk<NSEEDS;kk++)
         r->seeds[kk] = r->seeds[kk+(MAX-NSEEDS)] ^ (r->seeds[kk] >> 1) ^ mag01[r->seeds[kk] % 2];      
      r->cur = 1;
      pos = 0;
   }
    y = r->seeds[pos];
    y ^= (y << 7) & 0x2b5b2500;
    y ^= (y << 15) & 0xdb8b0000;
    y ^= (y >> 16);
   return y;
}

static double rnd_float( rnd *r )
{
   double big = 4294967296.0;   
   return ((rnd_int(r) / big + rnd_int(r)) / big + rnd_int(r)) / big;
}

/**
   random_new : void -> 'random
   <doc>Create a new random with random seed</doc>
**/


#include<stdlib.h>


/**
   random_set_seed : 'random -> int -> void
   <doc>Set the generator seed</doc>
**/
void _hx_std_random_set_seed( Dynamic handle, int v )
{
   rnd_set_seed( getRnd(handle) ,v);
}

/**
   random_int : 'random -> max:int -> int
   <doc>Return a random integer modulo [max]</doc>
**/
int _hx_std_random_int( Dynamic handle, int max )
{
   if( max <= 0 )
      return 0;
   return (rnd_int( getRnd(handle)) & 0x3FFFFFFF) % max;
}

/**
   random_float : 'random -> float
   <doc>Return a random float</doc>
**/
double _hx_std_random_float( Dynamic handle )
{
   return rnd_float(getRnd(handle));
}


