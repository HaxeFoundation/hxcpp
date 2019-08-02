import sys.FileSystem;
using StringTools;

#if haxe4
import sys.thread.Mutex;
#elseif cpp
import cpp.vm.Mutex;
#else
import neko.vm.Mutex;
#end

class File
{
   static var mFileHashes = new Map<String,String>();
   public var mName:String;
   public var mDir:String;
   public var mDependHash:String;
   public var mDepends:Array<String>;
   public var mCompilerFlags:Array<String>;
   public var mGroup:FileGroup;
   public var mTags:String;
   public var mFilterOut:String;
   public var mEmbedName:String;
   public var mScramble:String;
   static public var mDependMutex = new Mutex();

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
      mEmbedName = null;
      mScramble = null;
      mTags = null;
   }
   
   inline public function getCacheProject() return mGroup.getCacheProject();

   public function isNvcc() return mGroup.mNvcc;

   public function isResource() return mName.endsWith(".rc");

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

   public function computeDependHash(localCache:Map<String,String>)
   {
      mDependHash = "";
      for(depend in mDepends)
         mDependHash += getFileHash(depend,localCache);
      mDependHash = haxe.crypto.Md5.encode(mDependHash);
   }

   public function getDependString()
   {
      return "FILES(" + mDepends.join(",") + ")";
   }

   public static function getFileHash(inName:String,localCache:Map<String,String>)
   {
      if (localCache==null)
      {
         var result = mFileHashes.get(inName);
         if (result==null)
         {
            var content = sys.io.File.getContent(inName);
            result = haxe.crypto.Md5.encode(content);
            mFileHashes.set(inName,result);
         }
         return result;
      }
      else
      {
         var result = localCache.get(inName);
         if (result!=null)
            return result;

         mDependMutex.acquire();
         result = mFileHashes.get(inName);
         mDependMutex.release();

         if (result==null)
         {
            var content = sys.io.File.getContent(inName);
            result = haxe.crypto.Md5.encode(content);
            mDependMutex.acquire();
            mFileHashes.set(inName,result);
            mDependMutex.release();
         }

         localCache.set(inName,result);
         return result;
     }
   }

   public function isOutOfDate(inObj:String, ?dependDebug:String->Void)
   {
      if (!FileSystem.exists(inObj))
      {
         return true;
      }

      var obj_stamp = FileSystem.stat(inObj).mtime.getTime();
      if (mGroup.isOutOfDate(obj_stamp))
      {
         if (dependDebug!=null)
            dependDebug(mName + "  - whole group is out of date " + mGroup.getNewestFile() + " " + obj_stamp + " < " + mGroup.mNewest);
         return true;
      }

      var source_name = mDir+mName;
      if (!FileSystem.exists(source_name))
      {
         Log.error("Could not find source file \"" + source_name + "\"");
         //throw "Could not find source '" + source_name + "'";
      }
      var source_stamp = FileSystem.stat(source_name).mtime.getTime();
      if (obj_stamp < source_stamp)
      {
         if (dependDebug!=null)
            dependDebug(mName + ' - stamped $obj_stamp < $source_stamp');
         return true;
      }
      for(depend in mDepends)
      {
         if (!FileSystem.exists(depend))
         {
            Log.error("Could not find dependency \"" + depend + "\" for \"" + mName + "\"");
            //throw "Could not find dependency '" + depend + "' for '" + mName + "'";
         }
         var dependTime =  FileSystem.stat(depend).mtime.getTime();
         if (dependTime > obj_stamp )
         {
            if (dependDebug!=null)
               dependDebug(mName + ' - depend $obj_stamp < $dependTime');
            return true;
         }
      }
      return false;
   }
}
