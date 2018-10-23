package tests;
import externs.Rectangle;

class TestRectangle extends haxe.unit.TestCase
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
      assertTrue( rectangle.area()==0 );

      var rect2 = rectangle;
      rect2.width = 2;
      rect2.height = 4;
      assertTrue( rect2.area()==8 );
      assertTrue( rectangle.area()==0 );


      // Take address ...
      var rectPtr:RectanglePtr = rectangle;

      // Pointer-like sysntax
      rectPtr.width = 3;
      rectPtr.height = 5;
      var dynamicPtr:Dynamic  = rectPtr;

      assertTrue( rectPtr.area()==15 );
      // Same object
      assertTrue( rectangle.area()==15 );

      var dynamicCopy:Dynamic = rectangle; // 3,4  3x5

      rectangle.width = 10;
      rectangle.height = 10;
      assertTrue( rectangle.area()==100 );

      // points to original object
      var fromDynamic:RectanglePtr = rectPtr;
      assertTrue( fromDynamic.area()==100 );

      // Restore from Dynamic ...
      rectangle = dynamicCopy;
      assertTrue( rectangle.area()==15 );
   }

   public function testReflect()
   {
      statRect = Rectangle.make(1,2,3,4);
      memRect = Rectangle.make(4,5,6,7);
      // This is not correct in the GC moving case ...
      //statRectPtr = memRect;
      statRectPtr = Rectangle.create(1,1,2,2);
      memRectPtr = statRect;

      assertTrue( statRectProp.area()==12 );
      assertTrue( memRectProp.area()==42 );
      assertTrue( statRectPtrProp.area()==4 );
      assertTrue( memRectPtrProp.area()==12 );

      var d:Dynamic = this;
      var r:Rectangle = d.memRect;
      assertTrue( r.area()==42 );
      var prop:Rectangle = Reflect.getProperty(d,"memRectProp");
      assertTrue( prop.area()==42 );
      var propPtr:RectanglePtr = Reflect.getProperty(d,"memRectPtrProp");
      assertTrue( propPtr.area()==12 );

      var d:Dynamic = TestRectangle;
      var r:Rectangle = d.statRect;
      assertTrue( r.area()==12 );
      var prop:Rectangle = Reflect.getProperty(d,"statRectProp");
      assertTrue( prop.area()==12 );
      var propPtr:RectanglePtr = Reflect.getProperty(d,"statRectPtrProp");
      assertTrue( propPtr.area()==4 );


      // No longer valid
      statRectPtr.delete();
      statRectPtr = null;
   }
}


