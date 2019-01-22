Build.xml
----------

The hxcpp build.xml build system is designed to make compiling, cross-compiling and linking easy on a large variety of operating systems and devices.  It was originally designed to build the haxe-generated c++ code but has evolved to replace the need for configuration tools in many open source libaries.

### Running
The source code for the tool lives in "tools/hxcpp" in this repo.  When compiled, it can be run with the haxe 'haxelib' library tool.  This is usually done automatically by the haxe compiler after the cpp code has been generated.  It can be done maunually like:
```
haxelib run hxcpp build.xml key=value .... [target]
```

### Configuration
The hxcpp build tool is configured using key-value pairs, or just using keys, known internally as 'defines'.  These can be set in severaly ways:
  - From system environment variables
  - From the command-line, with key=value
  - From haxe.  Keys defined in haxe with '-D key[=value]' are passed to the build too, where they can influence the build.  Certain defines need to be set on the haxe command line so that they can influence the generated code.
  - From the the .hxcpp_config.xml file in the users home(profile) directory.  This is a good place to set values the apply to the whole machine, like the location of SDKs etc.
  - The defines can be manipulated logically from within the build files themselves.

See [Defines.md](Defines.md) for a list of standard defines.


### Format
The compiler specification and target lists all use the same format.
  - uses xml parser
  - mostly declarative list of files and flags
  - order is important
    + overriding values is a valid technique
    + "commands" are run as they are parsed (eg, 'echo')
  - conditions via "if" and "unless" node attributes
  - substitution via '${VAR}' sysntax
  - need to define 'default' target

### Conditions/Substitution
Most of the xml nodes support 'if' and 'unless' attributes.  These will enable or disable the whole node according the existance or non-existance of a define.  These can be combined with a space for "and" or two pipes for "or".

Substitution is supported via the dollars-brace syntax, and does simple text substitution.  In addition, there are a few dynamic variables that can be used:
 - "${VAR}" - normal replacement
 - "${removeQuotes:VAR}" - strips surrounding quotes from VAR, it any
 - "${dospath:VAR}" - converts VAR to backwards-slash path
 - "${dir:PathWithFilename}" - just the directory part of filename
 - "${this_dir}" - the location of the containing build.xml file


### Example
The following code is saved in [example.xml](example.xml) in this directory
```xml
 <xml>
   <echo value="Hello ${WHO}" if="WHO" unless="SILENT" />
   <echo value="You are in ${haxelib:hxcpp}" unless="WHO||SILENT"/>
   <error value="Silent and who both specified" if="WHO SILENT"/>
   <target id="default" />
 </xml>
```

and some example uses:

```
 unsetenv SILENT
 haxelib run hxcpp example.xml
 haxelib run hxcpp example.xml WHO=world default
 setenv SILENT 1
 haxelib run hxcpp example.xml
 haxelib run hxcpp example.xml WHO=world
```

### Details
The build.xml file contains configuration, targets, compilers, linkers and files. The details can be found in this directory.
 - [Top Level](TopLevel.md)
 - [Files](Files.md)
 - [Targets](Targets.md)
 - [Compiler](Compiler.md)
 - [Linker](Linker.md)
 - [Stripper](Stripper.md)

When building from haxe, the "haxe" target is built.  You can see the details in [HaxeTarget](HaxeTarget.md).

You can extend the generated Build.xml from haxe code using [Xml injection](XmlInjection.md).

