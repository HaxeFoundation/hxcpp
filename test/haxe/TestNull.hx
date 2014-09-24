class TestNull extends haxe.unit.TestCase
{
   public function test()
   {
      var err = "";
      try
      {
        #if neko
	var _test = neko.Lib.load("test","test_null",0);
        #else
	var _test = cpp.Lib.load("test","test_null",0);
        #end

	_test();
      }
      catch(e:String)
      {
         err = e;
      }
      assertEquals("", err);
   }

}
