import sys.FileSystem;
import BuildTool;

class DirManager
{
   static var mMade = new Hash<Bool>();

   static public function deleteExtension(inExt:String)
   {
      var contents = FileSystem.readDirectory(".");
      for(item in contents)
      {
         if (item.length > inExt.length && item.substr(item.length-inExt.length)==inExt)
            deleteFile(item);
      }
   }

   static public function deleteFile(inName:String)
   {
      if (FileSystem.exists(inName))
      {
         BuildTool.log("rm " + inName);
         FileSystem.deleteFile(inName);
      }
   }

   static public function deleteRecurse(inDir:String)
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
               {
                  BuildTool.log("rm " + name);
                  FileSystem.deleteFile(name);
               }
            }
         }
         BuildTool.log("rmdir " + inDir);
         FileSystem.deleteDirectory(inDir);
      }
   }

   static public function make(inDir:String)
   {
      var parts = inDir.split("/");
      var total = "";
      for(part in parts)
      {
         if (part!="." && part!="")
         {
            if (total!="") total+="/";
            total += part;
            if (!mMade.exists(total))
            {
               mMade.set(total,true);
               if (!FileSystem.exists(total))
               {
                  try
                  {
                     #if haxe3
                     FileSystem.createDirectory(total + "/");
                     #else
                     FileSystem.createDirectory(total );
                     #end
                  } catch (e:Dynamic)
                  {
                     return false;
                  }
               }
            }
         }
      }
      return true;
   }

   static public function makeFileDir(inFile:String)
   {
      var parts = StringTools.replace (inFile, "\\", "/").split("/");
      if (parts.length<2)
         return;
      parts.pop();
      make(parts.join("/"));
   }

   public static function reset()
   {
      mMade = new Hash<Bool>();
   }
}