Tags
----

Tags are identifiers that link compiler flags with spefific files.  Usually, they are defined in a files group with the 'tags' attribute as a comma separated list, and with the 'tag' attribute on a compiler 'flag' node.  Files are then compiled with all the flags that have matching tags.

By restricting tags to certain files, only a sub-set of files needs to be recompiled when conditions change, and files without the relevant tags can reuse their object files.  This can save a lot of time, since come flags only apply to a few files.

Files can override the group tags by specifying their own 'tags' attribute.  Groups can add tags with the 'tag' node.

Some tags have standard meanings when compiling haxe code:
 - *haxe* - The haxe tag adds all the required compiler flags to get haxe-generated code to compile correctly, and should be added to files that depend directly or indirectly on hxcpp.h.
 - *static* - This will add the STATIC_LINK define when approtriate, which is used for generating cffi glue.  It should be added to cffi code that might generate static libraries.
 - *gc* - These flags only affect the garbage-collection files
 - *hxstring* - These flags only affect String.cpp
 - *optimization tags* - each file is assumed to have exactly 1 optimization tags.  If none is explicitly specified, then the default is used depending on whether it is a debug or release build.  They are:
    + optim-std = alias for 'debug' or 'release' depending on build
    + debug
    + release
    + optim-none
    + optim-size

  Setting one of these tags is useful for compiling your library in release mode, even if haxe has -debug.  Some very big files are slow to compile in release mode, so using a less optimized mode can be faster.


The tags attrubute can be added to a haxe-generated file using the `@:fileXml` meta, eg:
```haxe
@:fileXml("tags='haxe,optim-none'")
class MyClass { ...
```

Here, the class is compiled with the normal haxe flags, but has the optimizations disabled, which can lead to much faster compiler times in some circumstances.
