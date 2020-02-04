#if cpp
import cpp.Lib;
import cpp.vm.Gc;
#elseif neko
import neko.Lib;
import neko.vm.Gc;
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
   static var setRoot:Int->Dynamic->Void = Lib.load("prime", "setRoot", 2);
   static var getRoot:Int->Dynamic = Lib.load("prime", "getRoot", 1);
   static var clearRoots:Void->Void = Lib.load("prime", "clearRoots", 0);
   static var createAbstract:Void->Dynamic = Lib.load("prime", "createAbstract", 0);
   static var allocAbstract:Void->Dynamic = Lib.load("prime", "allocAbstract", 0);
   static var getAbstract:Dynamic->Int = Lib.load("prime", "getAbstract", 1);
   static var freeAbstract:Dynamic->Void = Lib.load("prime", "freeAbstract", 1);
   static var getAbstractFreeCount:Void->Int = Lib.load("prime", "getAbstractFreeCount", 0);
   static var createAnon:Void->Dynamic = Lib.load("prime", "createAnon", 0);
  

   static var cppObjectAsDynamic:cpp.Callable<Int->cpp.Object>;


   inline function getObjectAsString() : String
   {
      // Just test to see if this compiles
      if (cppObjectAsDynamic!=null)
         return cppObjectAsDynamic(1);
      return null;
   }

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
      assertTrue( getRoot!=null );
      assertTrue( setRoot!=null );

      assertTrue( createAnon!=null );


      assertFalse( valIsBuffer(null) );
      assertFalse( valIsBuffer(1) );
      assertFalse( valIsBuffer({}) );
      assertFalse( valIsBuffer("String Buf") );

      if (cppObjectAsDynamic!=null)
         assertTrue( getObjectAsString()==null);

      var anon = createAnon();
      for(f in Reflect.fields(anon))
      {
         #if cpp
         var value:Dynamic = Reflect.field(anon, f);
         //trace(f + " " + Type.typeof(value) );
         assertTrue( Std.string(Type.typeof(value)) == f );
         #end
      }

      for(i in 0...100)
        setRoot(i,[i]);

      Gc.run(true);



      var base = "Hello ";
      var bytes = haxe.io.Bytes.ofString(base).getData();

      #if !neko
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

      #if !neko
      assertEq(charString(99,97,116),"A cat");
      #end

      var bytes = haxe.io.Bytes.ofString("String Buffer");
      assertEq( byteDataSize(bytes), 13 );
      assertEq( byteDataByte(bytes,1), 't'.code );

      assertEq( getAbstractFreeCount(), 0 );

      var createdAbs = createAbstract();
      assertTrue( createdAbs!=null );
      assertEq( getAbstract(createdAbs), 99 );
      // Explicitly freeing abstract does not call finalizer
      freeAbstract( createdAbs );
      assertEq( getAbstractFreeCount(), 0 );
      assertEq( getAbstract(createdAbs), -1 );
      assertEq( getAbstractFreeCount(), 0 );
      createdAbs = null;
      Gc.run(true);
      assertEq( getAbstractFreeCount(), 0 );

      var allocatedAbs = allocAbstract();
      assertTrue( allocatedAbs!=null );
      assertEq( getAbstract(allocatedAbs), 99 );
      assertEq( getAbstractFreeCount(), 0 );
      freeAbstract( allocatedAbs );
      assertEq( getAbstract(allocatedAbs), -1 );
      assertEq( getAbstractFreeCount(), 0 );
      allocatedAbs = null;


      createDeepAbstracts(2);
      clearStack(12);

      Gc.run(true);

      var freeCount = getAbstractFreeCount();
      if (freeCount!=2)
      {
        Sys.println('\nWarning: $freeCount != 2');
      }

      for(i in 0...100)
        assertEq( getRoot(i)+"", [i]+"" );

      clearRoots();

      for(i in 0...100)
        assertEq( getRoot(i), null );

      assertEq( getAbstractFreeCount(), 2 );
   }

   function clearStack(count:Int, ?nothing:Dynamic):Dynamic
   {
      if (count==0)
         return 0;
      return clearStack(count-1);
   }

   // Try to hide references from GC stack marking
   function createDeepAbstracts(inDepth:Int)
   {
      if (inDepth==0)
      {
         createAbstract();
         allocAbstract();
      }
      else
        createDeepAbstracts(inDepth-1);
   }
}


