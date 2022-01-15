import sys.thread.Thread;
import sys.io.File;

@:cppInclude("./ThreadCode.cpp")
class Test
{
   static var mainThread:Thread;

   @:native("runThread")
   extern static function createNativeThread():Void;

   public static function callFromThread()
   {
      trace("Same:" + (mainThread==Thread.current()) );
      mainThread.sendMessage("Done");
   }

   public static function main()
   {
      var me = Thread.current();
      mainThread = me;
      Thread.create( function() {
        File.copy("a.txt","b.txt");
        me.sendMessage("Done");
        trace("Same thread:" + (me==Thread.current()) );
        } );
      var result = Thread.readMessage(true);
      trace(result);

      for(x in 0...20)
      {
         trace("call...");
         createNativeThread();
         trace("zzz...");
         Sys.sleep(1);
         var result = Thread.readMessage(true);
         trace(result);
      }
   }
}
