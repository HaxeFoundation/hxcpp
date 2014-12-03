import sys.FileSystem;

class CopyFile
{
   public var name:String;
   public var from:String;
   public var toolId:String;
   public var allowMissing:Bool;

   public function new(inName:String, inFrom:String, inAlowMissing:Bool, ?inToolId:String)
   {
      name = inName;
      from = inFrom;
      toolId = inToolId;
      allowMissing = inAlowMissing;
   }

   public function copy(inTo:String)
   {
      var fromFile = from + "/" + name;
      var toFile = inTo + name;

      if (!FileSystem.exists(fromFile))
      {
         if (allowMissing)
         {
            Log.v('Missing $fromFile - ignore');
            return;
         }
         Log.error("Error - source file does not exist " + fromFile);
      }
      try
      {
         Log.v('Copy $fromFile to $toFile');
         sys.io.File.copy( fromFile, toFile );
      }
      catch(e:Dynamic)
      {
         if (allowMissing)
         {
            Log.v('Could not copy to $toFile - ignore');
            return;
         }
         Log.error("Error - could not cooy to " + toFile);
      }
   }
}

