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
      var t0 = haxe.Timer.stamp();
		var success = r.run();
      trace(" Time : " + (haxe.Timer.stamp()-t0)*1000 );
		Sys.exit(success ? 0 : 1);
	}
}
