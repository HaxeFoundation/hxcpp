import sys.FileSystem;

class Compiler
{
   public var mFlags : Array<String>;
   public var mCFlags : Array<String>;
   public var mMMFlags : Array<String>;
   public var mCPPFlags : Array<String>;
   public var mOBJCFlags : Array<String>;
   public var mPCHFlags : Array<String>;
   public var mAddGCCIdentity: Bool;
   public var mExe:String;
   public var mOutFlag:String;
   public var mObjDir:String;
   public var mExt:String;

   public var mPCHExt:String;
   public var mPCHCreate:String;
   public var mPCHUse:String;
   public var mPCHFilename:String;
   public var mPCH:String;

   public var mGetCompilerVersion:String;
   public var mCompilerVersion:String;
   public var mCached:Bool;

   public var mID:String;

   public function new(inID,inExe:String,inGCCFileTypes:Bool)
   {
      mFlags = [];
      mCFlags = [];
      mCPPFlags = [];
      mOBJCFlags = [];
      mMMFlags = [];
      mPCHFlags = [];
      mAddGCCIdentity = inGCCFileTypes;
      mCompilerVersion = null;
      mObjDir = "obj";
      mOutFlag = "-o";
      mExe = inExe;
      mID = inID;
      mExt = ".o";
      mPCHExt = ".pch";
      mPCHCreate = "-Yc";
      mPCHUse = "-Yu";
      mPCHFilename = "/Fp";
      mCached = false;
   }

   function addIdentity(ext:String,ioArgs:Array<String>)
   {
      if (mAddGCCIdentity)
      {
         var identity = switch(ext)
           {
              case "c" : "c";
              case "m" : "objective-c";
              case "mm" : "objective-c++";
              case "cpp" : "c++";
              case "c++" : "c++";
              default:"";
         }
         if (identity!="")
         {
            ioArgs.push("-x");
            ioArgs.push(identity);
         }
      }
   }

   public function setPCH(inPCH:String)
   {
      mPCH = inPCH;
      if (mPCH=="gcc")
      {
          mPCHExt = ".h.gch";
          mPCHUse = "";
          mPCHFilename = "";
      }
   }

   public function needsPchObj()
   {
      return !mCached && mPCH!="gcc";
   }

   public function createCompilerVersion(inGroup:FileGroup)
   {
      if (mGetCompilerVersion!=null && mCompilerVersion==null)
      {
         var exe = mGetCompilerVersion;
         var args = new Array<String>();
         if (exe.indexOf (" ") > -1)
         {
            var splitExe = exe.split(" ");
            exe = splitExe.shift();
            args = splitExe.concat(args);
         }
 
         var versionString = Setup.readStderr(exe,args).join(" ");
         if (BuildTool.verbose)
         {
            BuildTool.println("--- Compiler verison ---" );
            BuildTool.println( versionString );
            BuildTool.println("------------------");
         }

         mCompilerVersion = haxe.crypto.Md5.encode(versionString);
         mCached = true;
      }

      return mCached;
   }

   public function precompile(inObjDir:String,inGroup:FileGroup)
   {
      var header = inGroup.mPrecompiledHeader;
      var file = inGroup.getPchName();

      var args = inGroup.mCompilerFlags.concat(mFlags).concat( mCPPFlags ).concat( mPCHFlags );

      var dir = inObjDir + "/" + inGroup.getPchDir() + "/";
      var pch_name = dir + file + mPCHExt;

      BuildTool.log("Make pch dir " + dir );
      DirManager.make(dir);

      if (mPCH!="gcc")
      {
         args.push( mPCHCreate + header + ".h" );

         // Create a temp file for including ...
         var tmp_cpp = dir + file + ".cpp";
         var outFile = sys.io.File.write(tmp_cpp,false);
         outFile.writeString("#include <" + header + ".h>\n");
         outFile.close();

         args.push( tmp_cpp );
         args.push(mPCHFilename + pch_name);
         args.push(mOutFlag + dir + file + mExt);
      }
      else
      {
         BuildTool.log("Make pch dir " + dir + header );
         DirManager.make(dir + header);
         args.push( "-o" );
         args.push(pch_name);
         args.push( inGroup.mPrecompiledHeaderDir + "/" + inGroup.mPrecompiledHeader + ".h" );
      }


      BuildTool.println("Creating " + pch_name + "...");
      var result = BuildTool.runCommand( mExe, args, true, false );
      if (result!=0)
      {
         if (FileSystem.exists(pch_name))
            FileSystem.deleteFile(pch_name);
         throw "Error creating pch: " + result + " - build cancelled";
      }
   }

   public function getObjName(inFile:File)
   {
      var path = new haxe.io.Path(inFile.mName);
      var dirId =
         haxe.crypto.Md5.encode(BuildTool.targetKey + path.dir).substr(0,8) + "_";

      return mObjDir + "/" + dirId + path.file + mExt;
   }

   public function compile(inFile:File,inTid:Int)
   {
      var path = new haxe.io.Path(mObjDir + "/" + inFile.mName);
      var obj_name = getObjName(inFile);

      var args = new Array<String>();
      
      args = args.concat(inFile.mCompilerFlags).concat(inFile.mGroup.mCompilerFlags).concat(mFlags);

      var ext = path.ext.toLowerCase();
      addIdentity(ext,args);

      var allowPch = false;
      if (ext=="c")
         args = args.concat(mCFlags);
      else if (ext=="m")
         args = args.concat(mOBJCFlags);
      else if (ext=="mm")
         args = args.concat(mMMFlags);
      else if (ext=="cpp" || ext=="c++")
      {
         allowPch = true;
         args = args.concat(mCPPFlags);
      }


      if (!mCached && inFile.mGroup.mPrecompiledHeader!="" && allowPch)
      {
         var pchDir = inFile.mGroup.getPchDir();
         if (mPCHUse!="")
         {
            args.push(mPCHUse + inFile.mGroup.mPrecompiledHeader + ".h");
            args.push(mPCHFilename + mObjDir + "/" + pchDir + "/" + inFile.mGroup.getPchName() + mPCHExt);
         }
         else
            args.unshift("-I"+mObjDir + "/" + pchDir);
      }


      var found = false;
      var cacheName:String = null;
      if (mCompilerVersion!=null)
      {
         var sourceName = inFile.mDir + inFile.mName;
         var contents = sys.io.File.getContent(sourceName);
         if (contents!="")
         {
            var md5 = haxe.crypto.Md5.encode(contents + args.join(" ") +
                inFile.mGroup.mDependHash + mCompilerVersion + inFile.mDependHash );
            cacheName = BuildTool.compileCache + "/" + md5;
            if (FileSystem.exists(cacheName))
            {
               sys.io.File.copy(cacheName, obj_name);
               BuildTool.println("use cache for " + obj_name + "(" + md5 + ")" );
               found = true;
            }
            else
            {
               BuildTool.log(" not in cache " + cacheName);
            }
         }
         else
            throw "Unkown source contents " + sourceName;
      }

      if (!found)
      {
         args.push( (new haxe.io.Path( inFile.mDir + inFile.mName)).toString() );

         var out = mOutFlag;
         if (out.substr(-1)==" ")
         {
            args.push(out.substr(0,out.length-1));
            out = "";
         }

         args.push(out + obj_name);
         var result = BuildTool.runCommand( mExe, args, true, inTid>=0 );
         if (result!=0)
         {
            if (FileSystem.exists(obj_name))
               FileSystem.deleteFile(obj_name);
            throw "Error : " + result + " - build cancelled";
         }
         if (cacheName!=null)
         {
           sys.io.File.copy(obj_name, cacheName );
           BuildTool.log(" caching " + cacheName);
         }
      }

      return obj_name;
   }
}