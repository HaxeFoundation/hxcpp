import sys.FileSystem;

class CompileCache
{
   public static var hasCache = false;
   public static var compileCache:String;


   public static function init(inDefines:Map<String, String>) : Bool
   {
      compileCache = "";
      hasCache = false;

      if (inDefines.exists("HXCPP_COMPILE_CACHE"))
      {
         compileCache = inDefines.get("HXCPP_COMPILE_CACHE");
         compileCache = compileCache.split("\\").join("/");
         // Don't get upset by trailing slash
         while(compileCache.length>1)
         {
            var l = compileCache.length;
            var last = compileCache.substr(l-1);
            if (last=="/")
               compileCache = compileCache.substr(0,l-1);
            else
               break;
         }

         if (!FileSystem.exists(compileCache))
         {
            try{
               PathManager.mkdir(compileCache);
            }
            catch(e:Dynamic)
            {
               Log.error("Could not create compiler cache directory \"" + compileCache + "\"");
            }
         }

         if (FileSystem.exists(compileCache) && FileSystem.isDirectory(compileCache))
         {
            hasCache = true;
         }
         else
         {
            Log.error("Could not find compiler cache \"" + compileCache + "\"");
            //throw "Could not find compiler cache: " + compileCache;
         }
      }


      if (hasCache)
      {
         Log.info("", "\x1b[33;1mUsing compiler cache: " + compileCache + "\x1b[0m");
      }

      return hasCache;
   }


/*
   public static function getPchCacheName(inProject:String,hash:String,header:String, inExt:String)
   {
      var dir = compileCache + "/" + inProject + "/pch" + hash.substr(0,8);
      try
      {
         if (!FileSystem.exists(dir))
            PathManager.mkdir(dir);
      } catch(e:Dynamic) { }
      return dir + "/" + header + inExt;
   }
*/

   public static function getPchDir(inProject:String,hash:String)
   {
      var dir = compileCache + "/" + inProject + "/pch" + hash.substr(0,8);
      try
      {
         if (!FileSystem.exists(dir))
            PathManager.mkdir(dir);
      } catch(e:Dynamic) { }
      return dir;
   }

   public static function getCacheName(inProject:String,hash:String,inExt:String)
   {
      var dir = compileCache + "/" + inProject + "/" + hash.substr(0,2);
      try
      {
         if (!FileSystem.exists(dir))
            PathManager.mkdir(dir);
      } catch(e:Dynamic) { }
      return dir + "/" + hash.substr(2) + inExt;
   }

   public static function clear(inDays:Int,inMB:Int,inLogInfo:Bool,inProject:String)
   {
      try
      {
        var projects = FileSystem.readDirectory(compileCache);
        var deleted = 0;
        var total = 0;
        var t0 = haxe.Timer.stamp();
        var tooOld = Date.now().getTime() - inDays * 24 * 3600 * 1000.0;
        var sizeKB:Float = 0;
        var fileInfo = [];

        for(project in projects)
        {
           if (inProject!=null && inProject!=project)
              continue;
           var projectHasDirs = false;
           var projDir = compileCache + "/" + project;
           if(!FileSystem.isDirectory(projDir))
               continue;
           var dirs = FileSystem.readDirectory(projDir);
           for(dir in dirs)
           {
              var path = projDir + "/" + dir;
              if(!FileSystem.isDirectory(path)) {
                  FileSystem.deleteFile(path);
                  continue;
              }
              if (dir.length!=2 && dir!="lib" && dir.substr(0,3)!="pch" )
              {
                 Log.warn('bad cache name "$dir" found - try manually clearing');
                 continue;
              }
              var dirFiles = FileSystem.readDirectory(path);
              var allDeleted = true;
              for(file in dirFiles)
              {
                 total++;
                 var filename = path + "/" + file;
                 var doDelete = true;
                 if (inDays>0)
                 {
                    var info = FileSystem.stat(filename);
                    var atime = info.atime;
                    var time = atime==null ? info.mtime.getTime() :
                                Math.max(info.atime.getTime(),info.mtime.getTime() );
                    if (time>=tooOld)
                       doDelete = false;
                 }
                 else if (inMB>0)
                 {
                    var info = FileSystem.stat(filename);
                    var atime = info.atime;
                    var time = atime==null ? info.mtime.getTime() :
                                Math.max(info.atime.getTime(),info.mtime.getTime() );
                    fileInfo.push( {filename:filename, time:time, size:info.size } );
                    sizeKB += info.size/1024;
                    doDelete = false;
                 }

                 if (doDelete)
                 {
                    try
                    {
                    FileSystem.deleteFile(filename);
                    deleted++;
                    }
                    catch(e:Dynamic)
                    {
                       Log.warn('Could not delete $filename');
                    }
                 }
                 else
                    allDeleted = false;
              }
              if (allDeleted)
              {
                 try
                 {
                    FileSystem.deleteDirectory(path);
                 }
                 catch(e:Dynamic)
                 {
                    Log.warn('Could not delete directory $path');
                 }
              }
              else
                 projectHasDirs = true;
           }
           if (!projectHasDirs)
           {
              try
              {
                 FileSystem.deleteDirectory(projDir);
              }
              catch(e:Dynamic)
              {
                Log.warn('Could not delete directory $projDir');
              }
           }
        }
          
        if (inMB*1024<sizeKB)
        {
           // newest first
           fileInfo.sort( function(a,b) return a.time > b.time ? -1 : 1 );
           var keepKB:Float = inMB*1024;
           for(info in fileInfo)
           {
              if (keepKB>0)
              {
                 sizeKB -= info.size/1024;
                 keepKB -= info.size/1024;
              }
              else
              {
                 try
                 {
                    FileSystem.deleteFile(info.filename);
                    deleted++;
                 }
                 catch(e:Dynamic)
                 {
                   Log.warn('Could not delete ${info.filename}');
                 }
              }
           }
        }

        var t = haxe.Timer.stamp()-t0;
        var projString = inProject==null ? "" : ' from project $inProject';
        var message = inMB > 0 ?
             'Cache: removed $deleted/$total files$projString, leaving ' + Std.int(sizeKB/1024) + 'MB, in $t seconds' :
             'Cache: removed $deleted/$total files$projString in $t seconds';
        if (inLogInfo)
           Log.info(message);
        else
           Log.v(message);
      }
      catch(error:Dynamic)
      {
         Log.warn("Error cleaning cache: " + error);
      }

   }

   public static function list(inDetails:Bool,inProject:String)
   {
      try
      {
        Sys.println('Cache Directory: $compileCache');
        var t0 = haxe.Timer.stamp();
        var files = new Array<String>();
        var projects = FileSystem.readDirectory(compileCache);
        var size = 0.0;
        var count = 0;

        for(project in projects)
        {
           if (inProject!=null && inProject!=project)
              continue;
           var projSize = size;
           var projCount = count;
           var projDir = compileCache + "/" + project;
           if(!FileSystem.isDirectory(projDir))
               continue;
           var dirs = FileSystem.readDirectory(projDir);
           for(dir in dirs)
           {
              var path = projDir + "/" + dir;
              if(!FileSystem.isDirectory(path))
                  continue;
              var dirFiles = FileSystem.readDirectory(path);
              for(file in dirFiles)
              {
                 var filename = path + "/" + file;
                 var info = FileSystem.stat(filename);
                 if (inDetails)
                 {
                    var atime = info.atime;
                    if (atime==null || atime.getTime()<info.mtime.getTime())
                       atime = info.mtime;
                    Sys.println('$filename : ${info.size} bytes, $atime');
                 }
                 count++;
                 size += info.size;
              }
              //files = files.concat(dirFiles);
           }
           projSize = Std.int( (size - projSize)/1024 );
           projCount = count - projCount;
           Sys.println('Project $project\t: ${projSize}k in $projCount files');
        }

        var k = Std.int(size/1024);
        var t = haxe.Timer.stamp()-t0;
        var projString = inProject==null ? "" : ' in project $inProject';
        Sys.println('Found: ${k}k in $count files$projString in $t seconds');
      }
      catch(error:Dynamic)
      {
         Log.warn("Error accessing cache: " + error);
      }
   }


}


