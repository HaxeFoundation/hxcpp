package hxcpp;

import haxe.io.Path;
import sys.FileSystem;

class Builder
{
   public var debug:Bool;
   public var verbose:Bool;

   public function new(inArgs:Array<String>)
   {
      debug = false;
      verbose = false;
      var targets = new Map<String, Array<String>>();
      var buildArgs = new Array<String>();

      try
      {
         var clean = false;
         var defaultTarget = true;
         for(arg in inArgs)
         {
            if (arg=="-debug")
            {
               debug = true;
               continue;
            }
            else if (arg=="-v" || arg=="-verbose")
            {
               verbose = true;
               Sys.putEnv("HXCPP_VERBOSE", "1");
               continue;
            }
            if (arg=="clean")
            {
               clean = true;
               continue;
            }


            var parts = arg.split("-");
            var linkStatic = allowStatic();
            var linkNdll = allowNdll();
            var explicitNdll = false;
            if (parts[0]=="static")
            {
               linkNdll = false;
               parts.shift();
            }
            else if (parts[0]=="ndll")
            {
               linkStatic = false;
               explicitNdll = true;
               parts.shift();
            }

            var target = parts.shift();
            if (target=="default")
               target = getDefault();

            switch(target)
            {
               case "ios", "android", "blackberry", "tizen", "emscripten", "webos", "windows", "msvc", "linux", "mac", "mingw", "tvos":
                  defaultTarget = false;
                  if (linkStatic)
                  {
                     var stat = "static-" + target;
                     targets.set(stat, parts);

                     if (target=="ios" && wantLegacyIosBuild())
                     {
                        var stat = "static-" + "ioslegacy";
                        targets.set(stat, parts);
                     }
                  }
                  if (linkNdll && target!="ios" && target!="emscripten" && target!="tvos" /*&& (target!="mingw" || explicitNdll)*/ )
                     targets.set(target, parts);

               default:
                  if (arg.substr(0,2)=="-D")
                     buildArgs.push(arg);
                  else
                     throw "Unknown arg '" + arg + "'";
            }
         }

         if (clean)
         {
            if (!cleanAll(buildArgs))
               return;

            if (defaultTarget) // Just clean
               return;
         }

         if (defaultTarget)
         {
            var target = getDefault();
            if (target!="mingw")
               targets.set(target,[]);
            targets.set("static-" +target,[]);
            onEmptyTarget();
            Sys.println("\nUsing default = " + target);
         }

         for(target in targets.keys())
         {
            var archs = targets.get(target);
            var validArchs = new Map<String, Array<String>>();
            var isStatic = false;
            if (target.substr(0,7)=="static-")
            {
               isStatic = true;
               target = target.substr(7);
            }
            var staticFlags = isStatic ? ["-Dstatic_link"] : [];
            if (target=="ios" || target=="tvos")
               staticFlags = ["-DHXCPP_CPP11"];

            switch(target)
            {
               case "linux", "mac":
                  validArchs.set("m32", ["-D"+target, "-DHXCPP_M32"].concat(staticFlags) );
                  validArchs.set("m64", ["-D"+target, "-DHXCPP_M64"].concat(staticFlags) );

               case "windows":
                  validArchs.set("m32", ["-D"+target, "-DHXCPP_M32"].concat(staticFlags) );
                  if (wantWindows64())
                     validArchs.set("m64", ["-D"+target, "-DHXCPP_M64"].concat(staticFlags) );
                  if (wantWindowsArm64())
                     validArchs.set("arm64", ["-D"+target, "-DHXCPP_ARM64"].concat(staticFlags) );

               case "msvc":
                  if (isStatic)
                  {
                     validArchs.set("2013m32", ["-D"+target, "-DHXCPP_M32", "HXCPP_MSVC_VER=120"].concat(staticFlags) );
                     validArchs.set("2015m32", ["-D"+target, "-DHXCPP_M32", "HXCPP_MSVC_VER=140"].concat(staticFlags) );
                     if (wantWindows64())
                     {
                        validArchs.set("2013m64", ["-D"+target, "-DHXCPP_M64", "HXCPP_MSVC_VER=120"].concat(staticFlags) );
                        validArchs.set("2015m64", ["-D"+target, "-DHXCPP_M64", "HXCPP_MSVC_VER=140"].concat(staticFlags) );
                     }
                  }
                  else
                  {
                     validArchs.set("m32", ["-D"+target, "-DHXCPP_M32"] );
                     if (wantWindows64())
                        validArchs.set("m64", ["-D"+target, "-DHXCPP_M64"] );
                  }

               case "mingw":
                  validArchs.set("m32", ["-Dwindows", "-DHXCPP_MINGW", "-DHXCPP_M32"].concat(staticFlags) );

               case "ios", "ioslegacy":
                  validArchs.set("armv6", ["-Diphoneos"].concat(staticFlags) );
                  validArchs.set("armv7", ["-Diphoneos", "-DHXCPP_ARMV7"].concat(staticFlags) );
                  validArchs.set("armv7s", ["-Diphoneos", "-DHXCPP_ARMV7S"].concat(staticFlags) );
                  validArchs.set("arm64", ["-Diphoneos", "-DHXCPP_ARM64", "-DHXCPP_M64"].concat(staticFlags) );
                  //validArchs.push("armv64");
                  validArchs.set("x86", ["-Diphonesim"].concat(staticFlags) );
                  validArchs.set("x86_64", ["-Diphonesim", "-DHXCPP_M64"].concat(staticFlags) );

               case "android":

                  if( archs.length == 0 )
                     throw("You must specify the archs you want for android");
               
                  validArchs.set("armv5", ["-Dandroid"].concat(staticFlags) );
                  validArchs.set("armv7", ["-Dandroid", "-DHXCPP_ARMV7"].concat(staticFlags) );
                  validArchs.set("arm64", ["-Dandroid", "-DHXCPP_ARM64"].concat(staticFlags) );
                  validArchs.set("x86", ["-Dandroid", "-DHXCPP_X86"].concat(staticFlags) );
                  validArchs.set("x86_64", ["-Dandroid", "-DHXCPP_X86_64"].concat(staticFlags) );
               
               case "blackberry":
                  validArchs.set("armv7", ["-Dblackberry"].concat(staticFlags) );
                  validArchs.set("x86", ["-Dblackberry", "-Dsimulator"].concat(staticFlags) );
               
               case "tizen":
                  validArchs.set("armv7", ["-Dtizen"].concat(staticFlags) );
                  validArchs.set("x86", ["-Dtizen", "-Dsimulator"].concat(staticFlags) );
               
               case "emscripten":
                  validArchs.set("x86", ["-Demscripten"].concat(staticFlags) );
               
               case "webos":
                  validArchs.set("armv7", ["-Dwebos"].concat(staticFlags) );
               
               case "tvos":
                  validArchs.set("arm64", ["-Dappletvos", "-DHXCPP_ARM64", "-DHXCPP_M64", "-DENABLE_BITCODE"].concat(staticFlags) );
                  // NOTE: removed as there's no 32bit support for the AppleTV simulator
                  //validArchs.set("x86", ["-Dappletvsim", "-DENABLE_BITCODE"].concat(staticFlags) );
                  validArchs.set("x86_64", ["-Dappletvsim", "-DHXCPP_M64", "-DENABLE_BITCODE"].concat(staticFlags) );

            }


            var valid = new Array<String>();
            for(key in validArchs.keys())
               valid.push(key);
            var buildArchs = archs.length==0 ? valid : archs;
            for(arch in buildArchs)
            {
               if (validArchs.exists(arch))
               {
                  var flags = validArchs.get(arch);
                  if (debug)
                     flags.push("-Ddebug");

                  flags = flags.concat(buildArgs);

                  runBuild(target, isStatic, arch, flags);
               }
            }
         }
      }
      catch( e:Dynamic )
      {
         if (e!="")
            Sys.println(e);
         showUsage(false);
      }
   }

   public function allowNdll() { return true; }
   public function allowStatic() { return true; }
   public function wantLegacyIosBuild() { return false; }
   public function wantWindows64() { return false; }
   public function wantWindowsArm64() { return false; }

   public function runBuild(target:String, isStatic:Bool, arch:String, buildFlags:Array<String>)
   {
      var args = ["run", "hxcpp", getBuildFile() ].concat(buildFlags);

      Sys.println('\nBuild $target, link=' + (isStatic?"lib":"ndll")+' arch=$arch');
      Sys.println("haxelib " + args.join(" ")); 
      if (Sys.command("haxelib",args)!=0)
      {
         Sys.println("#### Error building " + arch);
         Sys.exit(-1);
      }
   }

   public function getBuildFile()
   {
      return "Build.xml";
   }

   public function getCleanDir()
   {
      return "obj";
   }

   public function cleanAll(inBuildFlags:Array<String>) : Bool
   {
      var args = ["run", "hxcpp", getBuildFile(), "clean", "-DHXCPP_CLEAN_ONLY"].concat(inBuildFlags);

      Sys.println("haxelib " + args.join(" ")); 
      if (Sys.command("haxelib",args)!=0)
      {
         Sys.println("#### Error cleaning");
         Sys.exit(-1);
      }
      return true;
   }



   public function onEmptyTarget() : Void
   {
      showUsage(true);
   }

   static public function deleteRecurse(inDir:String) : Void
   {
      if (FileSystem.exists(inDir))
      {
         var contents = FileSystem.readDirectory(inDir);
         for(item in contents)
         {
            if (item!="." && item!="..")
            {
               var name = inDir + "/" + item;
               if (FileSystem.isDirectory(name))
                  deleteRecurse(name);
               else
                  FileSystem.deleteFile(name);
            }
         }
         FileSystem.deleteDirectory(inDir);
      }
   }


   public function showUsage(inShowSpecifyMessage:Bool) : Void
   {
      var link = allowStatic() && allowNdll() ? "[link-]" : "";
      Sys.println("Usage : neko build.n [clean] " + link +
                  "target[-arch][-arch] ...] [-debug] [-verbose] [-D...]");
      Sys.println("  target  : ios, android, windows, linux, mac, mingw, tvos");
      Sys.println("            default (=current system)");
      if (link!="")
      {
         Sys.println("  link    : ndll- or static-");
         Sys.println("            (none specified = both link types, mingw static only");
      }
      Sys.println("  arch    : -armv5 -armv6 -armv7 -arm64 -x86 -x86_64 -m32 -m64");
      Sys.println("            (none specified = all valid architectures");
      Sys.println("  -D...   : defines passed to hxcpp build system");
      if (link!="")
         Sys.println(" eg: neko build.n clean ndll-mac-m32-m64 = rebuild both mac ndlls");
      if (inShowSpecifyMessage)
         Sys.println(" Specify target or 'default' to remove this message");
   }

   public function getDefault() : String
   {
      var sys = Sys.systemName();
      if (new EReg("window", "i").match(sys))
         return "windows";
      else if (new EReg("linux", "i").match(sys))
         return "linux";
      else if (new EReg("mac", "i").match(sys))
         return "mac";
      else
         throw "Unknown host system: " + sys;
      return "";
   }

   public static function main()
   {
      new Builder( Sys.args() );
   }
}

