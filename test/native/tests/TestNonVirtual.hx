package tests;

import utest.Test;
import utest.Assert;

class Base
{
   public function new() {}

   @:nonVirtual public function getNvName() return "Base";
   public function getName() return "Base";
}


class Derived extends Base
{
   @:nonVirtual override public function getNvName() return "Derived";
   override public function getName() return "Derived";
}


class TestNonVirtual extends Test
{
   public function testOverride()
   {
      var derived = new Derived();

      Assert.equals( "Derived", derived.getName() );
      Assert.equals( "Derived", derived.getNvName() );
      var closure:Dynamic = derived.getNvName;
      Assert.equals( "Derived", closure() );

      var base:Base = derived;

      Assert.equals( "Derived", base.getName());
      Assert.equals( "Base", base.getNvName() );
      var closure:Dynamic = base.getNvName;
      Assert.equals( "Base", closure() );
   }
}
