package gc;
import haxe.io.Bytes;
import cpp.vm.Gc;

class CustomObject {
    public function new():Void {}
}

class TestGC extends haxe.unit.TestCase {
    public function testObject():Void {
        function innerFunction():Void {
            var object = { test: "abc" };
            Gc.doNotKill(object);
            Gc.run(true);
            assertTrue(Gc.getNextZombie() == null);
        }();
        Gc.run(true);
        var zombie = Gc.getNextZombie();
        assertTrue(zombie != null);
        assertEquals("abc", zombie.test);
    }

    public function testInt():Void {
        function innerFunction():Void {
            var object = 123;
            Gc.doNotKill(object);
            Gc.run(true);
            assertTrue(Gc.getNextZombie() == null);
        }();
        Gc.run(true);
        var zombie:Dynamic = Gc.getNextZombie();
        assertTrue(zombie != null);
        assertEquals(123, zombie);
    }

    public function testFunc():Void {
        function innerFunction():Void {
            var object = function() return "abc";
            Gc.doNotKill(object);
            Gc.run(true);
            assertTrue(Gc.getNextZombie() == null);
        }();
        Gc.run(true);
        var zombie = Gc.getNextZombie();
        assertTrue(zombie != null);
        assertEquals("abc", zombie());
    }

    public function testCustomObject():Void {
        function innerFunction():Void {
            var object = new CustomObject();
            Gc.doNotKill(object);
            Gc.run(true);
            assertTrue(Gc.getNextZombie() == null);
        }();
        Gc.run(true);
        var zombie = Gc.getNextZombie();
        assertTrue(zombie != null);
        assertTrue(Std.is(zombie, CustomObject));
    }

    public function testBytes():Void {
        function innerFunction():Void {
            var object = Bytes.alloc(1);
            Gc.doNotKill(object);
            Gc.run(true);
            assertTrue(Gc.getNextZombie() == null);
        }();
        Gc.run(true);
        var zombie = Gc.getNextZombie();
        assertTrue(zombie != null);
        assertTrue(Std.is(zombie, Bytes));
    }
}