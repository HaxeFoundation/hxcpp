/*
 Use the "hxRunLibrary" code to run the static version of the code
*/
#include <hxcpp.h>
#include <string>


extern "C"
{
void __hxcpp_lib_main();


std::string sgResultBuffer;

HXCPP_EXTERN_CLASS_ATTRIBUTES const HX_CHAR *hxRunLibrary()
{
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
