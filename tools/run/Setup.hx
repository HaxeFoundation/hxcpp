import sys.FileSystem;
import BuildTool;


class Setup
{
   public static function initHXCPPConfig(ioDefines:Hash<String> )
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
         Sys.println("Warning: No 'HOME' variable set - .hxcpp_config.xml might be missing.");
         return;
      }

      var  config = toPath(home+"/.hxcpp_config.xml");
      ioDefines.set("HXCPP_CONFIG",config);

      if (BuildTool.HXCPP!="")
      {
         var src = toPath(BuildTool.HXCPP + "build-tool/example.hxcpp_config.xml");
         if (!sys.FileSystem.exists(config))
         {
            try {
               if (BuildTool.verbose)
                   BuildTool.println("Copy config: " + src + " -> " + config );

               sys.io.File.copy(src,config);
            } catch(e:Dynamic)
            {
               Sys.println("Warning : could not create config: " + config );
            }
         }
      }
   }

   static public function getNdkVersion(inDirName:String) : Int
   {
      var extract_version = ~/android-ndk-r(\d+)*/;
      if (extract_version.match(inDirName))
      {
         return Std.parseInt( extract_version.matched(1) );
      }
      //throw 'Could not deduce NDK version from "$inDirName"';
      return 8;
   }

   static public function setupAndroidNdk(defines: Map<String,String>)
   {
     var root:String = null;

     if (!defines.exists("ANDROID_NDK_ROOT"))
     {
        if (defines.exists("ANDROID_NDK_DIR"))
        {
           root = Setup.findAndroidNdkRoot( defines.get("ANDROID_NDK_DIR") );
           if (BuildTool.verbose)
              BuildTool.println("Using found ndk root " + root);

           Sys.putEnv("ANDROID_NDK_ROOT", root);
           defines.set("ANDROID_NDK_ROOT", root);
        }
        else
           throw "ANDROID_NDK_ROOT or ANDROID_NDK_DIR should be set";
     }
     else
     {
        root = defines.get("ANDROID_NDK_ROOT");
        if (BuildTool.verbose)
           BuildTool.println("Using specified ndk root " + root);
     }

     // Find toolchain
     if (!defines.exists("TOOLCHAIN_VERSION"))
     {
        try
        {
          var files = FileSystem.readDirectory(root+"/toolchains");

          // Prefer clang?
          var extract_version = ~/^arm-linux-androideabi-(\d.*)/;
          var bestVer="";
          for(file in files)
          {
             if (extract_version.match(file))
             {
                var ver = extract_version.matched(1);
                if ( ver>bestVer || bestVer=="")
                {
                   bestVer = ver;
                }
             }
          }
          if (bestVer!="")
          {
             defines.set("TOOLCHAIN_VERSION",bestVer);
             if (BuildTool.verbose)
                BuildTool.println("Found TOOLCHAIN_VERSION " + bestVer);
          }
        }
        catch(e:Dynamic) { }
     }

     // See what ANDROID_HOST to use ...
     try
     {
        var prebuilt =  root+"/toolchains/arm-linux-androideabi-" + defines.get("TOOLCHAIN_VERSION") + "/prebuilt";
        var files = FileSystem.readDirectory(prebuilt);
        if (files.length==1)
        {
           defines.set("ANDROID_HOST", files[0]);
           if (BuildTool.verbose)
           {
              BuildTool.println("Found ANDROID_HOST " + files[0]);
           }
        }
        else if (BuildTool.verbose)
           BuildTool.println("Could not work out ANDROID_HOST (" + files + ") - using default");
     }
     catch(e:Dynamic) { }

     var found = false;
     for(i in 6...20)
        if (defines.exists("NDKV" + i))
        {
           found = true;
           if (BuildTool.verbose)
              BuildTool.println("Using specified android NDK " + i);
           break;
        }
     if (!found)
     {
        var version = Setup.getNdkVersion( defines.get("ANDROID_NDK_ROOT") );
        if (BuildTool.verbose)
            BuildTool.println("Deduced android NDK " + version);
        defines.set("NDKV" + version, "1" );
     }

     if (defines.exists("PLATFORM"))
     {
        BuildTool.log("Using specified android PLATFORM " +  defines.get("PLATFORM") );
     }
     else
     {
        var base = defines.get("ANDROID_NDK_ROOT") + "/platforms";
        var best = 0;
        try
        {
           for(file in FileSystem.readDirectory(base))
           {
              if (file.substr(0,8)=="android-")
              {
                 var platform = Std.parseInt(file.substr(8));
                 if (platform>best)
                    best = platform;
              }
           }
        } catch(e:Dynamic)
        {
        }

        if (best==0)
           throw "Could not find platform in " + base;

        BuildTool.log("Using biggest platform " + best);
        defines.set("PLATFORM", "android-" + best );
     }
   }


   static function findAndroidNdkRoot(inDir:String)
   {
      var files:Array<String> = null;
      try
      {
         files = FileSystem.readDirectory(inDir);
      }
      catch (e:Dynamic)
      {
         throw 'ANDROID_NDK_DIR "$inDir" does not point to a valid directory.';
      }

      var extract_version = ~/^android-ndk-r(\d+)([a-z]?)$/;
      var bestMajor = 0;
      var bestMinor = "";
      var result = "";
      for(file in files)
         if (extract_version.match(file))
         {
            var major = Std.parseInt( extract_version.matched(1) );
            var minor = extract_version.matched(2);
            if ( major>bestMajor || (major==bestMajor && minor>bestMinor))
            {
               bestMajor = major;
               bestMinor = minor;
               result = inDir + "/" + file;
            }
         }


      if (BuildTool.verbose)
      {
         var message = "Found NDK " + result;
         BuildTool.println(message);
      }

      if (result=="")
         throw 'ANDROID_NDK_DIR "$inDir" does not contain matching ndk downloads.'; 

      return result;
   }

   public static function setup(inWhat:String,ioDefines: Map<String,String>)
   {
      if (inWhat=="androidNdk")
         setupAndroidNdk(ioDefines);
      else if (inWhat=="msvc")
         setupMSVC(ioDefines, ioDefines.exists("HXCPP_M64"));
      else
         throw 'Unknown setup feature $inWhat';
   }

   static function toPath(inPath:String)
   {
      if (!BuildTool.isWindows)
         return inPath;
      var bits = inPath.split("/");
      return bits.join("\\");
   }
   
   public static function setupBlackBerryNativeSDK(ioDefines:Hash<String>)
   {
      if (ioDefines.exists ("BLACKBERRY_NDK_ROOT") && (!ioDefines.exists("QNX_HOST") || !ioDefines.exists("QNX_TARGET")))
      {
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
            catch( ex:haxe.io.Eof ) 
            {}
            fin.close();
         }
      }
   }

   public static function setupMSVC(ioDefines:Hash<String>, in64:Bool )
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
                  for(env in Sys.environment().keys())
                  {
                     if (env.substr(0,2)=="VS")
                        Sys.println("Found VS variable " + env);
                  }
                  throw "Could not find specified MSCV version " + ival;
               }
               ioDefines.set("HXCPP_MSVC", where );
               Sys.putEnv("HXCPP_MSVC", where);
               BuildTool.log('Using MSVC Ver $ival in $where ($varName)');
            }
            else
            {
               BuildTool.log('Using specified MSVC Ver $val');
               ioDefines.set("HXCPP_MSVC", val );
               Sys.putEnv("HXCPP_MSVC", val);
            }
        }
      }

      if (detectMsvc)
      {
         var extra = in64 ? "64" : "";
         var xpCompat = false;
         if (ioDefines.exists("HXCPP_WINXP_COMPAT"))
         {
            Sys.putEnv("HXCPP_WINXP_COMPAT","1");
            xpCompat = true;
         }

         var vc_setup_proc = new sys.io.Process("cmd.exe", ["/C", BuildTool.HXCPP + "build-tool\\msvc" + extra + "-setup.bat" ]);
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
                        "frameworkversion32", "devenvdir", "include", "lib", "libpath", "hxcpp_xp_define"
                      :
                        var value = str.substr(pos+1);
                        ioDefines.set(name,value);
                        Sys.putEnv(name,value);
                  }
               }
            }
          } catch (e:Dynamic) {
          };

          vc_setup_proc.close();
          if (!vars_found || error_string!="")
          {
             for(o in output)
                BuildTool.println(o);
             if (error_string!="")
                throw(error_string);
             else
                BuildTool.println("Missing HXCPP_VARS");

             throw("Could not automatically setup MSVC");
          }
       }
      
   
       try
       {
          var proc = new sys.io.Process("cl.exe",[]);
          var str = proc.stderr.readLine();
          proc.close();
          if (str>"")
          {
             var reg = ~/Version\s+(\d+)/i;
             if (reg.match(str))
             {
                var cl_version = Std.parseInt(reg.matched(1));
                if (BuildTool.verbose)
                   BuildTool.println("Using msvc cl version " + cl_version);
                ioDefines.set("MSVC_VER", cl_version+"");
                if (cl_version>=17)
                   ioDefines.set("MSVC17+","1");
                if (cl_version>=18)
                   ioDefines.set("MSVC18+","1");
                BuildTool.sAllowNumProcs = cl_version >= 14;
                if (Std.parseInt(ioDefines.get("HXCPP_COMPILE_THREADS"))>1 && cl_version>=18)
                    ioDefines.set("HXCPP_FORCE_PDB_SERVER","1");
             }
           }
       } catch(e:Dynamic){}
            //if (cl_version!="") BuildTool.println("Using cl version: " + cl_version);
    }

   public static function isRaspberryPi()
   {
      var proc = new sys.io.Process("uname",["-a"]);
      var str = proc.stdout.readLine();
      proc.close();
      return str.split(" ")[1]=="raspberrypi";
   }

   public static function readStderr(inCommand:String,inArgs:Array<String>)
   {
      var result = new Array<String>();
      var proc = new sys.io.Process(inCommand,inArgs);
      try
      {
         while(true)
         {
            var out = proc.stderr.readLine();
            result.push(out);
         }
      } catch(e:Dynamic){}
      proc.close();
      return result;
   }

   public static function readStdout(inCommand:String,inArgs:Array<String>)
   {
      var result = new Array<String>();
      var proc = new sys.io.Process(inCommand,inArgs);
      try
      {
         while(true)
         {
            var out = proc.stdout.readLine();
            result.push(out);
         }
      } catch(e:Dynamic){}
      proc.close();
      return result;
   }

}

