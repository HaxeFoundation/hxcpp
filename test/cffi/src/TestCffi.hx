#if cpp
import cpp.Lib;
#elseif neko
import neko.Lib;
#end

import haxe.io.BytesData;

class TestCffi extends TestBase
{
   static var isBool:Dynamic->Bool = Lib.load("prime", "isBool", 1);
   static var isNull:Dynamic->Bool = Lib.load("prime", "isNull", 1);
   static var allocNull:Void->Dynamic = Lib.load("prime", "allocNull", 0);
   static var appendString:BytesData->String->BytesData = Lib.load("prime", "appendString", 2);
   static var bufferToString:BytesData->String = Lib.load("prime", "bufferToString", 1);
   static var valIsBuffer:Dynamic->Bool = Lib.load("prime", "valIsBuffer", 1);

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
      var base = "Hello ";
      var bytes = haxe.io.Bytes.ofString(base).getData();
      bytes = appendString(bytes,"World");
      var result = bufferToString(bytes);
      trace(result);
      assertTrue( result == "Hello World");

      #if neko
      assertFalse( valIsBuffer(bytes) );
      #else
      assertTrue( valIsBuffer(bytes) );
      #end
      assertFalse( valIsBuffer(null) );
      assertFalse( valIsBuffer(1) );
      assertFalse( valIsBuffer({}) );
      assertFalse( valIsBuffer("String Buf") );

      //assertTrue( bufferToString(null) == null );
   }
}


