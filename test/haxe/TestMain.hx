package;
#if cpp
import gc.TestGC;
#end

class TestMain {

	static function main(){
		var r = new haxe.unit.TestRunner();
		r.add(new TestNull());
		#if cpp
		r.add(new TestGC());
		r.add(new TestIntHash());
                #end
		var success = r.run();
		Sys.exit(success ? 0 : 1);
	}
}
