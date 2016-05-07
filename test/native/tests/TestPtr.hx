package tests;

import NativeGen;
import cpp.NativeGc;
import cpp.Stdlib;
import cpp.Pointer;
using cpp.NativeArray;

@:unreflective
@:native("CVec")
extern class Vec {
	public var x:Float;
	public var y:Float;
	public var z:Float;
}


@:unreflective
@:structAccess
@:native("CVec")
extern class VecStruct {
	public var x:Float;
	public var y:Float;
	public var z:Float;

   @:native("new CVec")
   public static function create(val:Float) : Pointer<VecStruct>;

   public function set99(ioVec:VecStruct):Void { }
}

@:headerCode('
struct CVec{
   CVec(double inX=0) : x(inX), y(inX), z(inX) { }

	double x;
	double y;
	double z;

  void set99(CVec &ioVex) { ioVex.x=99; }
};')
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
	
    public function testExtened() {
      var test = NativeGc.allocateExtended( TestPtr, Stdlib.sizeof(Int) * 5 );
		var a : Pointer<Int> = cast Pointer.endOf(test);
      for(i in 0...5)
         a.setAt(i,i);
      for(i in 0...5)
         assertTrue( a.postIncRef() == i );
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

   private function anonOf(d:Dynamic) : Dynamic return {ptr:d};

   public function testStructAccess() {
      var e = VecStruct.create(1);
      var tmp = e.ptr;
      var tmp1 = e.ref;
      tmp.set99(tmp1);
      assertTrue(e.ptr.x==99);
   }

   public function testDynamic() {
      var a = [1];
      var intPtr = a.address(0);
      var d:Dynamic = intPtr;
      assertFalse(d==[2].address(0));
      assertTrue(d==a.address(0));
      var anon = anonOf(d);
      assertFalse([2].address(0)==d);
      assertTrue(a.address(0)==d);
      assertFalse(intPtr==[2].address(0));
      assertTrue(intPtr==a.address(0));
      assertFalse(anon.ptr==[2].address(0));
      assertTrue(anon.ptr==a.address(0));
      assertFalse([2].address(0)==anon.ptr);
      assertTrue(a.address(0)==anon.ptr);
   }
}
