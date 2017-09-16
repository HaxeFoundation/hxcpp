package gc;
import cpp.vm.Thread;
import haxe.crypto.Md5;
class TestGCWithSys extends haxe.unit.TestCase {

    public function testShouldNotExplode():Void {
       #if HXCPP_STRESS_TEST
       doThreadedWork(4,1000000);
       #else
       doThreadedWork(4,1000);
       #end
       assertTrue(true);
    }

   function doThreadedWork(numThreads, numWork):Void {
      var threads:Array<Thread> = makeThreads(numThreads);
      
      for (i in 0...numWork)
         threads[i % threads.length].sendMessage('doWork');

      for (i in 0...numThreads) {
         threads[i].sendMessage('exit'); 
         Thread.readMessage(true);
      }
   }

   function makeThreads(numThreads:Int):Array<Thread> {
      var text:String = sys.io.File.getContent('gc/testShouldNotExplode.txt');
      var main:Thread = Thread.current();
      var threads:Array<Thread> = [];
      for (i in 0...numThreads) {
         var thread:Thread = cpp.vm.Thread.create(function() {
            while(true) {
               var message:Dynamic = Thread.readMessage(true);
               if(message == 'exit')
                  break;
               else
                  Md5.encode(text);
            }
            main.sendMessage('done');
         });
         threads.push(thread);
      }
      return threads;
   }
}
