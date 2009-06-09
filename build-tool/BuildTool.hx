
class DirManager
{
   static var mMade = new Hash<Bool>();
	static public function make(inDir:String)
	{
	   var parts = inDir.split("/");
		var total = "";
		for(part in parts)
		{
		   if (total!="") total+="/";
			total += part;
			if (!mMade.exists(total))
			{
			   mMade.set(total,true);
				if (!neko.FileSystem.exists(total))
				   neko.FileSystem.createDirectory(total);
			}
		}
	}
}

class Compiler
{
   public var mFlags : Array<String>;
   public var mCFlags : Array<String>;
   public var mCPPFlags : Array<String>;
   public var mExe:String;
   public var mOutFlag:String;
   public var mObjDir:String;
   public var mExt:String;

   public function new(inExe:String)
   {
      mFlags = [];
      mCFlags = [];
      mCPPFlags = [];
      mObjDir = "obj";
      mOutFlag = "-o";
      mExe = inExe;
      mExt = ".o";
   }

	public function compile(inFile:File)
	{
	   var path = new neko.io.Path(mObjDir + "/" + inFile.mName);
		var obj_name = path.dir + "/" + path.file + mExt;
		DirManager.make(path.dir);

		if (!inFile.isOutOfDate(obj_name))
		   return obj_name;

		var args = mFlags.concat(inFile.mCompilerFlags);

		if (path.ext.toLowerCase()=="c")
		   args = args.concat(mCFlags);
		else
		   args = args.concat(mCPPFlags);

		args.push( (new neko.io.Path(inFile.mDir + "/" + inFile.mName)).toString() );

		var out = mOutFlag;
		if (out.substr(-1)==" ")
		{
		   args.push(out);
			out = "";
		}
		args.push(out + obj_name);
		neko.Lib.println( mExe + " " + args.join(" ") );
		var result = neko.Sys.command( mExe, args );
		if (result!=0)
		   throw "Error : " + result + " - build cancelled";
		return obj_name;
	}
}


class Linker
{
   public var mExe:String;
   public var mFlags : Array<String>;
   public var mOutFlag:String;
   public var mExt:String;
   public var mNamePrefix:String;

   public function new(inExe:String)
   {
      mFlags = [];
      mOutFlag = "-o";
      mExe = inExe;
		mNamePrefix = "";
   }
	public function link(inTarget:Target,inObjs:Array<String>)
	{
		var out_name = mNamePrefix + inTarget.mOutput + mExt;
		if (isOutOfDate(out_name,inObjs))
		{
			var args = new Array<String>();
			var out = mOutFlag;
			if (out.substr(-1)==" ")
			{
				args.push(out);
				out = "";
			}
			args.push(out + out_name);
			args = args.concat(mFlags).concat(inTarget.mLibs).concat(inTarget.mFlags).concat(inObjs);

			neko.Lib.println( mExe + " " + args.join(" ") );
			var result = neko.Sys.command( mExe, args );
			if (result!=0)
				throw "Error : " + result + " - build cancelled";
		}
	}
	function isOutOfDate(inName:String, inObjs:Array<String>)
	{
	   if (!neko.FileSystem.exists(inName))
		   return true;
		var stamp = neko.FileSystem.stat(inName).mtime.getTime();
		for(obj in inObjs)
		{
		   if (!neko.FileSystem.exists(obj))
			   throw "Could not find " + obj + " required by " + inName;
		   if (neko.FileSystem.stat(inName).mtime.getTime()>stamp)
			   return true;
		}
		return false;
	}
}

class File
{
   public function new(inName:String, inDir:String, inDepends:Array<String>, inFlags:Array<String>)
   {
      mName = inName;
		mDir = inDir;
		mDepends = inDepends.copy();
		mCompilerFlags = inFlags.copy();
   }
	public function isOutOfDate(inObj:String)
	{
	   if (!neko.FileSystem.exists(inObj))
		   return true;
		var obj_stamp = neko.FileSystem.stat(inObj).mtime.getTime();
		var source_name = mDir+"/"+mName;
	   if (!neko.FileSystem.exists(source_name))
			throw "Could not find source '" + source_name + "'";
		var source_stamp = neko.FileSystem.stat(source_name).mtime.getTime();
		if (obj_stamp < source_stamp)
		   return true;
		for(depend in mDepends)
		{
	      if (!neko.FileSystem.exists(depend))
			   throw "Could not find dependency '" + depend + "' for '" + mName + "'";
		   if (neko.FileSystem.stat(depend).mtime.getTime() > obj_stamp )
			   return true;
		}
	   return false;
	}
   public var mName:String;
   public var mDir:String;
   public var mDepends:Array<String>;
   public var mCompilerFlags:Array<String>;
}

typedef FileGroup = Array<File>;

typedef FileGroups = Hash<FileGroup>;

class Target
{
   public function new(inOutput:String, inTool:String)
	{
	   mOutput = inOutput;
		mTool = inTool;
		mFiles = [];
		mLibs = [];
		mFlags = [];
		mSubTargets = [];
		mFlags = [];
	}
	public function addFiles(inFiles:FileGroup)
	{
	   mFiles = mFiles.concat(inFiles);
	}
   public var mOutput:String;
	public var mTool:String;
	public var mFiles:Array<File>;
	public var mSubTargets:Array<String>;
	public var mLibs:Array<String>;
	public var mFlags:Array<String>;
}

typedef Targets = Hash<Target>;

class BuildTool
{
   var mDefines : Hash<String>;
   var mCompiler : Compiler;
   var mLinker : Linker;
	var mFileGroups : FileGroups;
	var mTargets : Targets;


   public function new(inMakefile:String,inDefines:Hash<String>,inTargets:Array<String>)
   {
      mDefines = inDefines;
		mFileGroups = new FileGroups();
      mCompiler = null;
		mTargets = new Targets();
      var make_contents = neko.io.File.getContent(inMakefile);
      var xml_slow = Xml.parse(make_contents);
      var xml = new haxe.xml.Fast(xml_slow.firstElement());

      for(el in xml.elements)
      {
         if (valid(el))
         {
            switch(el.name)
            {
                case "set" : 
                   var name = el.att.name;
                   var value = substitute(el.att.value);
                   mDefines.set(name,value);
                case "compiler" : 
                   if (mCompiler!=null)
                   	 throw "Only one compiler may be set";
                   mCompiler = createCompiler(el);
                case "linker" : 
                   if (mLinker!=null)
                   	 throw "Only one linker may be set";
                   mLinker = createLinker(el);
                case "files" : 
                   var name = el.att.id;
                   mFileGroups.set(name,createFiles(el));
                case "target" : 
                   var name = el.att.id;
                   mTargets.set(name,createTarget(el));
            }
         }
      }

		for(target in inTargets)
		   buildTarget(target);
   }

	public function buildTarget(inTarget:String)
	{
	   neko.Lib.println("Building " + inTarget + "...");
	   if (!mTargets.exists(inTarget))
		   throw "Could not find target '" + inTarget + "' to build.";
      var target = mTargets.get(inTarget);
	   for(sub in target.mSubTargets)
	      buildTarget(sub);

      var objs = new Array<String>();
		for(file in target.mFiles)
		   objs.push( mCompiler.compile(file) );

      switch(target.mTool)
		{
		   case "linker": mLinker.link(target,objs);
		}
	   neko.Lib.println("Built " + inTarget);
	}

   public function createCompiler(inXML:haxe.xml.Fast) : Compiler
   {
      var c = new Compiler(inXML.att.exe);
      for(el in inXML.elements)
      {
         if (valid(el))
            switch(el.name)
            {
                case "flag" : c.mFlags.push(substitute(el.att.value));
                case "cflag" : c.mCFlags.push(substitute(el.att.value));
                case "cppflag" : c.mCPPFlags.push(substitute(el.att.value));
                case "objdir" : c.mObjDir = substitute((el.att.value));
                case "outflag" : c.mOutFlag = substitute((el.att.value));
                case "ext" : c.mExt = substitute((el.att.value));
            }
      }

      return c;
   }

   public function createLinker(inXML:haxe.xml.Fast) : Linker
   {
      var l = new Linker(inXML.att.exe);
      for(el in inXML.elements)
      {
         if (valid(el))
            switch(el.name)
            {
                case "flag" : l.mFlags.push(substitute(el.att.value));
                case "ext" : l.mExt = (substitute(el.att.value));
                case "outflag" : l.mOutFlag = (substitute(el.att.value));
            }
      }

      return l;
   }

   public function createFiles(inXML:haxe.xml.Fast) : FileGroup
   {
	   var files = new FileGroup();
		var dir = inXML.has.dir ? substitute(inXML.att.dir) : ".";
		var depends = new Array<String>();
		var flags = new Array<String>();
      for(el in inXML.elements)
      {
         if (valid(el))
            switch(el.name)
            {
                case "file" : files.push( new File(substitute(el.att.name),dir,depends,flags) );
                case "depend" : depends.push( substitute(el.att.name) );
                case "compilerflag" : flags.push( substitute(el.att.value) );
            }
      }

      return files;
   }


   public function createTarget(inXML:haxe.xml.Fast) : Target
   {
      var output = inXML.has.output ? substitute(inXML.att.output) : "";
      var tool = inXML.has.tool ? inXML.att.tool : "";
		var target = new Target(output,tool);
      for(el in inXML.elements)
      {
         if (valid(el))
            switch(el.name)
            {
                case "target" : target.mSubTargets.push( substitute(el.att.id) );
                case "lib" : target.mLibs.push( substitute(el.att.name) );
                case "flag" : target.mFlags.push( substitute(el.att.value) );
                case "files" : var id = el.att.id;
					    if (!mFileGroups.exists(id))
						    throw "Could not find filegroup " + id; 
					    target.addFiles( mFileGroups.get(id) );
            }
      }

      return target;
   }


   public function valid(inEl:haxe.xml.Fast) : Bool
   {
      if (inEl.x.get("if")!=null)
         if (!defined(inEl.x.get("if"))) return false;

      if (inEl.has.unless)
         if (defined(inEl.att.unless)) return false;

      return true;
   }

   public function defined(inString:String) : Bool
   {
      return mDefines.exists(inString);
   }
   
   static var mVarMatch = new EReg("\\${(.*?)}","");
   public function substitute(str:String) : String
   {
      while( mVarMatch.match(str) )
      {
         str = mVarMatch.matchedLeft() + mDefines.get( mVarMatch.matched(1) ) + mVarMatch.matchedRight();
      }

      return str;
   }
   
   
   // Process args and environment.
   static public function main()
   {
      var args = neko.Sys.args();
   
      var targets = new Array<String>();
      var defines = new Hash<String>();
      var makefile:String="";

      var os = neko.Sys.getEnv("OSTYPE");
      if ( (new EReg("windows","i")).match(os) )
         defines.set("windows","windows");
      else if ( (new EReg("linux","i")).match(os) )
         defines.set("linux","linux");
      else if ( (new EReg("darwin","i")).match(os) )
         defines.set("darwin","darwin");

      for(arg in args)
      {
         if (arg.substr(0,2)=="-D")
            defines.set(arg.substr(2),"");
         else if (makefile.length==0)
            makefile = arg;
         else
            targets.push(arg);
      }

      if (targets.length==0)
         targets.push("default");
   
      if (makefile=="")
      {
         neko.Lib.println("Usage :  BuildTool makefile.xml [-DFLAG1] ...  [-DFLAGN] ... [target1]...[targetN]");
      }
      else
      {
         for(env in neko.Sys.environment().keys())
            defines.set(env, neko.Sys.getEnv(env) );
         new BuildTool(makefile,defines,targets);
      }
   }
   
}
