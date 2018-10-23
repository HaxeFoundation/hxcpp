package impl;

import api.HaxeObject;

@:keep
class HaxeImpl implements HaxeObject
{
   var parentName:String;
   var name:String;

   public function new(?inParent:HaxeImpl)
   {
      parentName = inParent==null ? "" : inParent.name;
   }

   public function getName( ):cpp.StdString
   {
      return cpp.StdString.ofString(name);
   }

   public function setName( inName:cpp.StdStringRef ) : Void
   {
      name = inName.toString();
   }

   public function createChild() : HaxeObject
   {
      var child = new HaxeImpl(this);
      return child;
   }

   public function printInt(x:Int):Void
   {
      Sys.println( Std.string(x) );
   }
}



