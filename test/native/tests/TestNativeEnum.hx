package tests;

import utest.Test;
import utest.Assert;

// Uses native enum, which does not play nice with Dynamic - must use @:unreflective
@:unreflective
#if (haxe_ver >= 4.0) extern enum #else @:extern @:enum #end abstract SystemMetric(SystemMetricImpl) {
    @:native("wxSYS_MOUSE_BUTTONS")      var MOUSE_BUTTONS;
    @:native("wxSYS_OS")      var OS;
}
@:unreflective
@:native("wxSystemMetric")
extern class SystemMetricImpl { }



// Wraps enum in struct, which does play nice...
#if (haxe_ver >= 4.0) extern enum #else @:extern @:enum #end abstract SystemMetricStruct(SystemMetricStructImpl) {
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
class TestNativeEnum extends Test
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
      Assert.isTrue( isX(SystemMetric.MOUSE_BUTTONS)==true );
      Assert.isTrue( isX(SystemMetric.OS)==false );
      Assert.isTrue( isXStruct(SystemMetricStruct.MOUSE_BUTTONS)==true );
      Assert.isTrue( isXStruct(SystemMetricStruct.OS)==false );
      var d:Dynamic = this;
      Assert.isNull( d.x );
      Assert.notNull( d.xStruct );
      Assert.isNull( d.isX );
      Assert.notNull( d.isXStruct );
      var func: (SystemMetricStruct)->Bool = d.isXStruct;
      Assert.notNull(func);
      Assert.isTrue(func(SystemMetricStruct.MOUSE_BUTTONS)==true );
      Assert.isTrue(func(SystemMetricStruct.OS)==false );
   }

}


