package gc;
import cpp.vm.Thread;
import haxe.crypto.Md5;
class TestGCWithSys extends haxe.unit.TestCase {

    public function testShouldNotExplode():Void {
        var main:Thread = Thread.current();
        var num:Int = 10000;
        for (i in 0...num) {
            cpp.vm.Thread.create(function() {
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                main.sendMessage(0);
            });
        }

        for (i in 0...num) {
           Thread.readMessage(true);
        }
        assertTrue(true);
    }
}
