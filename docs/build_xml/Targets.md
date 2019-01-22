Targets
-------

Targets are used to produce binaries, or to group other targets.  When compiling exes or dynamic libraries, they provide the additional link libraries.

By default, hxcpp will try to compile the 'default' target, so it is easiest to define this one - perhaps by simply adding a dependence on your other targets(s).

The target is defined with a 'toolid' attribute; exe, static_link or dll.  This defines which linker is run, but many of the target entries will be the same even if the linker is changed.

Targets can contain the following nodes:

- *subTargetName* - Build another target before building this one.
   ```xml
   <target id="subTargetName" />
   ```

- *merge* - Combine fields from another target.  This is usful if you want a target to function as static library or dll when compiled in its own, but also allow it to be used as a list of object files if another target wants to link in the object files directly.
   ```xml
   <merge id="otherTargetName" />
   ```

- *files* - Add a named group of compiled files to target.
   ```xml
   <files id="filesId"/>
   ```

- *section* - Group items - usually sharing common condition
   ```xml
   <section > </section>
   ```

- *lib* - Add a library to the link line.
   ```xml
   <lib (name|hxbase|base)="libName" />
   ```
     + name = the complete name is specified
     + base = the name without compiler-specific extension (.a/.lib) is specified
     + hxbase = the name without extension and achitecture (-v7/.iphoinesim) is specified

- *flag* - Add a single link flag.
   ```xml
   <flag value="flag"/>
   ```

- *vflag* - Add a pair of link flags.
   ```xml
   <vflag name="flag1" value="flag2"/>
   ```

- *depend* - Target depends on given filename.
   ```xml
   <depend name="filename"/>
   ```

- *dir* - Add a directory to the targets directory list.  These directories will get removed then the target is cleaned.
   ```xml
   <dir name="tempPath"/>
   ```

- *outdir* - Directory for build results - including "copyFile" targets
   ```xml
   <outdir name="path"/>
   ```

- *ext* - Extension for generated files.
   ```xml
   <ext name=".ext"/>
   ```
     + ext = extension - should contain "."

- *builddir* - The directory from which the targets build commands are run, and therefore the 
       relative base for some filenames, and destination for some compiler generated temps.
   ```xml
   <builddir name="path"/>
   ```

- *libpath* Add library search path to build command
   ```xml
   <libpath name="directory"/>
   ```
     + name = directory.  The particular linker will add the required flags

