package gc;
import haxe.io.Bytes;
import cpp.vm.Gc;

class CustomObject {
	public function new():Void {}
}

class TestGC extends haxe.unit.TestCase {
	static var delaySeconds:Float = 2;

	function createAbc():Void {
		var object = { test: "abc" };
		Gc.doNotKill(object);
	}
	public function testObject():Void {
		createAbc();
		Sys.sleep(delaySeconds);
		Gc.run(true);
		var zombie = Gc.getNextZombie();
		assertTrue(zombie != null);
		assertEquals("abc", zombie.test);
		Gc.run(true);
		assertTrue(Gc.getNextZombie() == null);
	}

	function create123():Void {
		var object:Null<Int> = 123;
		Gc.doNotKill(object);
	};
 
	public function testBoxedInt():Void {
		create123();
		Sys.sleep(delaySeconds);
		Gc.run(true);
		var zombie:Dynamic = Gc.getNextZombie();
		assertTrue(zombie != null);
		assertEquals(123, zombie);
		Gc.run(true);
		assertTrue(Gc.getNextZombie() == null);
	}

	function createFunction():Void {
		var object = function() return "abc";
		Gc.doNotKill(object);
	};

	public function testFunc():Void {
		createFunction();
		Sys.sleep(delaySeconds);
		Gc.run(true);
		var zombie = Gc.getNextZombie();
		assertTrue(zombie != null);
		assertEquals("abc", zombie());
		Gc.run(true);
		assertTrue(Gc.getNextZombie() == null);
	}

	function createCustom():Void {
		var object = new CustomObject();
		Gc.doNotKill(object);
	};
 
	public function testCustomObject():Void {
		createCustom();
		Sys.sleep(delaySeconds);
		Gc.run(true);
		var zombie = Gc.getNextZombie();
		assertTrue(zombie != null);
		assertTrue(Std.is(zombie, CustomObject));
		Gc.run(true);
		assertTrue(Gc.getNextZombie() == null);
	}

	function createBytes():Void {
		var object = Bytes.alloc(1);
		Gc.doNotKill(object);
	};
  

	public function testBytes():Void {
		createBytes();
		Sys.sleep(delaySeconds);
		Gc.run(true);
		var zombie = Gc.getNextZombie();
		assertTrue(zombie != null);
		assertTrue(Std.is(zombie, Bytes));
		Gc.run(true);
		assertTrue(Gc.getNextZombie() == null);
	}
}
