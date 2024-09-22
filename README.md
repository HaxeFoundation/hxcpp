# hxcpp

[![Build Status](https://dev.azure.com/HaxeFoundation/GitHubPublic/_apis/build/status/HaxeFoundation.hxcpp?branchName=master)](https://dev.azure.com/HaxeFoundation/GitHubPublic/_build/latest?definitionId=3&branchName=master)

hxcpp is the runtime support for the c++ backend of the [haxe](http://haxe.org/) compiler. This contains the headers, libraries and support code required to generate a fully compiled executable from haxe code.

## Installing from haxelib

```sh
haxelib install hxcpp
```

## Installing from git

```sh
haxelib git hxcpp https://github.com/HaxeFoundation/hxcpp
```

Alternatively, if you plan on modifying hxcpp for development, you can clone manually and use `haxelib dev`:

```sh
git clone https://github.com/HaxeFoundation/hxcpp
haxelib dev hxcpp ./hxcpp
```

### Build the tools

When installing from git, it is necessary to build the hxcpp build tool:

```sh
haxelib run hxcpp setup
```

### cppia

You first need to build the cppia host.

```sh
REPO=$(pwd)
cd ${REPO}/project
haxe compile-cppia.hxml
cd $REPO
```

Then you can do `haxelib run hxcpp file.cppia`.
