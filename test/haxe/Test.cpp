#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
#define NEKO_COMPATIBLE
#endif

#include <hx/CFFI.h>
#include <string.h>

# define fdatasync fsync

static value test_null() {
    return alloc_null();
}

DEFINE_PRIM(test_null,0);

extern "C" int test_register_prims() { return 0; }