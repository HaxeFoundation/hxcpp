import sys.FileSystem;

class CopyFile
{
   public var name:String;
   public var from:String;
   public var toolId:String;
   public var allowMissing:Bool;
   public var overwrite:Bool;

   public function new(inName:String, inFrom:String, inAlowMissing:Bool, inOverwrite:Bool, ?inToolId:String)
   {
      name = inName;
      from = inFrom;
      toolId = inToolId;
      allowMissing = inAlowMissing;
      overwrite = inOverwrite;
   }

   public function copy(inTo:String)
   {
      var fromFile = from + "/" + name;
      var toFile = inTo + name;
      copyFile(fromFile, toFile, allowMissing, overwrite);
   }


   public static function copyFile(fromFile:String, toFile:String, allowMissing = false, overwrite:Bool = true, addExePermission=false)
   {
      if (!FileSystem.exists(fromFile))
      {
         if (allowMissing)
         {
            Log.v('Missing "$fromFile" - ignore');
            return;
         }
         Log.error("Error - source file does not exist " + fromFile);
      }
      try
      {
         Log.v('Copy "$fromFile" to "$toFile"');
         if(!overwrite && sys.FileSystem.exists(toFile))
         {
            Log.v('The "$toFile" file exists. Skip copy');
         }
         else
         {
            sys.io.File.copy( fromFile, toFile );
            if (addExePermission)
            {
               Log.v("chmod 755 " + toFile );
               Sys.command("chmod", ["755", toFile]);
            }
         }
      }
      catch(e:Dynamic)
      {
         if (allowMissing)
         {
            Log.v('Could not copy to "$toFile" - ignore');
            return;
         }
         Log.error('Error $e - could not copy to "$toFile"');
      }
   }
}

