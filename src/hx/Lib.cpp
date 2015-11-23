#include <hxcpp.h>

#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <stdlib.h>

#ifdef ANDROID
#include <android/log.h>
#include <unistd.h>
#endif


#if !defined(HX_WINRT) && !defined(EPPC)
#include <sys/types.h>
#include <sys/stat.h>
#endif
#if defined(BLACKBERRY)
using namespace std;
#endif
#ifdef HXCPP_LOAD_DEBUG
bool gLoadDebug = true;
#else
bool gLoadDebug = false;
#endif

#ifdef _WIN32

#include <windows.h>
typedef HMODULE Module;

#ifdef HX_WINRT

Module hxLoadLibrary(String inLib) { return LoadPackagedLibrary(inLib.__WCStr(),0); }

#else // Windows, not WinRT
Module hxLoadLibrary(String inLib)
{
   HMODULE result = LoadLibraryW(inLib.__WCStr());
   if (gLoadDebug)
   {
      if (result)
         printf("Loaded module : %S.\n", inLib.__WCStr());
   }
   return result;
}
#endif

void *hxFindSymbol(Module inModule, const char *inSymbol) { return (void *)GetProcAddress(inModule,inSymbol); }

void hxFreeLibrary(Module inModule) { FreeLibrary(inModule); }

#elif (defined (IPHONE) || defined(EMSCRIPTEN) || defined(STATIC_LINK) || defined(APPLETV) ) && !defined(HXCPP_DLL_IMPORT) && (!defined(HXCPP_DLL_EXPORT) || defined(HXCPP_SCRIPTABLE) )

typedef void *Module;
Module hxLoadLibrary(const String &) { return 0; }
void hxFreeLibrary(Module) { }

#else

typedef void *Module;

#include <dlfcn.h>
typedef void *Module;
Module hxLoadLibrary(String inLib)
{
   int flags = RTLD_GLOBAL;
   #if defined(HXCPP_RTLD_LAZY) || defined(IPHONE) || defined(EMSCRIPTEN) || defined(STATIC_LINK) || defined(APPLETV)
   flags |= RTLD_LAZY;
   #else
   flags |= RTLD_NOW;
   #endif
   
   Module result = dlopen(inLib.__CStr(), flags);
   if (gLoadDebug)
   {
#ifdef HX_WINRT
      if (result)
         WINRT_LOG("Loaded : %s.\n", inLib.__CStr());
      else
         WINRT_LOG("Error loading library: (%s) %s\n", inLib.__CStr(), dlerror());
#else
      if (result)
         printf("Loaded : %s.\n", inLib.__CStr());
      else
         printf("Error loading library: (%s) %s\n", inLib.__CStr(), dlerror());
#endif
   }
   return result;
}
void *hxFindSymbol(Module inModule, const char *inSymbol) { return dlsym(inModule,inSymbol); }

void hxFreeLibrary(Module inModule) { dlclose(inModule); }

#endif

#ifdef HX_UTF8_STRINGS
typedef std::map<std::string,Module> LoadedModule;
#else
typedef std::map<std::wstring,Module> LoadedModule;
#endif

static LoadedModule sgLoadedModule;
typedef std::vector<Module> ModuleList;
static ModuleList sgOrderedModules;

typedef hx::Object * (*prim_0)();
typedef hx::Object * (*prim_1)(hx::Object *);
typedef hx::Object * (*prim_2)(hx::Object *,hx::Object *);
typedef hx::Object * (*prim_3)(hx::Object *,hx::Object *,hx::Object *);
typedef hx::Object * (*prim_4)(hx::Object *,hx::Object *,hx::Object *,hx::Object *);
typedef hx::Object * (*prim_5)(hx::Object *,hx::Object *,hx::Object *,hx::Object *,hx::Object *);
typedef hx::Object * (*prim_mult)(hx::Object **inArray,int inArgs);

typedef void *(*FundFunc)(); 

 extern const char* EXTERN_CLASS_NAME;

class ExternalPrimitive : public hx::Object
{
public:
   ExternalPrimitive(void *inProc,int inArgCount,const String &inName) :
       mProc(inProc), mArgCount(inArgCount), mName(inName)
   {
     functionName = ("extern::cffi "+mName).dupConst().__CStr();
   }

   virtual int __GetType() const { return vtFunction; }
   String __ToString() const { return mName; }

   Dynamic __run()
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0, functionName, __FILE__, __LINE__,0);
      if (mArgCount!=0) throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_0)mProc)();
   }
   Dynamic __run(D a)
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0,  functionName, __FILE__, __LINE__,0);
      if (mArgCount!=1) throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_1)mProc)(a.GetPtr());
   }
   Dynamic __run(D a,D b)
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0,  functionName, __FILE__, __LINE__,0);
      if (mArgCount!=2) throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_2)mProc)(a.GetPtr(),b.GetPtr());
   }
   Dynamic __run(D a,D b,D c)
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0,  functionName, __FILE__, __LINE__,0);
      if (mArgCount!=3) throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_3)mProc)(a.GetPtr(),b.GetPtr(),c.GetPtr());
   }
   Dynamic __run(D a,D b,D c,D d)
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0,  functionName, __FILE__, __LINE__,0);
      if (mArgCount!=4) throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_4)mProc)(a.GetPtr(),b.GetPtr(),c.GetPtr(),d.GetPtr());
   }
   Dynamic __run(D a,D b,D c,D d,D e)
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0,  functionName, __FILE__, __LINE__,0);
      if (mArgCount!=5) throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_5)mProc)(a.GetPtr(),b.GetPtr(),c.GetPtr(),d.GetPtr(),e.GetPtr());
   }

   Dynamic __Run(const Array<Dynamic> &inArgs)
   {
      HX_STACK_FRAME(EXTERN_CLASS_NAME, "cffi",0,  functionName, __FILE__, __LINE__,0);
      if (mArgCount!=-1 && mArgCount!=inArgs->length)
         throw HX_INVALID_ARG_COUNT;
      if (mProc==0) hx::Throw( HX_NULL_FUNCTION_POINTER );
      return ((prim_mult)mProc)( (hx::Object **)inArgs->GetBase(), inArgs->length );
   }

   void __Mark(hx::MarkContext *__inCtx) {  HX_MARK_MEMBER(mName); }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) {  HX_VISIT_MEMBER(mName); }
   #endif

   int __Compare(const hx::Object *inRHS) const
   {
      const ExternalPrimitive *other = dynamic_cast<const ExternalPrimitive *>(inRHS);
      if (!other)
         return -1;
      return mProc==other->mProc;
   }


   void        *mProc;
   int         mArgCount;
   String      mName;
   const char* functionName;
};


typedef void (*SetLoaderProcFunc)(void *(*)(const char *));
typedef void *(*GetNekoEntryFunc)();
typedef void (*NekoEntryFunc)();

String GetFileContents(String inFile)
{
#ifndef _WIN32
   FILE *file = fopen(inFile.__CStr(),"rb");
#else
   FILE *file = _wfopen(inFile.__WCStr(),L"rb");
#endif
   if (!file)
   {
      // printf("Could not open %S\n", inFile.__s);
      return null();
   }

   char buf[2049];
   int bytes = fread(buf,1,2048,file);
   fclose(file);
   if (bytes<1)
      return null();
   buf[bytes]='\0';
   return String(buf,strlen(buf)).dup();
}

#ifndef HX_WINRT
String GetEnv(const char *inPath)
{
   const char *env  = getenv(inPath);
   String result(env,env?strlen(env):0);
   return result;
}

String FindHaxelib(String inLib)
{
   bool loadDebug = getenv("HXCPP_LOAD_DEBUG");

   // printf("FindHaxelib %S\n", inLib.__s);

   String haxepath;

   #if !defined(HX_WINRT) && !defined(EPPC)
      struct stat s;
      if ( (stat(".haxelib",&s)==0 && (s.st_mode & S_IFDIR) ) )
         haxepath = HX_CSTRING(".haxelib");
      if (loadDebug)
          printf( haxepath.length ? "Found local .haxelib\n" : "No local .haxelib\n");
   #endif

   if (haxepath.length==0)
   {
      haxepath = GetEnv("HAXELIB_PATH");
      if (loadDebug)
         printf("HAXELIB_PATH env:%s\n", haxepath.__s);
   }

   if (haxepath.length==0)
   {
       #ifdef _WIN32
       String home = GetEnv("HOMEDRIVE") + GetEnv("HOMEPATH") + HX_CSTRING("/.haxelib");
       #else
       String home = GetEnv("HOME") + HX_CSTRING("/.haxelib");
       #endif
       haxepath = GetFileContents(home);
       if (loadDebug)
          printf("HAXEPATH home:%s\n", haxepath.__s);
   }

   if (haxepath.length==0)
   {
      haxepath = GetEnv("HAXEPATH");
      if (loadDebug)
         printf("HAXEPATH env:%s\n", haxepath.__s);
      if (haxepath.length>0)
      {
         haxepath += HX_CSTRING("/lib");
      }
   }

   if (loadDebug)
      printf("HAXEPATH dir:%s\n", haxepath.__s);

   if (haxepath.length==0)
   {
       haxepath = GetFileContents(HX_CSTRING("/etc/.haxepath"));
       if (loadDebug) printf("HAXEPATH etc:%s\n", haxepath.__s);
   }

   if (haxepath.length==0)
   {
      #ifdef _WIN32
      haxepath = HX_CSTRING("C:\\HaxeToolkit\\haxe\\lib");
      #else
      haxepath = HX_CSTRING("/usr/lib/haxe/lib");
      #endif
       if (loadDebug) printf("HAXEPATH default:%s\n", haxepath.__s);
   }

   String dir = haxepath + HX_CSTRING("/") + inLib + HX_CSTRING("/");


   String dev = dir + HX_CSTRING(".dev");
   String path = GetFileContents(dev);
   if (loadDebug) printf("Read dev location from file :%s, got %s\n", dev.__s, path.__s);
   if (path.length==0)
   {
      path = GetFileContents(dir + HX_CSTRING(".current"));
      if (path.length==0)
         return null();
      // Replace "." with "," ...
      String with_commas;
      for(int i=0;i<path.length;i++)
         if (path.getChar(i)=='.')
            with_commas += HX_CSTRING(",");
         else
            with_commas += path.substr(i,1);

      path = dir + with_commas + HX_CSTRING("/");
   }

   return path;
}

#endif

typedef std::map<std::string,void *> RegistrationMap;
RegistrationMap *sgRegisteredPrims=0;



static std::vector<std::string> sgLibPath;
static bool sgLibPathIsInit = false;

String __hxcpp_get_bin_dir()
{
   return
#if defined(HX_WINRT)
  #ifdef HXCPP_M64
    HX_CSTRING("WinRT64");
  #else
    HX_CSTRING("WinRT");
  #endif
#elif defined(_WIN32)
  #ifdef HXCPP_M64
    HX_CSTRING("Windows64");
  #else
    HX_CSTRING("Windows");
  #endif
// Unix...
#elif defined(__APPLE__)
  #ifdef HXCPP_M64
    HX_CSTRING("Mac64");
  #else
    HX_CSTRING("Mac");
  #endif
#elif defined (ANDROID)
    HX_CSTRING("Android");
#elif defined(WEBOS)
    HX_CSTRING("webOS");
#elif defined(BLACKBERRY)
    HX_CSTRING("BlackBerry");
#elif defined(RASPBERRYPI)
    HX_CSTRING("RPi");
#elif defined(EMSCRIPTEN)
	HX_CSTRING("Emscripten");
#elif defined(TIZEN)
    HX_CSTRING("Tizen");
#elif defined(IPHONESIM)
    HX_CSTRING("IPhoneSim");
#elif defined(IPHONEOS)
    HX_CSTRING("IPhoneOs");
#elif defined(APPLETVSIM)
    HX_CSTRING("AppleTVSim");
#elif defined(APPLETVOS)
    HX_CSTRING("AppleTVOS");
#else
  #ifdef HXCPP_M64
    HX_CSTRING("Linux64");
  #else
    HX_CSTRING("Linux");
  #endif
#endif
}

String __hxcpp_get_dll_extension()
{
   return
#if defined(_WIN32) || defined(HX_WINRT)
    HX_CSTRING(".dll");
#elif defined(IPHONEOS)
    HX_CSTRING(".ios.dylib");
#elif defined(IPHONESIM)
    HX_CSTRING(".sim.dylib");
#elif defined(APPLETVOS)
    HX_CSTRING(".tvos.dylib");
#elif defined(APPLETVSIM)
    HX_CSTRING(".sim.dylib");
#elif defined(__APPLE__)
    HX_CSTRING(".dylib");
#elif defined(ANDROID) || defined(GPH) || defined(WEBOS)  || defined(BLACKBERRY) || defined(EMSCRIPTEN) || defined(TIZEN)
    HX_CSTRING(".so");
#else
    HX_CSTRING(".dso");
#endif
}

void __hxcpp_push_dll_path(String inPath)
{
   int last = inPath.length-1;
   if (last>=0 && inPath.__s[last]!='\\' && inPath.__s[last]!='/')
      sgLibPath.push_back( (inPath + HX_CSTRING("/")).__s );
   else
      sgLibPath.push_back( inPath.__s );
}








#if (defined(IPHONE) || defined(EMSCRIPTEN) || defined(STATIC_LINK) || defined(APPLETV) ) && !defined(HXCPP_DLL_IMPORT) && (!defined(HXCPP_DLL_EXPORT) || defined(HXCPP_SCRIPTABLE) )

Dynamic __loadprim(String inLib, String inPrim,int inArgCount)
{
   String full_name = inPrim;
   switch(inArgCount)
   {
      case 0: full_name += HX_CSTRING("__0"); break;
      case 1: full_name += HX_CSTRING("__1"); break;
      case 2: full_name += HX_CSTRING("__2"); break;
      case 3: full_name += HX_CSTRING("__3"); break;
      case 4: full_name += HX_CSTRING("__4"); break;
      case 5: full_name += HX_CSTRING("__5"); break;
      default:
          full_name += HX_CSTRING("__MULT");
   }


   if (sgRegisteredPrims)
   {
      void *registered = (*sgRegisteredPrims)[full_name.__CStr()];
      // Try with lib name ...
      if (!registered)
      {
         String libString = inLib + HX_CSTRING("_") + full_name;
         registered = (*sgRegisteredPrims)[libString.__CStr()];
      }

      if (registered)
      {
         return Dynamic( new ExternalPrimitive(registered,inArgCount,HX_CSTRING("registered@")+full_name) );
      }
   }

   printf("Primitive not found : %s\n", full_name.__CStr() );
   return null();
}

void *__hxcpp_get_proc_address(String inLib, String inPrim,bool ,bool inQuietFail)
{
   if (sgRegisteredPrims)
      return (*sgRegisteredPrims)[inPrim.__CStr()];

   if (!inQuietFail)
      printf("Primitive not found : %s\n", inPrim.__CStr() );
   return 0;
}

int __hxcpp_unload_all_libraries() { return 0; }


#else


extern "C" void *hx_cffi(const char *inName);


#if !defined(ANDROID) && !defined(HX_WINRT) && !defined(IPHONE) && !defined(EMSCRIPTEN) && !defined(STATIC_LINK) && !defined(APPLETV)
    #define HXCPP_TRY_HAXELIB
#endif



void *__hxcpp_get_proc_address(String inLib, String full_name,bool inNdllProc,bool inQuietFail)
{
   String bin = __hxcpp_get_bin_dir();
   String deviceExt = __hxcpp_get_dll_extension();


   #ifdef HX_ANDROID
   String module_name = HX_CSTRING("lib") + inLib;
   #else
   String module_name = inLib;
   #endif

   #if defined(HX_WINRT) && defined(HXCPP_DEBUG_LINK)
   gLoadDebug = true;   
   #elif defined(IPHONE) || defined(APPLETV)
   gLoadDebug = true;
   setenv("DYLD_PRINT_APIS","1",true);

   #elif !defined(HX_WINRT)
   gLoadDebug = gLoadDebug || getenv("HXCPP_LOAD_DEBUG");
   #endif

   if (!sgLibPathIsInit)
   {
      sgLibPathIsInit = true;
      #ifndef HX_WINRT 
      sgLibPath.push_back("./");
	  #endif
      #ifdef HX_MACOS
      sgLibPath.push_back("@executable_path/");
      #endif
      sgLibPath.push_back("");

      #ifdef HXCPP_TRY_HAXELIB
      String hxcpp = GetEnv("HXCPP");
      if (hxcpp.length==0)
         hxcpp = FindHaxelib( HX_CSTRING("hxcpp") );
      if (hxcpp.length!=0)
         __hxcpp_push_dll_path(hxcpp+HX_CSTRING("/bin/") + bin + HX_CSTRING("/"));
      #endif
   }


   Module module = sgLoadedModule[module_name.__s];

   bool new_module = module==0;

   if (!module && sgRegisteredPrims)
   {
      void *registered = (*sgRegisteredPrims)[full_name.__CStr()];
      // Try with lib name ...
      if (!registered)
      {
         String libString = inLib + HX_CSTRING("_") + full_name;
         registered = (*sgRegisteredPrims)[libString.__CStr()];
      }

      if (registered)
         return registered;
   }

   if (!module && gLoadDebug)
   {
      #ifdef HX_WINRT
      WINRT_LOG("Searching for %s...\n", inLib.__s);
      #elif defined(ANDROID)
       __android_log_print(ANDROID_LOG_INFO, "loader", "Searching for %s...", module_name.__s);
      #else
      printf("Searching for %s...\n", inLib.__s);
      #endif
   }

   String haxelibPath;

#ifdef HX_WINRT
   for (int e = 0; module == 0 && e<1; e++) //only accept DLL
#else
   for(int e=0; module==0 && e<3; e++)
#endif
   {
      String extension = e==0 ? deviceExt : e==1 ? HX_CSTRING(".ndll") : HX_CSTRING("");

      for(int path=0;path<sgLibPath.size();path++)
      {
#if defined(HX_WINRT)
         if (gLoadDebug)
		 {
            WINRT_LOG(" module_name: [%s]  extension: [%s]\n", module_name.__s, extension.__s);
		 }
		 String testPath = module_name + extension;
#else
         String testPath = String( sgLibPath[path].c_str() ) +  module_name + extension;
#endif
         if (gLoadDebug)
         {
            #ifdef HX_WINRT
            WINRT_LOG(" try %s...\n", testPath.__s);

            #elif !defined(ANDROID)
            printf(" try %s...\n", testPath.__s);
            #else
            __android_log_print(ANDROID_LOG_INFO, "loader", "Try %s", testPath.__s);
            #endif
         }
         module = hxLoadLibrary(testPath);
         if (module)
         {
            if (gLoadDebug)
            {
               #ifdef HX_WINRT
               WINRT_LOG("Found %s\n", testPath.__s);
               #elif !defined(ANDROID)
               printf("Found %s\n", testPath.__s);
               #else
               __android_log_print(ANDROID_LOG_INFO, "loader", "Found %s", testPath.__s);
               #endif
            }
            break;
         }
      }

      #ifdef HXCPP_TRY_HAXELIB
      if (!module)
      {
         if (e==0)
            haxelibPath = FindHaxelib(inLib);

         if (haxelibPath.length!=0)
         {
            String testPath  = haxelibPath + HX_CSTRING("/ndll/") + bin + HX_CSTRING("/") + inLib + extension;
            if (gLoadDebug)
               printf(" try %s...\n", testPath.__s);
            module = hxLoadLibrary(testPath);
            if (module && gLoadDebug)
            {
               printf("Found %s\n", testPath.__s);
            }
         }
      }
      #endif
   }

   if (!module)
   {
      hx::Throw(HX_CSTRING("Could not load module ") + inLib + HX_CSTRING("@") + full_name);
   }


   if (new_module)
   {
      sgLoadedModule[module_name.__s] = module;

      sgOrderedModules.push_back(module);

      SetLoaderProcFunc set_loader = (SetLoaderProcFunc)hxFindSymbol(module,"hx_set_loader");
      if (set_loader)
         set_loader(hx_cffi);

      GetNekoEntryFunc func = (GetNekoEntryFunc)hxFindSymbol(module,"__neko_entry_point");
      if (func)
      {
         NekoEntryFunc entry = (NekoEntryFunc)func();
         if (entry)
            entry();
      }
   }

   FundFunc proc_query = (FundFunc)hxFindSymbol(module,full_name.__CStr());
   if (!proc_query)
       proc_query = (FundFunc)hxFindSymbol(module, (inLib + HX_CSTRING("_") + full_name).__CStr());

   if (!proc_query && !inQuietFail)
   {
      #ifdef ANDROID
       __android_log_print(ANDROID_LOG_ERROR, "loader", "Could not find primitive %s in %p",
        full_name.__CStr(), module);
      #else
      fprintf(stderr,"Could not find primitive %s.\n", full_name.__CStr());
      #endif
      return 0;
   }

   if (!inNdllProc)
      return (void *)proc_query;

   void *proc = proc_query();
   if (!proc && !inQuietFail)
   {
      #ifdef ANDROID
      __android_log_print(ANDROID_LOG_ERROR, "loader", "Could not identify primitive %s in %s",
        full_name.__CStr(), inLib.__CStr() );
      #else
      fprintf(stderr,"Could not identify primitive %s in %s\n", full_name.__CStr(),inLib.__CStr());
      #endif
   }

   return proc;
}

int __hxcpp_unload_all_libraries()
{
   int unloaded = 0;
   while(sgOrderedModules.size())
   {
      Module module = sgOrderedModules.back();
      sgOrderedModules.pop_back();
      hxFreeLibrary(module);
      unloaded++;
   }
   return unloaded;
}


Dynamic __loadprim(String inLib, String inPrim,int inArgCount)
{
   String full_name = inPrim;
   switch(inArgCount)
   {
      case 0: full_name += HX_CSTRING("__0"); break;
      case 1: full_name += HX_CSTRING("__1"); break;
      case 2: full_name += HX_CSTRING("__2"); break;
      case 3: full_name += HX_CSTRING("__3"); break;
      case 4: full_name += HX_CSTRING("__4"); break;
      case 5: full_name += HX_CSTRING("__5"); break;
      default:
          full_name += HX_CSTRING("__MULT");
   }
   void *proc = __hxcpp_get_proc_address(inLib,full_name,true);

   if (proc)
   {
      // Split this to avoid a collect after ExternalPrimitive::new, but before its
      //  inline constructor is run...
      String primName = inLib+HX_CSTRING("@")+full_name;
      return Dynamic( new ExternalPrimitive(proc,inArgCount,primName) );
   }
   return null();
}


#endif // not IPHONE

void __hxcpp_run_dll(String inLib, String inFunc)
{
   typedef void (*VoidVoid)();

   void *result = __hxcpp_get_proc_address(inLib,inFunc,false);
   if (result)
      ((VoidVoid)result)();
}

// This can be used to find symbols in static libraries

int __hxcpp_register_prim(const char *inName,void *inProc)
{
   if (sgRegisteredPrims==0)
      sgRegisteredPrims = new RegistrationMap();
   void * &proc = (*sgRegisteredPrims)[inName];
   if (proc)
   {
      printf("Warning : duplicate symbol %s\n", inName);
   }
   proc = inProc;
   return 0;
}

