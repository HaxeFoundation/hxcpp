package tests;

import NativeGen;

class TestNativeGen extends haxe.unit.TestCase
{
   @:unreflective var unreflectiveValue:NativeGen;

   @:unreflective public function unreflectiveFunction(inGen:NativeGen)
   {
      unreflectiveValue = inGen;
      return unreflectiveValue.x==22;
   }

   public function testCreate()
   {
      var nGen:NativeGenStruct = null;
      nGen.x = 22;
      assertTrue(nGen.getValue()==22);

      assertTrue(unreflectiveFunction(nGen) );
   }
}
