#if cpp
import cpp.Lib;
#elseif neko
import neko.Lib;
#end

import Loader;

import haxe.io.BytesData;

class TestCffi extends TestBase
{
   static var isBool:Dynamic->Bool = Lib.load("prime", "isBool", 1);
   static var isNull:Dynamic->Bool = Lib.load("prime", "isNull", 1);
   static var allocNull:Void->Dynamic = Lib.load("prime", "allocNull", 0);
   static var appendString:BytesData->String->BytesData = Lib.load("prime", "appendString", 2);
   static var bufferToString:BytesData->String = Lib.load("prime", "bufferToString", 1);
   static var valIsBuffer:Dynamic->Bool = Lib.load("prime", "valIsBuffer", 1);
   static var valToString:Dynamic->Dynamic->String = Lib.load("prime", "valToString", 2);
   static var subBuffer:String->Int->String = Lib.load("prime", "subBuffer", 2);
   static var charString:Int->Int->Int->String = Lib.load("prime", "charString", 3);
   static var byteDataSize:haxe.io.Bytes->Int = Lib.load("prime", "byteDataSize", 1);
   static var byteDataByte:haxe.io.Bytes->Int->Int = Lib.load("prime", "byteDataByte", 2);

   public function testCffi()
   {
      cpp.Prime.nekoInit("prime");

      assertTrue( isBool!=null );
      assertTrue( isBool(true) );
      assertTrue( isBool(false) );
      assertFalse( isBool(21) );
      assertFalse( isBool("Hello") );
      assertFalse( isBool(null) );

      assertTrue( isNull!=null );
      assertTrue( isNull(null) );
      assertFalse( isNull(false) );
      assertFalse( isNull(32) );
      assertFalse( isNull("") );

      assertTrue( allocNull!=null );
      assertEquals(null, allocNull() );

      assertTrue( appendString!=null );
      assertTrue( bufferToString!=null );


      assertFalse( valIsBuffer(null) );
      assertFalse( valIsBuffer(1) );
      assertFalse( valIsBuffer({}) );
      assertFalse( valIsBuffer("String Buf") );


      var base = "Hello ";
      var bytes = haxe.io.Bytes.ofString(base).getData();

      #if neko
      assertFalse( valIsBuffer(bytes) );
      #else
      assertTrue( valIsBuffer(bytes) );
      // Can't acess neko buffer from haxe code
      bytes = appendString(bytes,"World");
      var result = bufferToString(bytes);
      assertEq(result,"Hello World");
      #end

      assertEq(valToString(null,1),"String:null1");
      assertEq(valToString("x",1.1),"String:x1.1");
      assertEq(valToString("Hello"," World"),"String:Hello World");
      assertEq(valToString([1],[]),"String:[1][]");

      assertEq(subBuffer("hello",4),"Cold as hell");

      assertEq(charString(99,97,116),"A cat");

      var bytes = haxe.io.Bytes.ofString("String Buffer");
      assertEq( byteDataSize(bytes), 13 );
      assertEq( byteDataByte(bytes,1), 't'.code );
   }
}


