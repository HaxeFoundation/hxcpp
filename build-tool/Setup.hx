class Setup
{
   public static function initHXCPPConfig(ioDefines:Hash<String> )
   {
      var env = neko.Sys.environment();
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
         neko.Lib.println("Warning: No 'HOME' variable set - .hxcpp_config.xml might be missing.");
         return;
      }

      var  config = toPath(home+"/.hxcpp_config.xml");
      ioDefines.set("HXCPP_CONFIG",config);

      var src = toPath(BuildTool.HXCPP + "build-tool/example.hxcpp_config.xml");
      if (!neko.FileSystem.exists(config))
      {
         try {
            if (BuildTool.verbose)
                neko.Lib.println("Copy config: " + src + " -> " + config );

            neko.io.File.copy(src,config);
         } catch(e:Dynamic)
         {
            neko.Lib.println("Warning : could not create config: " + config );
         }
      }
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
      if (ioDefines.exists ("BLACKBERRY_NDK_ROOT") && (!ioDefines.exists("QNX_HOST") || !ioDefines.exists("QNX_TARGET"))
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
         var fin = neko.io.File.read(fileName, false);
         try
         {
            while(true)
            {
               var str = fin.readLine();
               var split = str.split ("=");
               var name = StringTools.trim (split[0].substr (split[0].indexOf (" ") + 1));
               switch (name)
               {
                  case "QNX_HOST", "QNX_TARGET":
                     var value = split[1];
                     if (StringTools.startsWith(value, "\""))
                     {
                        value = value.substr (1);
                     }
                     if (StringTools.endsWith(value, "\""))
                     {
                        value = value.substr (0, value.length - 1);
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

   public static function setupMSVC(ioDefines:Hash<String> )
   {
      if (!ioDefines.exists("NO_AUTO_MSVC"))
      {
         var vc_setup_proc = new neko.io.Process("cmd.exe", ["/C", BuildTool.HXCPP + "build-tool\\msvc-setup.bat" ]);
         var vars_found = false;
         try{
            while(true)
            {
               var str = vc_setup_proc.stdout.readLine();
               if (str=="HXCPP_VARS")
                  vars_found = true;
               else if (!vars_found)
                  neko.Lib.println(str);
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
                        //trace(name + " = " + value);
                        ioDefines.set(name,value);
                        neko.Sys.putEnv(name,value);
                  }
               }
            }
          } catch (e:Dynamic) { };

          vc_setup_proc.close();
          if (!vars_found)
             throw("Could not automatically setup MSVC");
       }
      
   
       var cl_version = "";
       try
       {
          var proc = new neko.io.Process("cl.exe",[]);
          var str = proc.stderr.readLine();
          proc.close();
          if (str>"")
          {
             var reg = ~/Version\s+(\d+)/i;
             if (reg.match(str))
             {
                cl_version = reg.matched(1);
                if (BuildTool.verbose)
                   neko.Lib.println("Using msvc cl version " + cl_version);
                BuildTool.sAllowNumProcs = Std.parseInt(reg.matched(1)) >= 14;
             }
           }
       } catch(e:Dynamic){}
            //if (cl_version!="") neko.Lib.println("Using cl version: " + cl_version);
    }
}

