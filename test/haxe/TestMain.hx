package;
import gc.TestGC;
class TestMain {

    static function main(){
        var r = new haxe.unit.TestRunner();
        r.add(new TestGC());
        var success = r.run();
        Sys.exit(success ? 0 : 1);
    }
}