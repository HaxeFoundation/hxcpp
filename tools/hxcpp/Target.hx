class Target
{
   public var mBuildDir:String;
   public var mOutput:String;
   public var mOutputDir:String;
   public var mTool:String;
   public var mToolID:String;
   private var mExt:String;
   public var mFullOutputName:String;
   public var mFullUnstrippedName:String;

   // These attributes are merged by the "merge" command
   public var mFiles:Array<File>;
   public var mFileGroups:Array<FileGroup>;
   public var mDepends:Array<String>;
   public var mSubTargets:Array<String>;
   public var mAutoLibs:Array<String>;
   public var mLibs:Array<String>;
   public var mLibPaths:Array<String>;
   public var mFlags:Array<String>;
   public var mErrors:Array<String>;
   public var mDirs:Array<String>;
   
   public function new(inOutput:String, inTool:String,inToolID:String)
   {
      mOutput = inOutput;
      mOutputDir = "";
      mBuildDir = "";
      mToolID = inToolID;
      mTool = inTool;
      mFiles = [];
      mDepends = [];
      mAutoLibs = [];
      mLibs = [];
      mLibPaths = [];
      mFlags = [];
      mExt = null;
      mFullOutputName = null;
      mSubTargets = [];
      mFileGroups = [];
      mFlags = [];
      mErrors=[];
      mDirs=[];
   }

   public function merge(other:Target)
   {
      mFiles = mFiles.concat(other.mFiles);
      mFileGroups = mFileGroups.concat(other.mFileGroups);
      mDepends = mDepends.concat(other.mDepends);
      mSubTargets = mSubTargets.concat(other.mSubTargets);
      mLibPaths = mLibPaths.concat(other.mLibPaths);
      mAutoLibs = mAutoLibs.concat(other.mAutoLibs);
      mLibs = mLibs.concat(other.mLibs);
      mFlags = mFlags.concat(other.mFlags);
      mErrors = mErrors.concat(other.mErrors);
      mDirs = mDirs.concat(other.mDirs);
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

   public function addFiles(inGroup:FileGroup, inAsLibrary:Bool)
   {
      inGroup.mAsLibrary = inGroup.mAsLibrary || inAsLibrary;
      mFiles = mFiles.concat([for(file in inGroup.mFiles) file]);
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
