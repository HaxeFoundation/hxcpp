# hxcpp

[![TravisCI Build Status](https://travis-ci.org/HaxeFoundation/hxcpp.svg?branch=master)](https://travis-ci.org/HaxeFoundation/hxcpp)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/HaxeFoundation/hxcpp?branch=master&svg=true)](https://ci.appveyor.com/project/HaxeFoundation/hxcpp)

hxcpp is the runtime support for the c++ backend of the [haxe](http://haxe.org/) compiler. This contains the headers, libraries and support code required to generate a fully compiled executable from haxe code.


# rebuilding

Running neko on the build.n script will rebuild the supported architectures on your current platform.

```
cd project
neko build.n clean
neko build.n
```


In the same folder, you can cross build to other platforms using the run.n with the said platform name.

For example : 

```
neko build.n android
```

You can save time if you know that you are only going to use, say, the dynamic libraries (ndlls) on a particular architecture on a particular platform.
```
neko build.n ndll-android-armv5
```

You can enable debugging in the standard runtime libraries using the debug flag, eg:
```
neko build.n windows -debug
```
which may help with native debugging.  Don't forget to rebuild without debugging for release.


For experts, you can configure the compilation scripts that will be used for executables and library production in the 'toolchain' folder.
