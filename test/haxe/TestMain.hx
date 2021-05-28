package;
import gc.TestGC;

class TestMain #if nme extends nme.display.Sprite #end {

   static function runTests():Int
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
      #if !nme
      r.add(new gc.TestGCThreaded());
      #end
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
            return 1;
      }
      return 0;
   }

   #if nme
   var frameCount = 0;
   var tf:nme.text.TextField;
   public function new()
   {
       super();
       tf = new nme.text.TextField();
       tf.text="RUN...";
       addChild(tf);
       addEventListener( nme.events.Event.ENTER_FRAME, onFrame );
   }

   function onFrame(_)
   {
      var err = runTests();
      tf.text = "" + (++frameCount);
      stage.opaqueBackground = err==0 ? 0xff00ff00: 0xffff0000;

   }

   static function endTest(error:Int) trace(error==0 ? "All tests OK" : "Tests Failed!");
   #else
   public static function main()
   {
      Sys.exit(runTests());
   }
   public static function endTest(error:Int) Sys.exit(error);
   #end
}


