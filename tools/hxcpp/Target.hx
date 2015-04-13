class Target
{
   public var mBuildDir:String;
   public var mOutput:String;
   public var mOutputDir:String;
   public var mTool:String;
   public var mToolID:String;
   public var mFiles:Array<File>;
   public var mFileGroups:Array<FileGroup>;
   public var mDepends:Array<String>;
   public var mSubTargets:Array<String>;
   public var mLibs:Array<String>;
   public var mFlags:Array<String>;
   public var mErrors:Array<String>;
   public var mDirs:Array<String>;
   private var mExt:String;
   
   public function new(inOutput:String, inTool:String,inToolID:String)
   {
      mOutput = inOutput;
      mOutputDir = "";
      mBuildDir = "";
      mToolID = inToolID;
      mTool = inTool;
      mFiles = [];
      mDepends = [];
      mLibs = [];
      mFlags = [];
      mExt = null;
      mSubTargets = [];
      mFileGroups = [];
      mFlags = [];
      mErrors=[];
      mDirs=[];
   }

   public function toString() return mToolID;

   public function getExt(inDefault:String)
   {
      return mExt==null ? inDefault : mExt;
   }

   public function setExt(inExt:String)
   {
      mExt = inExt;
   }

   public function addError(inError:String)
   {
      mErrors.push(inError);
   }

   public function addFiles(inGroup:FileGroup)
   {
      mFiles = mFiles.concat(inGroup.mFiles);
      mFileGroups.push(inGroup);
   }

   public function checkError()
   {
      if (mErrors.length>0)
      {
         Log.error(mErrors.join(", "));
         //throw mErrors.join("/");
      }
   }

   public function clean()
   {
      for(dir in mDirs)
      {
         Log.info("Remove " + dir + "...");
         PathManager.removeDirectory(dir);
      }
   }

   public function getKey()
   {
      return mOutput + (mExt==null ? "" : mExt);
   }
}
