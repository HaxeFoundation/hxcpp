
class TestMain
{
   public static function main()
   {
      var r = new haxe.unit.TestRunner();
      r.add(new TestPrime());
      r.add(new TestCffi());

      var t0 = haxe.Timer.stamp();
      var success = r.run();
      trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
      Sys.exit(success ? 0 : 1);
   }
}


