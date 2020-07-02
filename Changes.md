
* Added Arm64 suport on windows
* Fixed crash with zero-sized alloc and generational GC
* Fixed crash with generational GC when old objects come back to life
* Fixed compile error with @:fixed Anons and arrays (socket select)
* Fixed lastIndexOf
* Optimized some equality functions

4.1.1
------------------------------------------------------------
* Added functions for haxe 4.1 Api.
* Added HXCPP_DEBUG_LINK_AND_STRIP to preserve symbolic information when creating android release binaries.
* Added optional HXCPP_SIGNAL_THROW to convert memory errors to haxe exceptions without needing additional code
* Added string_hash_map_substr and __hxcpp_parse_substr_float/int to allow some substring processing without extra allocation

4.0.64
------------------------------------------------------------

* Upgrade buildserver to 4.01
* Better generational collection in high fragmentation case
* typeinfo include fix for MSVC
* Fix MySQL connections
* Fix bugs with HXCPP_GC_GENERATIONAL
* Add map.clear
* Better c++11 iOS support

4.0.19
------------------------------------------------------------

* Add Array.keyValueIterator
* General Utf16 string improvements
* Limit the amount of recursion in toString function
* Add float32 support to cppia
* Fix Gc race condition
* Throw exceptions according to the spec when casting
* Introduce hxcpp_smart_strings for unicode text

4.0.4
------------------------------------------------------------
* Compile Cppia against haxe 4.0 preview 4

4.0.2
------------------------------------------------------------
* Default Cppia to 64 bits on windows

4.0.1
------------------------------------------------------------
* More logic for determining the android NDK version
* Updated various opensource libraries (thanks robocoder)
* Updated version of zlib
* Updated version of sljit
* Updated version of pcre
* Updated version of sqlit3
* Updated version of mbedtls
* Some work on supporting utf16 strings (hx_smart_strings)
* Added process_kill
* Change root when calculating haxelib in build.xml files
* Fix cppia super calls across cpp boundary
* Add Array.resize
* Be consistent with mod in cppia
* Fix Sys.stderr
* Add 'embedName' file attribute to allow text to cpp conversion
* Updates for Msvc
* Updates for Xcode

3.4.188
------------------------------------------------------------
* Fix some threading crashes

3.4.185
------------------------------------------------------------
* Do not ship static libraries
* Use more lock-free structures in GC processing
* Added some documentation
* Added HXCPP_GC_SUMMARY option
* Added HXCPP_GC_GENERATIONAL option
* Added HXCPP_GC_DYNAMIC_SIZE option
* Some MSVC 2017 support
* Compile Cppia with JIT as an option by default

3.4.64
------------------------------------------------------------
* Fixed cppia native interface implementation
* Fixed debugger breakpoints
* More compatibility for inet_pton and inet_ntop
* Correct the order of thread housekeeping data

3.4.49
------------------------------------------------------------
* Fixed 2d-Arrays and unserialize

3.4.43
------------------------------------------------------------

* Added more options for code-size optimizations on android (thanks madrazo)
* Added version of stpcpy on android to allow building with platform > 21, and running on older devices
* Added some initial support for ipv6
* Experimental support for Cppia JIT
* Fixed issue with stale objects that use new pch files in cache
* Rethrowing exception now preserves stack correctly


3.4.2
------------------------------------------------------------

* Align float reads from memory for Arm architecture
* Removed some virtual functions not needed by newer versions of haxe
* Reworked the logic for compacting fragmented heaps with HXCPP_GC_MOVING
* Expose StackContext to allow inlining of allocation routine, and combine with Cppia context
* Fix some compare-with-dynamic issues
* Added WatchOs support
* Fixed for android NDK 13
* Fix Array closure equality
* Refactor the Cppia code
* Fix return codes for atomic decrease
* Fix some GC zone issues in the standard library
* Set minimum MacOS deployment target to 10.6
* Do not use typedefs for 'Int' and 'Bool' for newer api levels
* Added dll_link to create output dll
* Improved ObjC support
* Make Cppia order of operations of '+=' et al consistent with other targets
* Added NO_RECURSE flag to PCRE
* Fix bsd_signal undefines on android
* Add create/free abstract

3.3.49
------------------------------------------------------------
* Fix Dynamic != for haxe 3.2.1
* Fix Command line parsing on windows for triple quotes

3.3.45
------------------------------------------------------------
* Much better compile cache support
* Added tags to compiler flags to allow better targeting
* Added UCP support to regexp
* Added Array::fromData
* Added AtomicInt operations
* Added _hx_imod
* More improvements for tvos
* Fix blocking deque issue
* Improved native testing
* Added 'hxcpp run hxcpp cache ...' commands for managing cache
* Added cpp.Variant class for query of field values to avoid boxing
* Added more efficient version of finalizer
* Add non allocating version of __hxcpp_print
* More WinRT fixes
* Output 'HxcppConfig.h' with defines included for easier external integration
* Output list of output files if requested
* Add support functions for StdLib - alloc/free/sizeof
* Fix crash when marking stack names from GCRoots
* Add bitcode support for iOS
* Rename RegisterClass to avoid conflicts with windows
* Added 'VirtualArray' for arrays of unknown types
* Split Macros.tpl
* Added optional ShowParam to process_run
* Added inline functions for Int64 externs
* Add error check for allocating from a finalizer
* Fix null strings on Cffi Prime
* Use slow path if required for Win64 Tls
* Expand logic for detecting android toolchain from NDK name
* Remove the need for hxcpp binaries by compiling source directly into target
* Adjust the default verbosity level, and add HXCPP_VERBOSE/HXCPP_QUIET/HXCPP_SILENT
* Added some control options for copyFile directive
* Fix cppia decrement
* Add Array.removeRange, which does not require a return value
* Do not call setbuf(0) on stdin, since it messes with readLine
* Cppia now throws an error if loading fails
* Allocate EnumParam data inline to cut down on allocations
* Allow anonymous object data to be allocated inline to avoid allocations
* Add SSL library code
* Add NativeGen framework for interfaces
* Add macros to allow neater generated code
* Allow larger memory space with -D HXCPP_GC_BIG_BLOCKS
* Improve Array.join speed

3.2.205
------------------------------------------------------------
* Initial support for HXCPP_OPTIMIZE_FOR_SIZE
* Support HXCPP_DEBUG_LINK on more targets
* Support for cross compiling to windows from linux
* Added array removeAt
* Some telemety fixes (thanks Jeff)
* Check contents when comparing Dynamics with same pointer (Math.Nan!=Math.Nan)
* Numerous WinRT fixes (thanks madrazo)
* Fixed bug causing GC to crash marking constant strings (eg, resources)
* Updated default SDK for Tizen (thanks Joshua)
* Fixed command line args on linux (thanks Andy)

3.2.193
------------------------------------------------------------
* Some improvements for tvos
* Start on some GC defragging code
* Fix android thread access to GC structures
* Add socket socket_recv_from and socket_send_to
* Fixed memory leak in GC collection code
* Allow cross-compile to windows via MINGW
* Fix overflow error that meant GC would work with a too-small buffer in some cases

3.2.180
------------------------------------------------------------
* Initial support for tvos
* Change name of ObjectType to hxObjectType to avoid clashes with iOS
* Try to keep windows.h out of haxe-generated code
* Fix null access bug in array-of-array
* Create separate library for msvc 19

------------------------------------------------------------
* Try to get the pdb server working better for MSVS 2015
* So not export symbols on windows unless HXCPP_DLL_EXPORT is set (-D dll_export) - makes exe smaller
* Avoid dynamic-cast if possible when converting 2D arrays
* Some RPi fixes
* Some CFFI Prime fixes (thanks Joshua)
* Fix build tool for next version of neko
* Improve msvc cl.exe version checking for non-English environments
* Add more control over how much Gc memory is used
* Add faster(inline) thread local storage for Gc on windows.
* Add some Gc load balancing when marking large arrays with multiple threads
* Change the Gc memory layout to be a bit larger, but simpler.  This allows most of the allocation to be simplified and inlined.
* Explicitly scan registers for Gc references because the stack scanning was missing them sometimes
* Some additions to Undefine.h for windows
* When static linking using MSVC 2015, compile the libraries directly into the exe to avoid compatibility issues
* Move standard libraries into their own build.xml files
* Make it easier to change the generated output filename
* Allow targets from one build.xml file to be merged into another
* Some more work on HXCPP_COMPILE_CACHE
* Allow automatic grouping of obj files into librarys to avoid linking all the symbols in all obj files
* Add implicit conversion to referenced type from cpp.Reference
* Allow build.xml files to be imported relative to importing file
* Allow '-' in command-line defines
* Fix warnings from Hash class
* Fix setsockopt for Mac
* Support to MSVC2015
* Fix for Blackberry 10.3
* Fix debug break by linenumber
* Better objc integration (thanks Caue)
* Increase number of variables captured in closures to 20
* Initial support for telemetry (thanks Jeff)
* Align allocations for better emscripten support

------------------------------------------------------------
* Fix gc_lock error in remove_dir
* Some cppia bug fixes - enum and resources overrides
* More android atof fixes
* Improved haxelib seek logic

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
* Allow up to 27 dynamic arguments
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
