package tests;

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


class TestNonVirtual extends haxe.unit.TestCase
{
   public function testOverride()
   {
      var derived = new Derived();

      assertTrue( derived.getName() == "Derived" );
      assertTrue( derived.getNvName() == "Derived" );
      var closure:Dynamic = derived.getNvName;
      assertTrue( closure() == "Derived" );

      var base:Base = derived;

      assertTrue( base.getName() == "Derived" );
      assertTrue( base.getNvName() == "Base" );
      var closure:Dynamic = base.getNvName;
      assertTrue( closure() == "Base" );
   }
}
