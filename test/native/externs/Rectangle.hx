package externs;

import externs.RectangleApi;

@:include("./RectangleDef.h")
@:structAccess
@:native("Rectangle")
extern class Rectangle
{
   @:native("Rectangle::instanceCount")
   static var instanceCount:Int;

   public var x:Int;
   public var y:Int;
   public var width:Int;
   public var height:Int;

   public function area() : Int;

   @:native("new Rectangle")
   @:overload( function():cpp.Star<Rectangle>{} )
   @:overload( function(x:Int):cpp.Star<Rectangle>{} )
   @:overload( function(x:Int, y:Int):cpp.Star<Rectangle>{} )
   @:overload( function(x:Int, y:Int, width:Int):cpp.Star<Rectangle>{} )
   static function create(x:Int, y:Int, width:Int, height:Int):cpp.Star<Rectangle>;

   @:native("Rectangle")
   @:overload( function():Rectangle {} )
   @:overload( function(x:Int):Rectangle {} )
   @:overload( function(x:Int, y:Int):Rectangle {} )
   @:overload( function(x:Int, y:Int, width:Int):Rectangle {} )
   static function make(x:Int, y:Int, width:Int, height:Int):Rectangle;

   @:native("~Rectangle")
   public function delete():Void;
}

typedef RectanglePtr = cpp.Star<Rectangle>;
