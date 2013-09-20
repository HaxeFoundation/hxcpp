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
                   Sys.println("Copy config: " + src + " -> " + config );

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
     if (!defines.exists("ANDROID_NDK_ROOT"))
     {
        if (defines.exists("ANDROID_NDK_DIR"))
        {
           var root = Setup.findAndroidNdkRoot( defines.get("ANDROID_NDK_DIR") );
           if (BuildTool.verbose)
              Sys.println("Using found ndk root " + root);

           Sys.putEnv("ANDROID_NDK_ROOT", root);
           defines.set("ANDROID_NDK_ROOT", root);
        }
        else
           throw "ANDROID_NDK_ROOT or ANDROID_NDK_DIR should be set";
     }
     else
     {
        if (BuildTool.verbose)
           Sys.println("Using specified ndk root " + defines.get("ANDROID_NDK_ROOT") );
     }

     var found = false;
     for(i in 6...20)
        if (defines.exists("NDKV" + i))
        {
           found = true;
           if (BuildTool.verbose)
              Sys.println("Using specified android NDK " + i);
           break;
        }
     if (!found)
     {
        var version = Setup.getNdkVersion( defines.get("ANDROID_NDK_ROOT") );
        if (BuildTool.verbose)
            Sys.println("Deduced android NDK " + version);
        defines.set("NDKV" + version, "1" );
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
         Sys.println(message);
      }

      if (result=="")
         throw 'ANDROID_NDK_DIR "$inDir" does not contain matching ndk downloads.'; 

      return result;
   }

   public static function setup(inWhat:String,ioDefines: Map<String,String>)
   {
      if (inWhat=="androidNdk")
         setupAndroidNdk(ioDefines);
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
      if (!ioDefines.exists("NO_AUTO_MSVC"))
      {
         var extra = in64 ? "64" : "";
         var vc_setup_proc = new sys.io.Process("cmd.exe", ["/C", BuildTool.HXCPP + "build-tool\\msvc" + extra + "-setup.bat" ]);
         var vars_found = false;
         var error_found = false;
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
                     error_found = true;
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
                        "frameworkversion32", "devenvdir", "include", "lib", "libpath"
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
          if (!vars_found || error_found)
          {
             for(o in output)
                Sys.println(o);
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
                   Sys.println("Using msvc cl version " + cl_version);
                ioDefines.set("MSVC_VER", cl_version+"");
                if (cl_version>=17)
                   ioDefines.set("MSVC17+","1");
                BuildTool.sAllowNumProcs = cl_version >= 14;
             }
           }
       } catch(e:Dynamic){}
            //if (cl_version!="") Sys.println("Using cl version: " + cl_version);
    }

   public static function isRaspberryPi()
   {
      var proc = new sys.io.Process("uname",["-a"]);
      var str = proc.stdout.readLine();
      proc.close();
      return str.split(" ")[1]=="raspberrypi";
   }
}

