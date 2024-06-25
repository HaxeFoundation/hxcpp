package tests;

import utest.Test;
import utest.Assert;
import cpp.Stdio;

using cpp.NativeArray;

class TestStdio extends Test
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

      Assert.equals(val, ints[0]);
   }
}


