package;

class Native
{
	static function main()
   {
		var r = new haxe.unit.TestRunner();
		r.add(new tests.TestRgb());
      var t0 = haxe.Timer.stamp();
		var success = r.run();
      trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
		Sys.exit(success ? 0 : 1);
	}
}
