import sys.FileSystem;

class File
{
   public var mName:String;
   public var mDir:String;
   public var mDependHash:String;
   public var mDepends:Array<String>;
   public var mCompilerFlags:Array<String>;
   public var mGroup:FileGroup;
   public var mTags:String;
   public var mFilterOut:String;
   
   public function new(inName:String, inGroup:FileGroup)
   {
      mName = inName;
      mDir = "";
      if (inGroup.mDir != "" && !PathManager.isAbsolute(mName))
         mDir = inGroup.mDir;
      if (mDir!="") mDir += "/";
      // Do not take copy - use reference so it can be updated
      mGroup = inGroup;
      mDepends = [];
      mCompilerFlags = [];
      mTags = null;
   }
   
   inline public function getCacheProject() return mGroup.getCacheProject();

   public function isNvcc() return mGroup.mNvcc;

   public function keep(inDefines:Map<String,String>)
   {
      return mFilterOut==null || !inDefines.exists(mFilterOut);
   }

   public function getTags()
   {
      return mTags==null ? mGroup.mTags : mTags;
   }

   public function setTags(inTags:String)
   {
      return mTags=inTags;
   }

   public function computeDependHash()
   {
      mDependHash = "";
      for(depend in mDepends)
         mDependHash += getFileHash(depend);
      mDependHash = haxe.crypto.Md5.encode(mDependHash);
   }

   public static function getFileHash(inName:String)
   {
      var content = sys.io.File.getContent(inName);
      var md5 = haxe.crypto.Md5.encode(content);
      return md5;
   }

   public function isOutOfDate(inObj:String)
   {
      if (!FileSystem.exists(inObj))
         return true;

      var obj_stamp = FileSystem.stat(inObj).mtime.getTime();
      if (mGroup.isOutOfDate(obj_stamp))
         return true;

      var source_name = mDir+mName;
      if (!FileSystem.exists(source_name))
      {
         Log.error("Could not find source file \"" + source_name + "\"");
         //throw "Could not find source '" + source_name + "'";
      }
      var source_stamp = FileSystem.stat(source_name).mtime.getTime();
      if (obj_stamp < source_stamp)
         return true;
      for(depend in mDepends)
      {
         if (!FileSystem.exists(depend))
         {
            Log.error("Could not find dependency \"" + depend + "\" for \"" + mName + "\"");
            //throw "Could not find dependency '" + depend + "' for '" + mName + "'";
         }
         if (FileSystem.stat(depend).mtime.getTime() > obj_stamp )
            return true;
      }
      return false;
   }
}
