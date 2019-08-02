package file;

class TestFile extends haxe.unit.TestCase
{
   public function testGetContentEmptyFile()
   {
      assertEquals('', sys.io.File.getContent('./file/empty.txt'));
   }

}
