# hxcpp

[![Build Status](https://dev.azure.com/HaxeFoundation/GitHubPublic/_apis/build/status/HaxeFoundation.hxcpp?branchName=master)](https://dev.azure.com/HaxeFoundation/GitHubPublic/_build/latest?definitionId=3&branchName=master)

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
