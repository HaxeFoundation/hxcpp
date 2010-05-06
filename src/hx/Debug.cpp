#include <hxcpp.h>

#ifdef HXCPP_DEBUG

#ifdef HX_WINDOWS
#include <windows.h>
#endif


namespace hx
{

void CriticalError(const wchar_t *inErr)
{
   fprintf(stderr,"Critical Error: %S\n", inErr);

   #ifdef HX_WINDOWS
   MessageBoxW(0,inErr,L"Critial Error - program must terminate",MB_ICONEXCLAMATION|MB_OK);
   exit(1);
   #endif
   *(int *)0=0;
}

}


#endif
