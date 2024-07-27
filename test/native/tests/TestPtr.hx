package tests;

import utest.Test;
import utest.Assert;
import NativeGen;
import cpp.NativeGc;
import cpp.Stdlib;
import cpp.Pointer;
import cpp.RawPointer;
import cpp.UInt8;
using cpp.NativeArray;
using cpp.NativeString;

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
extern class VecStructAccess {
   public var x:Float;
   public var y:Float;
   public var z:Float;

   @:native("new CVec")
   public static function create(val:Float) : Pointer<VecStructAccess>;

   public function set99(ioVec:VecStructAccess):Void;
}


@:unreflective
@:native("cpp::Struct<CVec>")
extern class VecStruct {
   public var x:Float;
   public var y:Float;
   public var z:Float;
}

@:native("::SomeStruct")
extern class Native_SomeStruct {
    var data:RawPointer<cpp.UInt8>;
    var dataLength:Int;

    inline function getDataBytes():haxe.io.Bytes {
        var bdata:haxe.io.BytesData = [];
        cpp.NativeArray.setData(bdata, cast data, dataLength);  // Null Function Pointer
        return haxe.io.Bytes.ofData(bdata);
    }

    inline function getUnmanagedDataBytes():haxe.io.Bytes {
        var bdata:haxe.io.BytesData = [];
        cpp.NativeArray.setUnmanagedData(bdata, cast data, dataLength);  // Null Function Pointer
        return haxe.io.Bytes.ofData(bdata);
    }

}
@:native("::cpp::Reference<SomeStruct>")
extern class SomeStructRef extends Native_SomeStruct {}
@:native("::cpp::Struct<SomeStruct>")
extern class SomeStruct extends SomeStructRef {}

class IntHolder
{
   public var ival:Int;

   public function new(inVal:Int = 1) ival = inVal;
}

@:headerCode('
struct CVec{
   CVec(double inX=0) : x(inX), y(inX), z(inX) { }

   double x;
   double y;
   double z;

  void set99(CVec &ioVex) { ioVex.x=99; }
};

  struct SomeStruct {
     SomeStruct() : data((unsigned char *)"Hi!"), dataLength(3) { }

     unsigned char *data;
     int dataLength;
  };
')
@:cppFileCode('
  int callPointer(CVec *) { return 5; }
')
class TestPtr extends Test
{   
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
      Assert.notNull( a );
      Assert.notNull( a.raw );
      a.ptr.x = 66;
      Assert.equals( 66f64, a.ptr.x );
      Stdlib.free(a);
   }
   
    public function testExtened() {
      var test = NativeGc.allocateExtended( TestPtr, Stdlib.sizeof(Int) * 5 );
      var a : Pointer<Int> = cast Pointer.endOf(test);
      for(i in 0...5)
         a.setAt(i,i);
      for(i in 0...5)
         Assert.equals( i, a.postIncRef() );
   }

   function test9194() {
      // will fail during C++ compile
      var buffer: cpp.RawPointer<cpp.Void> = null;
      var floatBuffer: cpp.RawPointer<cpp.Float32> = cast buffer;
      // generates incorrect: float* floatBuffer = buffer
      // the lack of native casting means the compiler throws an error here

      var buffer: cpp.Star<cpp.Void> = null;
      var floatBuffer: cpp.Star<cpp.Float32> = cast buffer;
      // generates correct: float* floatBuffer = ( (float*) buffer ) 

      Assert.isNull(floatBuffer);
   }


   public function testNull() {
      var nullP : Pointer<Vec> = null;
      var nullRawP = nullP.raw;
      Assert.isTrue( nullP==null );
      Assert.isTrue( null==nullP );
      Assert.isFalse( nullP!=null );
      Assert.isTrue( nullRawP==null );
      Assert.isFalse( nullRawP!=null );
      nullRawP = null;
      Assert.isTrue( nullRawP==null );
   }

   private function anonOf(d:Dynamic) : Dynamic return {ptr:d};

   public function testStructAccess() {
      var e = VecStructAccess.create(1);
      var tmp = e.ptr;
      var tmp1 = e.ref;
      tmp.set99(tmp1);
      Assert.equals(99f64, e.ptr.x);
   }

   @:native("callPointer")
   extern private static function callPointer(ptr:cpp.Pointer<Vec>):Int;

   public function testPointerCast() {
      var map = new Map<Int, cpp.Pointer<Vec> >();
      map.set(1,null);
      var result = callPointer( map.get(2) );
      Assert.equals(5, result);
   }

   public function testDynamic() {
      var a = [1];
      var intPtr = a.address(0);
      var d:Dynamic = intPtr;
      Assert.notEquals(d, [2].address(0));
      Assert.equals(d, a.address(0));
      var anon = anonOf(d);
      Assert.notEquals([2].address(0), d);
      Assert.equals(a.address(0), d);
      Assert.notEquals(intPtr, [2].address(0));
      Assert.equals(intPtr, a.address(0));
      Assert.notEquals(anon.ptr, [2].address(0));
      Assert.equals(anon.ptr, a.address(0));
      Assert.notEquals([2].address(0), anon.ptr);
      Assert.equals(a.address(0), anon.ptr);
   }

   function getAnonI(a:Dynamic) : Dynamic
   {
      return a.i;
   }


   public function testAnon() {
      var a = [1];
      var intPtr = a.address(0);
      var anon = { i:intPtr };
      Assert.equals( getAnonI(anon), intPtr );

      var vecPtr = VecStructAccess.create(1);
      var anon = { i:vecPtr };
      Assert.equals( getAnonI(anon), vecPtr );

      var vec:VecStruct = null;
      vec.x = 123;
      var anon = { i:vec };
      Assert.equals( getAnonI(anon), vec );
   }

   static function callMe(x:Int) return 10+x;

   static function notProcAddress(module:String, func:String) return null;

   public function testArrayAccess() {
      var array = [ 0.0, 1.1, 2.2, 3.3 ];
      var ptr = cpp.Pointer.arrayElem(array, 0);
      Assert.equals( ptr[1], 1.1 );
      ptr[1] = 2;
      Assert.equals( ptr[1], 2 );
      ptr[1]++;
      Assert.equals( ptr[1], 3 );
      ptr[1]-=2.5;
      Assert.equals( ptr[1], 0.5 );

      var raw = ptr.raw;
      Assert.equals( raw[2], 2.2 );
      raw[2] = 2;
      Assert.equals( raw[2], 2 );
      raw[2]++;
      Assert.equals( raw[2], 3 );
      raw[2]-=2.5;
      Assert.equals( raw[2], 0.5 );
   }

   public function testFromRaw()
   {
      var i = new IntHolder(3);
      var ptr = cpp.Pointer.fromRaw(cpp.Pointer.addressOf(i).rawCast());
      Assert.equals( ptr.ref.ival, i.ival );
      ptr.ref.ival==23;
      Assert.equals( ptr.ref.ival, i.ival );
   }

   private static var output:cpp.Pointer<Array<Int>>;

   private static var arrayValue:Array<Int>;
   private static function makeValue():{ a:cpp.Pointer<Array<Int>> }
   {
      arrayValue = [9];
      return { a: cpp.Pointer.addressOf(arrayValue) };
   }

   @:analyzer(no_fusion)
   public function testDynamicOutput()
   {
      // Declared as structure (just `var val = ...` works too)
      var val:{ a:cpp.Pointer<Array<Int>> } = makeValue();

      var a:cpp.Pointer<Array<Int>> = val.a;
      output = a;
      output = (val.a:Dynamic);
      output = val.a;
      output = (val.a:cpp.Pointer<Array<Int>>);
      val.a = output;

      // Declared as Dynamic
      var val2:Dynamic = makeValue();
      a = val2.a;
      output = a;
      output = (val2.a:Dynamic);
      output = val2.a;
      output = (val2.a:cpp.Pointer<Array<Int>>);
      val2.a = output;
      Assert.equals( val2.a, output );
      Assert.equals( output, val.a );
   }



   public function testAutoCast() {
      var z = [ 1, 2, 3 ];
      Assert.isTrue( cpp.NativeArray.address(z, 0).ptr == cpp.NativeArray.address(z, 0).ptr );
      Assert.isTrue( cpp.NativeArray.address(z, 1).ptr != cpp.NativeArray.address(z, 0).ptr );
      Assert.isTrue( cpp.NativeArray.address(z, 1).gt(cpp.NativeArray.address(z, 0)) );
      Assert.isTrue( cpp.NativeArray.address(z, 1).geq(cpp.NativeArray.address(z, 0)) );
      Assert.isTrue( cpp.NativeArray.address(z, 1).geq(cpp.NativeArray.address(z, 1)) );
      Assert.isTrue( cpp.NativeArray.address(z, 0).leq(cpp.NativeArray.address(z, 0)) );
      Assert.isTrue( cpp.NativeArray.address(z, 1).leq(cpp.NativeArray.address(z, 2)) );
      Assert.isTrue( cpp.NativeArray.address(z, 1).leq(cpp.NativeArray.address(z, 2)) );
      Assert.equals( cpp.NativeArray.address(z, 0), cpp.Pointer.ofArray(z) );
      Assert.equals( cpp.NativeArray.address(z, 1), cpp.Pointer.arrayElem(z,1) );
      Assert.notEquals( cpp.NativeArray.address(z, 1), cpp.Pointer.fromHandle(null) );
      Assert.equals(11, cpp.Function.fromStaticFunction(callMe)(1));
      Assert.exception(() -> cpp.Function.fromStaticFunction(notProcAddress)!=cpp.Function.getProcAddress("nomodule","nofunc!"), String);
   }

   static function functionCaller(fn:cpp.Function<Void->Int,cpp.abi.Abi>) {
      var a = fn.call();
   }

   public function testFunctionStructAccess() {
      Assert.notNull( functionCaller );
   }

   public function testSetData() {
      var ss:SomeStruct = null;
      ss.dataLength = 4;
      ss.data = cast "bye!".c_str();
      var b = ss.getDataBytes();
      Assert.equals( "bye!", b.getString(0, b.length) );

      var ss:SomeStruct = null;
      var b = ss.getUnmanagedDataBytes();
      Assert.equals( "Hi!", b.getString(0, b.length) );
   }

   public function testZero() {
      var a = [1,2,3];
      a.zero();
      Assert.equals(3, a.length);
      Assert.equals(0, a[0]);
      Assert.equals(0, a[1]);
      Assert.equals(0, a[2]);
   }

   public function testMemcmp() {
      var a = [1,2,3];
      var b = [2,2,3];
      Assert.equals( -1, a.memcmp(b));
      Assert.equals(  1, b.memcmp(a) );
      Assert.equals(  0, a.memcmp(a) );
   }

   public function testCapacity() {
      var a = [1,2,3];
      Assert.isTrue( a.capacity() < 1000 );
      a.reserve(1000);
      Assert.equals( 1000, a.capacity() );
      a[1000] = 1;
      Assert.isTrue( a.capacity() > 1000 );
   }


   public function testElementSize() {
      var a = [1];
      Assert.equals( cpp.Stdlib.sizeof(Int), a.getElementSize() );
      var a = ["hello!"];
      Assert.equals( cpp.Stdlib.sizeof(String), a.getElementSize() );
      var a = [7.1];
      Assert.equals( cpp.Stdlib.sizeof(Float), a.getElementSize() );
   }


   public function testBlit() {
      var a = [1,2,3,4];
      var b = [0,0,0,0];
      b.blit(0,a,0,a.length);
      for(i in 0...4)
        Assert.equals(b[i], a[i]);
      for(i in 0...4)
         b.blit(i,a,0,1);
      for(i in 0...4)
        Assert.equals(b[i], a[0]);
      for(i in 0...4)
         b.blit(i,a,2,1);
      for(i in 0...4)
        Assert.equals(b[i], a[2]);
   }
}

