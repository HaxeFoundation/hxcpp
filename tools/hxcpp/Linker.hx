import haxe.io.Path;
import sys.FileSystem;

class Linker
{
   public var mExe:String;
   public var mFlags:Array<String>;
   public var mOutFlag:String;
   public var mExt:String;
   public var mNamePrefix:String;
   public var mLibDir:String;
   public var mRanLib:String;
   public var mFromFile:String;
   public var mLibs:Array<String>;
   public var mExpandArchives:Bool;
   public var mRecreate:Bool;

   public function new(inExe:String)
   {
      mFlags = [];
      mOutFlag = "-o";
      mExe = inExe;
      mNamePrefix = "";
      mLibDir = "";
      mRanLib = "";
      mExpandArchives = false;
      // Default to on...
      mFromFile = "@";
      mLibs = [];
      mRecreate = false;
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

   public function link(inTarget:Target,inObjs:Array<String>,inCompiler:Compiler)
   {
      var ext = inTarget.getExt(mExt);
      var file_name = mNamePrefix + inTarget.mOutput + ext;
      
      try
      {
         PathManager.mkdir(inTarget.mOutputDir);  
      }
      catch (e:Dynamic)
      {
         Log.error("Unable to create output directory \"" + inTarget.mOutputDir + "\"");
         //throw "Unable to create output directory " + inTarget.mOutputDir; 
      }
      
      var out_name = inTarget.mOutputDir + file_name;

      var libs = inTarget.mLibs.concat(mLibs);
      var v18Added = false;
      var isOutOfDateLibs = false;

      for(i in 0...libs.length)
      {
         var lib = libs[i];
         var parts = lib.split("{MSVC_VER}");
         if (parts.length==2)
         {
            var ver = "";
            if (BuildTool.isMsvc())
            {
               var current = parts[0] + "-" + BuildTool.getMsvcVer() + parts[1];
               if (FileSystem.exists(current))
               {
                  Log.info("", " - \x1b[1mUsing current compiler library:\x1b[0m " + current);
                  libs[i]=current;
               }
               else
               {
                  var v18 = parts[0] + "-18" + parts[1];
                  if (FileSystem.exists(v18))
                  {
                     Log.info("", " - \x1b[1mUsing MSVC18 compatible library:\x1b[0m " + v18);
                     libs[i]=v18;
                     if (!v18Added)
                     {
                        v18Added=true;
                        libs.push( BuildTool.HXCPP + "/lib/Windows/libmsvccompat-18.lib");
                     }
                  }
                  else
                  {
                     Log.error("Could not find compatible library for " + lib + ", " + v18 + " does not exist");
                  }
               }
            }
            else
               libs[i] = parts[0] + parts[1];
         }

         if (!isOutOfDateLibs)
         {
            var lib = libs[i];
            if (FileSystem.exists(lib))
               isOutOfDateLibs = isOutOfDate(out_name,[lib]);
         }

         if (BuildTool.isMingw())
         {
            var libMatch = ~/^([a-zA-Z0-9_]+).lib$/;
            if (libMatch.match(libs[i]))
               libs[i] = "-l" + libMatch.matched(1);
         }

      }

      if (isOutOfDateLibs || isOutOfDate(out_name,inObjs) || isOutOfDate(out_name,inTarget.mDepends))
      {
         var args = new Array<String>();
         var out = mOutFlag;
         if (out.substr(-1)==" ")
         {
            args.push(out.substr(0,out.length-1));
            out = "";
         }
         // Build in temp dir, and then move out so all the crap windows
         //  creates stays out of the way
         if (mLibDir!="")
         {
            PathManager.mkdir(mLibDir);
            args.push(out + mLibDir + "/" + file_name);
         }
         else
         {
            if (mRecreate && FileSystem.exists(out_name))
            {
               Log.info(" clean " + out_name );
               FileSystem.deleteFile(out_name);
            }
            args.push(out + out_name);
         }

         args = args.concat(mFlags).concat(inTarget.mFlags);

         var objs = inObjs.copy();

         if (mExpandArchives)
         {
            var isArchive = ~/\.a$/;
            var libArgs = new Array<String>();
            for(lib in libs)
            {
               if (isArchive.match(lib))
               {
                  var libName = Path.withoutDirectory(lib);
                  var libObjs = ProcessManager.readStdout(mExe, ["t", lib ]);
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
         }

         // Place list of obj files in a file called "all_objs"
         if (mFromFile=="@")
         {
            PathManager.mkdir(inCompiler.mObjDir);
            var fname = inCompiler.mObjDir + "/all_objs";
            var fout = sys.io.File.write(fname,false);
            for(obj in objs)
               fout.writeString(obj + "\n");
            fout.close();
            args.push("@" + fname );
         }
         else
            args = args.concat(objs);

         args = args.concat(libs);
         
         var result = ProcessManager.runCommand("", mExe, args);
         if (result!=0)
         {
            Sys.exit(result);
            //throw "Error : " + result + " - build cancelled";
         }

         if (mRanLib!="")
         {
            args = [out_name];
            var result = ProcessManager.runCommand("", mRanLib, args);
            if (result!=0)
            {
               Sys.exit(result);
               //throw "Error : " + result + " - build cancelled";
            }
         }

         if (mLibDir!="")
         {
            sys.io.File.copy( mLibDir+"/"+file_name, out_name );
            FileSystem.deleteFile( mLibDir+"/"+file_name );
         }
         return out_name;
      }

      return "";
   }
}
