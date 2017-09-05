Linker
------
Generally one linker is run per target to build a static library, dynamic library or exe.  The 'id' attribute of the linker specifies whch type of linking is performed.


- *exe* - Overwrite the exe command for this linker.
   ```xml
   <exe name="command" />
   ```

- *flag* - Add a single link flag.
   ```xml
   <flag value="flag"/>
   ```

- *ext* - Default extension for generated files - if not overridden by target.
   ```xml
   <ext value=".ext"/>
   ```
     + value = extension, including "."

- *outflag* - Flag for specifying lnker output name.
   ```xml
   <outflag value="-o"/>
   ```
     + value = linker flag.  Note that it should contain a space character
      if the actual name should be a separate argument, like "-o ", or "-o"/"-out:" if it does not.

- *section* - Group items - usually sharing common condition
   ```xml
   <section > </section>
   ```

- *libdir* - A temp directory name to build into.  This will capture the extra files the compiler
       generates, and then the desired file will be copied to the correct location.
   ```xml
   <libdir name="name"/>
   ```

- *lib* - Add a library to the link line.
   ```xml
   <lib (name|hxbase|base)="libName" />
   ```
     + name = the complete name is specified
     + base = the name without compiler-specific extension (.a/.lib) is specified
     + hxbase = the name without extension and achitecture (-v7/.iphoinesim) is specified

- *prefix* - Prefix for generated files.
   ```xml
   <prefix value="lib"/>
   ```
     + value = prefix.  This will usually be "lib" or nothing.

- *ranlib* - Whether ranlib needs to be run, and what command to use.  Usually only for unix-style static libraries
   ```xml
   <ranlib name="ranlib command"/>
   ```

- *libpathflag* - Flag used for adding library paths to command line.  It will be combined with *lib* entries.
   ```xml
   <libpathflag value="-L"/>
   ```

- *recreate* - Whether to delete the target file before re-running link command.
      The archive "ar" command likes to add obj files to existing archives, so deleting first can help.
   ```xml
   <recreate value="true"/>
   ```

- *expandAr* - Whether to extract the individual obj files from an archive and add these, rather than
       add the archive as a single library.  Can solve some link-order and static-initialization issues,
       but may make final exe bigger.
   ```xml
   <expandAr value="true"/>
   ```

- *fromfile* - If the linker supports taking a list of objs in a file, then this is flag for specifying the file.
   ```xml
   <fromfile value="flag" needsQuotes="true" />
   ```
     + value = flag for specifying file. 
     If the filename should be a separate argument, then the flag should end with a space.
     Usually `@` or `-filelist `.  Use empty to disable.
     + needsQuotes = is whether to quote the obj names in the file


