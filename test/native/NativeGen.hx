
@:nativeGen
@:structAccess
class NativeGen
{
  public var x:Float;
  public static var y:Int;
  public function  getValue():Float return x;

  public static function someNativeFunction()
  {
     trace("Hi!");
  }
}

@:native("cpp::Struct<NativeGen>")
@:include("NativeGen.h")
extern class NativeGenStruct extends NativeGen
{
}


