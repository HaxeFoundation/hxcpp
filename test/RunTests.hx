class RunTests
{
   static var baseDir:String;
   static var allGood = true;
   static var binDir = "";
   static var ext = "";
   static var m64 = true;
   static var m64Def = "HXCPP_M64";
   static var windows = false;

   public static function cffi()
   {
      setDir("cffi/project");

      command("haxelib", ["run", "hxcpp", "build.xml", "-debug", '-D$m64Def'] );

      setDir("cffi");
      command("haxe", ["compile.hxml", "-debug"] );
      command("haxe", ["compile-neko.hxml", "-debug"] );
      copy('project/ndll/$binDir/prime$ext', 'bin/neko/prime.ndll');

      setDir("cffi");
      command("bin/cpp/TestMain-debug",[]);

      setDir("cffi/bin/neko");
      command("neko", ["TestMain.n"]);
   }

   public static function runHaxe()
   {
      setDir("haxe");
      command("haxe", ["compile.hxml", "-debug", "-D", m64Def] );
      command("bin/TestMain-debug",[]);
   }


   public static function ndlls()
   {
      setDir("ndlls");

      command("haxe", ["compile.hxml"] );
      command("cpp/Test",[]);

      command("haxe", ["compile64.hxml"] );
      command("cpp64/Test",[]);

      command("haxe", ["compile-static.hxml"]);
      command("scpp/Test",[]);
   }



   public static function setDir(name:String)
   {
      Sys.println("Enter " + baseDir + "/" + name);
      Sys.setCwd(baseDir + "/" + name);
   }

   public static function command(prog:String, args:Array<String>)
   {
      Sys.println( prog + " " + args.join(" ") );
      var code = Sys.command(prog,args);
      if (code!=0)
         throw( "failed:" + prog + " " + args.join(" ") );
   }

   public static function run(name:String, func:Void->Void)
   {
      try
      {
         func();
      }
      catch(e:Dynamic)
      {
         trace('Error running $name : $e');
         allGood = false;
      }
   }

   public static function copy(from:String, to:String)
   {
      command( windows ? "copy" : "cp", [ from, to ] );
   }

   public static function main()
   {
      var systemName = Sys.systemName().toLowerCase();
      switch(systemName.substr(0,3) )
      {
         case "mac":
            m64 = true;
            binDir = "Mac64";
            ext = ".dylib";
         case "lin":
            m64 = true;
            binDir = "Linux64";
            ext = ".dso";
         case "win":
            m64 = false;
            binDir = "Windows";
            ext = ".dll";
            windows = true;
         default:
            throw 'Unknown system "$systemName"';
      }

      m64Def = m64 ? "HXCPP_M64" : "HXCPP_M32";
            
      baseDir = Sys.getCwd();

      run("cffi", cffi);
      run("haxe", runHaxe);
      run("ndlls", ndlls);

      Sys.exit( allGood ? 0 : -1 );
   }
}

