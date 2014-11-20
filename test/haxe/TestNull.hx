class TestNull extends haxe.unit.TestCase
{
   public function test()
   {
      var err = "";
      try
      {
        #if neko
	var _test = neko.Lib.load("test","test_null",0);
        var _init = neko.Lib.load("test","neko_init",5);
        _init(function(s) return new String(s), function(len:Int) { var r = []; if (len > 0) r[len - 1] = null; return r; }, null, true, false);
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
