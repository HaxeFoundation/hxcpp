

class TestPrime extends TestBase
{
   static var add = Loader.load("addInts", "iii" );
   #if cpp
   static var printString = Loader.load("printString", "cv" );
   #end
   static var distance3d = Loader.load("distance3D", "iiid" );
   static var fields = Loader.load("fields", "ov" );
   static var select = Loader.load("select", "iooooo" );
   static var floats = Loader.load("floats", "bfff" );
   static var stringVal = Loader.load("stringVal", "ss" );
   static var multi5 = Loader.load("multi5",  "iiiiii" );
   static var multi6 = Loader.load("multi6",  "iiiiiii" );
   static var multi7 = Loader.load("multi7",  "iiiiiiii" );
   static var multi8 = Loader.load("multi8",  "iiiiiiiii" );
   static var multi9 = Loader.load("multi9",  "iiiiiiiiii" );
   static var multi10 = Loader.load("multi10","iiiiiiiiiii" );
   static var multi11 = Loader.load("multi11","iiiiiiiiiiii" );
   static var multi12 = Loader.load("multi12","iiiiiiiiiiiii" );

   public function testPrime()
   {
      cpp.Prime.nekoInit("prime");

      assertTrue(add!=null);
      assertEquals(7, add.call(2,5));

      #if cpp
      printString.call("Hello World"); 
      #end

      var len = distance3d.call(3,4,5);
      assertClose(50,len*len);

      fields.call( { x:11, name:"Hello" } );

      assertEquals("Ok", stringVal.call("HxString"));

      assertEquals( ""+[1], ""+select.call(0, [1], "Hello", {x:1}, add) );
      assertEquals( ""+"Hello", ""+select.call(1, [1], "Hello", {x:1}, add));
      assertEquals( ""+{x:1}, ""+select.call(2, [1], "Hello", {x:1}, add) );
      assertEquals( ""+add, ""+select.call(3, [1], "Hello", {x:1}, add) );

      assertClose( 7.3, floats.call(true,4.2,3.1) );
      assertClose( 1.1, floats.call(false,4.2,3.1) );

      assertEquals( 5, multi5.call(1,1,1,1,1) );
      assertEquals( 6, multi6.call(1,1,1,1,1,1) );
      assertEquals( 7, multi7.call(1,1,1,1,1,1,1) );
      assertEquals( 8, multi8.call(1,1,1,1,1,1,1,1) );
      assertEquals( 9, multi9.call(1,1,1,1,1,1,1,1,1)  );
      assertEquals( 10, multi10.call(1,1,1,1,1,1,1,1,1,1) );
      assertEquals( 11, multi11.call(1,1,1,1,1,1,1,1,1,1,1) );
      assertEquals( 12, multi12.call(1,1,1,1,1,1,1,1,1,1,1,1) );
   }
}
