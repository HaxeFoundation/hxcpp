Compiler
--------
Compilers are run over each of the changed files in each of the filegroups in a target to create object files, which are then linked into the target.  Modification dates or hashes are used to tell if files need recompiling, of if the object file can be reused.



- *flag* - Add single arg to command-line.
  ```xml
  <flag value="value" tag="tag" />
  ```
    + value = text for flag added to command line
    + tag = optional filter to restrict flag to files with matching tag. See [Tags.md](Tags.md).

- *cflag/cppflag/objcflag/mmflag* - Add flag when compiling specific file types.
  ```xml
  <cflag value="value" />
  <cppflag value="value" />
  <objcflag value="value" />
  <mmflag value="value" />
  ```
    + cflag = only added to .c files
    + cppflag = only added to .cpp files
    + objcflag = only added to .objc files
    + mmflag = only added to .mm objc++ files

- *pchflag* - Add flag when compiling precompiled header .h files.
  ```xml
  <pchflag value="value" />
  ```
    + pchflag = Usually `["-x", "c++-header"]` for apple to specify the "identity" of the header


- *pch* - Set the precompiled header style - "gcc" or "msvc".
  ```xml
  <pch value="gcc|msvc" />
  ```

- *objdir* - set name of directory used to store object files.  Should be unique for given set of compiler flags to avoid linking against wrong architecture.
  ```xml
  <objdir value="obj/somewhere" />
  ```
    + value = usually built progtamatically, like `obj/msvc${MSVC_VER}-rt${OBJEXT}${OBJCACHE}${XPOBJ}`

- *output* - Flag used to specifying compiler output name.
  ```xml
  <outflag value="-flag" />
  ```
    + value = flag value.  Note that it should contain a space character
      if the actual name should be a separate argument, like "-o ", or "-o"/"-out:" if it does not.

- *exe* = Override the executable command specified in the compiler attribute.
  ```xml
  <exe name="command" />
  ```
    + name = command.  Usually you would use 'path' to add the directory, then this is just the filename part.

- *ext* - Specify the object file extension
  ```xml
  <ext name=".obj" />
  ```
    + name = extension, including ".". Usually ".o" or ".obj".

- *getversion* - The command-line used to create text describing the version of the compiler.
   This is used for working out if the compiler has changed, and therefore the objs need recompiling.
  ```xml
  <getversion value="command" />
  ```
    + value = command to run.  It defaults to `compiler --version` which is usual for gcc based compilers.
       Setting it empty will disable caching.

- *section* - Group entries - usually sharing common condition
  ```xml
  <section > </section>
  ```

- *include* - include compiler options from another file.  Most compilers should include `<include name="toolchain/common-defines.xml" />` to add defines used by hxcpp.
  ```xml
  <include name="filename" />
  ```



