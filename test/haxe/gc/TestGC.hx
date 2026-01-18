package gc;

import utest.Test;
import utest.Assert;
import haxe.io.Bytes;
import cpp.vm.Gc;

class CustomObject {
	public function new():Void {}
}

class TestGC extends Test {
	function createDummy(val:Dynamic):Dynamic {
      return { dummy: val };
   }

	function gc():Dynamic {
      Gc.run(true);
		return Gc.getNextZombie();
	}

	/**
		For avoiding the simple call being optimized in some way.
	*/
	function create(f:Void->Void):Void {
		f();
      clearStack(10);
	}

   function clearStack(count:Int, ?nothing:Dynamic):Dynamic
   {
      if (count==0)
         return 0;
      return clearStack(count-1);
   }

   // NOTE : previously the objects below were created in the same thread as the assertions and the clear
   // stack function above attempted to remove references to it so it was eligable for collection.
   // With the callable changes it seems clang can do some more aggressive optimisations which broke these tests,
   // so now the objects are created on a separate thread and we sleep for 1s to give time for the threads to exit and unregister from the GC.

	function createAbc():Void {
		var object = { test: "abc" };
		Gc.doNotKill(object);
	}
	public function testObject():Void {
		sys.thread.Thread.create(createAbc);
		Sys.sleep(1);
		var zombie:Dynamic = gc();
		Assert.notNull(zombie);
		Assert.equals("abc", zombie.test);
		Assert.isNull(gc());
	}

   // Null<int> for numbers < 256 are special cases
   // Infact, there are no guarantees that Null<Int> will be actual objects in the future
   /*
	function create1234():Void {
		var object:Null<Int> = 1234;
		Gc.doNotKill(object);
	};
	public function testBoxedInt():Void {
		create(create1234);
		var zombie:Dynamic = gc();
		assertTrue(zombie != null);
		assertEquals(1234, zombie);
		assertTrue(gc() == null);
	}
   */

	function createFunction():Void {
		var object = function() return "abc";
		Gc.doNotKill(object);
	};
	public function testFunc():Void {
		sys.thread.Thread.create(createFunction);
		Sys.sleep(1);
		var zombie:Dynamic = gc();
		Assert.notNull(zombie);
		Assert.equals("abc", zombie());
		Assert.isNull(gc());
	}

	function createCustom():Void {
		var object = new CustomObject();
		Gc.doNotKill(object);
	};
	public function testCustomObject():Void {
		sys.thread.Thread.create(createCustom);
		Sys.sleep(1);
		var zombie = gc();
		Assert.notNull(zombie);
		Assert.isOfType(zombie, CustomObject);
		Assert.isNull(gc());
	}

	function createBytes():Void {
		var object = Bytes.alloc(1);
		Gc.doNotKill(object);
	};
	public function testBytes():Void {
		sys.thread.Thread.create(createBytes);
		Sys.sleep(1);
		var zombie = gc();
		Assert.notNull(zombie);
		Assert.isOfType(zombie, Bytes);
		Assert.isNull(gc());
	}

	public function testBigStack():Void {
		Assert.isTrue( TestBigStack.test() );
   	}

   #if !cppia
	public function testConstStrings():Void {
		// Const strings void Gc overhead
		var strings = new Array<String>();
		strings.push( haxe.Resource.getString("TestMain.hx") );
		strings.push( "some string" );
		var chars = "abc123";
		// Optimization for single chars...
		for(c in 0...chars.length)
		strings.push( chars.substr(c,1) );
		for(string in strings)
		Assert.isTrue( untyped __global__.__hxcpp_is_const_string(string) );
		Gc.run(true);
		for(string in strings)
		Assert.isTrue( untyped __global__.__hxcpp_is_const_string(string) );

		var strings = new Array<String>();
		strings.push( haxe.Resource.getString("TestMain.hx").substr(10) );
		strings.push( "some string" + chars );
		for(c in 0...chars.length-1)
			strings.push( chars.substr(c,2) );

		for(string in strings)
			Assert.isFalse( untyped __global__.__hxcpp_is_const_string(string) );
   }
   #end
}
