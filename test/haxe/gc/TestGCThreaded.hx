package gc;
import haxe.io.Bytes;
import haxe.crypto.Md5;
import cpp.vm.Gc;
import sys.io.File;
#if haxe4
import sys.thread.Thread;
#else
import cpp.vm.Thread;
#end

class Wrapper
{
  public var a:Int;
  public function new(inA:Int) a = inA;
}


@:cppInclude("./ZoneTest.cpp")
@:depend("./ZoneTest.cpp")
class TestGCThreaded extends haxe.unit.TestCase
{
  @:keep
  static var keepRunning = false;
  @:keep
  static var nativeRunning = false;
  var bigText:String;


   override public function setup()
   {
      var lines = [ for(i in 0...100000) "abc123\n" ];
      #if nme
      bigText = lines.join("");
      #else
      File.saveContent( "gc/big.txt", lines.join("") );
      #end
   }

   public function testThreadOnce():Void
   {
      startNative();
      doThreadedWork(4,100);
      stopNative();
      assertTrue(true);
   }


   public function testThreadMany():Void
   {
      startNative();
      for(i in  0...10)
         #if nme
         doThreadedWork(4,100);
         #else
         doThreadedWork(100,100);
         #end
      stopNative();
      assertTrue(true);
   }

   @:native("nativeLoop")
   extern static function nativeLoop() : Void;

   function startNative()
   {
      Thread.create( () -> {
         nativeRunning = true;
         keepRunning = true;
         nativeLoop();
      });
   }
   function stopNative()
   {
      keepRunning = false;
      while(nativeRunning)
         Sys.sleep(0.1);
   }

   function doThreadedWork(numThreads, numWork):Void
   {
      var threads:Array<Thread> = makeThreads(numThreads);

      for (i in 0...numWork)
         threads[i % threads.length].sendMessage('doWork');

      for (i in 0...numThreads)
      {
         threads[i].sendMessage('exit'); 
         Thread.readMessage(true);
      }
   }

   function makeThreads(numThreads:Int):Array<Thread>
   {
      #if nme
      var text:String = bigText;
      #else
      var text:String = File.getContent("gc/big.txt");
      #end
      var main:Thread = Thread.current();
      var threads:Array<Thread> = [];
      for (i in 0...numThreads)
      {
         threads.push( Thread.create(function() {
            while(true) {
               var message:Dynamic = Thread.readMessage(true);
               if(message == 'exit')
                  break;
               else
                  Md5.encode(text);
                  var arrays = new Array<Array<Wrapper>>();
                  for(i in 0...100)
                  {
                     var wrappers = new Array<Wrapper>();
                     arrays.push(wrappers);
                     for(j in 0...1000)
                        wrappers.push( new Wrapper(1) );
                  }
                  var sum = 0;
                  for(a in arrays)
                     for(w in a)
                        sum += w.a;

                  assertTrue(sum==100000);
            }
            main.sendMessage('done');
         }) );
      }
      return threads;
   }

}

