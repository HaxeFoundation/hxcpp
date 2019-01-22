package externs;

import cpp.UInt8;
import cpp.Pointer;

@:include("./../lib/LibInclude.h")
@:sourceFile("./../lib/RGB.cpp")
@:native("RGB")
extern class RGB
{
   public var r:UInt8;
   public var g:UInt8;
   public var b:UInt8;

   public function getLuma():Int;
   public function toInt():Int;

   @:native("new RGB")
   public static function create(r:Int, g:Int, b:Int):Pointer<RGB>;

   @:native("~RGB")
   public function deleteMe():Void;
}



// By extending RGB we keep the same API as far as haxe is concerned, but store the data (not pointer)
//  The native Reference class knows how to take the reference to the structure
@:native("cpp.Reference<RGB>")
extern class RGBRef extends RGB
{
}



// By extending RGBRef, we can keep the same api, 
//  rather than a pointer
@:native("cpp.Struct<RGB>")
extern class RGBStruct extends RGBRef
{
}



