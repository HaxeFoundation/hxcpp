class Setup
{
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
             throw("Could not automaticall setup MSVC");
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
                BuildTool.sAllowNumProcs = Std.parseInt(reg.matched(1)) >= 14;
             }
           }
       } catch(e:Dynamic){}
            //if (cl_version!="") neko.Lib.println("Using cl version: " + cl_version);
    }
}

