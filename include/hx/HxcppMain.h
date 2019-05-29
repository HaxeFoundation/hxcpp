#ifdef HXCPP_DLL_IMPORT

   extern "C" EXPORT_EXTRA void __main__()
   {
     __boot_all();
     __hxcpp_main();
   }

#elif defined(HX_ANDROID) && !defined(HXCPP_EXE_LINK)

   // Java Main....
   #include <jni.h>
   #include <hx/Thread.h>
   #include <android/log.h>

   extern "C" EXPORT_EXTRA void hxcpp_main()
   {
      HX_TOP_OF_STACK
      try
      {
         hx::Boot();
         __boot_all();
         __hxcpp_main();
      }
      catch (Dynamic e)
      {
         __hx_dump_stack();
         __android_log_print(ANDROID_LOG_ERROR, "Exception", "%s", e==null() ? "null" : e->toString().__CStr());
      }
      hx::SetTopOfStack((int *)0,true);
   }

   extern "C" EXPORT_EXTRA JNIEXPORT void JNICALL Java_org_haxe_HXCPP_main(JNIEnv * env)
   {
      hxcpp_main();
   }

#elif defined(HX_WINRT) && defined(__cplusplus_winrt)

   #include <Roapi.h>
   [ Platform::MTAThread ]
   int main(Platform::Array<Platform::String^>^)
   {
      HX_TOP_OF_STACK
      RoInitialize(RO_INIT_MULTITHREADED);
      hx::Boot();
      try
      {
         __boot_all();
         __hxcpp_main();
      }
      catch (Dynamic e)
      {
         __hx_dump_stack();
         return -1;
      }
      return 0;
   }

#else

   #if defined(HX_WIN_MAIN) && !defined(_WINDOWS_)
   #ifndef HINSTANCE
   #define HINSTANCE void*
   #endif
   #ifndef LPSTR
   #define LPSTR char*
   #endif
   extern "C" int __stdcall MessageBoxA(void *,const char *,const char *,int);
   #endif


   #if defined(TIZEN)
   extern "C" EXPORT_EXTRA int OspMain (int argc, char* pArgv[])
   {
   #elif defined(HX_WIN_MAIN)
   int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
   {
   #else

   extern int _hxcpp_argc;
   extern char **_hxcpp_argv;
   int main(int argc,char **argv)
   {
      _hxcpp_argc = argc;
      _hxcpp_argv = argv;
   #endif
      HX_TOP_OF_STACK
      hx::Boot();
      try
      {
         __boot_all();
         __hxcpp_main();
      }
      catch (Dynamic e)
      {
         __hx_dump_stack();
         #ifdef HX_WIN_MAIN
         MessageBoxA(0,  e==null() ? "null" : e->toString().__CStr(), "Error", 0);
         #else
         printf("Error : %s\n",e==null() ? "null" : e->toString().__CStr());
         #endif
         return -1;
      }
      return 0;
   }
   #if 0
   }
   }
   #endif

#endif


