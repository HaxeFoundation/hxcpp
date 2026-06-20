#ifndef HX_STRING_ALLOC
#define HX_STRING_ALLOC

#include <stdlib.h>
#include <stdint.h>

namespace hx
{

class IStringAlloc
{
   public:
      virtual void *allocBytes(size_t inBytes) = 0;

   protected:
      ~IStringAlloc(){};
};

#ifndef HXCPP_OVERRIDE
   #if HXCPP_API_LEVEL >= 500 && (__cplusplus >= 201103L || (defined(_MSC_VER) && _MSVC_LANG >= 201103L))
      #define HXCPP_OVERRIDE override
   #else
      #define HXCPP_OVERRIDE
   #endif
#endif

template<int STACK>
class StringAlloc : public IStringAlloc
{
   char buffer[STACK];
   size_t alloced;
   char   *heap;

   public:
      inline StringAlloc() : alloced(0), heap(0) { }
      ~StringAlloc()
      {
         if (heap)
            free(heap);
      }
      void *allocBytes(size_t inBytes) HXCPP_OVERRIDE
      {
         if (inBytes<=STACK)
            return buffer;
         if (inBytes>alloced)
         {
            alloced = inBytes;
            heap = (char *)realloc(heap, alloced);
         }
         return heap;
      }
   private:
      StringAlloc(const StringAlloc &);
      void operator=(const StringAlloc &);
};

typedef StringAlloc<100> strbuf;

} // end namespace hx

#endif
