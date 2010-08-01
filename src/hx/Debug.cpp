#include <hxcpp.h>

#ifdef HXCPP_DEBUG

#ifdef HX_WINDOWS
#include <windows.h>
#endif


namespace hx
{

void CriticalError(const String &inErr)
{
   #ifdef HX_UTF8_STRINGS
   fprintf(stderr,"Critical Error: %s\n", inErr.__s);
   #else
   fprintf(stderr,"Critical Error: %S\n", inErr.__s);
   #endif

   #ifdef HX_WINDOWS
      #ifdef HX_UTF8_STRINGS
      MessageBoxA(0,inErr.__s,"Critial Error - program must terminate",MB_ICONEXCLAMATION|MB_OK);
      #else
      MessageBoxW(0,inErr.__s,L"Critial Error - program must terminate",MB_ICONEXCLAMATION|MB_OK);
      #endif
   exit(1);
   #endif
   *(int *)0=0;
}

}


#endif
