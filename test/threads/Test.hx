import cpp.vm.Thread;
import sys.io.File;

class Test
{
   public static function main()
   {
      var me = Thread.current();
      Thread.create( function() {
        File.copy("a.txt","b.txt");
        me.sendMessage("Done");
        } );
      var result = Thread.readMessage(true);
      trace(result);
   }
}
