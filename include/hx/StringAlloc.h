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
   size_t alloced;
   char   *heap;

   public:
      inline StringAlloc() : alloced(0), heap(0) { }
      ~StringAlloc()
      {
         if (heap)
            free(heap);
      }
      void *allocBytes(size_t inBytes)
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
