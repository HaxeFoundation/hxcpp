package;
import gc.TestGC;
class TestMain {

    static function main(){
        var r = new haxe.unit.TestRunner();
        r.add(new TestGC());
        r.run();
    }
}