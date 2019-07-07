class TestString extends haxe.unit.TestCase
{
   public function testEuro()
   {
      var string = "€";
      assertEquals("€", string.charAt(0));
   }

}
