package tests;

import NativeGen;
import cpp.Stdlib;
import cpp.Pointer;

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
	
   /*
    Alternate version
	@:generic
	public static inline function malloc<T>(size:Int) : cpp.Pointer<T>{
		var p : cpp.RawPointer<cpp.Void> = untyped __cpp__("malloc({0})",size);
		return cast cpp.Pointer.fromRaw( cast p );
	}
   */
	
    public function testMalloc() {
		var a : Pointer<Vec> = Stdlib.malloc( Stdlib.sizeof(Vec) );
      assertTrue( a!=null );
      assertTrue( a.raw!=null );
		a.ptr.x = 66;
		assertTrue( a.ptr.x == 66 );
      Stdlib.free(a);
	}
   public function testNull() {
		var nullP : Pointer<Vec> = null;
		var nullRawP = nullP.raw;
      assertTrue( nullP==null );
      assertTrue( null==nullP );
      assertFalse( nullP!=null );
      assertTrue( nullRawP==null );
      assertFalse( nullRawP!=null );
      nullRawP = null;
      assertTrue( nullRawP==null );
	}
}
