Files
------
The files node defines a group of files that all share the same attributes, including relative directory, default compiler flags and dependencies.  The node can be used to define a set of header files on which other files can depend, or a set of source files to be compiled and included in a terget.

- *depend* - Declare that all files in the group depend on another file or another file group.
  ```xml
  <depend name="filename" />
  <depend files="filesId" />
  ```
    + name = If the named file changes then then all the files in the group need recompiling.
    + files = If any of the files in the named group changes then then all the files in the group need recompiling.

- *options* - Name of file containing compiler flags.  When the cache is not used, Options.txt helps detect when the options have changed, and therefore whether files need recompiling.
  ```xml
  <options name="Options.txt" />
  ```

- *config* - Name of file to generate that contains the #defines that were active when code was compiled.
  ```xml
  <config name="outfile.h" />
  ```

- *tag* - Add a default compiler flags tag too all files in group.  See [Tags.md](Tags.md).
  ```xml
  <tag value="tag" />
  ```

- *addTwice* - When compiled to a library, add the library twice to the link line - once at the beginning and once at then end to satisfy linux selective linking.
  ```xml
  <addTwice value="tue" />
  ```

- *cache* - Use compiler cache for files in group.  See [compile cache](../CompileCache.md) for more details.
  ```xml
  <cache value="true" project="name" asLibrary="true" />
  ```
    + project = name of project used to manage and group object files in the cache
    + asLibrary = link the objs into a .lib file, which can skip unneeded objs, but
     will also skip things that rely on static initializers to register handlers, so be careful.

- *include* - Include an external file list
  ```xml
  <include name="filename.xml" />
  ```

- *section* - Groups block of elements - usually ones that all respect the same if/unless condition.
  ```xml
  <section name="id" />  </section>
  ```


- *compilerflag* - Add a compilerflag when compiling files in group.
  ```xml
  <compilerflag name="name" value="value" />
  <compilerflag value="value" />
  ```
    + name, value = add 2 flags when compiling
    + value = add 1 flag when compiling

- *nvcc* - This group is compiled with nvcc.
  ```xml
  <nvcc />
  ```

- *objprefix* - An id prepended to generated obj name to allow alphabetical grouping of similar objs.
  ```xml
  <objprefix value="prefix" />
  ```

- *precompiledheader* - Use a precompiledheader of given name when compiling group
  ```xml
  <precompiledheader name="name" dir="directory" />
  ```
    + name = the include used when precompiling these files (without the .h)
    + directory = the location of this file

  eg, for `#include <lib/Header.h>`
    + name = "lib/Header"
    + directory = "${haxelib:somelib}/include"

- *file* - Add file to group, with optional attributes
  ```xml
  <file name="filename" tags="tag,tag1" filterout="define" embedName="embed" >
     <depend name="filename1" />
     <depend name="filename2" />
  </file>
  ```
     + name = name of file - may be absolute or relative to files.dir
     + tags = optional override of group tags.  See [Tags.md](Tags.md).
     + filterout = allows files to be skipped at compile-time if the named define exists.
       This is useful when the define is set sometime after the file list is parsed.
     +  depend name = filename of additional dependency
     + embed = causes the file to be embedded as a enxtern c++ 'const char *' string constant of the specified name

