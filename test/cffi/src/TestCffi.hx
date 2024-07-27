#if cpp
import cpp.Lib;
import cpp.vm.Gc;
#elseif neko
import neko.Lib;
import neko.vm.Gc;
#end

import Loader;

import haxe.io.BytesData;
import utest.Test;
import utest.Assert;

class TestCffi extends Test
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

      Assert.isTrue( isBool!=null );
      Assert.isTrue( isBool(true) );
      Assert.isTrue( isBool(false) );
      Assert.isFalse( isBool(21) );
      Assert.isFalse( isBool("Hello") );
      Assert.isFalse( isBool(null) );

      Assert.isTrue( isNull!=null );
      Assert.isTrue( isNull(null) );
      Assert.isFalse( isNull(false) );
      Assert.isFalse( isNull(32) );
      Assert.isFalse( isNull("") );

      Assert.isTrue( allocNull!=null );
      Assert.isNull( allocNull() );

      Assert.isTrue( appendString!=null );
      Assert.isTrue( bufferToString!=null );
      Assert.isTrue( getRoot!=null );
      Assert.isTrue( setRoot!=null );

      Assert.isTrue( createAnon!=null );


      Assert.isFalse( valIsBuffer(null) );
      Assert.isFalse( valIsBuffer(1) );
      Assert.isFalse( valIsBuffer({}) );
      Assert.isFalse( valIsBuffer("String Buf") );

      if (cppObjectAsDynamic!=null)
         Assert.isNull( getObjectAsString() );

      var anon = createAnon();
      for(f in Reflect.fields(anon))
      {
         #if cpp
         var value:Dynamic = Reflect.field(anon, f);
         //trace(f + " " + Type.typeof(value) );
         Assert.isTrue( Std.string(Type.typeof(value)) == f );
         #end
      }

      for(i in 0...100)
        setRoot(i,[i]);

      Gc.run(true);



      var base = "Hello ";
      var bytes = haxe.io.Bytes.ofString(base).getData();

      #if !neko
      Assert.isTrue( valIsBuffer(bytes) );
      // Can't acess neko buffer from haxe code
      bytes = appendString(bytes,"World");
      var result = bufferToString(bytes);
      Assert.equals("Hello World", result);
      #end

      Assert.equals("String:null1", valToString(null,1));
      Assert.equals("String:x1.1", valToString("x",1.1));
      Assert.equals("String:Hello World", valToString("Hello"," World"));
      Assert.equals("String:[1][]", valToString([1],[]));

      Assert.equals("Cold as hell", subBuffer("hello",4));

      #if !neko
      Assert.equals("A cat", charString(99,97,116));
      #end

      var bytes = haxe.io.Bytes.ofString("String Buffer");
      Assert.equals( 13, byteDataSize(bytes) );
      Assert.equals( 't'.code, byteDataByte(bytes,1) );

      Assert.equals( 0, getAbstractFreeCount() );

      var createdAbs = createAbstract();
      Assert.notNull( createdAbs );
      Assert.equals( 99, getAbstract(createdAbs) );
      // Explicitly freeing abstract does not call finalizer
      freeAbstract( createdAbs );
      Assert.equals( 0, getAbstractFreeCount() );
      Assert.equals( -1, getAbstract(createdAbs) );
      Assert.equals( 0, getAbstractFreeCount() );
      createdAbs = null;
      Gc.run(true);
      Assert.equals( 0, getAbstractFreeCount() );

      var allocatedAbs = allocAbstract();
      Assert.notNull( allocatedAbs );
      Assert.equals( 99, getAbstract(allocatedAbs) );
      Assert.equals( 0, getAbstractFreeCount() );
      freeAbstract( allocatedAbs );
      Assert.equals( -1, getAbstract(allocatedAbs) );
      Assert.equals( 0, getAbstractFreeCount() );
      allocatedAbs = null;

      createDeepAbstracts(2);
      clearStack(12);

      Gc.run(true);

      var freeCount = getAbstractFreeCount();
      if (freeCount!=2)
      {
        Assert.warn('$freeCount != 2');
      }

      for(i in 0...100)
        Assert.equals( [i]+"", getRoot(i)+"" );

      clearRoots();

      for(i in 0...100)
        Assert.isNull( getRoot(i) );

      Assert.equals( 2, getAbstractFreeCount() );
   }

   function clearStack(count:Int, ?_:Dynamic, ?_:Dynamic, ?_:Dynamic, ?_:Dynamic, ?_:Dynamic):Dynamic
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


