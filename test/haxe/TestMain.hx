package;

import utest.Runner;
import utest.ui.Report;
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

      final runner = new Runner();
      runner.addCase(new TestTypes());
      runner.addCase(new TestKeywords());
      runner.addCase(new TestSort());
      runner.addCase(new TestGC());
      #if !nme
      runner.addCase(new gc.TestGCThreaded());
      #end
      runner.addCase(new TestIntHash());
      runner.addCase(new TestStringHash());
      runner.addCase(new TestObjectHash());
      runner.addCase(new TestWeakHash());
      #if !nme
      runner.addCase(new file.TestFile());
      #end
      
      #if cpp
      runner.addCase(new native.TestFinalizer());
      #end

      final report = Report.create(runner);

      runner.run();

      for(i in 0...passes)
      {
         var t0 = haxe.Timer.stamp();
         runner.run();
         trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
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

   #else
   public static function main()
   {
      utest.UTest.run([
         new TestTypes(),
         new TestKeywords(),
         new TestSort(),
         new TestGC(),
         new gc.TestGCThreaded(),
         new TestIntHash(),
         new TestStringHash(),
         new TestObjectHash(),
         new TestWeakHash(),
         new file.TestFile(),
         new native.TestFinalizer()
      ]);
   }
   #end
}


