package gc;
import cpp.vm.Thread;
import haxe.crypto.Md5;
class TestGCWithSys extends haxe.unit.TestCase {

    public function testShouldNotExplode():Void {
        var main:Thread = Thread.current();
       
        for (i in 0...10000) {
            cpp.vm.Thread.create(function() {
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                Md5.encode(sys.io.File.getContent('gc/testShouldNotExplode.txt'));
                main.sendMessage(0);
            });
        }

        for (i in 0...256) {
           Thread.readMessage(true);
        }
        assertTrue(true);
    }
}
