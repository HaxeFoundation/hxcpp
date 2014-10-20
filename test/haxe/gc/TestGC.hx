package gc;
import haxe.io.Bytes;
import cpp.vm.Gc;

class CustomObject {
	public function new():Void {}
}

class TestGC extends haxe.unit.TestCase {
	/**
		Repeatly call gc to get a zombie.
	*/
	function gc():Dynamic {
		var zombie = null;
		for (i in 0...20) {
			Sys.sleep(0.01);
			Gc.run(true);
			if ((zombie = Gc.getNextZombie()) != null) {
				return zombie;
			}
		}
		return zombie;
	}

	/**
		For avoiding the simple call being optimized in some way.
	*/
	function create(f:Void->Void):Void {
		f();
	}

	function createAbc():Void {
		var object = { test: "abc" };
		Gc.doNotKill(object);
	}
	public function testObject():Void {
		create(createAbc);
		var zombie = gc();
		assertTrue(zombie != null);
		assertEquals("abc", zombie.test);
		assertTrue(gc() == null);
	}

	function create123():Void {
		var object:Null<Int> = 123;
		Gc.doNotKill(object);
	};
	public function testBoxedInt():Void {
		create(create123);
		var zombie:Dynamic = gc();
		assertTrue(zombie != null);
		assertEquals(123, zombie);
		assertTrue(gc() == null);
	}

	function createFunction():Void {
		var object = function() return "abc";
		Gc.doNotKill(object);
	};
	public function testFunc():Void {
		create(createFunction);
		var zombie = gc();
		assertTrue(zombie != null);
		assertEquals("abc", zombie());
		assertTrue(gc() == null);
	}

	function createCustom():Void {
		var object = new CustomObject();
		Gc.doNotKill(object);
	};
	public function testCustomObject():Void {
		create(createCustom);
		var zombie = gc();
		assertTrue(zombie != null);
		assertTrue(Std.is(zombie, CustomObject));
		assertTrue(gc() == null);
	}

	function createBytes():Void {
		var object = Bytes.alloc(1);
		Gc.doNotKill(object);
	};
	public function testBytes():Void {
		create(createBytes);
		var zombie = gc();
		assertTrue(zombie != null);
		assertTrue(Std.is(zombie, Bytes));
		assertTrue(gc() == null);
	}
}
