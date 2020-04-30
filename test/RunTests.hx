class RunTests
{
   static var baseDir:String;
   static var errors = new Array<String>();
   static var cppAst:Array<String> = [];
   static var sysArgs = new Array<String>();
   static var binDir = "";
   static var ext = "";
   static var m64 = true;
   static var m64Def = "HXCPP_M64";
   static var windows = false;
   static var sep = "/";

   public static function cffi()
   {
      setDir("cffi/project");

      command("haxelib", ["run", "hxcpp", "build.xml", "-debug", '-D$m64Def']);

      setDir("cffi");
      command("haxe", ["compile.hxml", "-debug"] );
      command("haxe", ["compile-utf8.hxml", "-debug"] );
      command("haxe", ["compile-neko.hxml", "-debug"] );

      copy('project/ndll/$binDir/prime$ext', 'bin/neko/prime.ndll');

      setDir("cffi");
      command("bin" + sep + "cpp" + sep + "TestMain-debug",[]);
      command("bin" + sep + "cpp-utf8" + sep + "TestMain-debug",[]);

      if (m64Def=="HXCPP_M64")
      {
         setDir("cffi/bin/neko");
         command("neko", ["TestMain.n"]);
      }
   }

   public static function runHaxe()
   {
      setDir("haxe");
      command("haxe", ["compile.hxml", "-debug", "-D", m64Def].concat(cppAst) );
      command("bin" + sep + "TestMain-debug",[]);
   }

   public static function runTelemetry()
   {
      setDir("telemetry");

      // Telemetry should work in debug and non-debug modes
      // TODO: do we need m64Def?
      command("haxe", ["compile.hxml", "-debug"].concat(cppAst) );
      command("bin" + sep + "TestMain-debug",[]);

      command("haxe", ["compile.hxml"].concat(cppAst) );
      command("bin" + sep + "TestMain",[]);

   }


   public static function debugger()
   {
      setDir("debugger");

      command("haxe", ["compile.hxml"] );
      command("bin" + sep + "App-debug",[]);
   }

   public static function opMatrix()
   {
      setDir("opMatrix");

      command("haxe", ["--run","MkOps.hx"] );
   }


   public static function cppia()
   {
      setDir("cppia");

      command("haxe", ["compile-host.hxml"] );
      command("haxe", ["compile-client.hxml"] );
      command("bin" + sep + "CppiaHost",[ "bin" + sep + "client.cppia" ]);
   }

   public static function native()
   {
      setDir("native");

      command("haxe", ["compile.hxml"] );
      command("bin" + sep + "Native",[]);
   }

   public static function std32()
   {
      setDir("std");

      command("haxe", ["compile32.hxml"] );
      command("cpp32"+sep+"Test",[]);
   }

   public static function std64()
   {
      setDir("std");

      command("haxe", ["compile64.hxml"] );
      command("cpp64"+sep+"Test",[]);
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
      var args = sysArgs;
      if (args.length>0 && args.indexOf(name)<0)
      {
         Sys.println("Skip test " + name);
         return;
      }

      try
      {
         func();
      }
      catch(e:Dynamic)
      {
         trace('Error running $name : $e');
         errors.push('Error running $name : $e');
      }
   }

   public static function copy(from:String, to:String)
   {
      if (windows)
      {
         from = from.split("/").join("\\");
         to = to.split("/").join("\\");
      }
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
            m64 = true;
            binDir = "Windows64";
            ext = ".dll";
            windows = true;
            sep = "\\";
         default:
            throw 'Unknown system "$systemName"';
      }

      sysArgs = Sys.args();
      if (sysArgs.remove("-cppast"))
         cppAst = ["-D", "cppast"];

      m64Def = m64 ? "HXCPP_M64" : "HXCPP_M32";
            
      baseDir = Sys.getCwd();

      run("cppia", cppia);
      run("cffi", cffi);
      //run("opMatrix", opMatrix);
      run("haxe", runHaxe);
      run("telemetry", runTelemetry);
      run("std32", std32);
      run("std64", std64);
      run("native", native);
      run("debugger", debugger);

      Sys.println("");

      if (errors.length==0)
      {
         Sys.println("All good!");
         Sys.exit(0);
      }
      Sys.println("There were errors:");
      for(error in errors)
         Sys.println(error);
      Sys.exit(-1);
   }
}

