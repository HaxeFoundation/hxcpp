import sys.FileSystem;

class CompileCache
{
   public static var hasCache = false;
   public static var compileCache:String;


   public static function init(inDefines:Map<String, String>)
   {
      compileCache = "";
      hasCache = false;

      if (inDefines.exists("HXCPP_COMPILE_CACHE"))
      {
         compileCache = inDefines.get("HXCPP_COMPILE_CACHE");
         // Don't get upset by trailing slash
         while(compileCache.length>1)
         {
            var l = compileCache.length;
            var last = compileCache.substr(l-1);
            if (last=="/" || last=="\\")
               compileCache = compileCache.substr(0,l-1);
            else
               break;
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

   }

   public static function getCacheName(hash:String)
   {
      var dir = compileCache + "/" + hash.substr(0,2);
      try
      {
         if (!FileSystem.exists(dir))
            FileSystem.createDirectory(dir);
      } catch(e:Dynamic) { }
      return dir + "/" + hash.substr(2);
   }

}


