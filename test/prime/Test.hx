class Test
{
   static var add = cpp.Lib.loadPrime("project/ndll/Windows/prime.dll", "addInts", "iii" );



   public static function main()
   {
      trace(add);
      var x = add.call(2,5);
      trace(x);
   }
}
