package file;

import utest.Test;
import utest.Assert;

class TestFile extends Test
{
   public function testGetContentEmptyFile()
   {
      Assert.equals('', sys.io.File.getContent('./file/empty.txt'));
   }
}