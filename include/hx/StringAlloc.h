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

template<int STACK>
class StringAlloc : public IStringAlloc
{
   char buffer[STACK];
   size_t allocated;
   char   *heap;

   public:
      inline StringAlloc() : allocated(0), heap(0) { }
      ~StringAlloc()
      {
         if (heap)
            free(heap);
      }
      void *allocBytes(size_t inBytes)
      {
         if (inBytes<=STACK)
            return buffer;
         if (inBytes>allocated)
         {
            allocated = inBytes;
            heap = (char *)realloc(heap, allocated);
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
