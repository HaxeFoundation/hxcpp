package;
import gc.TestGC;

class TestMain {

   static function main()
   {
      var passes = 1;
      var args = Sys.args();
      if (args.length>0)
         passes = Std.parseInt(args[0]);

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
      r.add(new file.TestFile());
      
      #if cpp
      r.add(new native.TestFinalizer());
      #end

      for(i in 0...passes)
      {
         var t0 = haxe.Timer.stamp();
         var success = r.run();
         trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
         if (!success)
            Sys.exit(1);
      }
      Sys.exit(0);
   }
}
