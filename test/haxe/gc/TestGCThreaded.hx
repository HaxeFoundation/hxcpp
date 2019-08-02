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

class TestGCThreaded extends haxe.unit.TestCase {

   override public function setup()
   {
      var lines = [ for(i in 0...100000) "abc123\n" ];
      File.saveContent( "gc/big.txt", lines.join("") );
   }

   public function testThreadOnce():Void
   {
     doThreadedWork(4,100);
     assertTrue(true);
   }


   public function testThreadMany():Void
   {
     for(i in  0...10)
        doThreadedWork(4,100);
     assertTrue(true);
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
      var text:String = File.getContent("gc/big.txt");
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
            }
            main.sendMessage('done');
         }) );
      }
      return threads;
   }

}

