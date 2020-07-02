import sys.FileSystem;

class PathManager
{
   private static var directoryCache = new Map<String,Bool>();
   private static var haxelibPaths = new Map<String,String>();
   
   public static function combine(firstPath:String, secondPath:String):String
   {
      if (firstPath == null || firstPath == "")
      {   
         return secondPath;  
      }
      else if (secondPath != null && secondPath != "" && secondPath!=".")
      {
         if (BuildTool.isWindows)
         {
            if (secondPath.indexOf (":") == 1)
            {
               return secondPath;
            }
         }
         else
         {
            if (secondPath.substr (0, 1) == "/")
            {
               return secondPath;
            }
         }
         
         var firstSlash = (firstPath.substr(-1) == "/" || firstPath.substr(-1) == "\\");
         var secondSlash = (secondPath.substr(0, 1) == "/" || secondPath.substr(0, 1) == "\\");
         
         if (firstSlash && secondSlash)
         {
            return firstPath + secondPath.substr(1);
         }
         else if (!firstSlash && !secondSlash)
         {
            return firstPath + "/" + secondPath;
         }
         else
         {
            return firstPath + secondPath;
         }
      }
      else
      {
         return firstPath;
      }
   }
   
   public static function escape(path:String):String
   {  
      if (!BuildTool.isWindows)
      {  
         path = StringTools.replace(path, "\\ ", " ");
         path = StringTools.replace(path, " ", "\\ ");
         path = StringTools.replace(path, "\\'", "'");
         path = StringTools.replace(path, "'", "\\'");   
      }
      else
      {  
         path = StringTools.replace(path, "^,", ",");
         path = StringTools.replace(path, ",", "^,"); 
      }
      return expand(path);
   }

   public static function expand(path:String):String
   {  
      if (path == null)
      {
         path = "";
      }
      
      if (!BuildTool.isWindows)
      {
         if (StringTools.startsWith(path, "~/"))
         {
            path = Sys.getEnv("HOME") + "/" + path.substr(2);  
         }
      }
      
      return path;
   }

   public static function getHaxelib (haxelib:String, version:String = "", validate:Bool = true, clearCache:Bool = false):String
   {   
      var name = haxelib;
      if (version != "")
      {
         name += ":" + version;
      }
      
      if (clearCache)
      {
         haxelibPaths.remove(name); 
      }
      
      if (!haxelibPaths.exists(name))
      {
         var cache = Log.verbose;
         Log.verbose = false;
         var output = "";
         
         try
         {
            output = ProcessManager.runProcess(Sys.getEnv ("HAXEPATH"), "haxelib", [ "path", name ], true, false);
         }
         catch (e:Dynamic) {}
         
         Log.verbose = cache;
         
         var lines = output.split("\n");
         var result = "";
         var re = new EReg("^-D " + haxelib + "(=.*)?$", ""); //matches "-D hxcpp=3.1.0" or "-D hxcpp", but not "-D hxcpp-extras"
         for (i in 1...lines.length)
         {
            if (re.match(StringTools.trim(lines[i])))
            {
               result = StringTools.trim(lines[i - 1]);
            }
         }
         
         if (result == "")
         {   
            for (line in lines)
            {
               if (line != "" && line.substr(0, 1) != "-")
               {
                  try
                  {
                     if (FileSystem.exists(line))
                     {
                        result = line;
                     }
                  }
                  catch (e:Dynamic) {}
               }
            }
         }
         
         if (validate)
         {
            if (result == "")
            {
               if (output.indexOf("does not have") > -1)
               {
                  var directoryName = "";
                  if (BuildTool.isWindows)
                  {
                     directoryName = "Windows";
                  }
                  else if (BuildTool.isMac)
                  {
                     directoryName = BuildTool.is64 ? "Mac64" : "Mac"; 
                  }
                  else
                  {
                     directoryName = BuildTool.is64 ? "Linux64" : "Linux";
                  }
                  
                  Log.error ("haxelib \"" + haxelib + "\" does not have an \"ndll/" + directoryName + "\" directory");
               }
               else
               {
                  if (version != "")
                  {
                     Log.error("Could not find haxelib \"" + haxelib + "\" version \"" + version + "\", does it need to be installed?");
                  }
                  else
                  {
                     Log.error("Could not find haxelib \"" + haxelib + "\", does it need to be installed?");
                  }
               }
            }
         }
         if ( result!="" )
         {
            var rootPath = result;
            var depth = 0;
            while( FileSystem.exists(rootPath) && FileSystem.isDirectory(rootPath) && depth<10 )
            {
               if (FileSystem.exists(rootPath + "/haxelib.json"))
               {
                   result = rootPath;
                   break;
               }
               depth++;
               rootPath = haxe.io.Path.directory(rootPath);
            }
         }
         haxelibPaths.set(name,result);
      }
      
      return haxelibPaths.get(name);
   }

   public static function isAbsolute(path:String):Bool
   {
      if (BuildTool.isWindows)
      {
         if (path != null && path.length > 2 && path.charAt(1) == ":" && (path.charAt(2) == "\\" || path.charAt(2) == "/"))
         {
            return true;
         }
      }
      else
      {
         if (StringTools.startsWith(path, "/") || StringTools.startsWith(path, "\\"))
         {
            return true;
         }
      }
      return false;
   }

   public static function mkdir(directory:String, skipFilePart=false):Void
   {
      directory = StringTools.replace(directory, "\\", "/");
      var total = "";
      
      if (directory.substr(0, 1) == "/")
      {
         total = "/";   
      }
      
      var parts = directory.split("/");
      if (skipFilePart && parts.length>0)
         parts.pop();
      
      if (parts.length > 0 && parts[0].indexOf(":") > -1)
      {
         total = parts.shift();
      }
      
      for (part in parts)
      {
         if (part != "." && part != "")
         {
            if (total != "" && total != "/")
            {
               total += "/";  
            }
            
            total += part;
            
            if (!directoryCache.exists (total))
            {
               //directoryCache.set(total, true);
               if (!FileSystem.exists(total))
               {
                  Log.info("", " - \x1b[1mCreating directory:\x1b[0m " + total);
                  FileSystem.createDirectory(total);
               }
            }
         }
      }
      
   }

   public static function removeDirectory(directory:String):Void
   {
      if (FileSystem.exists(directory))
      {
         var files;
         try
         {
            files = FileSystem.readDirectory(directory);
         }
         catch (e:Dynamic)
         {   
            return;  
         }
         
         for (file in FileSystem.readDirectory(directory))
         {   
            var path = directory + "/" + file;
            try
            {   
               if (FileSystem.isDirectory(path))
               {   
                  removeDirectory(path);
               }
               else
               {
                  FileSystem.deleteFile(path);
               }
            }
            catch (e:Dynamic) {}
         }
         
         Log.info("", " - \x1b[1mRemoving directory:\x1b[0m " + directory);
         
         try
         {   
            FileSystem.deleteDirectory(directory);
         }
         catch (e:Dynamic) {}
      }
   }

   static public function removeFile(file:String)
   {
      if (FileSystem.exists(file))
      {
         Log.info("", " - \x1b[1mRemoving file:\x1b[0m " + file);
         FileSystem.deleteFile(file);
      }
   }

   static public function removeFilesWithExtension(inExt:String)
   {
      var contents = FileSystem.readDirectory(".");
      for(item in contents)
      {
         if (item.length > inExt.length && item.substr(item.length-inExt.length)==inExt)
            removeFile(item);
      }
   }

   public static function resetDirectoryCache():Void
   {
      directoryCache = new Map<String,Bool>();
   }

   public static function standardize(path:String, trailingSlash:Bool = false):String
   {
      path = StringTools.replace (path, "\\", "/");
      path = StringTools.replace (path, "//", "/");
      path = StringTools.replace (path, "//", "/");
      
      if (!trailingSlash && StringTools.endsWith(path, "/"))
      {   
         path = path.substr(0, path.length - 1);
      }
      else if (trailingSlash && !StringTools.endsWith(path, "/"))
      {   
         path += "/";  
      }
      
      return path;
   }

   public static function clean(path:String)
   {
      var result = new Array<String>();
      for(part in standardize(path).split("/"))
      {
         if (part!=".")
         {
            if (part=="..")
            {
               if (result.length==0)
                  Log.error("Bad relative path " + path);
               result.pop();
            }
            else
               result.push(part);
         }
      }
      return result.join("/");
   }
}
