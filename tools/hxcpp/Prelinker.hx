import haxe.io.Path;
import sys.FileSystem;

class Prelinker
{
   public var mExe:String;
   public var mFlags:Array<String>;
   public var mOutFlag:String;
   public var mFromFile:String;
   public var mExpandArchives:Bool;

   public function new(inExe:String)
   {
      mFlags = [];
      mOutFlag = "-o";
      mExe = inExe;
      mExpandArchives = false;
      // Default to on...
      mFromFile = "@";
   }

   function isOutOfDate(inName:String, inObjs:Array<String>)
   {
      if (!FileSystem.exists(inName))
         return true;
      var stamp = FileSystem.stat(inName).mtime.getTime();
      for(obj in inObjs)
      {
         if (!FileSystem.exists(obj))
         {
            Log.error("Could not find \"" + obj + "\" required by \"" + inName + "\"");
            //throw "Could not find " + obj + " required by " + inName;
         }
         var obj_stamp = FileSystem.stat(obj).mtime.getTime();
         if (obj_stamp > stamp)
            return true;
      }
      return false;
   }

   public function prelink(inTarget:Target,inObjs:Array<String>,inCompiler:Compiler)
   {
      var file_name = "prelink.o";

      try
      {
         PathManager.mkdir(inTarget.mOutputDir);
      }
      catch (e:Dynamic)
      {
         Log.error("Unable to create output directory \"" + inTarget.mOutputDir + "\"");
         //throw "Unable to create output directory " + inTarget.mOutputDir;
      }

      var out_name = inCompiler.mObjDir + "/" + file_name;

      if (isOutOfDate(out_name,inObjs) || isOutOfDate(out_name,inTarget.mDepends))
      {
         var args = new Array<String>();
         var out = mOutFlag;
         if (out.substr(-1)==" ")
         {
            args.push(out.substr(0,out.length-1));
            out = "";
         }

         args.push(out + out_name);
         //args = args.concat(mFlags).concat(inTarget.mFlags);
         args = args.concat(mFlags);

         var objs = inObjs.copy();

         /*if (mExpandArchives)
         {
            var isArchive = ~/\.a$/;
            var libArgs = new Array<String>();
            for(lib in libs)
            {
               if (isArchive.match(lib))
               {
                  var libName = Path.withoutDirectory(lib);
                  var libObjs = Setup.readStdout(mExe, ["t", lib ]);
                  var objDir = inCompiler.mObjDir + "/" + libName;
                  PathManager.mkdir(objDir);
                  ProcessManager.runCommand (objDir, mExe, ["x", lib]);
                  for(obj in libObjs)
                     objs.push( objDir+"/"+obj );
               }
               else
                  libArgs.push(lib);
            }
            libs = libArgs;
         }*/

         // Place list of obj files in a file called "all_objs"
         if (mFromFile=="@")
         {
            var fname = inCompiler.mObjDir + "/all_objs";
            var fout = sys.io.File.write(fname,false);
            for(obj in objs)
               fout.writeString(obj + "\n");
            fout.close();
            args.push("@" + fname );
         }
         else
            args = args.concat(objs);

         //args = args.concat(libs);

         var result = ProcessManager.runCommand("", mExe, args);
         if (result!=0)
         {
            Tools.exit(result);
            //throw "Error : " + result + " - build cancelled";
         }

         return out_name;
      }

      return "";
   }
}
