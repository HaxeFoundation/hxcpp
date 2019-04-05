# hxcpp

[![TravisCI Build Status](https://travis-ci.org/HaxeFoundation/hxcpp.svg?branch=master)](https://travis-ci.org/HaxeFoundation/hxcpp)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/HaxeFoundation/hxcpp?branch=master&svg=true)](https://ci.appveyor.com/project/HaxeFoundation/hxcpp)

hxcpp is the runtime support for the c++ backend of the [haxe](http://haxe.org/) compiler. This contains the headers, libraries and support code required to generate a fully compiled executable from haxe code.


# building the tools

```
REPO=$(pwd)
cd ${REPO}/tools/run
haxe compile.hxml
cd ${REPO}/tools/hxcpp
haxe compile.hxml
cd $REPO
```

# cppia

You first need to build the cppia host.

```
REPO=$(pwd)
cd ${REPO}/project
haxe compile-cppia.hxml
cd $REPO
```

Then you can do `haxelib run hxcpp file.cppia`.
