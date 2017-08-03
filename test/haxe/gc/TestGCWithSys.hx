package gc;
import haxe.crypto.Md5;
class TestGCWithSys extends haxe.unit.TestCase {

    public function testShouldNotExplode():Void {
        for (i in 0...1000) {
            cpp.vm.Thread.create(function() {
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
            });
        }
        assertTrue(true);
    }
}
