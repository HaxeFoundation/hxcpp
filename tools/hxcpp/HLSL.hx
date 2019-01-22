import haxe.io.Path;
import sys.FileSystem;

class HLSL
{
   var file:String;
   var profile:String;
   var target:String;
   var variable:String;

   public function new(inFile:String, inProfile:String, inVariable:String, inTarget:String)
   {
      file = inFile;
      profile = inProfile;
      variable = inVariable;
      target = inTarget;
   }

   public function build()
   {
      if (!FileSystem.exists(Path.directory (target))) 
      {
         PathManager.mkdir(Path.directory (target));
      }
     
      //DirManager.makeFileDir(target);

      var srcStamp = FileSystem.stat(file).mtime.getTime();
      if (!FileSystem.exists(target) || FileSystem.stat(target).mtime.getTime() < srcStamp)
      {
         var exe = "fxc.exe";
         var args =  [ "/nologo", "/T", profile, file, "/Vn", variable, "/Fh", target ];
         
         var result = ProcessManager.runCommand("", exe, args);
         if (result!=0)
         {
            Log.error("Could not compile shader \"" + file + "\"");
            //throw "Error : Could not compile shader " + file + " - build cancelled";
         }
      }
   }
}