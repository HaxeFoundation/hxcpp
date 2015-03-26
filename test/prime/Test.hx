
class Test
{
   static var add = Loader.load("addInts", "iii" );
   #if cpp
   static var print = Loader.load("printString", "cv" );
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

   public static function main()
   {
      cpp.Prime.nekoInit("prime");

      trace(add);
      var x = add.call(2,5);
      trace(x);

      #if cpp
      print.call("Hello World"); 
      #end

      var len = distance3d.call(3,4,5);
      trace(len*len);

      fields.call( { x:11, name:"Hello" } );

      var result = stringVal.call("HxString");
      trace(result);

      for(i in 0...4)
         trace( select.call(i, [1], "Hello", {x:1}, add) );

      trace( floats.call(true,4.2,3.1) );
      trace( floats.call(false,4.2,3.1) );

      trace( multi5.call(1,1,1,1,1) );
      trace( multi6.call(1,1,1,1,1,1) );
      trace( multi7.call(1,1,1,1,1,1,1) );
      trace( multi8.call(1,1,1,1,1,1,1,1) );
      trace( multi9.call(1,1,1,1,1,1,1,1,1) );
      trace( multi10.call(1,1,1,1,1,1,1,1,1,1) );
      trace( multi11.call(1,1,1,1,1,1,1,1,1,1,1) );
      trace( multi12.call(1,1,1,1,1,1,1,1,1,1,1,1) );
   }
}
