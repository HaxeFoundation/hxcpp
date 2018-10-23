Structure of the top-level
---------------------------
The top-level nodes live inside an "xml" node, and can be:

- *set* - Set a "define", define being a general varaible.
  ```xml
  <set name="name" value="1" />
  ```

- *setenv* - Sets an hxcpp define and an envorinment variable for child processes.
   ```xml
   <setenv name="name" value="1" />
   ```
- *unset* - Unset a define.  if="name" will no longer be true
  ```xml
  <unset name="name" />
  ```

- *setup* - Used internally to call custom setup code to find SDKs etc.
  ```xml
  <setup name="androidNdk|blackberry|msvc|pdbserver|mingw|emscripten|nvcc" />
  ```

- *echo* - Print value to console.  Good for debugging.
  ```xml
  <echo value="text" />
  ```

- *error* - Print value to console and force error.  Good for checking prerequisites.
  ```xml
  <errot value="error message" />
  ```

- *pleaseUpdateHxcppTool* - Used to tell people updating git version that they need to recompile the build tool.
  ```xml
  <pleaseUpdateHxcppTool version="1" />
  ```

- *path* - Add an directory to the exe search path.
  ```xml
  <path name="directory_to_add" />
  ```

- *mkdir* - Create a directory.
  ```xml
  <mkdir name="directory" />
  ```

- *section* - Groups block of elements - usually ones that all respect the same if/unless condition.
  ```xml
  <section name="id" />  </section>
  ```

- *copy* - Copy file when node is parsed.
  ```xml
  <copy to="destination" from="src" />
  ```

- *import*/*include* - Read xml from another file. 'import' resets the relative base to the new file, include does not.
  ```xml
  <import name="filename" section="filter" noerror="true" />
  <include name="filename" section="filter" noerror="true" />
  ```
    + noerror - setting the optional noerror allows the file to be missing
    + section - setting the optional section will only reand the named section from the xml file.  Used by hxcpp_config.xml.

- *pragma* - Only include build file once, even with multiple include statements.
  ```xml
  <pragma once="true" />
  ```

- *nvccflag* - Add flag to all nvcc compiles.
  ```xml
  <nvccflag name="?name" value="-IincludePath" />
  ```

- *nvcclinkflag* - Add flag when linking with nvcc code.
  ```xml
  <nvcclinkflag name="?name" value="-arch=sm_30" />
  ```

- *files* - Define a file group, and set default tags.
  ```xml
  <files dir="dir" name="name" tags="tag1,tag2,tag3" >
   ...
  </files>
  ```
    + dir = directory to which the filenames in the group are relative
    + tags = comma separated list of flags tags

- *target* - Define a target, and set its toolid(link mode) and output name.
  ```xml
  <target name="name" overwrite="true" append="true" tool="linker" toolid="${haxelink}" output="filename" >
   ...
  </target>
  ```

- *copyFile* - Copy a file after given toolId is run into target output directory
  ```xml
  <copyFile name="destination" from="src" allowMissing="true" overwrite="true" toolId="filter" >
  ```

- *magiclib* - Internal for replacing dlls with object files
  ```xml
  <magiclib name="libname" replace="old dll" />
  ```

- *compiler* - Define a compiler.
  ```xml
  <compiler id="id" exe="command" replace="true" >
   ...
  </compiler>
  ```
    + Use optional 'replace' to overwrite, otherwise append
    + It is assumed only 1 compiler is active
    + exe can be overridden in the body of the definition

- *stripper* - Define a stripper, to remove debug information for release from gcc executables
  ```xml
  <stripper exe="command" replace="true" > </stripper>
  ```
    + Use optional 'replace' to overwrite, otherwise append

- *linker* - Define a linker.
  ```xml
  <linker id="id" exe="command" replace="true" > </linker>
  ```
    + Use optional 'replace' to overwrite, otherwise append
    + id could be 'static_link', 'dll' or 'exe'.  Usually all 3 linkers are defined.
    + exe can be overridden in the body of the definition

- *prelinker* - Define a prelinker.
  ```xml
  <prelinker name="id" replace="true" />
   ...
  </prelinker>
  ```


