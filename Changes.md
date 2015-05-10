
Haxe 3.2.0
------------------------------------------------------------

* Improve testing
* Allow dll_load path to be set programatically and simplified the dll search sequence.
* Improved cffi_prime, and added String class
* Fixed static linking of mysql5
* Moved static link code in general to cpp.link package, not hxcpp package
* URL decode now does not need to performe reallocs
* Ensure HXCPP_API_LEVEL is always defined
* Added __hxcpp_unload_all_libraries to cleanly unload dlls
* Added some utc date functions
* Better support for non-console apps in windows XP 64
* Increased use of HXCPP_DEBUG_LINK for gcc based targets
* Class 'hasField' is now more consistent with other functions/targets
* 'haxelib run hxcpp test.cppia' will run Cppia on the specified file
* Add fast-select option for sockets
* Allow code to run without HXCPP_VISIT_ALLOCS defined
* Fix debugger thread deadlocks
* Allow up to 27 dynamic arguements
* Fixes for Emscripten - byte align access and disable threads
* Allow emscripten to generate 'executables' (.js/.html) and add options for specifying memory
* Allow spaces in exe names again
* Make cpp::Struct compare via memcmp, and mark correctly
* Fix catch block in cppia
* Treat '-debug' as an alias for "-Ddebug"
* Expose ArrayBase for use with some generic or external code
* Clarify the role of 'buffer' in cffi

------------------------------------------------------------
* Only put a minimal run.n in source-control, and use this to boot hxcpp.n
* Added cpp.Struct and cpp.Reference classes, which are handy for extern classes
* Moved Class to hx namespace
* Simplified 'main' logic
* Allow new android compilers to work for old devices (thanks google)
* Correctly read hxcpp_api_level from Build.xml
* Verbose logging prints which file is being compiled
* Handle undefining the INT_ constants differently to allow std::string to still compile
* Remove entries form Options.txt that do not influence the cpp build
* Add optional destination= command-line option to allow copying the result to named file
* Static libraries will be prefixed with 'lib' now
* val_is_buffer always returns false on neko
* Add val_iter_field_vals, which is like val_iter_fields but consistent with neko
* Remove NekoApi binaries
* Add Cppia binaries
* Add Windows64 binaries
* Make compares between Dynamic and numeric types false, unless the Dynamic is actaully numeric

------------------------------------------------------------
* Even more optimizations for hashes
* Some more optimizations for small hashes
* Fix for google changing inlining in platform21 headers (atof, rand, srand)
* Re-tuned Hash for small objects too (improves Anon object perforamce)
* Reverted change that automatically threw 'BadCast'.  Now required HXCPP_STRICT_CASTS

------------------------------------------------------------
* Cached dynamic versions of small ints and 1-char-strings for speed
* Added support for weak hashes - needs latest haxe version
* Use internal hash structure for maps - now faster.  New version of haxe makes it faster still.
* Changed the way development versions are bootstrapped to avoid committing binaries
* Improved mingw support
* Dont append -debug to dll name
* Reorder xml includes to allow early parts to correctly influence older parts
* Fix busy wait in semaphore lock
* Fixed GC issue when constructing exrernal primitive objects
* Added armv7s and arm64 targets for ios
* Some fixes for neko cffi - wstring and warning for neko_init
* Fix file read (and copy) from thread

------------------------------------------------------------
* Compile fix for blackberry
* Pass on haxe_api_level
* Add -nocolor flag

------------------------------------------------------------
* Add support for prelinker
* Cygwin toolchain fix
* Add HXCPP_NO_COLOUR  and HXCPP_NO_M32
* Fix windows trace output
* Add initial support for GCWO compile
* Fix bug with losing GC references in Array.sort
* Fix bug with zombie marking
* Add support for optimised sort routines
* Add support for haxe.ds.Vector optimisation
* Add support for cpp.Pointer, cpp.NativeArray, cpp.NativeString

------------------------------------------------------------
* Add BlackBerry and Tizen binaries
* Fix issues when using names like ANDROID or IPHONE in an enum
* Added more info in verbose mode (setenv HXCPP_VERBOSE)
* Refactor build files to allow greater customisation
* Fix bug with 'lock' where some threads may not get released
* Add optimised arrays access
* Add optimised memory operations for arrays and haxe.io.Bytes
* Avoid blocking in gethostbyname
* Upgrade run tool output and layout
* Restore sys_time for windows

3.1.1
------------------------------------------------------------
* Fixed MSVC support for 64-bit targets (vc11, vc12)
* Initial work on cpp.Pointer (not fully functional)
* Fixed callstack when throwing from native function

3.1.0
------------------------------------------------------------

* VC 2013 support - used as default now
* Add winxp compatibility flags
* Allow cross-compiling from mac to linux
* Added NSString helper conversion
* Better auto-detection for android toolchain
* Allow foreign threads to easily attach and detach from GC system
* Weak references to closures keep object alive
* Added HXCPP_API_LEVEL define to allow for future compatibility
* Fixed clearing finalizers twice
* Int multiply and minus are performed with integers now
* Fix comparing +- infinities
* Use multiple threads in the mark phase of GC
* IOS now defaults cpp11 binary linkage
* Added HXCPP_VERBOSE environment var to enable extra output
* Fixed spin loop in pthread_cond_wait
* Added ability to link several .a files into a single .a file
* Removed dependence on STL runtime for supplied modules
* Renamed some directories to be more standard
* Moved some extra build files into obj directory
* Use sys.io.Process instead of Sys.command to avoid threading slowdown writing to console
* Add hxcpp.Builder to help with building multiple binaries
* Add android x86 support
* Drop pre-compiled support for everything excepth windows,mac,linux,ios and android
* Allow libraries and files to accumulated in the build.xml
* Supply pre-build lib files for static linking on supported platforms
* Support for static linking of all modules
* Support for hxcpp-debugger project
* Binaries have been removed from repo, and are built using a server
* Use build.n script to build all appropriate binaries
* Some initial support for mysql and sqlite databases
* Add free_abstract for safe releasing of data references
* Change process lauching to get better thread usage on mac
* Fix GC error in string resources
* Give obj files in libraries unique names

3.0.2
------------------------------------------------------------
* Fix Dynamic + Int logic
* Reverted linux compiler to older version
* Cast Array at call site if required
* Tweak Array.map return value

3.0.1
------------------------------------------------------------
* Added nekoapi for linux64
* Upgrade nekoapi to v2
* Added haxe vector support
* Added socket_set_fast_send
* Fixed android build
* Expanded native memory access methods
* Fix exception dump
* Added initial Emscriptm support
* Allow specification of ANDROID_HOST
* Inital work on auto-setup of win64
* Support call-site casting of Arrays


3.0.0
------------------------------------------------------------
* Support haxe3 syntax
* Added socket poll function
* Added some initial support for dll_import/dll_export
* Allow full path name when loading dynamic libraries
* Allow dynamic toString function
* Added initial support for Raspberry Pi
* Array sort now uses std::stable_sort
* Fixed Dynamic+null string output
* Fix splice size calculation
* Add object ids for use in maps
* Add map/filter functions to arrays
* GC will now collect more often when big arrays are used
* You can specify a number of args > 5 for cffi functions if you want
* Fix internal hash size variable
* Class static field list does not report super members now
* Fix casting of null to any object
* Do not read input twice in sys_getch
* Link in PCH generated obj data on msvs 2012
* Date is now consistent with UTC
* Hash 'remove' now returns correct value
* CPP native WeakRef now works, and has a 'set' function
* Fixed compile error when assigning to a base class
* Fixed compile error when using != and Dynamic
* Math/floor/ceil/trunc/min/max now pass unit tests
* More control over android sdk installation
* Regexp_match fix
* Fix val_callN CFFI

2.10.3
------------------------------------------------------------
* Added initial build support for WinRT
* Android toolchain improvements
* Minor compile fixes
* Other minor improvements

2.10.2
------------------------------------------------------------
* Fixes for BlackBerry 10 compatibility
* Fixes for iOS 6 compatibility
* CFFI improvements
* Minor Linux improvements
* Minor OS X improvements

2.10.1
------------------------------------------------------------
* Fix trace() output
* Clang options for OS X compiler
* Small fixes

2.10.0
------------------------------------------------------------
* GC upgrades - moving/defragging/releasing
* Built-in profiler
* Build-in debugger
* Fix mac ndll finding bug
* Add Int32 member functions
* Clang options for ios compiler
* Add a few pre-boxed constants
* Some general bug fixes

2.09.3
------------------------------------------------------------
* Fix Xml enum usage

2.09.2
------------------------------------------------------------
* Resolve library paths when launching Mac apps from Finder
* Compile fix for the BlackBerry toolchain
* Fix interface comparison
* Fix api_val_array_value for NekoApi
* Add workaround for optional Strings in interfaces 
* Tweak the timing og the GC run
* Remove setProperty conditional compiles
* String charCodeAt only returns positive values
* Fix modulo for negative numbers
* Remove extra space from array output
* Treat '.' and '_' as literals in urlEncode
* Dynamically generated, 0 param, enum instances match the static version


2.09
------------------------------------------------------------
* Improved precision in random implementations
* Added some experimental support for float32
* Added some experimental support for generic getProcAddress
* String::fromCharCode generates single-byte strings
* Fix method compares
* Plug memory leak in finalizers
* Fix debug link flags
* Separate get/SetField from get/setProperty
* Added Null<T> for optional parameters

2.08.3
------------------------------------------------------------
* Actually add blackberry toolchain

2.08.2
------------------------------------------------------------
* Add blackberry support
* Add armv7 options
* Support new xcode layout
* Fix const qualifiers on interface functions
* Fix webOS obj directory

2.08.1
------------------------------------------------------------
* Fix Math.random returning 1.0 sometimes
* Std.is( 2.0, Int ) is now true
* Make static library building more separated - refactor defines to control this 
* Do not use @files for linking on mac
* toString on Anon objects will now get called
* Fix fast memory access with --no-inline
* Android tool host now set to linux-x86
* Allow use of __compare as operator== overload
* Add toNativeInt
* Add weak references
* Implement some neko/cffi compatibility operations
* Fix mac deployment using environment variable
* Fix reentrant mutexes
* Do not explicitly specify version of g++
* Speedup some code by avoiding dynamic_cast if possible
* Some fixes to allow Android multi-threading in normal operation

2.08
------------------------------------------------------------
* Do not create a new class definition for each member function
* Allow 5 fast and up to 20 slow dynamic function arguments
* Support utf8 class
* Added "Fast Memory" API similar to flash
* Added support for webOS
* Fix uncompress buffers
* Added file to undefined pesky processor macros
* Setup default config in user area
* Auto-detect msvc and iphone version
* Force compilation for mac 10.5
* Some support for cygwin compilers
* Remove Boehm GC as an option
* Integrate properly now with Android ndk-r6
* Make Int32 pass haxe unit tests (shift/modulo)
* Fix bug in "join"
* Fix bug with marking the "this" pointer in closures
* Fix bug with returning NAN from parseFloat
* Fix linux link flags
* Fix bug where string of length 0 would be null
* Made String cca return value consistent
* Added control over @file syntax
* Removed need for nekoapi.ndll
* Allow for neko.so to end in ".0"

2.07
------------------------------------------------------------
* Added initial support for Mac64, Linux64, MinGW and GPH and refactored build tool.
* Return the count of traced objects
* Fix interface operator ==
* Initial work on msvc10 batch file
* Add bounds check on String.cca
* Build static libraries, if requrested
* Added exe stripping
* Added val_field_name, val_iter_fields
* Fixed nekoapi string length
* Fixed Sys.args

2.06.1
------------------------------------------------------------
* Close files if required in GC
* Added fix for File.write
* Fixed String UTF8 Encode
* Nekoapi is now a "ndll", not a "dso".
* Fix array compile issue on linux
* Fix stack setting on firced collect

2.06.0
------------------------------------------------------------
* Updates to match haxe 2.06 compiler features
* Numerous bug fixes
* Add additional context to GC collection process
* Swapped from wchar_t* to utf8 char*
* Added templated iterators
* Use strftime for Dates
* Fix socket select and "_s" members
* Seed Math.random
* Fixed dynamic integer compare
* Added __hxcpp_obj_id
* Added some Android support

2.05.1
------------------------------------------------------------
* Updated windows nekoapi.dll binary
* Added -m32 compile flags to force 32 bit

2.05.0
------------------------------------------------------------

* Default to IMMIX based internal garbage collection.
* Reorginised files - split big ones, and moved common ones out of "runtime".
* Put internal classes in "hx" namespace, or HX_ prefix for macros.
* Remove multiple-inheritance, and use delegation instead.
* Write "Options.txt" from compiler so dependency can be determined.
* Require -D HXCPP_MULTI_THREADED for multi-threaded classes - to avoid overhead if not required.
* Build thread code into executable for better control.
* Fix return values of parseINt/parseFloat.
* Added comprehensive list of reserved member names.
* Put if/else statements in blocks.
* Added assert, NULL, LITTLE_ENDIAN, BIG_ENDIAN as keywords.
* Added control over how fast-cffi routines are created by requiring cpp.rtti.FastIntergerLookup to be "implemented".
* Construct anonymous object fields in deterministic (as declared) order.
* Fix code generation for some complex inline cases.
* Added cpp.zip.Compress
* Change "Reflect" class to be more standard
* Use array of dynamics for StringBuf.
* Fix setting of attributes in XML nodes.

Build-tool:
* Allow multiple build threads (via setenv HXCPP_COMPILE_THREADS N) for faster building on multi-code boxes.
* Added FileGroup dependencies
* Added pre-compiled headers (windows only, at the moment since gcc seems buggy)


1.0.7
-----------------
Changelog starts.
