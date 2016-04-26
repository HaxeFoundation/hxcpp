#ifdef HX_MEMORY_H_OVERRIDE
// Users can define their own header to use here, but there is no API
// compatibility gaurantee for future changes.
#include HX_MEMORY_H_OVERRIDE
#else

#ifndef HX_MEMORY_H
#define HX_MEMORY_H

inline void *HxAlloc(size_t size) {
   return malloc(size);
}

inline void *HxAllocGCBlock(size_t size) {
   return HxAlloc(size);
}

inline void HxFree(void *p) {
   return free(p);
}

#endif

#endif
