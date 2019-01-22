
class App
{
   public static var hasRunBreakMe = false;
   public static var hasRunBreakMe2 = false;

   function breakMe()
   {
      hasRunBreakMe = true;
   }

   function breakMe2() hasRunBreakMe2 = true;


   public function new()
   {
      breakMe();
      breakMe2();
      Lines.lineStep();
   }

   public static function main()
   {
      TestDebugger.setup();

      new App();

      if (!TestDebugger.finished)
      {
         Sys.println("Not all breakpoints triggered");
         Sys.exit(-1);
      }
      else if (!TestDebugger.ok)
      {
         Sys.println("Some debugger checks failed");
         Sys.exit(-1);
      }
      else
      {
         Sys.println("All good!");
      }

   }
}

