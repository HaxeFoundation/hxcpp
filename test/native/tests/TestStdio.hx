package tests;
import cpp.Stdio;
using cpp.NativeArray;

class TestStdio extends haxe.unit.TestCase
{
   public function test()
   {
      var file = Stdio.fopen("test.txt", "wb");
      var ints = [1];
      var size:cpp.SizeT = cpp.Stdlib.sizeof(Int);
      Stdio.fwrite( ints.address(0).raw, size, 1, file );
      Stdio.fclose(file);

      var bytes = sys.io.File.getBytes("test.txt");
      var input = new haxe.io.BytesInput(bytes);
      var val = input.readInt32();

      assertTrue(val==ints[0]);
   }
}


