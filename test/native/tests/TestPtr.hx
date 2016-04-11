package tests;

import NativeGen;

@:unreflective
@:native("CVec")
extern class Vec {
	public var x:Float;
	public var y:Float;
	public var z:Float;
}

@:headerCode('
typedef struct CVec{
	double x;
	double y;
	double z;
} CVec;')
class TestPtr extends haxe.unit.TestCase{
	
	@:generic
	public static inline function malloc<T>(size:Int) : cpp.Pointer<T>{
		var p : cpp.RawPointer<cpp.Void> = untyped __cpp__("malloc({0})",size);
		return cast cpp.Pointer.fromRaw<T>( cast p );
	}
	
    public function testMalloc() {
		var a : cpp.Pointer<Vec> = malloc(3 * 4);
		a.ptr.x = 66;
		assertTrue( a.ptr.x == 66 );
	}
}