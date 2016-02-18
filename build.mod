cd tools/hxcpp 
haxe compile.hxml
cd ../build
haxe compile.hxml
cd ../../project
neko build.n clean
neko build.n
