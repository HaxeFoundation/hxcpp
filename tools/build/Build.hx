class Build extends hxcpp.Builder
{
   // Create a build in 'bin' directory, with the "stdlibc++" flags for compatibility
   //  This flasg should not make a difference because hxcpp does not use stdlibc++
   override public function wantLegacyIosBuild() { return true; }

   override public function wantWindows64() { return true; }


   // Override to ensure this version if hxcpp is used, even if haxelib says otherwise
   override public function runBuild(target:String, isStatic:Bool, arch:String, inFlags:Array<String>)
   {
       var args = ["run.n", "Build.xml"].concat(inFlags);
       var here = Sys.getCwd().split("\\").join("/");

       var parts = here.split("/");
       if (parts.length>0 && parts[parts.length-1]=="")
          parts.pop();
       if (parts.length>0)
          parts.pop();
       var hxcppDir = parts.join("/");

       // This is how haxelib calls a 'run.n' script...
       Sys.setCwd(hxcppDir);
       args.push(here);
       Sys.println("neko " + args.join(" ")); 
       if (Sys.command("neko",args)!=0)
       {
          Sys.println("#### Error building neko " + inFlags.join(" "));
          Sys.exit(-1);
       }
       Sys.setCwd(here);
   }

   public static function main()
   {
      new Build( Sys.args() );
   }
}
