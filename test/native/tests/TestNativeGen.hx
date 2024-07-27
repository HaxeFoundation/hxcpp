package tests;

import NativeGen;
import utest.Test;
import utest.Assert;

class TestNativeGen extends Test
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
      Assert.equals(22f64, nGen.getValue());
      Assert.isTrue(unreflectiveFunction(nGen));
   }
}
