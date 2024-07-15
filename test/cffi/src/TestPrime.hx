import utest.Assert;
import utest.Test;

class TestPrime extends Test
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

      Assert.notNull(add);
      Assert.equals(7, add(2,5));

      #if cpp
      printString("Hello World"); 
      #end

      var len = distance3d(3,4,5);
      Assert.floatEquals(50,len*len, 0.001);

      fields( { x:11, name:"Hello" } );
      fields( null );

      Assert.equals("Ok", stringVal("HxString"));

      Assert.isNull(getNullString());

      Assert.equals( ""+[1], ""+select(0, [1], "Hello", {x:1}, add) );
      var shouldBeNull:String = "" + select(0, null, "Hello", {x:1}, add);
      trace( "null ?" +  shouldBeNull + "/" + shouldBeNull.length );
      Assert.equals( "null", shouldBeNull );
      //Assert.equals( "null", ""+select(0, null, "Hello", {x:1}, add) );
      Assert.equals( ""+"Hello", ""+select(1, [1], "Hello", {x:1}, add));
      Assert.equals( ""+{x:1}, ""+select(2, [1], "Hello", {x:1}, add) );
      Assert.equals( ""+add, ""+select(3, [1], "Hello", {x:1}, add) );

      Assert.floatEquals( 7.3, floats(true,4.2,3.1), 0.001 );
      Assert.floatEquals( 1.1, floats(false,4.2,3.1), 0.001 );

      Assert.equals( 5, multi5(1,1,1,1,1) );
      Assert.equals( 6, multi6(1,1,1,1,1,1) );
      Assert.equals( 7, multi7(1,1,1,1,1,1,1) );
      Assert.equals( 8, multi8(1,1,1,1,1,1,1,1) );
      Assert.equals( 9, multi9(1,1,1,1,1,1,1,1,1)  );
      Assert.equals( 10, multi10(1,1,1,1,1,1,1,1,1,1) );
      Assert.equals( 11, multi11(1,1,1,1,1,1,1,1,1,1,1) );
      Assert.equals( 12, multi12(1,1,1,1,1,1,1,1,1,1,1,1) );

      var s0 = "hello";
      var s1 = "こんにちは";
      Assert.isTrue( addStrings(s0,s0) == s0+s0 );
      var s01 = addStrings(s0,s1);
      Assert.isTrue( s01 == s0+s1 );
      var s11 = addStrings(s1,s1);
      Assert.isTrue( s11 == s1+s1 );

   }
}
