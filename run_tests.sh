#!/bin/bash
set -e

cd ${BUILD_DIR}/tools/run
haxe compile.hxml

cd ${BUILD_DIR}/tools/hxcpp
haxe compile.hxml
cd ${BUILD_DIR}/project
neko build.n -verbose
haxe compile-cppia.hxml

cd ${BUILD_DIR}/test
haxe --run RunTests

cd ${HAXE_DIR}/tests/unit
haxe compile-cpp.hxml -D HXCPP_M32 && ./bin/cpp/Test-debug && rm -rf bin/cpp
haxe compile-cpp.hxml -D HXCPP_M64 && ./bin/cpp/Test-debug
haxe compile-cppia.hxml -debug && haxelib run hxcpp unit.cppia
