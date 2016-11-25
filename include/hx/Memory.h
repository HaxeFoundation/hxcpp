#ifdef HX_MEMORY_H_OVERRIDE
// Users can define their own header to use here, but there is no API
// compatibility gaurantee for future changes.
#include HX_MEMORY_H_OVERRIDE

// Todo - special version?
inline void HxFreeGCBlock(void *p) {
   HxFree(p);
}

#else

#ifndef HX_MEMORY_H
#define HX_MEMORY_H

#include <stdlib.h>

inline void *HxAlloc(size_t size) {
   return malloc(size);
}

inline void HxFree(void *p) {
   free(p);
}

void *HxAllocGCBlock(size_t size);
void HxFreeGCBlock(void *p);


#endif

#endif
