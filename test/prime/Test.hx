class Test
{
   public static function __init__()
   {
      untyped __global__.__hxcpp_push_dll_path( "project/ndll/" + __global__.__hxcpp_get_bin_dir() );
   }
   static var add = cpp.Lib.loadPrime("prime", "addInts", "iii" );



   public static function main()
   {
      trace(add);
      var x = add.call(2,5);
      trace(x);
   }
}
