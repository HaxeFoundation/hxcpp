import haxe.crypto.Md5;
import haxe.io.Path;
import sys.FileSystem;

private class FlagInfo
{
   var flag:String;
   var tag:String;
   public function new(inFlag:String, inTag:String)
   {
      flag = inFlag;
      tag = inTag;
   }
   public function add(args:Array<String>, inFilter:Array<String>)
   {
      var allowSpace = inFilter.indexOf("nvcc")<0;
      if ((tag==""&&allowSpace) || inFilter.indexOf(tag)>=0)
         args.push(flag);
   }
   public function toString():String
   {
      if (tag=="")
         return flag;
      else
         return '$flag($tag)';
   }
}

class Compiler
{
   private var mFlags:Array<FlagInfo>;
   public var mCFlags:Array<String>;
   public var mNvccFlags:Array<String>;
   public var mMMFlags:Array<String>;
   public var mCPPFlags:Array<String>;
   public var mOBJCFlags:Array<String>;
   public var mPCHFlags:Array<String>;
   public var mAddGCCIdentity:Bool;
   public var mExe:String;
   public var mOutFlag:String;
   public var mObjDir:String;
   public var mRelObjDir:String;
   public var mExt:String;

   public var mPCHExt:String;
   public var mPCHCreate:String;
   public var mPCHUse:String;
   public var mPCHFilename:String;
   public var mPCH:String;

   public var mRcExe:String;
   public var mRcExt:String;
   public var mRcFlags:Array<String>;

   public var mGetCompilerVersion:String;
   public var mCompilerVersion:String;
   public var mCompilerVersionString:String;
   public var mCached:Bool;

   public var mID:String;

   //testing...
   public var useCacheInPlace = true;
   //public var useCacheInPlace = false;

   public function new(inID,inExe:String)
   {
      mFlags = [];
      mCFlags = [];
      mNvccFlags = [];
      mCPPFlags = [];
      mOBJCFlags = [];
      mMMFlags = [];
      mPCHFlags = [];
      mAddGCCIdentity = false;
      mCompilerVersion = null;
      mRcExt = ".res";
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
      mRcFlags = [];
   }

   public function getFlagStrings()
   {
      var result = new Array<String>();
      for(f in mFlags)
         result.push( f.toString() );
      return result;
   }

   public function addFlag(inFlag:String, inTag:String)
   {
      mFlags.push( new FlagInfo(inFlag, inTag) );
   }


   public function objToAbsolute()
   {
      if (mRelObjDir==null)
         mRelObjDir = mObjDir;
      mObjDir = Path.normalize( PathManager.combine( Sys.getCwd(), mRelObjDir ) );
   }

   public function getTargetPrefix()
   {
      var dir = mRelObjDir!=null ? mRelObjDir : mObjDir;
      dir = dir.split("\\").join("/");
      var parts = dir.split("/");
      // Trailing slash?
      var prefix = parts.pop();
      if (prefix=="")
         prefix = parts.pop();
      if (prefix==null)
         prefix = "";
      prefix = prefix.split("-").join("_");
      return prefix;
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

   function addOptimTags(tagFilter:Array<String>)
   {
      var optimFlags = (tagFilter.indexOf("debug") >= 0 ? 1 : 0) +
                       (tagFilter.indexOf("release") >= 0 ? 1 : 0) +
                       (tagFilter.indexOf("optim-std") >= 0 ? 1 : 0) +
                       (tagFilter.indexOf("optim-none") >= 0 ? 1 : 0) +
                       (tagFilter.indexOf("optim-size") >= 0 ? 1 : 0);
      if (optimFlags==0)
         tagFilter.push("optim-std");
      else if (optimFlags>1)
         Log.error("More than one optimization tag has been set:" + tagFilter);
   }


   public function getCompilerDefines(inTags:String)
   {
      var args = new Array<String>();
      var tagFilter = inTags.split(",");
      addOptimTags(tagFilter);
      for(flag in mFlags)
         flag.add(args,tagFilter);
      return args;
   }

   function getArgs(inFile:File)
   {
      var nvcc = inFile.isNvcc();
      var isRc = mRcExe!=null && inFile.isResource();
      var args = nvcc ? inFile.mGroup.mCompilerFlags.concat( BuildTool.getNvccFlags() ) :
                       inFile.mCompilerFlags.concat(inFile.mGroup.mCompilerFlags);
      var tagFilter = inFile.getTags().split(",");
      addOptimTags(tagFilter);
      if (!isRc)
         for(flag in mFlags)
            flag.add(args,tagFilter);
      var ext = mExt.toLowerCase();
      var ext = new Path(inFile.mName).ext;
      if (ext!=null)
         ext = ext.toLowerCase();
      else
         Log.error("Unkown extension for " + inFile.mName);


      addIdentity(ext,args);

      var allowPch = false;
      if (nvcc)
         args = args.concat(mNvccFlags);
      else if (isRc)
         args = args.concat(mRcFlags);
      else if (ext=="c")
         args = args.concat(mCFlags);
      else if (ext=="m")
         args = args.concat(mOBJCFlags);
      else if (ext=="mm")
         args = args.concat(mMMFlags);
      else if (ext=="cpp" || ext=="c++" || ext=="cc")
      {
         allowPch = true;
         args = args.concat(mCPPFlags);
      }

      if (isRc || inFile.getTags()!=inFile.mGroup.getTags())
         allowPch = false;

      if (inFile.mGroup.isPrecompiled() && allowPch)
      {
         var pchDir = getPchDir(inFile.mGroup);
         if (mPCHUse!="")
         {
            args.push(mPCHUse + inFile.mGroup.mPrecompiledHeader + ".h");
            args.push(mPCHFilename + pchDir + "/" + inFile.mGroup.getPchName() + mPCHExt);
         }
         else
            args.unshift("-I"+pchDir);
      }

      return args;
   }

   public function createEmbedFile(srcName:String, destName:String, embedName:String, scramble:String)
   {
      try
      {
          var content = sys.io.File.getContent(srcName);
          var output = new Array<String>();

          if (scramble==null)
          {
             output.push( 'const char *$embedName = ' );
             content = content.split("\r").join("");
             content = content.split("\\").join( String.fromCharCode(1) );
             content = content.split('"').join('\\"');
             content = content.split(String.fromCharCode(1)).join("\\\\" );
             var lines = content.split("\n");
             for(line in lines)
                 output.push( '"$line\\n"' );
             output.push(";\n");
          }
          else
          {
             var bytes = haxe.io.Bytes.ofString(content);
             var byteLen = bytes.length;
             var key = haxe.io.Bytes.ofString(scramble);
             var keyLen = key.length;
             var state = 0;
             var line = "";

             output.push( 'int ${embedName}_len = $byteLen;' );
             output.push( 'static const unsigned char data[] = {' );
             for(i in 0...byteLen)
             {
                var ch = bytes.get(i);
                state = (((state + key.get(i%keyLen)) ^ ch) & 0xff);
                line += state + ",";
                if ( (i%10)==9 )
                {
                   output.push(line);
                   line = "";
                }
             }
             if (line!="")
                output.push(line);
             output.push( '};' );
             output.push( 'const unsigned char * $embedName = data;' );
          }
          sys.io.File.saveContent(destName, output.join("\n") );
      }
      catch(e:Dynamic)
      {
         Log.warn('Error creating $destName from $srcName: $e');
      }
   }

   public function cleanTmp(file:String)
   {
      if (BuildTool.keepTemp())
         return;

      try
      {
         if (file!=null && FileSystem.exists(file))
            FileSystem.deleteFile(file);
      }
      catch(e:Dynamic) { }
   }

   public function compile(inFile:File,inTid:Int,headerFunc:Void->Void,pchTimeStamp:Null<Float>)
   {
      var obj_name = getObjName(inFile);
      var args = getArgs(inFile);
      var nvcc = inFile.isNvcc();
      var exe = nvcc ? BuildTool.getNvcc() : mExe;
      var isRc =  mRcExe!=null && inFile.isResource();
      if (isRc)
         exe = mRcExe;

      var found = false;
      var cacheName:String = null;
      if (mCompilerVersion!=null && inFile.mGroup.isCached())
      {
         cacheName = getHashedName(inFile, args);
         if (useCacheInPlace)
         {
            //Log.info(""," link cache for " + obj_name );
            obj_name = cacheName;
         }

         if (FileSystem.exists(cacheName))
         {
            var newer = true;
            if (pchTimeStamp!=null || inFile.mGroup.mRespectTimestamp)
            {
               var time = FileSystem.stat(cacheName).mtime.getTime();

               if (pchTimeStamp!=null)
                  newer = time>=pchTimeStamp;
               if (inFile.mGroup.mRespectTimestamp)
                  newer = newer && time>= FileSystem.stat(inFile.mDir + "/" + inFile.mName).mtime.getTime();
            }
            if (newer)
            {
               //Log.info(""," copy cache for " + obj_name );
               if (!useCacheInPlace)
                  sys.io.File.copy(cacheName, obj_name);
               found = true;
            }
         }
      }

      if (!found)
      {
         if (headerFunc!=null)
            headerFunc();

         var tmpFile:String = null;
         var delayedFilename:String = null;

         if (inFile.mEmbedName!=null)
         {
            var srcDir =  Path.directory( inFile.mDir + "/" + inFile.mName);
            tmpFile = new Path( srcDir + "/" + inFile.mEmbedName + ".cpp").toString();
            Log.v("Creating temp file " + tmpFile);
            createEmbedFile( inFile.mDir + "/" + inFile.mName, tmpFile, inFile.mEmbedName, inFile.mScramble );
            args.push( tmpFile );
         }
         else
         {
            if (isRc)
               delayedFilename = (new Path( inFile.mDir + inFile.mName)).toString();
            else
               args.push( (new Path( inFile.mDir + inFile.mName)).toString() );
         }

         var out = nvcc ? "-o " : mOutFlag;
         if (out.substr(-1)==" ")
         {
            args.push(out.substr(0,out.length-1));
            out = "";
         }

         args.push(out + obj_name);

         if (delayedFilename!=null)
           args.push(delayedFilename);

         var tagInfo = inFile.mTags==null ? "" : " " + inFile.mTags.split(",");

         var fileName = inFile.mName;
         var split = fileName.split ("/");
         if (split.length > 1)
         {
            fileName = " \x1b[2m-\x1b[0m \x1b[33m" + split.slice(0, split.length - 1).join("/") + "/\x1b[33;1m" + split[split.length - 1] + "\x1b[0m";
         }
         else
         {
            fileName = " \x1b[2m-\x1b[0m \x1b[33;1m" + fileName + "\x1b[0m";
         }
         fileName += " \x1b[3m" + tagInfo + "\x1b[0m";


         if (inTid >= 0)
         {
            if (BuildTool.threadExitCode == 0)
            {
               if (!Log.verbose)
               {
                  Log.info(fileName);
               }
               var err = ProcessManager.runProcessThreaded(exe, args, null);
               cleanTmp(tmpFile);
               if (err!=0)
               {
                  if (FileSystem.exists(obj_name))
                     FileSystem.deleteFile(obj_name);
                  BuildTool.setThreadError(err);
               }
            }
         }
         else
         {
            if (!Log.verbose)
            {
               Log.info(fileName);
            }
            var result = ProcessManager.runProcessThreaded(exe, args, null);
            cleanTmp(tmpFile);
            if (result!=0)
            {
               if (FileSystem.exists(obj_name))
                  FileSystem.deleteFile(obj_name);
               Tools.exit (result);
               //throw "Error : " + result + " - build cancelled";
            }
         }

         if (cacheName!=null && !useCacheInPlace)
         {
            Log.info("", " caching " + cacheName);
            sys.io.File.copy(obj_name, cacheName);
         }
      }

      return obj_name;
   }

   public function createCompilerVersion(inGroup:FileGroup)
   {
      if ( mCompilerVersion==null)
      {
         var versionString = "";
         var command = "";

         if (mGetCompilerVersion==null)
         {
            command = mExe + " --version";
            versionString = ProcessManager.readStdout(mExe,["--version"]).join(" ");
         }
         else
         {
            command = mGetCompilerVersion;
            versionString = ProcessManager.readStderr(mGetCompilerVersion,[]).join(" ");
         }

         if (versionString=="" || versionString==null)
            Log.error("Could not deduce compiler version with " + command);

         Log.info("", "Compiler version: " +  versionString);

         mCompilerVersionString = versionString;
         mCompilerVersion = Md5.encode(versionString);
         mCached = true;
      }

      return mCached;
   }

   function getObjName(inFile:File)
   {
      var isRc = mRcExe!=null && inFile.isResource();

      var path = new Path(inFile.mName);
      var dirId = Md5.encode(BuildTool.targetKey + path.dir + inFile.mGroup.mId).substr(0,8) + "_";

      return PathManager.combine(mObjDir, inFile.mGroup.mObjPrefix + dirId + path.file + (isRc ? mRcExt : mExt) );
   }

   function getHashedName(inFile:File, args:Array<String>)
   {
      var sourceName = inFile.mDir + inFile.mName;
      var contents = sys.io.File.getContent(sourceName);
      if (contents!="")
      {
         var md5 = Md5.encode(contents + args.join(" ") +
             inFile.mGroup.mDependHash + mCompilerVersion + inFile.mDependHash );
         return CompileCache.getCacheName(inFile.getCacheProject(),md5,mExt);
      }
      else
         throw "Unkown source contents " + sourceName;
      return "";
   }

   public function getCacheString(inFile:File)
   {
      var args = getArgs(inFile);
      return ("<contents>" + args.join(" ") + " " + inFile.mGroup.getDependString() +  " " + mCompilerVersionString + " " + inFile.getDependString() );
   }

   public function getCachedObjName(inFile:File)
   {
      if (mCompilerVersion!=null && useCacheInPlace && inFile.mGroup.isCached())
      {
         //trace(inFile.mName + " " + inFile.getTags().split(",") + " " + getFlagStrings() + " " + getArgs(inFile));
         return getHashedName(inFile, getArgs(inFile));
      }
      else
         return getObjName(inFile);
   }

   public function needsPchObj()
   {
      return mPCH!="gcc";
   }

/*
   public function getPchObjName(group:FileGroup)
   {
      var pchDir = getPchDir(group);
      if (pchDir != "")
         return pchDir + "/" + group.getPchName() + mExt;
      throw "Missing precompiled directory name";
   }
*/
   public function getPchCompileFlags(inGroup:FileGroup)
   {
      var args = inGroup.mCompilerFlags.copy();
      var tags = inGroup.mTags.split(",");
      addOptimTags(tags);
      for(flag in mFlags)
         flag.add(args,tags);

      return  args.concat( mCPPFlags );
   }

   public function getPchDir(inGroup:FileGroup)
   {
      if (!inGroup.isCached())
         return inGroup.getPchDir(mObjDir);

      var args = getPchCompileFlags(inGroup);
      var md5 = Md5.encode(args.join(" ") + inGroup.mPrecompiledHeader +
                    inGroup.mDependHash + mCompilerVersion );
      return CompileCache.getPchDir(inGroup.getCacheProject(),md5);
   }

   public function precompile(inGroup:FileGroup, inReuseIfPossible:Bool)
   {
      // header will be like "hxcpp" or "wx/wx"
      var header = inGroup.mPrecompiledHeader;
      // file will be like "hxcpp" or "wx"
      var file = inGroup.getPchName();

      var args = getPchCompileFlags(inGroup);

      // Local output dir
      var dir = getPchDir(inGroup);

      // Like objs/hxcpp.pch or objs/wx.gch
      var pch_name = dir + "/" + file + mPCHExt;
      if (inGroup.isCached() || inReuseIfPossible)
      {
          // No obj needed for gcc
          var obj = mPCH=="gcc" ? null : PathManager.combine(dir, file + mExt);
          if (FileSystem.exists(pch_name) && (obj==null || FileSystem.exists(obj)) )
             return obj;
      }

      args = args.concat( mPCHFlags );


      //Log.info("", "Make pch dir " + dir );
      PathManager.mkdir(dir);

      if (mPCH!="gcc")
      {
         args.push( mPCHCreate + header + ".h" );
         var symbol = "link" + Md5.encode( PathManager.combine(dir, file + mExt) );
         args.push( "-Yl" + symbol  );

         // Create a temp file for including ...
         var tmp_cpp = dir + "/" + file + ".cpp";
         var outFile = sys.io.File.write(tmp_cpp,false);
         outFile.writeString("#include <" + header + ".h>\n");
         outFile.close();

         args.push( tmp_cpp );
         args.push(mPCHFilename + pch_name);
         args.push(mOutFlag + PathManager.combine(dir, file + mExt));
      }
      else
      {
         Log.info("", 'Creating PCH directory "$dir"');
         PathManager.mkdir(dir);
         args.push( "-o" );
         args.push(pch_name);
         args.push( inGroup.mPrecompiledHeaderDir + "/" + inGroup.mPrecompiledHeader + ".h" );
      }

      Log.info("Creating " + pch_name + "...", " - Precompile " + pch_name );
      var result = ProcessManager.runCommand("", mExe, args);
      if (result!=0)
      {
         var goes = 10;
         for(attempt in 0...goes)
         {
            try {
               if (FileSystem.exists(pch_name))
                  FileSystem.deleteFile(pch_name);
               break;
            }
            catch(error:Dynamic)
            {
               Log.warn('Error cleaning PCH file $pch_name: $error');
               if (attempt<goes-1)
                  Sys.sleep(0.25);
            }
         }
         Log.error("Could not create PCH");
         //throw "Error creating pch: " + result + " - build cancelled";
      }

      if (mPCH!="gcc")
         return  PathManager.combine(dir, file + mExt);
      return null;
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

   public function initPrecompile(inDefault:String)
   {
      if (mPCH==null)
         setPCH(inDefault);
      return mPCH!=null;
   }

}
