package gc;
import haxe.io.Bytes;
import cpp.vm.Gc;
class TestGC extends haxe.unit.TestCase {

    public function testObjectLeak():Void {
        var object:CustomObject = new CustomObject();
        Gc.doNotKill(object);
        Gc.run(true);
        assertTrue(isLeak());
        object = null;
        Gc.run(true);
        assertFalse(isLeak());
    }

    public function testObjectNonLeak():Void {
        var object:CustomObject = new CustomObject();
        Gc.doNotKill(object);
        object = null;
        Gc.run(true);
        assertFalse(Gc.getNextZombie() == null);
    }

    public function testBytesLeak():Void {
        var object:Bytes = Bytes.alloc(1);
        Gc.doNotKill(object);
        Gc.run(true);
        assertTrue(isLeak());
        object = null;
        Gc.run(true);
        assertFalse(isLeak());
    }

    function isLeak():Bool {
        return Gc.getNextZombie() == null;
    }
}