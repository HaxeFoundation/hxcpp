package;
import gc.TestGC;

class TestMain {

	static function main(){
		var r = new haxe.unit.TestRunner();
		r.add(new TestTypes());
		r.add(new TestKeywords());
		r.add(new TestSort());
		r.add(new TestGC());
		r.add(new TestIntHash());
		r.add(new TestStringHash());
		r.add(new TestObjectHash());
		r.add(new TestWeakHash());
      #if cpp
		r.add(new native.TestFinalizer());
      #end
      var times:Int = 1;
      #if TEST_FLAKINESS
      times = 10;
      #end
      for (i in 0...times) {
         var t0 = haxe.Timer.stamp();
         var success = r.run();
         Sys.println("Time : " + (haxe.Timer.stamp()-t0)*1000 + " ms");
         if(!success) {
            Sys.exit(1);
         }
      }
      #if TEST_FLAKINESS
      Sys.println('SUCCESS $times/$times');
      #end
      Sys.exit(0);
	}
}
