#include <hxObject.h>
#include <string>
#include <neko.h>
#include <map>
#include <vector>

#ifdef _WIN32

#include <windows.h>
typedef HMODULE Module;
Module hxLoadLibrary(const wchar_t *inLib) { return LoadLibraryW(inLib); }
void *hxFindSymbol(Module inModule, const char *inSymbol) { return GetProcAddress(inModule,inSymbol); }
#else
#include <dlfcn.h>
typedef void *Module;
Module hxLoadLibrary(const wchar_t *inLib)
{
   char utf8_str[2048];
   wcstombs(utf8_str,inLib,2048);
   utf8_str[2047]='\0';
   return dlopen(utf8_str,RTLD_NOW);
}
void *hxFindSymbol(Module inModule, const char *inSymbol) { return dlsym(inModule,inSymbol); }
#endif

typedef std::map<std::wstring,Module> LoadedModule;

static LoadedModule sgLoadedModule;

typedef Dynamic (*prim_0)();
typedef Dynamic (*prim_1)(Dynamic);
typedef Dynamic (*prim_2)(Dynamic,Dynamic);
typedef Dynamic (*prim_3)(Dynamic,Dynamic,Dynamic);
typedef Dynamic (*prim_4)(Dynamic,Dynamic,Dynamic,Dynamic);
typedef Dynamic (*prim_5)(Dynamic,Dynamic,Dynamic,Dynamic,Dynamic);
typedef Dynamic (*prim_mult)(Dynamic *inArray,int inArgs);

typedef void *(*func)(); 

static Dynamic sgInvalidArgCount(STRING(L"Invalid arguement count",23));

class ExternalPrimitive : public hxObject
{
public:
   ExternalPrimitive(void *inProc,int inArgCount,const String &inName) :
       mProc(inProc), mArgCount(inArgCount), mName(inName)
   {
   }

   virtual int __GetType() const { return vtFunction; }
   String __ToString() const { return mName; }

   Dynamic __run()
   {
      if (mArgCount!=0) throw sgInvalidArgCount;
      return ((prim_0)mProc)();
   }
   Dynamic __run(D a)
   {
      if (mArgCount!=1) throw sgInvalidArgCount;
      return ((prim_1)mProc)(a);
   }
   Dynamic __run(D a,D b)
   {
      if (mArgCount!=2) throw sgInvalidArgCount;
      return ((prim_2)mProc)(a,b);
   }
   Dynamic __run(D a,D b,D c)
   {
      if (mArgCount!=3) throw sgInvalidArgCount;
      return ((prim_3)mProc)(a,b,c);
   }
   Dynamic __run(D a,D b,D c,D d)
   {
      if (mArgCount!=4) throw sgInvalidArgCount;
      return ((prim_4)mProc)(a,b,c,d);
   }
   Dynamic __run(D a,D b,D c,D d,D e)
   {
      if (mArgCount!=5) throw sgInvalidArgCount;
      return ((prim_5)mProc)(a,b,c,d,e);
   }
   Dynamic __run(D a,D b,D c,D d,D e,D f)
   {
      return ((prim_mult)mProc)( Array_obj<Dynamic>::__new(0,6)
           .Add(a).Add(b).Add(c).Add(d).Add(e).Add(f)->Pointer(),6);
   }
   Dynamic __run(D a,D b,D c,D d,D e,D f,D g)
   {
      return ((prim_mult)mProc)( Array_obj<Dynamic>::__new(0,7)
           .Add(a).Add(b).Add(c).Add(d).Add(e).Add(f).Add(g)->Pointer(),7);
   }
   Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h)
   {
      return ((prim_mult)mProc)( Array_obj<Dynamic>::__new(0,8)
           .Add(a).Add(b).Add(c).Add(d).Add(e).Add(f).Add(g).Add(h)->Pointer(),8);
   }
   Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h,D i)
   {
      return ((prim_mult)mProc)( Array_obj<Dynamic>::__new(0,9)
           .Add(a).Add(b).Add(c).Add(d).Add(e).Add(f).Add(g).Add(h).Add(i)->Pointer(),9);
   }
   Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j)
   {
      return ((prim_mult)mProc)( Array_obj<Dynamic>::__new(0,10)
           .Add(a).Add(b).Add(c).Add(d).Add(e).Add(f).Add(g).Add(h).Add(i).Add(j)->Pointer(),10);
   }
   Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j,D k)
   {
      return ((prim_mult)mProc)( Array_obj<Dynamic>::__new(0,11)
           .Add(a).Add(b).Add(c).Add(d).Add(e).Add(f).Add(g).Add(h).Add(i).Add(j).Add(k)->Pointer(),11);
   }

   void __Mark() {  MarkMember(mName); }


   void        *mProc;
   int         mArgCount;
   String      mName;
};


typedef void *(*GetNekoEntryFunc)();
typedef void (*NekoEntryFunc)();

String GetFileContents(String inFile)
{
#ifndef _WIN32
   char utf8_str[2048];
   wcstombs(utf8_str,inFile.__s,2048);
   utf8_str[2047]='\0';
   FILE *file = fopen(utf8_str,"rb");
#else
   FILE *file = _wfopen(inFile.__s,L"rb");
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
   return String(buf,-1);
}

String GetEnv(const char *inPath)
{
   String result(getenv(inPath),-1);
   //printf("Got env %S\n", result.__s );
   return result;
}

String FindHaxelib(String inLib)
{
   //printf("FindHaxelib %S\n", inLib.__s);
   String haxepath = GetEnv("HAXEPATH");
   if (haxepath.length==0)
   {
       String home = GetEnv("HOME") + L"/.haxelib";
       haxepath = GetFileContents(home);
   }
   else
      haxepath += L"/lib";

   if (haxepath.length==0)
   {
       haxepath = GetFileContents(String("/etc/.haxepath",-1));
   }

   if (haxepath.length==0)
      #ifdef _WIN32
      haxepath = L"C:\\Program Files\\Motion-Twin\\haxe\\lib";
      #else
      haxepath = L"/usr/lib/haxe/lib";
      #endif

   String dir = haxepath + L"/" + inLib + L"/";


   String dev = dir + L".dev";
   String path = GetFileContents(dev);
   if (path.length==0)
   {
      path = GetFileContents(dir + L".current");
      if (path.length==0)
         return null();
      path = dir + path + L"/";
   }

   return path;
}

typedef std::map<std::wstring,void *> RegistrationMap;
RegistrationMap *sgRegisteredPrims=0;


Dynamic __loadprim(String inLib, String inPrim,int inArgCount)
{
   bool debug = getenv("HXCPP_LOAD_DEBUG");
   String ext =
#ifdef _WIN32
    String(L".dll",4);
#else
// Unix...
#ifdef __APPLE__
    String(L".dylib",6);
#else
    String(L".dso",4);
#endif
#endif
   String bin =
#ifdef _WIN32
    String(L"Windows",7);
#else
// Unix...
#ifdef __APPLE__
    String(L"Mac",3);
#else
    String(L"Linux",5);
#endif
#endif


   String full_name = inPrim;
   switch(inArgCount)
   {
      case 0: full_name += L"__0"; break;
      case 1: full_name += L"__1"; break;
      case 2: full_name += L"__2"; break;
      case 3: full_name += L"__3"; break;
      case 4: full_name += L"__4"; break;
      case 5: full_name += L"__5"; break;
      default:
          full_name += L"__MULT";
   }

   String dll_ext = inLib + ext;

   Module module = sgLoadedModule[inLib.__s];
   if (!module)
   {
      if (debug)
         printf("Searching for %S...\n", dll_ext.__s);
      module = hxLoadLibrary(dll_ext.__s);
      if (!module)
      {
         String hxcpp = GetEnv("HXCPP");
         if (hxcpp.length!=0)
         {
             String name = hxcpp + L"/bin/" + bin + L"/" + dll_ext;
             if (debug)
                printf(" try %S...\n", name.__s);
             module = hxLoadLibrary(name.__s);
         }
      }
   
      if (!module)
      {
         String hxcpp = FindHaxelib(L"hxcpp");
         if (hxcpp.length!=0)
         {
             String name = hxcpp + L"/bin/" + bin + L"/" + dll_ext;
             if (debug)
                printf(" try %S...\n", name.__s);
             module = hxLoadLibrary(name.__s);
         }
      }

      if (!module)
      {
         String path = FindHaxelib(inLib);
         if (path.length!=0)
         {
            String full_path  = path + L"/ndll/" + bin + L"/" + dll_ext;
            if (debug)
               printf(" try %S...\n", full_path.__s);
            module = hxLoadLibrary(full_path.__s);
         }
      }

      if (!module && sgRegisteredPrims)
      {
         void *registered = (*sgRegisteredPrims)[full_name.__s];
         if (registered)
         {
            return Dynamic( new ExternalPrimitive(registered,inArgCount,L"registered@"+full_name) );
         }
      }

      if (!module)
        throw Dynamic(String(L"Could not load module ") + dll_ext + String(L"@") + full_name);


      sgLoadedModule[inLib.__s] = module;

      GetNekoEntryFunc func = (GetNekoEntryFunc)hxFindSymbol(module,"__neko_entry_point");
      if (func)
      {
         NekoEntryFunc entry = (NekoEntryFunc)func();
         if (entry)
            entry();
      }
   }
   // No "wchar_t" version
   std::vector<char> name(full_name.length+1);
   for(int i=0;i<full_name.length;i++)
      name[i] = full_name.__s[i];
   name[full_name.length] = '\0';

   func proc_query = (func)hxFindSymbol(module,&name[0]);
   if (!proc_query)
   {
      fwprintf(stderr,L"Could not find primitive %s in %s\n", full_name.__s,inLib.__s);
      return 0;
   }

   void *proc = proc_query();
   if (!proc)
   {
      fwprintf(stderr,L"Could not identify primitive %s in %s\n", full_name.__s,inLib.__s);
      return 0;
   }


   return Dynamic( new ExternalPrimitive(proc,inArgCount,inLib+L"@"+full_name) );

   return 0;
}

// This can be used to find symbols in static libraries

hxPrimRegisterer::hxPrimRegisterer(wchar_t *inName,void *inProc)
{
   if (sgRegisteredPrims==0)
      sgRegisteredPrims = new RegistrationMap();
   (*sgRegisteredPrims)[inName] = inProc;
}

void hxLibMark()
{
	MarkMember(sgInvalidArgCount);
}
