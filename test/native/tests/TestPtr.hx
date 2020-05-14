package tests;

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

   function test9194() {
      // will fail during C++ compile
      var buffer: cpp.RawPointer<cpp.Void> = null;
      var floatBuffer: cpp.RawPointer<cpp.Float32> = cast buffer;
      // generates incorrect: float* floatBuffer = buffer
      // the lack of native casting means the compiler throws an error here

      var buffer: cpp.Star<cpp.Void> = null;
      var floatBuffer: cpp.Star<cpp.Float32> = cast buffer;
      // generates correct: float* floatBuffer = ( (float*) buffer ) 

      assertTrue(floatBuffer==null);
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
      var e = VecStructAccess.create(1);
      var tmp = e.ptr;
      var tmp1 = e.ref;
      tmp.set99(tmp1);
      assertTrue(e.ptr.x==99);
   }

   @:native("callPointer") @:extern
   private static function callPointer(ptr:cpp.Pointer<Vec>):Int;

   public function testPointerCast() {
      var map = new Map<Int, cpp.Pointer<Vec> >();
      map.set(1,null);
      var result = callPointer( map.get(2) );
      assertTrue(result==5);
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

   function getAnonI(a:Dynamic) : Dynamic
   {
      return a.i;
   }


   public function testAnon() {
      var a = [1];
      var intPtr = a.address(0);
      var anon = { i:intPtr };
      assertTrue( getAnonI(anon)==intPtr );

      var vecPtr = VecStructAccess.create(1);
      var anon = { i:vecPtr };
      assertTrue( getAnonI(anon)==vecPtr );

      var vec:VecStruct = null;
      vec.x = 123;
      var anon = { i:vec };
      assertTrue( getAnonI(anon)==vec );
   }

   static function callMe(x:Int) return 10+x;

   static function notProcAddress(module:String, func:String) return null;

   public function testArrayAccess() {
       var array = [ 0.0, 1.1, 2.2, 3.3 ];
       var ptr = cpp.Pointer.arrayElem(array, 0);
       assertTrue( ptr[1]==1.1 );
       ptr[1] = 2;
       assertTrue( ptr[1]==2 );
       ptr[1]++;
       assertTrue( ptr[1]==3 );
       ptr[1]-=2.5;
       assertTrue( ptr[1]==0.5 );

       var raw = ptr.raw;
       assertTrue( raw[2]==2.2 );
       raw[2] = 2;
       assertTrue( raw[2]==2 );
       raw[2]++;
       assertTrue( raw[2]==3 );
       raw[2]-=2.5;
       assertTrue( raw[2]==0.5 );

   }

   public function testFromRaw()
   {
      var i = new IntHolder(3);
      var ptr = cpp.Pointer.fromRaw(cpp.Pointer.addressOf(i).rawCast());
      assertTrue( ptr.ref.ival==i.ival );
      ptr.ref.ival==23;
      assertTrue( ptr.ref.ival==i.ival );
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
       assertTrue( val2.a==output );
       assertTrue( output==val.a );
     }



   public function testAutoCast() {
       var z = [ 1, 2, 3 ];
       assertTrue( cpp.NativeArray.address(z, 0).ptr == cpp.NativeArray.address(z, 0).ptr );
       assertTrue( cpp.NativeArray.address(z, 1).ptr != cpp.NativeArray.address(z, 0).ptr );
       assertTrue( cpp.NativeArray.address(z, 1).gt(cpp.NativeArray.address(z, 0)) );
       assertTrue( cpp.NativeArray.address(z, 1).geq(cpp.NativeArray.address(z, 0)) );
       assertTrue( cpp.NativeArray.address(z, 1).geq(cpp.NativeArray.address(z, 1)) );
       assertTrue( cpp.NativeArray.address(z, 0).leq(cpp.NativeArray.address(z, 0)) );
       assertTrue( cpp.NativeArray.address(z, 1).leq(cpp.NativeArray.address(z, 2)) );
       assertTrue( cpp.NativeArray.address(z, 1).leq(cpp.NativeArray.address(z, 2)) );
       assertTrue( cpp.NativeArray.address(z, 0) == cpp.Pointer.ofArray(z) );
       assertTrue( cpp.NativeArray.address(z, 1) == cpp.Pointer.arrayElem(z,1) );
       assertTrue( cpp.NativeArray.address(z, 1) != cpp.Pointer.fromHandle(null) );
       assertTrue( cpp.Function.fromStaticFunction(callMe)(1)==11 );
       try
       {
          assertTrue( cpp.Function.fromStaticFunction(notProcAddress)!=cpp.Function.getProcAddress("nomodule","nofunc!") );
       }
       catch(e:Dynamic)
       {
         // Could not load module - expected
       }
   }

   static function functionCaller(fn:cpp.Function<Void->Int,cpp.abi.Abi>) {
        var a = fn.call();
    }

   public function testFunctionStructAccess() {
       assertTrue( functionCaller != null );
   }

   public function testSetData() {
      var ss:SomeStruct = null;
      ss.dataLength = 4;
      ss.data = cast "bye!".c_str();
      var b = ss.getDataBytes();
      assertTrue( b.getString(0, b.length) == "bye!" );

      var ss:SomeStruct = null;
      var b = ss.getUnmanagedDataBytes();
      assertTrue( b.getString(0, b.length) == "Hi!" );
   }

   public function testZero() {
      var a = [1,2,3];
      a.zero();
      assertTrue(a.length==3);
      assertTrue(a[0]==0);
      assertTrue(a[1]==0);
      assertTrue(a[2]==0);
   }

   public function testMemcmp() {
      var a = [1,2,3];
      var b = [2,2,3];
      assertTrue( a.memcmp(b) == -1 );
      assertTrue( b.memcmp(a) == 1 );
      assertTrue( a.memcmp(a) == 0 );
   }

   public function testCapacity() {
      var a = [1,2,3];
      assertTrue( a.capacity() < 1000 );
      a.reserve(1000);
      assertTrue( a.capacity() == 1000 );
      a[1000] = 1;
      assertTrue( a.capacity() > 1000 );
   }


   public function testElementSize() {
      var a = [1];
      assertTrue( a.getElementSize() == cpp.Stdlib.sizeof(Int)  );
      var a = ["hello!"];
      assertTrue( a.getElementSize() == cpp.Stdlib.sizeof(String)  );
      var a = [7.1];
      assertTrue( a.getElementSize() == cpp.Stdlib.sizeof(Float)  );
   }


   public function testBlit() {
      var a = [1,2,3,4];
      var b = [0,0,0,0];
      b.blit(0,a,0,a.length);
      for(i in 0...4)
        assertTrue(b[i] == a[i]);
      for(i in 0...4)
         b.blit(i,a,0,1);
      for(i in 0...4)
        assertTrue(b[i] == a[0]);
      for(i in 0...4)
         b.blit(i,a,2,1);
      for(i in 0...4)
        assertTrue(b[i] == a[2]);
   }
}

