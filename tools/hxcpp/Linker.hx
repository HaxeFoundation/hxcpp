import haxe.io.Path;
import sys.FileSystem;
import haxe.crypto.Md5;

using StringTools;

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
   public var mFromFileNeedsQuotes:Bool;
   public var mLibs:Array<String>;
   public var mExpandArchives:Bool;
   public var mRecreate:Bool;
   public var mLastOutName:String;
   public var mAddLibPath:String;

   public function new(inExe:String)
   {
      mFlags = [];
      mOutFlag = "-o";
      mAddLibPath = "-L";
      mExe = inExe;
      mNamePrefix = "";
      mLibDir = "";
      mRanLib = "";
      mExpandArchives = false;
      // Default to on...
      mFromFile = "@";
      mFromFileNeedsQuotes = true;
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

   function getSimpleFilename(inTarget:Target)
   {
      var ext = inTarget.getExt(mExt);
      // Remove arch from ext ...
      var idx = ext.indexOf('.');
      if (idx>0)
         ext = ext.substr(idx);

      return mNamePrefix + inTarget.mOutput + ext;
   }
   public function getUnstrippedFilename(inObjDir, inTarget:Target)
   {
      if (inTarget.mFullUnstrippedName!=null)
      {
         PathManager.mkdir( inTarget.mFullUnstrippedName, true );
         return inTarget.mFullUnstrippedName;
      }
      return inObjDir + "/" + getSimpleFilename(inTarget);
   }

   public function link(inTarget:Target,inObjs:Array<String>,inCompiler:Compiler,extraDeps:Array<String> )
   {
      var ext = inTarget.getExt(mExt);
      var file_name = mNamePrefix + inTarget.mOutput + ext;

      var tmpDir = inCompiler.mObjDir;

      try
      {
         PathManager.mkdir(inTarget.mOutputDir);
      }
      catch (e:Dynamic)
      {
         Log.error("Unable to create output directory \"" + inTarget.mOutputDir + "\"");
         //throw "Unable to create output directory " + inTarget.mOutputDir;
      }

      var out_name = Path.normalize(PathManager.combine( inTarget.mBuildDir, inTarget.mOutputDir + file_name));
      var hashFile = out_name + ".hash";
      if (inTarget.mFullOutputName!=null)
      {
         PathManager.mkdir( inTarget.mFullOutputName, true );
         out_name = inTarget.mFullOutputName;
      }

      mLastOutName = out_name;


      var lastLib = "";
      var libs = new Array<String>();
      for(l in inTarget.mAutoLibs)
         if (l!=lastLib)
         {
            libs.push(l);
            lastLib = l;
         }

      for(l in inTarget.mLibs)
         if (l!=lastLib)
         {
            libs.push(l);
            lastLib = l;
         }

      for(l in mLibs)
         if (l!=lastLib)
         {
            libs.push(l);
            lastLib = l;
         }

      var v18Added = false;
      var isOutOfDateLibs = false;

      var md5 = Md5.encode(inObjs.join(";"));
      if (!FileSystem.exists(hashFile) || sys.io.File.getContent(hashFile)!=md5)
         isOutOfDateLibs = true;

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

      if (isOutOfDateLibs || isOutOfDate(out_name,inObjs) || isOutOfDate(out_name,inTarget.mDepends) || isOutOfDate(out_name,extraDeps) )
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
               Log.info("\x1b[1mClean: \x1b[0m" + out_name);
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
                  var objDir = tmpDir + "/" + libName + ".unpack";
                  PathManager.mkdir(objDir);
                  ProcessManager.runCommand (objDir, mExe, ["x", lib], true, true, false, " - Unpack : " + lib);
                  for(obj in libObjs)
                     objs.push( objDir+"/"+obj );
               }
               else
                  libArgs.push(lib);
            }
            libs = libArgs;
         }

         var here = Path.normalize(Sys.getCwd()) + "/";
         var hereLen = here.length;
         for(oid in 0...objs.length)
         {
            var obj = Path.normalize( objs[oid] );
            if (obj.startsWith(here))
               objs[oid] = obj.substr(hereLen);
         }

         // Place list of obj files in a file called "all_objs"
         if (mFromFile!="")
         {
            PathManager.mkdir(tmpDir);
            var fname = tmpDir + "/all_objs";

            var local = Path.normalize(fname);
            if (local.startsWith(here))
               fname = local.substr(hereLen);

            var fout = sys.io.File.write(fname,false);
            if (mFromFileNeedsQuotes)
            {
               for(obj in objs)
                  fout.writeString('"' + obj + '"\n');
            }
            else
            {
               for(obj in objs)
                  fout.writeString(obj + '\n');
            }
            fout.close();
            var parts = mFromFile.split(" ");
            var last = parts.pop();
            args = args.concat(parts);
            args.push(last + fname );
         }
         else
            args = args.concat(objs);

         for(libpath in inTarget.mLibPaths)
         {
            var path = Path.normalize(libpath);
            if (path.startsWith(here))
               path = path.substr(hereLen);
            args.push( mAddLibPath + path );
         }

         args = args.concat(libs);

         var result = ProcessManager.runCommand("", mExe, args, true, true, false,
             "\x1b[1mLink: \x1b[0m" + out_name);
         if (result!=0)
         {
            Tools.exit(result);
            //throw "Error : " + result + " - build cancelled";
         }

         if (mRanLib!="")
         {
            args = [out_name];
            var result = ProcessManager.runCommand("", mRanLib, args, true, true, false, "\x1b[1mRanlib:\x1b[0m " + out_name);
            if (result!=0)
            {
               Tools.exit(result);
               //throw "Error : " + result + " - build cancelled";
            }
         }

         if (mLibDir!="")
         {
            sys.io.File.copy( mLibDir+"/"+file_name, out_name );
            FileSystem.deleteFile( mLibDir+"/"+file_name );
         }

         sys.io.File.saveContent(hashFile,md5);
         return out_name;
      }

      return "";
   }
}
