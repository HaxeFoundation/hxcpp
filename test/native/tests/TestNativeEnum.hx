package tests;

// Uses native enum, which does not play nice with Dynamic - must use @:unreflective
@:unreflective
@:enum extern abstract SystemMetric(SystemMetricImpl) {
    @:native("wxSYS_MOUSE_BUTTONS")      var MOUSE_BUTTONS;
    @:native("wxSYS_OS")      var OS;
}
@:unreflective
@:native("wxSystemMetric")
extern class SystemMetricImpl { }



// Wraps enum in struct, which does play nice...
@:enum extern abstract SystemMetricStruct(SystemMetricStructImpl) {
    @:native("wxSYS_MOUSE_BUTTONS")      var MOUSE_BUTTONS;
    @:native("wxSYS_OS")      var OS;
}
@:native("cpp::Struct<wxSystemMetric, cpp::EnumHandler>")
extern class SystemMetricStructImpl { }

@:headerCode('
enum wxSystemMetric
{
   wxSYS_OS = 3,
   wxSYS_MOUSE_BUTTONS = 27,
};
')
class TestNativeEnum extends haxe.unit.TestCase
{
   var x:SystemMetric = SystemMetric.MOUSE_BUTTONS;
   var xStruct:SystemMetricStruct = SystemMetricStruct.MOUSE_BUTTONS;

   function isX(val:SystemMetric)
   {
      return (val==x);
   }

   function isXStruct(val:SystemMetricStruct)
   {
      return (val==xStruct);
   }


   public function test()
   {
      assertTrue( isX(SystemMetric.MOUSE_BUTTONS)==true );
      assertTrue( isX(SystemMetric.OS)==false );
      assertTrue( isXStruct(SystemMetricStruct.MOUSE_BUTTONS)==true );
      assertTrue( isXStruct(SystemMetricStruct.OS)==false );
      var d:Dynamic = this;
      assertTrue( d.x==null );
      assertTrue( d.xStruct!=null );
      assertTrue( d.isX==null );
      assertTrue( d.isXStruct!=null );
      var func = d.isXStruct;
      assertTrue(func!=null);
      assertTrue(func(SystemMetricStruct.MOUSE_BUTTONS)==true );
      assertTrue(func(SystemMetricStruct.OS)==false );
   }

}


