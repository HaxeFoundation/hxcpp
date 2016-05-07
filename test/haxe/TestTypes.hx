class TestTypes extends haxe.unit.TestCase
{
   var i0:Int = 1;
   var i1:cpp.Int32 = 1;
   var i2:cpp.UInt32 = 1;
   var i3:cpp.Int64 = 1;
   var i4:cpp.UInt64 = 1;
   var i5:cpp.Int8 = 1;
   var i6:cpp.UInt8 = 1;
   var i7:cpp.Int16 = 1;
   var i8:cpp.UInt16 = 1;

   public function new() super();


   public function test()
   {
      var d:Dynamic = this;
      assertTrue(d.i0==1);
      assertTrue(d.i1==1);
      assertTrue(d.i2==1);
      assertTrue(d.i3==1);
      assertTrue(d.i4==1);
      assertTrue(d.i5==1);
      assertTrue(d.i6==1);
      assertTrue(d.i7==1);
   }
}
