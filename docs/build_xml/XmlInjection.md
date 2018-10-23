Xml Injection
-------------

When using external code in hxcpp, it is often useful to add libraries, include paths or compiler flags to the build process.  This can be done with the `@:buildXml` class meta-data. eg,

```haxe
@:buildXml("
<target id='haxe'>
  <lib name='${haxelib:nme}/lib/${BINDIR}/libnme${LIBEXTRA}${LIBEXT}'/>
</target>
")
@:keep
class StaticNme
{
  ...
```

So, by referencing a given class (you just 'import' the class, no need to use it because it has the @:keep meta-data), the xml fragment is also included.

Here, the xml fragment is copied verbatim into the generated Build.xml immediately after the standard file lists.  This example adds a library to the haxe target, but you could also add flags to files nodes, or files to another files node or target.  Another possibility is to add an include command to pull in a whole external xml file.  This can help avoid some syntax awkwardness needed when quoting strings in meta-data, and allows a normal xml editor to be used.

It is also possible to replace the `__main__` file group to skip the standard initialization code and use a custom bootstrap procedure.

