package api;

@:nativeGen
interface HaxeObject
{
   public function getName( ):cpp.StdString;
   public function setName( inName:cpp.StdStringRef ) : Void;
   public function createChild() : HaxeObject;
   public function printInt(x:Int):Void;
}


