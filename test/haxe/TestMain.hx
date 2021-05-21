package;
import gc.TestGC;

class TestMain #if nme extends nme.display.Sprite #end {

   static function runTests()
   {
      var passes = 1;
      #if !nme
      var args = Sys.args();
      if (args.length>0)
         passes = Std.parseInt(args[0]);
      #end

      var r = new haxe.unit.TestRunner();
      r.add(new TestTypes());
      r.add(new TestKeywords());
      r.add(new TestSort());
      r.add(new TestGC());
      r.add(new gc.TestGCThreaded());
      r.add(new TestIntHash());
      r.add(new TestStringHash());
      r.add(new TestObjectHash());
      r.add(new TestWeakHash());
      #if !nme
      r.add(new file.TestFile());
      #end
      
      #if cpp
      r.add(new native.TestFinalizer());
      #end

      for(i in 0...passes)
      {
         var t0 = haxe.Timer.stamp();
         var success = r.run();
         trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
         if (!success)
         {
            endTest(1);
            return;
         }
      }
      endTest(0);
   }

   #if nme
   public function new()
   {
       super();
       runTests();
   }
   static function endTest(error:Int) trace(error==0 ? "All tests OK" : "Tests Failed!");
   #else
   public static function main() runTests();
   public static function endTest(error:Int) Sys.exit(error);
   #end
}


