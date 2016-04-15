import sys.FileSystem;

class CopyFile
{
   public var name:String;
   public var from:String;
   public var toolId:String;
   public var allowMissing:Bool;
   public var overwrite:Overwrite;

   public function new(inName:String, inFrom:String, inAlowMissing:Bool, inOverwrite:Overwrite, ?inToolId:String)
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


   public static function copyFile(fromFile:String, toFile:String, allowMissing = false, overwrite:Overwrite = Overwrite.ALWAYS, addExePermission=false)
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

         var applyCopy = true;
         switch(overwrite)
         {
            case Overwrite.IF_NEWER:
               if(FileSystem.exists(toFile)
                  && (FileSystem.stat(fromFile).mtime.getTime() - FileSystem.stat(toFile).mtime.getTime()) <= 0)
               {
                  Log.v('The "$fromFile" is older or it hasn\'t changes. Skip copy');
                  applyCopy = false;
               }

            case Overwrite.NEVER:
               if(FileSystem.exists(toFile))
               {
                  Log.v('The "$toFile" file exists. Skip copy');
                  applyCopy = false;
               }

            case Overwrite.ALWAYS:

            default:
               Log.v('The Overwrite option "$overwrite" is not supported. ' +
                     'Possible values: {${Overwrite.ALWAYS}, ${Overwrite.IF_NEWER}, ${Overwrite.NEVER}}. ' +
                     'Applying "${Overwrite.ALWAYS}" by default');
         }

         if(applyCopy)
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

@:enum
abstract Overwrite(String) from String to String
{
    var ALWAYS = "always";
    var IF_NEWER = "ifNewer";
    var NEVER = "never";
}