The Hxcpp Cache
---------------
The hxcpp compile cache is used to share object files between projects. This can alleviate the need to ship static libraries with external projects, since developers who use the library can compile the library just once, and then reuse it between projects.

The cache uses a hashing mechanism to tell if the contents of a file or its dependencies has changed, and combines this with the compiler version and flags to make specific object files for each change and each compiler congiguration.  This also allows some common haxe runtime and haxe generated files to share their object files between projects, even if in different directories.

Additional benefits include keeping some files outside the source tree, and being able to remove these temp files easily.

### Setup
A directory needs to be set aside to enable the cache.  If possible, this should be on fast storage, such as a SSD.  This is most easily done with an entry in the .hxcpp_config.xml file:
```xml
 <set name="HXCPP_COMPILE_CACHE" value="c:/hxcpp/cache" />
 <set name="HXCPP_CACHE_MB" value="4000" />
```
Keeping the path short can help in some border-line cases with some compilers where command-line length can become an issue.

The cache size defaults to 1 Gig.  For many cases, this is big enough.  However, on large projects, with several architectures and lots of debug information, this default can lead to "cache churn" where some files are evicted from the cache, even though they are likely to be used again.  Increasing the number of mega-bytes allocated to the cache can help here.

### Using The Cache
To use the cashe with your own libraries, the files group should have 'cache' entry to tell hxcpp that you have considered dependency issues when designing the group.

  ```xml
  <cache value="true" project="name" asLibrary="true" />
  ```

  - project = name of project used to manage and group object files in the cache
  - asLibrary = link the objs into a .lib file.

When linking a file group 'asLibrary', the object files are compiled and then the library tool is used to make a library from these object files.  This library is then added to the linker.  This has a few implications:
   - Object files that to not resolve any symbols directly are not added to the final executable
     + Can make final exe size smaller
     + If the object file only contains a handler that is self-registering via static constructor,
        then the constructor may not get called, leading to bugs.
   - Can help on some systems where the linker command-line length is an issue.


### Management
When compiling normally, hxcpp will check the cache size and evict the least used files to maintain the specified cache size.
Object files in the cache are grouped into "projects" to make management easier, and the hxcpp build tool can be used to explicitly manage the object files.
```
 haxelib run hxcpp cache [command] [project]
   Perform command on cache, either on specific project or all. commands:
    clear -- remove all files from cache
    days #days -- remove files older than "days"
    resize #megabytes -- Only keep #megabytes MB
    list -- list cache usage
    details -- list cache usage, per file
```
Start with
```
haxelib run hxcpp cache list
```
To get an idea of cache usage.


