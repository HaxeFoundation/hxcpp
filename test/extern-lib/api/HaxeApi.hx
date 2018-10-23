package api;

@:nativeGen
@:structAccess
class HaxeApi
{
   @:keep
   public static function createBase( ) : HaxeObject  return new impl.HaxeImpl();
}

