#ifndef HX_CFFI_LOADER_H
#define HX_CFFI_LOADER_H

/*
  This file will only be incuded in one cpp file in the ndll library -
    the one with IMPLEMENT_API #defined.

  The other files will refer to the val_ functions via the "extern" in CFFI.h

  For dynamic linking, a macro (DEFFUNC) implements the "val_..." functions as function pointers,
   and the cpp code calls these function pointers directly.
  The pointers starts off as function pointers to bootstrap code, so when they are first called
   the bootstrap uses the "ResolveProc" to find the correct version of the function for the particular
   platform, and replaces the function pointer with this value. Subsequent calls then go directly
   to the correct fucntion.

  The ResolveProc can come from:
   Explicitly setting - the proc is set when a dll is loaded into the hxcpp exe
   Via 'GetProcAddress' on the exe - if symbols are needed and the proc has not been set
   Internal implementation (CFFINekoLoader) - when linking agaist a neko process.
    - Old code used to find this in NekoApi.dll, but the glue code is now built into each ndll directly.

  For static linking, the functions are resolved at link time.

  For HXCPP_JS_PRIME, these functions are implemented in CFFIJsPrime
*/

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef NEKO_WINDOWS
#include <windows.h>
#include <stdio.h>
// Stoopid windows ...
#ifdef RegisterClass
#undef RegisterClass
#endif
#ifdef abs
#undef abs
#endif

#else // NOT NEKO_WINDOWS

#ifdef NEKO_LINUX
#define EXT "dso"
#define NEKO_EXT "so"
//#define __USE_GNU 1

#elif defined(HX_MACOS)
#include <mach-o/dyld.h>
#define EXT "dylib"
#define NEKO_EXT "dylib"

#else
#if defined(EMSCRIPTEN)
#define EXT "ll"
#else
#define EXT "so"
#endif

#endif

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


#endif
#if defined(BLACKBERRY)
using namespace std;
#endif
typedef void *(*ResolveProc)(const char *inName);
static ResolveProc sResolveProc = 0;

extern "C" {
EXPORT void hx_set_loader(ResolveProc inProc)
{
   #ifdef ANDROID
   __android_log_print(ANDROID_LOG_INFO, "haxe plugin", "Got Load Proc %p", inProc );
   #endif
   sResolveProc = inProc;
}
}



#ifdef HXCPP_JS_PRIME // { js prime

#define DEFFUNC(name,ret,def_args,call_args) \
extern "C" ret name def_args;

#include "CFFIJsPrime.h"

#elif defined(STATIC_LINK)  // js prime }   { not js prime, static link

#define DEFFUNC(name,ret,def_args,call_args) \
extern "C" ret name def_args;

#else  // static link }   { Dynamic link

   #ifdef NEKO_COMPATIBLE

      #include "CFFINekoLoader.h"

   #endif // NEKO_COMPATIBLE


   // This code will get run when the library is compiled against a newer version of hxcpp,
   //  and the application code uses an older version.
   bool default_val_is_buffer(void *inBuffer)
   {
      typedef void *(ValToBufferFunc)(void *);
      static ValToBufferFunc *valToBuffer = 0;
      if (!valToBuffer)
         valToBuffer = (ValToBufferFunc *)sResolveProc("val_to_buffer");

      if (valToBuffer)
         return valToBuffer(inBuffer)!=0;

      return false;
   }

   // Neko, old cpp and js_prime are all utf8 based - and go through here
   #ifdef HXCPP_PRIME
   struct DynAlloc : public hx::IStringAlloc
   {
      #define WANT_DYNALLOC_ALLOC_BYTES
      void *allocBytes(size_t n);
   };


   HxString default_string_wchar(const wchar_t *src,int len)
   {
      hx::strbuf buf;
      const char *str = cffi::to_utf8(src,len,&buf);
      return HxString(str,len);
   }
   HxString default_string_utf8(const char *str,int len)
   {
      return HxString(str,len);
   }
   HxString default_string_utf16(const char16_t *src,int len)
   {
      hx::strbuf buf;
      const char *str = cffi::to_utf8(src,len,&buf);
      return HxString(str,len);
   }

   const char *default_to_utf8(const HxString &str,hx::IStringAlloc *alloc)
   {
      return str.c_str();
   }
   const wchar_t *default_to_wchar(const HxString &str,hx::IStringAlloc *alloc)
   {
      DynAlloc d;
      if (!alloc)
         alloc = &d;
      return cffi::from_utf8<wchar_t>(str.c_str(),str.size(),alloc);
   }
   const char16_t *default_to_utf16(const HxString &str,hx::IStringAlloc *alloc)
   {
      DynAlloc d;
      if (!alloc)
         alloc = &d;
      return cffi::from_utf8<char16_t>(str.c_str(),str.size(),alloc);
   }
   #endif


   hx::StringEncoding default_get_encoding(void *inPtr) { return hx::StringUtf8; }

   void * default_alloc_empty_string(int) { return 0; }

   // Do nothing on earlier versions of hxcpp that do not know what to do
   void default_gc_change_managed_memory(int,const char *) { }

   void *ResolveDefault(const char *inName)
   {
      void *result = sResolveProc(inName);
      if (result)
         return result;
      if (!strcmp(inName,"val_is_buffer"))
         return (void *)default_val_is_buffer;
      if (!strcmp(inName,"alloc_empty_string"))
         return (void *)default_alloc_empty_string;
      if (!strcmp(inName,"gc_change_managed_memory"))
         return (void *)default_gc_change_managed_memory;
      if (!strcmp(inName,"hxs_encoding"))
         return (void *)default_get_encoding;
      #ifdef HXCPP_PRIME
      if (!strcmp(inName,"alloc_hxs_wchar"))
         return (void *)default_string_wchar;
      if (!strcmp(inName,"alloc_hxs_utf16"))
         return (void *)default_string_utf16;
      if (!strcmp(inName,"alloc_hxs_utf8"))
         return (void *)default_string_utf8;
      if (!strcmp(inName,"hxs_utf8"))
         return (void *)default_to_utf8;
      if (!strcmp(inName,"hxs_utf16"))
         return (void *)default_to_utf16;
      if (!strcmp(inName,"hxs_wchar"))
         return (void *)default_to_wchar;
      #endif

      return 0;
   }

   #ifdef NEKO_WINDOWS // {

   void *LoadFunc(const char *inName)
   {
      #ifndef HX_WINRT
      static const char *modules[] = { 0, "hxcpp", "hxcpp-debug" };
      for(int i=0; i<3 && sResolveProc==0; i++)
      {
         HMODULE handle = GetModuleHandleA(modules[i]);
         if (handle)
         {
            sResolveProc = (ResolveProc)GetProcAddress(handle,"hx_cffi");
            if (sResolveProc==0)
               FreeLibrary(handle);
         }
      }
      #endif

      #ifdef NEKO_COMPATIBLE
      if (sResolveProc==0)
      {
         sResolveProc = InitDynamicNekoLoader();
      }
      #endif

      if (sResolveProc==0)
      {
         fprintf(stderr,"Could not link plugin to process (hxCFFILoader.h %d)\n",__LINE__);
         exit(1);
      }
      return ResolveDefault(inName);
   }

   #else // windows }  { not windows


   void *LoadFunc(const char *inName)
   {
      #ifndef ANDROID // {
         if (sResolveProc==0)
         {
            sResolveProc = (ResolveProc)dlsym(RTLD_DEFAULT,"hx_cffi");
         }

         #ifdef NEKO_COMPATIBLE
         if (sResolveProc==0)
         {
            sResolveProc = InitDynamicNekoLoader();
         }
         #endif
      #endif // !Android  }

      if (sResolveProc==0)
      {
         #ifdef ANDROID
         __android_log_print(ANDROID_LOG_ERROR, "CFFILoader.h", "Could not API %s", inName);
         return 0;
         #else
         #ifdef NEKO_COMPATIBLE
         fprintf(stderr,"Could not link plugin to process (CFFILoader.h %d) - with neko\n",__LINE__);
         #else
         fprintf(stderr,"Could not link plugin to process (CFFILoader.h %d)\n",__LINE__);
         #endif
         exit(1);
         #endif
      }
      return ResolveDefault(inName);
   }

   #undef EXT

   #endif // not windows }



   #ifndef ANDROID // not android {

   #define DEFFUNC(name,ret,def_args,call_args) \
   typedef ret (*FUNC_##name)def_args; \
   extern FUNC_##name name; \
   ret IMPL_##name def_args \
   { \
      name = (FUNC_##name)LoadFunc(#name); \
      if (!name) \
      { \
         fprintf(stderr,"Could not find function:" #name " \n"); \
         exit(1); \
      } \
      return name call_args; \
   }\
   FUNC_##name name = IMPL_##name;
    
   #ifdef NEKO_COMPATIBLE
   DEFINE_PRIM(neko_init,5)
   #endif

   #else // not android }  { android


   #define DEFFUNC(name,ret,def_args,call_args) \
   typedef ret (*FUNC_##name)def_args; \
   extern FUNC_##name name; \
   ret IMPL_##name def_args \
   { \
      name = (FUNC_##name)LoadFunc(#name); \
      if (!name) \
      { \
         __android_log_print(ANDROID_LOG_ERROR,"CFFILoader", "Could not find function:" #name "\n"); \
      } \
      return name call_args; \
   }\
   FUNC_##name name = IMPL_##name;
    

   #endif // android }

#endif // dynamic link }

#endif


