package tests;

import utest.Test;
import utest.Assert;
import externs.Rectangle;

class TestRectangle extends Test
{
   static var statRect:Rectangle;
   static var statRectPtr:RectanglePtr;

   static var statRectProp(get,set):Rectangle;
   static var statRectPtrProp(get,set):RectanglePtr;

   var memRect:Rectangle;
   var memRectPtr:RectanglePtr;

   var memRectProp(get,set):Rectangle;
   var memRectPtrProp(get,set):RectanglePtr;

   static function get_statRectProp() return statRect;
   static function set_statRectProp(val:Rectangle) return statRect=val;
   static function get_statRectPtrProp() return statRectPtr;
   static function set_statRectPtrProp(val:RectanglePtr) return statRectPtr=val;


   function get_memRectProp() return memRect;
   function set_memRectProp(val:Rectangle) return memRect=val;
   function get_memRectPtrProp() return memRectPtr;
   function set_memRectPtrProp(val:RectanglePtr) return memRectPtr=val;


   public function testRect()
   {
      // Struct - copy semantic
      var rectangle = Rectangle.make(3,4);
      Assert.equals( 0, rectangle.area() );

      var rect2 = rectangle;
      rect2.width = 2;
      rect2.height = 4;
      Assert.equals( 8, rect2.area() );
      Assert.equals( 0, rectangle.area() );


      // Take address ...
      var rectPtr:RectanglePtr = rectangle;

      // Pointer-like sysntax
      rectPtr.width = 3;
      rectPtr.height = 5;
      var dynamicPtr:Dynamic  = rectPtr;

      Assert.equals( 15, rectPtr.area() );
      // Same object
      Assert.equals( 15, rectangle.area() );

      var dynamicCopy:Dynamic = rectangle; // 3,4  3x5

      rectangle.width = 10;
      rectangle.height = 10;
      Assert.equals( 100, rectangle.area() );

      // points to original object
      var fromDynamic:RectanglePtr = rectPtr;
      Assert.equals( 100, fromDynamic.area() );

      // Restore from Dynamic ...
      rectangle = dynamicCopy;
      Assert.equals( 15, rectangle.area() );
   }

   public function testReflect()
   {
      statRect = Rectangle.make(1,2,3,4);
      memRect = Rectangle.make(4,5,6,7);
      // This is not correct in the GC moving case ...
      //statRectPtr = memRect;
      statRectPtr = Rectangle.create(1,1,2,2);
      memRectPtr = statRect;

      Assert.equals( statRectProp.area(), 12 );
      Assert.equals( memRectProp.area(), 42 );
      Assert.equals( statRectPtrProp.area(), 4 );
      Assert.equals( memRectPtrProp.area(), 12 );

      var d:Dynamic = this;
      var r:Rectangle = d.memRect;
      Assert.equals( r.area(), 42 );
      var prop:Rectangle = Reflect.getProperty(d,"memRectProp");
      Assert.equals( prop.area(), 42 );
      var propPtr:RectanglePtr = Reflect.getProperty(d,"memRectPtrProp");
      Assert.equals( propPtr.area(), 12 );

      var d:Dynamic = TestRectangle;
      var r:Rectangle = d.statRect;
      Assert.equals( r.area(), 12 );
      var prop:Rectangle = Reflect.getProperty(d,"statRectProp");
      Assert.equals( prop.area(), 12 );
      var propPtr:RectanglePtr = Reflect.getProperty(d,"statRectPtrProp");
      Assert.equals( propPtr.area(), 4 );


      // No longer valid
      statRectPtr.delete();
      statRectPtr = null;
   }
}


