
// This is included in the main routine to boot the hxcpp library using
// explicit methods, rather than implicit, so we can have more control
// over the path.  (ie, use $HXCPP)
#include <windows.h>

void hxLoadDLL()
{
   const char *env = getenv("HXCPP");
   if (env)
   {
      int len = strlen(env);
      char *full_path = new char[len+20];
      strcpy(full_path,env);
      strcat(full_path,"/bin/hxcpp.dll");
      HMODULE mod = LoadLibraryA(full_path);
      //printf("Mod : %p\n",mod);
      delete [] full_path;
   }
}

