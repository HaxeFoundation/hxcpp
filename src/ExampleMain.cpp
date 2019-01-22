/*
 This is an example mainline that can be used to link a static version.
 First you need to build the static version of the standard libs, with:
 cd $HXCPP/project
 neko build.n clean static-windows

 Then the static verion of your application with (note: extra space before 'static_link'):

 haxe -main YourMain -cpp cpp -D static_link

 You then need to link the above libraries with this (or a modified version) main.
 You may choose to create a VisualStudio project, and add the libraries from
  $HXCPP/bin/Windows/(std,regexp,zlib).lib and your application library.
 
  Note also, that if you compile with the -debug flag, your library will have a different name.

  Linking from the command line for windows (developer command prompt for vs; user32.lib only required for debug version;
   ws2_32.lib might be necessary if there are error messages):

  cl ExampleMain.cpp cpp/YourMain.lib $HXCPP/bin/Windows/std.lib $HXCPP/bin/Windows/zlib.lib  $HXCPP/bin/Windows/regexp.lib user32.lib

  From other OSs, the compile+link command will be different.  Here is one for mac:

   g++ ExampleMain.cpp cpp/Test-debug.a $HXCPP/bin/Mac/regexp.a $HXCPP/bin/Mac/std.a $HXCPP/bin/Mac/zlib.a

  If you wish to add other static libraries besides these 3 (eg, nme) you will
   need to compile these with the "-Dstatic_link" flag too, and call their "register_prims"
   init call.  The inclusion of the extra static library will require the library
   in the link line, and may requires additional dependencies to be linked.
   Also note, that there may be licensing implications with static linking
   thirdparty libraries.

*/

#include <stdio.h>

extern "C" const char *hxRunLibrary();
extern "C" void hxcpp_set_top_of_stack();
	

// Declare additional library entry points...
//extern "C" int nme_register_prims();

extern "C" int main(int argc, char *argv[])	
{
	// Do this first
	hxcpp_set_top_of_stack();

   // Register additional ndll libaries ...
   // nme_register_prims();

	//printf("Begin!\n");
 	const char *err = hxRunLibrary();
	if (err) {
		// Unhandled exceptions ...
		fprintf(stderr,"Error %s\n", err );
		return -1;
	}
	//printf("Done!\n");
	return 0;
}
