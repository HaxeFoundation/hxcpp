
class RunMain
{
   public static function log(s:String) Sys.println(s);
   public static function showMessage()
   {
      var varName = "HXCPP_NONINTERACTIVE";
      var nonInteractive:Bool =Sys.getEnv(varName)!=null;
      if (!nonInteractive)
         for(arg in Sys.args())
            if (arg.indexOf("-D"+varName)==0 )
               nonInteractive = true;

      var dir = Sys.getCwd();

      if (nonInteractive)
      {
         Sys.println('HXCPP in $dir is missing hxcpp.n');
         Sys.exit(-1);
      }

      log('This version of hxcpp ($dir) appears to be a source/developement version.');
      log("Before this can be used, you need to:");
      log(" 1. Rebuild the main command-line tool, this can be done with:");
      log("     cd tools/hxcpp");
      log("     haxe compile.hxml");
      log(" 2. FOR HXCPP API < 330:");
      log("    Build the binaries appropriate to your system(s), this can be done with:");
      log("     cd project");
      log("     neko build.n");

      var gotUserResponse = false;
      neko.vm.Thread.create(function() {
         Sys.sleep(30);
         if (!gotUserResponse)
         {
            Sys.println("\nTimeout waiting for response.");
            Sys.println("Can't continue without hxcpp.n");
            Sys.exit(-1);
         }
      } );

      while(true)
      {
         Sys.print("\nWould you like to do this now [y/n]");
         var code = Sys.getChar(true);
         gotUserResponse = true;
         if (code<=32)
            break;
         var answer = String.fromCharCode(code);
         if (answer=="y" || answer=="Y")
         {
            log("");
            setup();
            if (!executeHxcpp())
               break;
            return;
         }
         if (answer=="n" || answer=="N")
            break;
      }

      Sys.println("\nCan't continue without hxcpp.n");
      Sys.exit(-1);
   }

   public static function setup()
   {
      log("Compiling hxcpp tool...");
      run("tools/hxcpp","haxe", [ "compile.hxml"]);
      log("Building binaries...");
      run("project","neko", [ "build.n"]);
      log("Initial setup complete.");
   }

   public static function run(dir:String, command:String, args:Array<String>)
   {
      var oldDir:String = "";
      if (dir!="")
      {
         oldDir = Sys.getCwd();
         Sys.setCwd(dir);
      }
      Sys.command(command,args);
      if (oldDir!="")
         Sys.setCwd(oldDir);
   }

   public static function executeHxcpp()
   {
      if (!sys.FileSystem.exists("./hxcpp.n"))
         return false;
      neko.vm.Loader.local().loadModule("./hxcpp.n");
      return true;
   }

   public static function main()
   {
     if (!executeHxcpp())
         showMessage();
   }
}
