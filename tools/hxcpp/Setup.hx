import haxe.io.Eof;
import sys.io.Process;
import sys.FileSystem;
import BuildTool;

class Setup
{
   static function findAndroidNdkRoot(defines: Map<String,String>, inBaseVersion:Int):String
   {
      var bestVersion = 0.0;
      var result:String = null;

      var ndkDir = defines.get("ANDROID_NDK_DIR");
      if (ndkDir!=null)
      {
         Log.v("Looking in ANDROID_NDK_DIR " + ndkDir);
      }
      else
      {
         Log.v("ANDROID_NDK_DIR not set");
         if (BuildTool.isMac)
         {
            var lib = defines.get("HOME") + "/Library/Android/sdk/ndk";
            if (FileSystem.exists(lib))
            {
               Log.v("trying default " + lib);
               ndkDir = lib;
            }
         }
      }

      if (ndkDir!=null)
      {
        ndkDir = ndkDir.split("\\").join("/");
        var files:Array<String> = null;
        var checkFiles:Bool = true;
        try
        {
             files = FileSystem.readDirectory(ndkDir);
        }
        catch (e:Dynamic)
        {
             Log.warn('ANDROID_NDK_DIR "$ndkDir" does not point to a valid directory');
             checkFiles=false;
        }
        if (checkFiles)
          for (file in files)
          {
             file = file.split("\\").join("/");
             var version = getNdkVersion(ndkDir + "/" + file);
             if (inBaseVersion==0 || Std.int(version)==inBaseVersion)
             {
                if (version>bestVersion)
                {
                   bestVersion = version;
                   result = ndkDir + "/" + file;
                }
             }
          }
      }

      Log.v("Looking in ANDROID_SDK/ndk-bundle");
      if (defines.exists("ANDROID_SDK"))
      {
        Log.v("checks default ndk-bundle in android sdk");
        var ndkBundle = defines.get("ANDROID_SDK")+"/ndk-bundle";
        var newStyle = false;
        if (!FileSystem.exists(ndkBundle) )
        {
           Log.v("ndk-bundle directory not found in sdk,try ndk");
           var altDir = defines.get("ANDROID_SDK")+"/ndk/";
           if (FileSystem.exists(altDir) )
           {
              var alt = findBestNdk(altDir);
              if (alt!=null)
              {
                 Log.v('using $alt ndk dir');
                 ndkBundle = alt;
              }
           }
        }
        ndkBundle = ndkBundle.split("\\").join("/");
        var version = getNdkVersion(ndkBundle, newStyle);
        if (version>bestVersion && (inBaseVersion==0 || inBaseVersion==Std.int(version)) )
        {
           Log.v("Using default ndk-bundle in android sdk:" + ndkBundle);
           result = ndkBundle;
        }
      }

      return result;
   }

   static function findBestNdk(root:String) : String
   {
     var versionMatch = ~/(\d+)\.(\d+\.\d+)/;
     var version:String = null;
     var best = 0.0;
     try
     {
        for (file in FileSystem.readDirectory(root))
        {
           if (versionMatch.match(file))
           {
               var maj = Std.parseInt(versionMatch.matched(1));
               var minor = Std.parseFloat(versionMatch.matched(2));
               var combined = maj*1000 + minor;
               Log.v("  found ndk:" + file);
               if (combined>best)
               {
                  best = combined;
                  version = file;
               }
           }
        }
      }
      catch(e:Dynamic)
      {
      }

      if (version!=null)
         return root + "/" + version;
    
      return null;
   }

   static var gotNdkVersion = 0.0;
   static var cachedNdkPath:Null<String> = null;
   static public function getNdkVersion(inDirName:String, newStyle=false):Float
   {
      if (gotNdkVersion!=0 && cachedNdkPath==inDirName)
         return gotNdkVersion;

      cachedNdkPath = inDirName;

      Log.v("Try to get version from source.properties");
      var src = toPath(inDirName+"/source.properties");
      if (sys.FileSystem.exists(src))
      {
         var fin = sys.io.File.read(src, false);
         try
         {
            while (true)
            {
               var str = fin.readLine();
               var split = str.split ("=");
               var name = StringTools.trim(split[0]);
               if (name == "Pkg.Revision")
               {
                  var revision = StringTools.trim(split[1]);
                  var split2 = revision.split( "." );
                  var result:Float = 1.0 * Std.parseInt(split2[0]) + 0.001 * Std.parseInt(split2[1]);
                  if (result>=8)
                  {
                     Log.v('Deduced NDK version '+result+' from "$inDirName/source.properties"');
                     fin.close();
                     gotNdkVersion = result;
                     return result;
                  }
               }
            }
         }
         catch (e:haxe.io.Eof)
         {
            Log.v('Could not deduce NDK version from "$inDirName"/source.properties');
         }
         fin.close();
      }

      var dir = inDirName.split("\\").join("/");
      Log.v('Try to get version from directory name "$dir"');
      var extract_version = ~/\/?(android-ndk-)?r(\d+)([a-z]?)$/;
      if (extract_version.match(dir))
      {
         var major:Int = Std.parseInt( extract_version.matched(2) );
         var result:Float = 1.0 * major;
         var minor = extract_version.matched(3);
         if (minor!=null && minor.length>0)
            result += 0.001 * (minor.toLowerCase().charCodeAt(0)-'a'.code);
         gotNdkVersion = result;
         return result;
      }

      Log.v('Could not deduce NDK version from "$inDirName" - assuming 8');
      gotNdkVersion = 8;
      return gotNdkVersion;
   }

   public static function initHXCPPConfig(ioDefines:Hash<String>)
   {
      var env = Sys.environment();
      // If the user has set it themselves, they mush know what they are doing...
      if (env.exists("HXCPP_CONFIG"))
         return;

      var home = "";
      if (env.exists("HOME"))
         home = env.get("HOME");
      else if (env.exists("USERPROFILE"))
         home = env.get("USERPROFILE");
      else
      {
         Log.warn("No $HOME variable set, \".hxcpp_config.xml\" might be missing");
         //Sys.println("Warning: No 'HOME' variable set - .hxcpp_config.xml might be missing.");
         return;
      }

      ioDefines.set("HXCPP_HOME", home);

      var  config = toPath(home+"/.hxcpp_config.xml");
      ioDefines.set("HXCPP_CONFIG",config);

      if (BuildTool.HXCPP!="")
      {
         var src = toPath(BuildTool.HXCPP + "/toolchain/example.hxcpp_config.xml");
         if (!sys.FileSystem.exists(config))
         {
            try
            {
               Log.info("", "Copying HXCPP config \"" + src + "\" to \"" + config + "\"");
               sys.io.File.copy(src,config);
            }
            catch(e:Dynamic)
            {
               Log.warn("Could not create HXCPP config \"" + config + "\"");
               //Sys.println("Warning : could not create config: " + config );
            }
         }
      }
   }

   public static function setupMingw(ioDefines:Hash<String>)
   {
      // Setup MINGW_ROOT or fail
      if (!ioDefines.exists("MINGW_ROOT"))
      {

         var haxelib = PathManager.getHaxelib("minimingw","",false);
         if (haxelib!=null && haxelib!="")
         {
            ioDefines.set("MINGW_ROOT", haxelib);
            Log.v('Using haxelib version of MinGW, $haxelib');
            return;
         }

         var guesses = ["c:/MinGW"];
         for (guess in guesses)
         {
            if (FileSystem.exists(guess))
            {
               ioDefines.set("MINGW_ROOT", guess);
               Log.v('Using default version of MinGW, $guess');
               return;
            }
         }

         if (ioDefines.exists("mingw"))
         {
            //when mingw is explicitly indicated but not properly configured, this log will be shown
            Log.error('Could not guess MINGW_ROOT (tried $guesses) - please set explicitly');
         }
         else
         {
            //when both mingw and MSVC is not properly configured, this log will be shown
            Log.error('Could not setup any C++ compiler, please install or reinstall a valid C++ compiler');
         }
      }
   }


   public static function setupEmscripten(ioDefines:Hash<String>)
   {
      // Setup EMSDK if possible - else assume developer has it in path
      if (!ioDefines.exists("EMSDK") )
      {
         var home = ioDefines.get("HXCPP_HOME");
         var file = home + "/.emscripten";
         Log.v('No EMSDK provided, checking $file');
         if (FileSystem.exists(file))
         {
            var content = sys.io.File.getContent(file);
            content = content.split("\r").join("");
            var value = ~/^(\w*)\s*=\s*'(.*)'/;
            for (line in content.split("\n"))
            {
               if (value.match(line))
               {
                  var name = value.matched(1);
                  var val= value.matched(2);
                  if (name=="EMSCRIPTEN_ROOT")
                  {
                     ioDefines.set("EMSDK", val);
                  }
                  if (name=="PYTHON")
                     ioDefines.set("EMSDK_PYTHON", val);
                  if (name=="NODE_JS")
                     ioDefines.set("EMSDK_NODE", val);
               }
            }
         }
      }
      else
      {
         Log.v('Using provided EMSDK ${ioDefines.get("EMSDK")}');
      }

      if (!ioDefines.exists("EMSDK_PYTHON"))
      {
         Log.v("No EMSDK_PYTHON provided, using 'python'");
      }
      else
         Log.v('Using provided EMSDK_PYTHON ${ioDefines.get("EMSDK_PYTHON")}');

      if (!ioDefines.exists("EMSDK_NODE"))
      {
         Log.v("No EMSDK_NODE provided, using 'node'");
         ioDefines.set("EMSDK_NODE", "node");
      }
   }


   public static function isRaspberryPi()
   {
       var modelFile = '/sys/firmware/devicetree/base/model';
       if( !FileSystem.exists( modelFile ) )
           return false;
       try {
           var model = sys.io.File.getContent( modelFile );
           return ~/Raspberry/.match( model );
       } catch(e:Dynamic) {
           trace( e );
       }
       return false;
   }

   static public function startPdbServer()
   {
      var oldPath = Sys.getCwd();
      try
      {
         // Run it in hxcpp directory so it does not lock the build directory after build finishes
         Sys.setCwd(BuildTool.HXCPP);
         var proc = new Process("mspdbsrv.exe",["-start"]);
         Tools.addOnExitHook(function(_) {
           proc.kill();
         });
      }
      catch(e:Dynamic)
      {
         Log.v("Could not start mspdbsrv:" + e);
      }
      Sys.setCwd(oldPath);
   }




   public static function setup(inWhat:String,ioDefines:Map<String,String>)
   {
      if (ioDefines.exists("HXCPP_CLEAN_ONLY"))
         return;

      Profile.push("setup " + inWhat);
      if (inWhat=="androidNdk")
      {
         setupAndroidNdk(ioDefines);
      }
      else if (inWhat=="blackberry")
      {
         setupBlackBerryNativeSDK(ioDefines);
      }
      else if (inWhat=="msvc")
      {
         setupMSVC(ioDefines, ioDefines.exists("HXCPP_M64"), ioDefines.exists("HXCPP_ARM64"), ioDefines.exists("winrt"));
      }
      else if (inWhat=="pdbserver")
      {
         startPdbServer();
      }
      else if (inWhat=="mingw")
      {
         setupMingw(ioDefines);
      }
      else if (inWhat=="emscripten")
      {
         setupEmscripten(ioDefines);
      }
      else if (inWhat=="nvcc")
      {
         BuildTool.setupNvcc();
      }
      else
      {
         Log.error('Unknown setup feature "$inWhat"');
         //throw 'Unknown setup feature $inWhat';
      }
      Profile.pop();
   }

   static public function setupAndroidNdk(defines:Map<String,String>)
   {
      var root:String = null;

      if (Log.verbose) Log.println("");

      var ndkVersion = 0;
      for (i in 6...20)
      {
         if (defines.exists("NDKV" + i))
         {
            ndkVersion = i;
            Log.info("", "\x1b[33;1mRequested Android NDK r" + i + "\x1b[0m");
            break;
         }
      }

      if (!defines.exists("ANDROID_NDK_ROOT") || ndkVersion!=0)
      {
         root = Setup.findAndroidNdkRoot( defines, ndkVersion );
         if (root==null)
         {
            var ndkDir = defines.exists("ANDROID_NDK_DIR") ? defines.get("ANDROID_NDK_DIR") : "not set";
            if (ndkVersion!=0)
               Log.error('ANDROID_NDK_DIR ["$ndkDir"] or ndk-bundle does not contain requested NDK $ndkVersion');
            else
               Log.error('ANDROID_NDK_DIR ["$ndkDir"] or ndk-bundle does not contain a matching NDK');
         }
         else
         {
            Sys.putEnv("ANDROID_NDK_ROOT", root);
            defines.set("ANDROID_NDK_ROOT", root);
         }
      }
      else
      {
         root = defines.get("ANDROID_NDK_ROOT");
         Log.setup("\x1b[33;1mUsing Android NDK root: " + root + "\x1b[0m");
      }

      if (ndkVersion==0)
      {
         var version = Setup.getNdkVersion( root );
         if (version > 0)
         {
            Log.setup("\x1b[33;1mDetected Android NDK " + version + "\x1b[0m");
            defines.set("NDKV" + Std.int(version), "1" );
            ndkVersion = Std.int(version);
         }
         else
            Log.error("Invalid ndk version");
      }
      for (i in 5...ndkVersion+1)
         defines.set("NDKV" + i + "+", "1");

      var arm_type = 'arm-linux-androideabi';
      var arm_64 = defines.exists('HXCPP_ARM64');
      if(arm_64) arm_type = 'aarch64-linux-android';

      // Find toolchain
      if (!defines.exists("TOOLCHAIN_VERSION"))
      {
         try
         {
            var files = FileSystem.readDirectory(root+"/toolchains");

            // Prefer clang?
            var extract_version = ~/^arm-linux-androideabi-(\d.*)/;
            if(arm_64) extract_version = ~/^aarch64-linux-android-(\d.*)/;

            var bestVer="";
            for (file in files)
            {
               if (extract_version.match(file))
               {
                  var ver = extract_version.matched(1);
                  if (ver<bestVer || bestVer=="")
                  {
                     bestVer = ver;
                  }
               }
            }
            if (bestVer!="")
            {
               defines.set("TOOLCHAIN_VERSION",bestVer);
               Log.setup("\x1b[33;1mDetected Android toolchain: "+arm_type+"-" + bestVer + "\x1b[0m");
            }
         }
         catch(e:Dynamic) { }
      }

      // See what ANDROID_HOST to use ...
      try
      {
         var prebuilt = root+"/toolchains/";
         if (defines.exists("TOOLCHAIN_VERSION"))
            prebuilt += arm_type + "-" + defines.get("TOOLCHAIN_VERSION") + "/prebuilt";
         else
            prebuilt += "llvm/prebuilt";
         var files = FileSystem.readDirectory(prebuilt);
         for (file in files)
         {
            if (!FileSystem.isDirectory (prebuilt + "/" + file))
            {
               files.remove (file);
            }
         }
         if (files.length==1)
         {
            defines.set("ANDROID_HOST", files[0]);
            Log.info("", "\x1b[33;1mDetected Android host: " + files[0] + "\x1b[0m");
         }
         else
         {
            Log.info("", "\x1b[33;1mCould not detect ANDROID_HOST (" + files + ") - using default\x1b[0m");
         }
      }
      catch(e:Dynamic) { }

      if (defines.exists('HXCPP_ANDROID_PLATFORM')) {
         Log.setup("\x1b[33;1mUsing Android NDK platform: " + defines.get("HXCPP_ANDROID_PLATFORM") + "\x1b[0m");
      }
      else if (defines.exists('NDKV19+')) {
         if (defines.exists("PLATFORM_NUMBER")) {
            Log.warn("The PLATFORM_NUMBER define is deprecated. Please use the HXCPP_ANDROID_PLATFORM define instead.");
            defines.set("HXCPP_ANDROID_PLATFORM", Std.string(defines.get("PLATFORM_NUMBER")));
         } else {
            var platformsJson = root + "/meta/platforms.json";

            var minPlatform:Null<Int> = try {
               haxe.Json.parse(sys.io.File.getContent(platformsJson)).min;
            } catch (e) {
               Log.warn("Unable to determine minimum supported Android platform: " + e.toString());
               null;
            };

            if (minPlatform == null) {
               Log.warn("Defaulting to Android platform 21");
               minPlatform = 21;
            }

            // only platform version 21 and above support 64 bit
            // https://developer.android.com/about/versions/lollipop#Perf
            if (minPlatform < 21 && (defines.exists('HXCPP_ARM64') || defines.exists('HXCPP_X86_64'))) {
               minPlatform = 21;
            }

            defines.set("HXCPP_ANDROID_PLATFORM", Std.string(minPlatform));
         }
      }
      else {
         globallySetThePlatform(root, defines);
      }
   }

   private static function globallySetThePlatform(root:String, defines:Map<String,String>) {
      var androidPlatform = 5;
      if (!defines.exists("PLATFORM"))
      {
         for (i in 5...100)
         {
            var test = "android-" + i;
            if (defines.exists(test))
            {
               defines.set("PLATFORM",test);
               break;
            }
         }
      }

      if (defines.exists("PLATFORM"))
      {
         var platform = defines.get("PLATFORM");
         var id = Std.parseInt( platform.substr("android-".length) );
         if (id==0 || id==null)
            Log.error('Badly formed android PLATFORM "$platform" - should be like android-123');
         androidPlatform = id;
         Log.info("", "\x1b[33;1mUsing Android NDK platform: " + defines.get("PLATFORM") + "\x1b[0m");
      }
      else
      {
         var base = root + "/platforms";
         var best = 0;
         try
         {
            for (file in FileSystem.readDirectory(base))
            {
               if (file.substr(0,8)=="android-")
               {
                  var platform = Std.parseInt(file.substr(8));
                  if (platform>best)
                     best = platform;
               }
            }
         } catch(e:Dynamic) {}

         if (best==0)
         {
            Log.error("Could not detect Android API platforms in \"" + base + "\"");
            //throw "Could not find platform in " + base;
         }

         Log.info("", "\x1b[33;1mUsing newest Android NDK platform: " + best + "\x1b[0m");
         defines.set("PLATFORM", "android-" + best);
         androidPlatform = best;
      }
      defines.set("HXCPP_ANDROID_PLATFORM", Std.string(androidPlatform));
      if (Log.verbose) Log.println("");
   }

   public static function setupBlackBerryNativeSDK(ioDefines:Hash<String>)
   {
      if (!ioDefines.exists ("BLACKBERRY_NDK_ROOT"))
      {
          Log.error("Could not find BLACKBERRY_NDK_ROOT variable");
      }

      var fileName = ioDefines.get ("BLACKBERRY_NDK_ROOT");
      if (BuildTool.isWindows)
      {
         fileName += "\\bbndk-env.bat";
      }
      else
      {
         fileName += "/bbndk-env.sh";
      }

      if (FileSystem.exists (fileName))
      {
         var fin = sys.io.File.read(fileName, false);
         try
         {
            while(true)
            {
               var str = fin.readLine();
               var split = str.split ("=");
               var name = StringTools.trim (split[0].substr (split[0].lastIndexOf (" ") + 1));
               switch (name)
               {
                  case "QNX_HOST", "QNX_TARGET", "QNX_HOST_VERSION", "QNX_TARGET_VERSION":
                     var value = split[1];
                     if (StringTools.startsWith (value, "${") && split.length > 2)
                     {
                        value = split[2].substr (0, split[2].length - 1);
                     }
                     if (StringTools.startsWith(value, "\""))
                     {
                        value = value.substr (1);
                     }
                     if (StringTools.endsWith(value, "\""))
                     {
                        value = value.substr (0, value.length - 1);
                     }
                     if (name == "QNX_HOST_VERSION" || name == "QNX_TARGET_VERSION")
                     {
                        if (Sys.getEnv (name) != null)
                        {
                           continue;
                        }
                     }
                     else
                     {
                        value = StringTools.replace (value, "$BASE_DIR", ioDefines.get ("BLACKBERRY_NDK_ROOT"));
                        value = StringTools.replace (value, "%BASE_DIR%", ioDefines.get ("BLACKBERRY_NDK_ROOT"));
                        value = StringTools.replace (value, "$TARGET", "qnx6");
                        value = StringTools.replace (value, "%TARGET%", "qnx6");
                        value = StringTools.replace (value, "$QNX_HOST_VERSION", Sys.getEnv("QNX_HOST_VERSION"));
                        value = StringTools.replace (value, "$QNX_TARGET_VERSION", Sys.getEnv("QNX_TARGET_VERSION"));
                        value = StringTools.replace (value, "%QNX_HOST_VERSION%", Sys.getEnv("QNX_HOST_VERSION"));
                        value = StringTools.replace (value, "%QNX_TARGET_VERSION%", Sys.getEnv("QNX_TARGET_VERSION"));
                     }
                     ioDefines.set(name,value);
                     Sys.putEnv(name,value);
               }
            }
         }
         catch( ex:Eof )
         {}
         fin.close();
      }
      else
      {
         Log.error("Could not find \"" + fileName + "\"");
      }
   }

   public static function setupMSVC(ioDefines:Hash<String>, in64:Bool, inArm64, isWinRT:Bool)
   {
      var detectMsvc = !ioDefines.exists("NO_AUTO_MSVC") &&
                       !ioDefines.exists("HXCPP_MSVC_CUSTOM");

      if (ioDefines.exists("HXCPP_MSVC_VER"))
      {
         var val = ioDefines.get("HXCPP_MSVC_VER");
         if (val=="")
            detectMsvc = false;
         else
         {
            var ival = Std.parseInt(ioDefines.get("HXCPP_MSVC_VER"));
            if (ival>0)
            {
               var varName = "VS" + ival+ "COMNTOOLS";
               var where = Sys.getEnv(varName);
               if (where==null)
               {
                  for (env in Sys.environment().keys())
                  {
                     if (env.substr(0,2)=="VS")
                     {
                        Log.info("Found VS variable: " + env);
                        //Sys.println("Found VS variable " + env);
                     }
                  }
                  Log.error("Could not find specified MSVC version: " + ival);
                  //throw "Could not find specified MSVC version " + ival;
               }
               ioDefines.set("HXCPP_MSVC", where );
               Sys.putEnv("HXCPP_MSVC", where);
               Log.setup('Using MSVC Ver $ival in $where ($varName)');
            }
            else
            {
               Log.setup('Using specified MSVC Ver $val');
               ioDefines.set("HXCPP_MSVC", val );
               Sys.putEnv("HXCPP_MSVC", val);
            }
        }
      }

      if (detectMsvc)
      {
        var extra:String = "";
        if (isWinRT)
            extra += "-winrt";
        if (inArm64)
            extra += "-arm64";
        else if (in64)
            extra += "64";
         var xpCompat = false;
         if (ioDefines.exists("HXCPP_WINXP_COMPAT"))
         {
            Sys.putEnv("HXCPP_WINXP_COMPAT","1");
            xpCompat = true;
         }
         Sys.putEnv("msvc_host_arch", ioDefines.exists("windows_arm_host") ? "x86" : "x64" );

         var vc_setup_proc = new Process("cmd.exe", ["/C", BuildTool.HXCPP + "\\toolchain\\msvc" + extra + "-setup.bat" ]);
         var vars_found = false;
         var error_string = "";
         var output = new Array<String>();
         try{
            while(true)
            {
               var str = vc_setup_proc.stdout.readLine();
               if (str=="HXCPP_VARS")
                  vars_found = true;
               else if (!vars_found)
               {
                  if (str.substr(0,5)=="Error" || ~/missing/.match(str) )
                     error_string = str;

                  output.push(str);
               }
               else
               {
                  var pos = str.indexOf("=");
                  var name = str.substr(0,pos);
                  switch(name.toLowerCase())
                  {
                     case "path", "vcinstalldir", "windowssdkdir","framework35version",
                        "frameworkdir", "frameworkdir32", "frameworkversion",
                        "frameworkversion32", "devenvdir", "include", "lib", "libpath", "hxcpp_xp_define",
                        "hxcpp_hack_pdbsrv"
                      :
                        var value = str.substr(pos+1);
                        ioDefines.set(name,value);
                        Log.v('  msvs $name=$value');
                        Sys.putEnv(name,value);
                  }
               }
            }
         } catch (e:Dynamic) {
         };

         vc_setup_proc.exitCode();
         vc_setup_proc.close();
         if (!vars_found || error_string!="")
         {
            for (o in output)
            {
               Log.info(o);
               //BuildTool.println(o);
            }
            if (error_string!="")
            {
               Log.error (error_string);
               //throw(error_string);
            }
            else
            {
               Log.info("Missing HXCPP_VARS");
               //BuildTool.println("Missing HXCPP_VARS");
            }

            Log.error("Could not automatically setup MSVC");
            //throw("Could not automatically setup MSVC");
         }
      }

      try
      {
         var proc = new Process("cl.exe",[]);
         var str = proc.stderr.readLine();
         proc.close();
         if (str>"")
         {
            Log.v("MSVC output:" + str);
            var reg = ~/(\d{2})\.\d+/i;
            if (reg.match(str))
            {
               var cl_version = Std.parseInt(reg.matched(1));
               Log.setup("Using MSVC version: " + cl_version);
               ioDefines.set("MSVC_VER", cl_version+"");
               if (cl_version>=17)
                  ioDefines.set("MSVC17+","1");
               if (cl_version>=18)
                  ioDefines.set("MSVC18+","1");
               if (cl_version==19)
                  ioDefines.set("MSVC19","1");
               BuildTool.sAllowNumProcs = cl_version >= 14;
               var threads = BuildTool.getThreadCount();
               if (threads>1 && cl_version>=18)
                  ioDefines.set("HXCPP_FORCE_PDB_SERVER","1");
            }
         }
      } catch(e:Dynamic){}
      //if (cl_version!="") BuildTool.println("Using cl version: " + cl_version);
   }

   static function toPath(inPath:String)
   {
      if (!BuildTool.isWindows)
         return inPath;
      var bits = inPath.split("/");
      return bits.join("\\");
   }
}
