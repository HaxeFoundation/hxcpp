import haxe.io.Path;
import sys.FileSystem;

class FileGroup
{
   public var mNewest:Float;
   public var mCompilerFlags:Array<String>;
   public var mMissingDepends:Array<String>;
   public var mOptions:Array<String>;
   public var mPrecompiledHeader:String;
   public var mPrecompiledHeaderDir:String;
   public var mFiles:Array<File>;
   public var mHLSLs:Array<HLSL>;
   public var mDir:String;
   public var mId:String;
   public var mDepends:Array<String>;
   public var mDependHash:String;
   
   public function new(inDir:String,inId:String)
   {
      mNewest = 0;
      mFiles = [];
      mCompilerFlags = [];
      mPrecompiledHeader = "";
      mDepends = [];
      mMissingDepends = [];
      mOptions = [];
      mHLSLs = [];
      mDir = inDir;
      mId = inId;
   }

   public function addCompilerFlag(inFlag:String)
   {
      mCompilerFlags.push(inFlag);
   }

   public function addDepend(inFile:String)
   {
      if (!FileSystem.exists(inFile))
      {
         mMissingDepends.push(inFile);
         return;
      }
      var stamp =  FileSystem.stat(inFile).mtime.getTime();
      if (stamp>mNewest)
         mNewest = stamp;

      mDepends.push(inFile);
   }

   public function addDependFiles(inGroup:FileGroup)
   {
      for(depend in inGroup.mDepends)
         addDepend(depend);
   }


   public function addHLSL(inFile:String,inProfile:String,inVariable:String,inTarget:String)
   {
      addDepend(inFile);

      mHLSLs.push( new HLSL(inFile,inProfile,inVariable,inTarget) );
   }

   public function addOptions(inFile:String)
   {
      mOptions.push(inFile);
   }

   public function checkDependsExist()
   {
      if (mMissingDepends.length>0)
      {
         Log.error("Could not find dependencies: [ " + mMissingDepends.join (", ") + " ]");
         //throw "Could not find dependencies: " + mMissingDepends.join(",");
      }
   }

   public function filterOptions(contents:String)
   {
      // Old-style
      if (contents.substr(0,1)==" ")
         return contents;

      var result = new Array<String>();
      for(def in contents.split("\n"))
      {
         var name = def.split("=")[0].toLowerCase();
         if (name.indexOf("hxcpp_link")>=0)
         {
            // Only effects linking, not compiling
         }
         else if (name.indexOf("hxcpp")>=0 || name=="scriptable" || name.indexOf("dll")>=0 || name=="no_console" )
            result.push(def);
      }

      return result.join("\n");
   }

   public function checkOptions(inObjDir:String)
   {
      var changed = false;
      for(option in mOptions)
      {
         if (!FileSystem.exists(option))
         {
            mMissingDepends.push(option);
         }
         else
         {
            var contents = filterOptions(sys.io.File.getContent(option));

            var dest = inObjDir + "/" + Path.withoutDirectory(option);
            var skip = false;

            if (FileSystem.exists(dest))
            {
               var dest_content = sys.io.File.getContent(dest);
               if (dest_content==contents)
                  skip = true;
            }
            if (!skip)
            {
               PathManager.mkdir(inObjDir);
               var stream = sys.io.File.write(dest,true);
               stream.writeString(contents);
               stream.close();
               changed = true;
            }
            addDepend(dest);
         }
      }
      return changed;
   }

   public function getPchDir()
   {
      return "__pch/" + mId ;
   }

   public function getPchName()
   {
      return Path.withoutDirectory(mPrecompiledHeader);
   }

   public function isOutOfDate(inStamp:Float)
   {
      return inStamp<mNewest;
   }

   public function preBuild()
   {
      for(hlsl in mHLSLs)
         hlsl.build();

      if (BuildTool.useCache)
      {
         mDependHash = "";
         for(depend in mDepends)
            mDependHash += File.getFileHash(depend);
         mDependHash = haxe.crypto.Md5.encode(mDependHash);
      }
   }

   public function setPrecompiled(inFile:String, inDir:String)
   {
      mPrecompiledHeader = inFile;
      mPrecompiledHeaderDir = inDir;
   }
}
