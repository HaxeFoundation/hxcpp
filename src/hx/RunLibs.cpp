/*
 Use the "hxRunLibrary" code to run the static version of the code
*/
#include <hxcpp.h>
#include <string>


extern "C"
{
void __hxcpp_lib_main();
int std_register_prims();
int regexp_register_prims();
int zlib_register_prims();



std::wstring sgResultBuffer;

const wchar_t *hxRunLibrary()
{
   std_register_prims();
   regexp_register_prims();
   zlib_register_prims();
    
   try { 
      __hxcpp_lib_main();
      return 0;
   }
   catch ( Dynamic d ) {
      sgResultBuffer = d->toString().__s;
      return sgResultBuffer.c_str();
   }
}



}
