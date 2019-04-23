#if haxe4
  import sys.thread.Thread;
  import sys.thread.Mutex;
#else
  #if neko
  import neko.vm.Thread;
  import neko.vm.Mutex;
  #else
  import cpp.vm.Thread;
  import cpp.vm.Mutex;
  #end
#end

#if cpp
import cpp.AtomicInt;
#end

class ThreadPool
{
   var threads:Array<Thread>;
   var mainThread:Thread;
   #if cpp
   var arrayIndex:AtomicInt;
   #else
   var arrayIndex:Int;
   #end
   var arrayCount:Int;

   public var mutex:Mutex;


   public function new(inCount:Int)
   {
      mutex = new Mutex();
      mainThread = Thread.current();
      threads = [];
      for(i in 0...inCount)
        threads.push( Thread.create( function() threadLoop(i) ) );

      // Is this needed?
      Tools.addOnExitHook( function(_) runJob(null) );
   }

   function threadLoop(threadId:Int)
   {
      while(true)
      {
         var job:Int->Void = Thread.readMessage(true);
         if (job==null)
         {
            mainThread.sendMessage("done");
            break;
         }

         try
         {
            job(threadId);
         }
         catch (error:Dynamic)
         {
            if (BuildTool.threadExitCode!=0)
               BuildTool.setThreadError(-1);
            else
               Log.warn("Error in compile thread: " + error);
         }

         mainThread.sendMessage("ok");
      }
   }

   public function setArrayCount(inCount:Int)
   {
      arrayIndex = 0;
      arrayCount = inCount;
   }
   public function getNextIndex() : Int
   {
       #if cpp
       var index = AtomicInt.atomicInc( cpp.Pointer.addressOf(arrayIndex) );
       #else
       mutex.acquire();
       var index = arrayIndex++;
       mutex.release();
       #end

       if (index<arrayCount)
          return index;
       return -1;
   }

   public function runJob(job:Int->Void)
   {
      for(thread in threads)
         thread.sendMessage( job );
      for(i in 0...threads.length)
         Thread.readMessage(true);

      // Already printed the error from the thread, just need to exit
      if (job!=null && BuildTool.threadExitCode!=0)
         Tools.exit(BuildTool.threadExitCode);
   }

}


