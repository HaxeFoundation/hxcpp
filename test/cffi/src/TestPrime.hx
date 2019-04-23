

class TestPrime extends TestBase
{
   static var add = Loader.load("addInts", "iii" );
   #if cpp
   static var printString = Loader.load("printString", "cv" );
   #end
   static var distance3d = Loader.load("distance3D", "iiid" );
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
   static var getNullString = Loader.load("getNullString","s" );
   static var addStrings = Loader.load("addStrings", 'sss');

   // Non-static test
   var fields = Loader.load("fields", "ov" );

   public function testPrime()
   {
      cpp.Prime.nekoInit("prime");

      assertTrue(add!=null);
      assertEquals(7, add(2,5));

      #if cpp
      printString("Hello World"); 
      #end

      var len = distance3d(3,4,5);
      assertClose(50,len*len);

      fields( { x:11, name:"Hello" } );
      fields( null );

      assertEquals("Ok", stringVal("HxString"));

      assertEquals(null, getNullString());

      assertEquals( ""+[1], ""+select(0, [1], "Hello", {x:1}, add) );
      var shouldBeNull:String = "" + select(0, null, "Hello", {x:1}, add);
      trace( "null ?" +  shouldBeNull + "/" + shouldBeNull.length );
      assertEquals( "null", shouldBeNull );
      //assertEquals( "null", ""+select(0, null, "Hello", {x:1}, add) );
      assertEquals( ""+"Hello", ""+select(1, [1], "Hello", {x:1}, add));
      assertEquals( ""+{x:1}, ""+select(2, [1], "Hello", {x:1}, add) );
      assertEquals( ""+add, ""+select(3, [1], "Hello", {x:1}, add) );

      assertClose( 7.3, floats(true,4.2,3.1) );
      assertClose( 1.1, floats(false,4.2,3.1) );

      assertEquals( 5, multi5(1,1,1,1,1) );
      assertEquals( 6, multi6(1,1,1,1,1,1) );
      assertEquals( 7, multi7(1,1,1,1,1,1,1) );
      assertEquals( 8, multi8(1,1,1,1,1,1,1,1) );
      assertEquals( 9, multi9(1,1,1,1,1,1,1,1,1)  );
      assertEquals( 10, multi10(1,1,1,1,1,1,1,1,1,1) );
      assertEquals( 11, multi11(1,1,1,1,1,1,1,1,1,1,1) );
      assertEquals( 12, multi12(1,1,1,1,1,1,1,1,1,1,1,1) );

      var s0 = "hello";
      var s1 = "こんにちは";
      assertTrue( addStrings(s0,s0) == s0+s0 );
      var s01 = addStrings(s0,s1);
      assertTrue( s01 == s0+s1 );
      var s11 = addStrings(s1,s1);
      assertTrue( s11 == s1+s1 );

   }
}
