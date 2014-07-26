package;
import gc.TestGC;

class TestMain {

	static function main(){
		var r = new haxe.unit.TestRunner();
		r.add(new TestGC());
		r.add(new TestIntHash());
		var success = r.run();
		Sys.exit(success ? 0 : 1);
	}
}
