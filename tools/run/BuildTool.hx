import haxe.io.Path;
import haxe.xml.Fast;
import haxe.Json;
import sys.io.Process;
import sys.FileSystem;
#if neko
import neko.vm.Thread;
import neko.vm.Mutex;
import neko.vm.Tls;
#else
import cpp.vm.Thread;
import cpp.vm.Mutex;
import cpp.vm.Tls;
#end

#if haxe3
typedef Hash<T> = haxe.ds.StringMap<T>;
#end
typedef FileGroups = Hash<FileGroup>;
typedef Targets = Hash<Target>;
typedef Prelinkers = Hash<Prelinker>;
typedef Linkers = Hash<Linker>;

class BuildTool
{
   var mDefines:Hash<String>;
   var mCurrentIncludeFile:String;
   var mIncludePath:Array<String>;
   var mCompiler:Compiler;
   var mStripper:Stripper;
   var mPrelinkers:Prelinkers;
   var mLinkers:Linkers;
   var mFileGroups:FileGroups;
   var mTargets:Targets;
   var m64:Bool;
   var m32:Bool;

   public static var os="";
   public static var sAllowNumProcs = true;
   public static var sCompileThreadCount = 1;
   public static var HXCPP = "";
   public static var is64 = false;
   public static var isWindows = false;
   public static var isLinux = false;
   public static var isRPi = false;
   public static var isMac = false;
   public static var useCache = false;
   public static var compileCache:String;
   public static var targetKey:String;
   public static var instance:BuildTool;
   public static var helperThread = new Tls<Thread>();
   static var mVarMatch = new EReg("\\${(.*?)}","");

   public static var exitOnThreadError = false;
   public static var threadExitCode = 0;

   public function new(inMakefile:String,inDefines:Hash<String>,inTargets:Array<String>,
        inIncludePath:Array<String> )
   {
      mDefines = inDefines;
      mFileGroups = new FileGroups();
      mCompiler = null;
      compileCache = "";
      mStripper = null;
      mTargets = new Targets();
      mPrelinkers = new Prelinkers();
      mLinkers = new Linkers();
      mCurrentIncludeFile = "";
      mIncludePath = inIncludePath;
      instance = this;

      m64 = mDefines.exists("HXCPP_M64");
      m32 = mDefines.exists("HXCPP_M32");
      if (m64==m32)
      {
         // Default to the current version of neko...
         m64 = (~/64$/).match(os);
         m32 = !m64;
         mDefines.remove(m32 ? "HXCPP_M64" : "HXCPP_M32");
      }


      include("toolchain/setup.xml");

      if (sAllowNumProcs)
      {
         var thread_var = mDefines.exists("HXCPP_COMPILE_THREADS") ?
            mDefines.get("HXCPP_COMPILE_THREADS") : Sys.getEnv("HXCPP_COMPILE_THREADS");

         if (thread_var == null)
         {
            sCompileThreadCount = getNumberOfProcesses();
         }
         else
         {
            sCompileThreadCount = (Std.parseInt(thread_var)<2) ? 1 : Std.parseInt(thread_var);
         }
         Log.v("Using " + sCompileThreadCount + " compile threads");
      }



      if (mDefines.exists("toolchain"))
      {   
         if (!mDefines.exists("BINDIR"))
         {   
            mDefines.set("BINDIR", Path.withoutDirectory(Path.withoutExtension(mDefines.get("toolchain"))));  
         }
      }
      else
         setDefaultToolchain(mDefines);


      if (mDefines.exists("dll_import"))
      {
         var path = new Path(mDefines.get("dll_import"));
         if (!mDefines.exists("dll_import_include"))
            mDefines.set("dll_import_include", path.dir + "/include" );
         if (!mDefines.exists("dll_import_link"))
            mDefines.set("dll_import_link", mDefines.get("dll_import") );
      }

      setupAppleDirectories(mDefines);


      include("toolchain/finish-setup.xml");


      var make_contents = "";
      try {
         make_contents = sys.io.File.getContent(inMakefile);
      } catch (e:Dynamic) {
         Log.error("Could not open build file \"" + inMakefile + "\"");
         //println("Could not open build file '" + inMakefile + "'");
         //Sys.exit(1);
      }

      if (!mDefines.exists("HXCPP_COMPILE_THREADS"))
         mDefines.set("HXCPP_COMPILE_THREADS", Std.string(getNumberOfProcesses()));

      var xml_slow = Xml.parse(make_contents);
      var xml = new Fast(xml_slow.firstElement());
      
      parseXML(xml,"");

      if (mDefines.exists("HXCPP_COMPILE_CACHE"))
      {
         compileCache = mDefines.get("HXCPP_COMPILE_CACHE");
         // Don't get upset by trailing slash
         while(compileCache.length>1)
         {
            var l = compileCache.length;
            var last = compileCache.substr(l-1);
            if (last=="/" || last=="\\")
               compileCache = compileCache.substr(0,l-1);
            else
               break;
         }

         if (FileSystem.exists(compileCache) && FileSystem.isDirectory(compileCache))
         {
            useCache = true;
         }
         else
         {
            Log.error("Could not find compiler cache \"" + compileCache + "\"");
            //throw "Could not find compiler cache: " + compileCache;
         }
      }

      if (useCache && (!mDefines.exists("haxe_ver") && !mDefines.exists("HXCPP_DEPENDS_OK")))
      {
         Log.info("", "Ignoring compiler cache because of possible missing dependencies");
         useCache = false;
      }

      if (useCache)
      {
         Log.info("", "Using compiler cache \"" + compileCache + "\"");
      }

      if (inTargets.remove("clear"))
         for(target in mTargets.keys())
            cleanTarget(target,false);

      if (inTargets.remove("clean"))
         for(target in mTargets.keys())
            cleanTarget(target,true);

      for(target in inTargets)
         buildTarget(target);
   }

   public static function setThreadError(inCode:Int)
   {
      threadExitCode = inCode;
      if (exitOnThreadError)
         Sys.exit(inCode);
   }

   public function buildTarget(inTarget:String)
   {
      // Sys.println("Build : " + inTarget );
      if (!mTargets.exists(inTarget))
      {
         Log.error ("Could not find build target \"" + inTarget + "\"");
         //throw "Could not find target '" + inTarget + "' to build.";
      }
      if (mCompiler==null)
      {
         Log.error("No compiler defined for the current build target");
         //throw "No compiler defined";
      }

      var target = mTargets.get(inTarget);
      target.checkError();

      for(sub in target.mSubTargets)
         buildTarget(sub);
 
      var threads = BuildTool.sCompileThreadCount;

      PathManager.resetDirectoryCache();
      var restoreDir = "";
      if (target.mBuildDir!="")
      {
         restoreDir = Sys.getCwd();
         Log.info("", " - \x1b[1mChanging directory:\x1b[0m " + target.mBuildDir);
         Sys.setCwd(target.mBuildDir);
      }

      targetKey = inTarget + target.getKey();
 
      var objs = new Array<String>();

      if (target.mFileGroups.length > 0)
         PathManager.mkdir(mCompiler.mObjDir);
      for(group in target.mFileGroups)
      {
         group.checkOptions(mCompiler.mObjDir);

         group.checkDependsExist();

         group.preBuild();

         var to_be_compiled = new Array<File>();

         for(file in group.mFiles)
         {
            var obj_name = mCompiler.getObjName(file);
            objs.push(obj_name);
            if (file.isOutOfDate(obj_name))
            {
               if (useCache)
                  file.computeDependHash();
               to_be_compiled.push(file);
            }
         }

         var cached = useCache && mCompiler.createCompilerVersion(group);

         if (!cached && group.mPrecompiledHeader!="")
         {
            if (to_be_compiled.length>0)
               mCompiler.precompile(mCompiler.mObjDir, group);

            if (mCompiler.needsPchObj())
            {
               var pchDir = group.getPchDir();
               if (pchDir != "")
            {
                  objs.push(mCompiler.mObjDir + "/" + pchDir + "/" + group.getPchName() + mCompiler.mExt);
            }
            }
         }

         if (threads<2)
         {
            for(file in to_be_compiled)
               mCompiler.compile(file,-1);
         }
         else
         {
            var mutex = new Mutex();
            Log.initMultiThreaded();
            var main_thread = Thread.current();
            var compiler = mCompiler;
            for(t in 0...threads)
            {
               Thread.create(function()
               {
                  try
                  {
                  while(threadExitCode==0)
                  {
                     mutex.acquire();
                     if (to_be_compiled.length==0)
                     {
                        mutex.release();
                        break;
                     }
                     var file = to_be_compiled.shift();
                     mutex.release();

                     compiler.compile(file,t);
                  }
                  }
                  catch (error:Dynamic)
                  {
                     if (threadExitCode!=0)
                        setThreadError(-1);
                  }
                  main_thread.sendMessage("Done");
               });
            }

            // Wait for theads to finish...
            for(t in 0...threads)
            {
               Thread.readMessage(true);
            }

            // Already printed the error from the thread, just need to exit
            if (threadExitCode!=0)
               Sys.exit(threadExitCode);
         }
      }

      switch(target.mTool)
      {
         case "linker":
            if (mPrelinkers.exists(target.mToolID))
            {
               var result = mPrelinkers.get(target.mToolID).prelink(target,objs, mCompiler);
               if (result != "")
                  objs.push(result);
               //throw "Missing linker :\"" + target.mToolID + "\"";
            }
            
            if (!mLinkers.exists(target.mToolID))
            {
               Log.error ("Could not find linker for \"" + target.mToolID + "\"");
               //throw "Missing linker :\"" + target.mToolID + "\"";
            }

            var exe = mLinkers.get(target.mToolID).link(target,objs, mCompiler);
            if (exe!="" && mStripper!=null)
               if (target.mToolID=="exe" || target.mToolID=="dll")
                  mStripper.strip(exe);
      }

      if (restoreDir!="")
         Sys.setCwd(restoreDir);
   }

   public function cleanTarget(inTarget:String,allObj:Bool)
   {
      // Sys.println("Build : " + inTarget );
      if (!mTargets.exists(inTarget))
      {
         Log.error("Could not find build target \"" + inTarget + "\"");
         //throw "Could not find target '" + inTarget + "' to build.";
      }
      if (mCompiler==null)
      {
         Log.error("No compiler defined");
         //throw "No compiler defined";
      }

      var target = mTargets.get(inTarget);
      target.checkError();

      for(sub in target.mSubTargets)
         cleanTarget(sub,allObj);

      var restoreDir = "";
      if (target.mBuildDir!="")
      {
         restoreDir = Sys.getCwd();
         Log.info("", " - \x1b[1mChanging directory:\x1b[0m " + target.mBuildDir);
         Sys.setCwd(target.mBuildDir);
      }

      PathManager.removeDirectory(mCompiler.mObjDir);
      PathManager.removeFile("all_objs");
      PathManager.removeFilesWithExtension(".pdb");
      if (allObj)
         PathManager.removeDirectory("obj");

      if (restoreDir!="")
         Sys.setCwd(restoreDir);
   }

   public function createCompiler(inXML:Fast,inBase:Compiler) : Compiler
   {
      var c = inBase;
      if (inBase==null || inXML.has.replace)
      {
         c = new Compiler(substitute(inXML.att.id),substitute(inXML.att.exe),mDefines.exists("USE_GCC_FILETYPES"));
         if (mDefines.exists("USE_PRECOMPILED_HEADERS"))
            c.setPCH(mDefines.get("USE_PRECOMPILED_HEADERS"));
      }

      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "flag" : c.mFlags.push(substitute(el.att.value));
               case "cflag" : c.mCFlags.push(substitute(el.att.value));
               case "cppflag" : c.mCPPFlags.push(substitute(el.att.value));
               case "objcflag" : c.mOBJCFlags.push(substitute(el.att.value));
               case "mmflag" : c.mMMFlags.push(substitute(el.att.value));
               case "pchflag" : c.mPCHFlags.push(substitute(el.att.value));
               case "objdir" : c.mObjDir = substitute((el.att.value));
               case "outflag" : c.mOutFlag = substitute((el.att.value));
               case "exe" : c.mExe = substitute((el.att.name));
               case "ext" : c.mExt = substitute((el.att.value));
               case "pch" : c.setPCH( substitute((el.att.value)) );
               case "getversion" : c.mGetCompilerVersion = substitute((el.att.value));
               case "section" : createCompiler(el,c);
               case "include" :
                  var name = substitute(el.att.name);
                  var full_name = findIncludeFile(name);
                  if (full_name!="")
                  {
                     var make_contents = sys.io.File.getContent(full_name);
                     var xml_slow = Xml.parse(make_contents);
                     createCompiler(new Fast(xml_slow.firstElement()),c);
                  }
                  else if (!el.has.noerror)
                  {
                     Log.error("Could not find include file \"" + name + "\"");
                     //throw "Could not find include file " + name;
                  }
               default:
                  Log.error("Unknown compiler option \"" + el.name + "\"");
                  //throw "Unknown compiler option: '" + el.name + "'";
            }
      }

      return c;
   }

   public function createFileGroup(inXML:Fast,inFiles:FileGroup,inName:String):FileGroup
   {
      var dir = inXML.has.dir ? substitute(inXML.att.dir) : ".";
      var group = inFiles==null ? new FileGroup(dir,inName) : inFiles;
      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "file" :
                  var file = new File(substitute(el.att.name),group);
                  for(f in el.elements)
                     if (valid(f,"") && f.name=="depend")
                        file.mDepends.push( substitute(f.att.name) );
                  group.mFiles.push( file );
               case "section" : createFileGroup(el,group,inName);
               case "depend" : group.addDepend( substitute(el.att.name) );
               case "hlsl" :
                  group.addHLSL( substitute(el.att.name), substitute(el.att.profile),
                  substitute(el.att.variable), substitute(el.att.target)  );
               case "options" : group.addOptions( substitute(el.att.name) );
               case "compilerflag" : group.addCompilerFlag( substitute(el.att.value) );
               case "compilervalue" : 
                  group.addCompilerFlag( substitute(el.att.name) );
                  group.addCompilerFlag( substitute(el.att.value) );
               case "precompiledheader" : 
                  group.setPrecompiled( substitute(el.att.name), substitute(el.att.dir) );
               case "include" : 
                  var subbed_name = substitute(el.att.name);
                  var full_name = findIncludeFile(subbed_name);
                  if (full_name!="")
                  {
                     var make_contents = sys.io.File.getContent(full_name);
                     var xml_slow = Xml.parse(make_contents);
                     createFileGroup(new Fast(xml_slow.firstElement()), group, inName);
                  } 
                  else
                  {
                     Log.error("Could not find include file \"" + subbed_name + "\"");

                  }
            }
      }

      return group;
   }

   public function createLinker(inXML:Fast,inBase:Linker):Linker
   {
      var l = (inBase!=null && !inXML.has.replace) ? inBase : new Linker(inXML.att.exe);
      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "flag" : l.mFlags.push(substitute(el.att.value));
               case "ext" : l.mExt = (substitute(el.att.value));
               case "outflag" : l.mOutFlag = (substitute(el.att.value));
               case "libdir" : l.mLibDir = (substitute(el.att.name));
               case "lib" : l.mLibs.push( substitute(el.att.name) );
               case "prefix" : l.mNamePrefix = substitute(el.att.value);
               case "ranlib" : l.mRanLib = (substitute(el.att.name));
               case "recreate" : l.mRecreate = (substitute(el.att.value)) != "";
               case "expandAr" : l.mExpandArchives = substitute(el.att.value) != "";
               case "fromfile" : l.mFromFile = (substitute(el.att.value));
               case "exe" : l.mExe = (substitute(el.att.name));
               case "section" : createLinker(el,l);
            }
      }

      return l;
   }

   public function createPrelinker(inXML:Fast,inBase:Prelinker):Prelinker
   {
      var l = (inBase!=null && !inXML.has.replace) ? inBase : new Prelinker(inXML.att.exe);
      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "flag" : l.mFlags.push(substitute(el.att.value));
               //case "ext" : l.mExt = (substitute(el.att.value));
               case "outflag" : l.mOutFlag = (substitute(el.att.value));
               case "expandAr" : l.mExpandArchives = substitute(el.att.value) != "";
               case "fromfile" : l.mFromFile = (substitute(el.att.value));
               case "exe" : l.mExe = (substitute(el.att.name));
               case "section" : createPrelinker(el,l);
            }
      }

      return l;
   }

   public function createStripper(inXML:Fast,inBase:Stripper):Stripper
   {
      var s = (inBase!=null && !inXML.has.replace) ? inBase :
                 new Stripper(inXML.att.exe);
      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
                case "flag" : s.mFlags.push(substitute(el.att.value));
                case "exe" : s.mExe = substitute((el.att.name));
            }
      }

      return s;
   }

   public function createTarget(inXML:Fast,?inTarget:Target) : Target
   {
      var target:Target = inTarget;
      if (target==null)
      {
         var output = inXML.has.output ? substitute(inXML.att.output) : "";
         var tool = inXML.has.tool ? inXML.att.tool : "";
         var toolid = inXML.has.toolid ? substitute(inXML.att.toolid) : "";
         target = new Target(output,tool,toolid);
      }

      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "target" : target.mSubTargets.push( substitute(el.att.id) );
               case "lib" : target.mLibs.push( substitute(el.att.name) );
               case "flag" : target.mFlags.push( substitute(el.att.value) );
               case "depend" : target.mDepends.push( substitute(el.att.name) );
               case "vflag" :
                  target.mFlags.push( substitute(el.att.name) );
                  target.mFlags.push( substitute(el.att.value) );
               case "dir" : target.mDirs.push( substitute(el.att.name) );
               case "outdir" : target.mOutputDir = substitute(el.att.name)+"/";
               case "ext" : target.mExt = (substitute(el.att.value));
               case "builddir" : target.mBuildDir = substitute(el.att.name);
               case "files" :
                  var id = el.att.id;
                  if (!mFileGroups.exists(id))
                     target.addError( "Could not find filegroup " + id ); 
                  else
                     target.addFiles( mFileGroups.get(id) );
               case "section" : createTarget(el,target);
            }
      }

      return target;
   }

   public function defined(inString:String):Bool
   {
      return mDefines.exists(inString);
   }

   function findIncludeFile(inBase:String):String
   {
      if (inBase == null || inBase=="") return "";
      var c0 = inBase.substr(0,1);
      if (c0!="/" && c0!="\\")
      {
         var c1 = inBase.substr(1,1);
         if (c1!=":")
         {
            if (mCurrentIncludeFile!="")
            {
               var relative = Path.directory(mCurrentIncludeFile);
               var name = PathManager.combine(relative, inBase);
               if (FileSystem.exists(name))
                  return name;
            }

            for(p in mIncludePath)
            {
               var name = PathManager.combine(p, inBase);
               if (FileSystem.exists(name))
                  return name;
            }
            return "";
         }
      }
      if (FileSystem.exists(inBase))
         return inBase;
      return "";
   }
   
   private static function getIs64():Bool
   {
      if (isWindows)
      {
         var architecture = Sys.getEnv ("PROCESSOR_ARCHITEW6432");
         if (architecture != null && architecture.indexOf ("64") > -1)
         {
            return true;
         }
         else
         {
            return false;
         }
      }
      else
      {
         var process = new Process("uname", [ "-m" ]);
         var output = process.stdout.readAll().toString();
         var error = process.stderr.readAll().toString();
         process.exitCode();
         process.close();
         
         if (output.indexOf("64") > -1)
         {
            return true;
         }
         else
         {
            return false;
         }
      }
   }

   static public function getMsvcVer()
   {
      return instance.mDefines.get("MSVC_VER");
   }

   // Setting HXCPP_COMPILE_THREADS to 2x number or cores can help with hyperthreading
   public static function getNumberOfProcesses():Int
   {
      var result = null;
      if (isWindows)
      {
         var env = Sys.getEnv("NUMBER_OF_PROCESSORS");
         if (env != null)
         {      
            result = env;   
         }   
      }
      else if (isLinux)
      {
         result = ProcessManager.runProcessLine("", "nproc", [], true, false);
         if (result == null)
         {
            var cpuinfo = ProcessManager.runProcess("", "cat", [ "/proc/cpuinfo" ], true, false);
            if (cpuinfo != null)
            {      
               var split = cpuinfo.split("processor");
               result = Std.string(split.length - 1);   
            }   
         }
      }
      else if (isMac)
      {
         var cores = ~/Total Number of Cores: (\d+)/;
         var output = ProcessManager.runProcess("", "/usr/sbin/system_profiler", [ "-detailLevel", "full", "SPHardwareDataType" ], true, false);
         if (cores.match(output))
         {
            result = cores.matched(1); 
         }
      }
      
      if (result == null || Std.parseInt(result) < 1)
      {   
         return 1;
      }
      else
      {   
         return Std.parseInt(result);
      }
   }
   
   private static function getVersion():String
   {
      try
      {
         var json = Json.parse (sys.io.File.getContent (PathManager.getHaxelib ("hxcpp") + "/haxelib.json"));
         return json.version;
      }
      catch (e:Dynamic)
      {
         return "0.0.0";
      }
   }

   public static function isMsvc()
   {
      return instance.mDefines.get("toolchain")=="msvc";
   }

   // Process args and environment.
   static public function main()
   {
      var targets = new Array<String>();
      var defines = new Hash<String>();
      var include_path = new Array<String>();
      var makefile:String="";

      include_path.push(".");

      var args = Sys.args();
      var env = Sys.environment();

      for(e in env.keys())
         defines.set(e, Sys.getEnv(e) );


      // Check for calling from haxelib ...
      if (args.length>0)
      {
         var last:String = (new Path(args[args.length-1])).toString();
         var slash = last.substr(-1);
         if (slash=="/"|| slash=="\\") 
            last = last.substr(0,last.length-1);
         if (FileSystem.exists(last) && FileSystem.isDirectory(last))
         {
            // When called from haxelib, the last arg is the original directory, and
            //  the current direcory is the library directory.
            HXCPP = PathManager.standardize(Sys.getCwd());
            defines.set("HXCPP",HXCPP);
            args.pop();
            Sys.setCwd(last);
         }
      }

      if (defines.exists("HXCPP_NO_COLOUR") || defines.exists("HXCPP_NO_COLOR"))
         Log.colorSupported = false;
      Log.verbose = defines.exists("HXCPP_VERBOSE");
      exitOnThreadError = defines.exists("HXCPP_EXIT_ON_ERROR");


      os = Sys.systemName();

      isWindows = (new EReg("window","i")).match(os);
      if (isWindows)
         defines.set("windows_host", "1");
      isMac = (new EReg("mac","i")).match(os);
      if (isMac)
         defines.set("mac_host", "1");
      isLinux = (new EReg("linux","i")).match(os);
      if (isLinux)
         defines.set("linux_host", "1");

      isRPi = isLinux && Setup.isRaspberryPi();

      is64 = getIs64();
      
      for(arg in args)
      {
         if (arg.substr(0,2)=="-D")
         {
            var val = arg.substr(2);
            var equals = val.indexOf("=");
            if (equals>0)
               defines.set(val.substr(0,equals), val.substr(equals+1) );
            else
               defines.set(val,"");
            if (val=="verbose")
               Log.verbose = true;
         }
         else if (arg=="-v" || arg=="-verbose")
            Log.verbose = true;
         else if (arg=="-nocolor")
            Log.colorSupported = false;
         else if (arg.substr(0,2)=="-I")
            include_path.push(PathManager.standardize(arg.substr(2)));
         else if (makefile.length==0)
            makefile = arg;
         else
            targets.push(arg);
      }

      Setup.initHXCPPConfig(defines);

      if (HXCPP=="" && env.exists("HXCPP"))
      {
         HXCPP = PathManager.standardize(env.get("HXCPP"));
         defines.set("HXCPP",HXCPP);
      }

      if (HXCPP=="")
      {
         if (!defines.exists("HXCPP"))
         {
            Log.error("Please run hxcpp using haxelib");
            //throw "HXCPP not set, and not run from haxelib";
         }
         HXCPP = PathManager.standardize(defines.get("HXCPP"));
         defines.set("HXCPP",HXCPP);
      }
      
      //Log.info("", "HXCPP : " + HXCPP);
      
      include_path.push(".");
      if (env.exists("HOME"))
        include_path.push(env.get("HOME"));
      if (env.exists("USERPROFILE"))
        include_path.push(env.get("USERPROFILE"));
      include_path.push(HXCPP);

      //trace(include_path);

      //var msvc = false;
      
      // Create alias...
      if (defines.exists("ios"))
      {
         if (defines.exists("simulator"))
            defines.set("iphonesim", "iphonesim");
         else if (!defines.exists ("iphonesim"))
            defines.set("iphoneos", "iphoneos");
         defines.set("iphone", "iphone");
      }

 
     

      if (makefile=="" || Log.verbose)
      {
         printBanner();
      }
      
      if (makefile=="")
      {
         Log.println(" \x1b[33;1mUsage:\x1b[0m\x1b[1m haxelib run hxcpp\x1b[0m Build.xml \x1b[3;37m[options]\x1b[0m");
         Log.println("");
         Log.println(" \x1b[33;1mOptions:\x1b[0m ");
         Log.println("");
         Log.println("  \x1b[1m-D\x1b[0;3mvalue\x1b[0m -- Specify a define to use when processing other commands");
         Log.println("  \x1b[1m-verbose\x1b[0m -- Print additional information (when available)");
         Log.println("");
      }
      else
      {
         Log.info("", "\x1b[33;1mUsing makefile: " + makefile + "\x1b[0m");
         Log.info("", "\x1b[33;1mReading HXCPP config: " + defines.get("HXCPP_CONFIG") + "\x1b[0m");
         Log.info("", "\x1b[33;1mUsing target toolchain: " + defines.get("toolchain") + "\x1b[0m");
         if (Log.verbose) Log.println("");
 

         if (targets.length==0)
            targets.push("default");
     
         new BuildTool(makefile,defines,targets,include_path);
      }
   }

   static function printBanner()
   {
      Log.println("\x1b[33;1m __                          ");             
      Log.println("/\\ \\                                      ");
      Log.println("\\ \\ \\___    __  _   ___   _____   _____   ");
      Log.println(" \\ \\  _ `\\ /\\ \\/'\\ /'___\\/\\ '__`\\/\\ '__`\\ ");
      Log.println("  \\ \\ \\ \\ \\\\/>  <//\\ \\__/\\ \\ \\L\\ \\ \\ \\L\\ \\");
      Log.println("   \\ \\_\\ \\_\\/\\_/\\_\\ \\____\\\\ \\ ,__/\\ \\ ,__/");
      Log.println("    \\/_/\\/_/\\//\\/_/\\/____/ \\ \\ \\/  \\ \\ \\/ ");
      Log.println("                            \\ \\_\\   \\ \\_\\ ");
      Log.println("                             \\/_/    \\/_/ \x1b[0m");
      Log.println("");
      Log.println("\x1b[1mhxcpp \x1b[0m\x1b[3;37m(Haxe C++ Runtime Support)\x1b[0m \x1b[1m(" + getVersion() + ")\x1b[0m");
      Log.println("");
   }

   function setDefaultToolchain(defines:Hash<String>)
   {
      if (defines.exists("iphoneos"))
      {
         defines.set("toolchain","iphoneos");
         defines.set("iphone","iphone");
         defines.set("apple","apple");
         defines.set("BINDIR","iPhone");
      }
      else if (defines.exists("iphonesim"))
      {
         defines.set("toolchain","iphonesim");
         defines.set("iphone","iphone");
         defines.set("apple","apple");
         defines.set("BINDIR","iPhone");
      }
      else if (defines.exists("android"))
      {
         defines.set("toolchain","android");
         defines.set("android","android");
         defines.set("BINDIR","Android");

         if (!defines.exists("ANDROID_HOST"))
         {
            if ( (new EReg("mac","i")).match(os) )
               defines.set("ANDROID_HOST","darwin-x86");
            else if ( (new EReg("window","i")).match(os) )
               defines.set("ANDROID_HOST","windows");
            else if ( (new EReg("linux","i")).match(os) )
               defines.set("ANDROID_HOST","linux-x86");
            else
            {
               Log.error ("Unknown android host \"" + os + "\"");
               //throw "Unknown android host:" + os;
            }
         }
      }
      else if (defines.exists("webos"))
      {
         defines.set("toolchain","webos");
         defines.set("webos","webos");
         defines.set("BINDIR","webOS");
      }
      else if (defines.exists("tizen"))
      {
         if (defines.exists ("simulator"))
         {
            defines.set("toolchain","tizen-x86");
         }
         else
         {
            defines.set("toolchain","tizen");
         }
         defines.set("tizen","tizen");
         defines.set("BINDIR","Tizen");
      }
      else if (defines.exists("blackberry"))
      {
         if (defines.exists("simulator"))
         {
            defines.set("toolchain", "blackberry-x86");
         }
         else
         {
            defines.set("toolchain", "blackberry");
         }
         defines.set("blackberry","blackberry");
         defines.set("BINDIR","BlackBerry");
      }
      else if (defines.exists("emcc") || defines.exists("emscripten"))
      {
         defines.set("toolchain","emscripten");
         defines.set("emcc","emcc");
         defines.set("emscripten","emscripten");
         defines.set("BINDIR","Emscripten");
      }
      else if (defines.exists("gph"))
      {
         defines.set("toolchain","gph");
         defines.set("gph","gph");
         defines.set("BINDIR","GPH");
      } else if (defines.exists ("gcw0")) {
	 defines.set ("toolchain", "gcw0");
	 defines.set ("gcw0", "gcw0");
	 defines.set ("BINDIR", "GCW0");
      } else if (defines.exists("mingw") || defines.exists("HXCPP_MINGW") )
      {
         set64(defines,m64);
         defines.set("toolchain","mingw");
         defines.set("mingw","mingw");
         defines.set("BINDIR",m64 ? "Windows64":"Windows");
      }
      else if (defines.exists("cygwin") || defines.exists("HXCPP_CYGWIN"))
      {
         set64(defines,m64);
         defines.set("toolchain","cygwin");
         defines.set("cygwin","cygwin");
         defines.set("linux","linux");
         defines.set("BINDIR",m64 ? "Cygwin64":"Cygwin");
      }
      else if ( (new EReg("window","i")).match(os) )
      {
         defines.set("windows_host","1");
         // Cross-compile?
         if (defines.exists("rpi"))
         {
            defines.set("toolchain","linux");
            defines.set("xcompile","1");
            defines.set("linux","linux");
            defines.set("rpi","1");
            defines.set("hardfp","1");
            defines.set("BINDIR", "RPi");
         }
         else
         {
            set64(defines,m64);
            defines.set("toolchain","msvc");
            defines.set("windows","windows");
            //msvc = true;
            if ( defines.exists("winrt") )
            {
               defines.set("BINDIR",m64 ? "WinRTx64":"WinRTx86");
            }
            else
            {
               defines.set("BINDIR",m64 ? "Windows64":"Windows");
            }
         }
      }
      else if ( isRPi )
      {
         defines.set("toolchain","linux");
         defines.set("linux","linux");
         defines.set("rpi","1");
         defines.set("hardfp","1");
         defines.set("BINDIR", "RPi");
      }
      else if ( (new EReg("linux","i")).match(os) )
      {
         set64(defines,m64);
         defines.set("toolchain","linux");
         defines.set("linux","linux");
         defines.set("BINDIR", m64 ? "Linux64":"Linux");
      }
      else if ( (new EReg("mac","i")).match(os) )
      {
         set64(defines,m64);
         // Cross-compile?
         if (defines.exists("linux"))
         {
            defines.set("mac_host","1");
            defines.set("linux","linux");
            defines.set("toolchain","linux");
            defines.set("xcompile","1");
            defines.set("BINDIR", m64 ? "Linux64":"Linux");
         }
         else
         {
            defines.set("toolchain","mac");
            defines.set("macos","macos");
            defines.set("apple","apple");
            defines.set("BINDIR",m64 ? "Mac64":"Mac");
         }
      }
   }



   function setupAppleDirectories(defines:Hash<String>)
   {
      if (defines.exists("apple") && !defines.exists("DEVELOPER_DIR"))
      {
         var developer_dir = ProcessManager.runProcessLine("", "xcode-select", ["--print-path"], true, false);
         if (developer_dir == null || developer_dir == "" || developer_dir.indexOf ("Run xcode-select") > -1)
            developer_dir = "/Applications/Xcode.app/Contents/Developer";
         if (developer_dir == "/Developer")
            defines.set("LEGACY_XCODE_LOCATION","1");
         defines.set("DEVELOPER_DIR",developer_dir);
      }

      if (defines.exists("iphone") && !defines.exists("IPHONE_VER"))
      {
         var dev_path = defines.get("DEVELOPER_DIR") + "/Platforms/iPhoneOS.platform/Developer/SDKs/";
         if (FileSystem.exists(dev_path))
         {
            var best="";
            var files = FileSystem.readDirectory(dev_path);
            var extract_version = ~/^iPhoneOS(.*).sdk$/;
            for(file in files)
            {
               if (extract_version.match(file))
               {
                  var ver = extract_version.matched(1);
                  if (Std.parseFloat (ver)>Std.parseFloat (best))
                     best = ver;
               }
            }
            if (best!="")
               defines.set("IPHONE_VER",best);
         }
      }
      
      if (defines.exists("macos") && !defines.exists("MACOSX_VER"))
      {
         var dev_path = defines.get("DEVELOPER_DIR") + "/Platforms/MacOSX.platform/Developer/SDKs/";
         if (FileSystem.exists(dev_path))
         {
            var best="";
            var files = FileSystem.readDirectory(dev_path);
            var extract_version = ~/^MacOSX(.*).sdk$/;
            for(file in files)
            {
               if (extract_version.match(file))
               {
                  var ver = extract_version.matched(1);
                  if (Std.parseFloat (ver)>Std.parseFloat (best))
                     best = ver;
               }
            }
            if (best!="")
               defines.set("MACOSX_VER",best);
         }
      }
      
      if (!FileSystem.exists(defines.get("DEVELOPER_DIR") + "/Platforms/MacOSX.platform/Developer/SDKs/"))
      {
         defines.set("LEGACY_MACOSX_SDK","1");
      }
   }

   function parseXML(inXML:Fast,inSection:String)
   {
      for(el in inXML.elements)
      {
         if (valid(el,inSection))
         {
            switch(el.name)
            {
               case "set" : 
                  var name = el.att.name;
                  var value = substitute(el.att.value);
                  mDefines.set(name,value);
                  if (name == "BLACKBERRY_NDK_ROOT")
                  {
                     Setup.setupBlackBerryNativeSDK(mDefines);
                  }
               case "unset" : 
                  var name = el.att.name;
                  mDefines.remove(name);
               case "setup" : 
                  var name = substitute(el.att.name);
                  Setup.setup(name,mDefines);
               case "echo" : 
                  Log.info(substitute(el.att.value));
               case "setenv" : 
                  var name = el.att.name;
                  var value = substitute(el.att.value);
                  mDefines.set(name,value);
                  Sys.putEnv(name,value);
               case "error" : 
                  var error = substitute(el.att.value);
                  throw(error);
               case "path" : 
                  var path = substitute(el.att.name);
                  Log.info("", "Adding path " + path);
                  var sep = mDefines.exists("windows_host") ? ";" : ":";
                  var add = path + sep + Sys.getEnv("PATH");
                  Sys.putEnv("PATH", add);
                  //trace(Sys.getEnv("PATH"));
               case "compiler" : 
                  mCompiler = createCompiler(el,mCompiler);
               case "stripper" : 
                  mStripper = createStripper(el,mStripper);
               case "prelinker" : 
                  if (mPrelinkers.exists(el.att.id))
                     createPrelinker(el,mPrelinkers.get(el.att.id));
                  else
                     mPrelinkers.set( el.att.id, createPrelinker(el,null) );
               case "linker" : 
                  if (mLinkers.exists(el.att.id))
                     createLinker(el,mLinkers.get(el.att.id));
                  else
                     mLinkers.set( el.att.id, createLinker(el,null) );
               case "files" : 
                  var name = el.att.id;
                  if (mFileGroups.exists(name))
                     createFileGroup(el, mFileGroups.get(name), name);
                  else
                     mFileGroups.set(name,createFileGroup(el,null,name));
               case "include" : 
                  var name = substitute(el.att.name);
                  var section = el.has.section ? el.att.section : "";
                  include(name, section, el.has.noerror);
               case "target" : 
                  var name = substitute(el.att.id);
                  var overwrite = name=="default";
                  if (el.has.overwrite)
                     overwrite = true;
                  if (el.has.append)
                     overwrite = false;
                  if (mTargets.exists(name) && !overwrite)
                     createTarget(el,mTargets.get(name));
                  else
                     mTargets.set( name, createTarget(el,null) );
               case "section" : 
                  parseXML(el,"");
            }
         }
      }
   }


   public function include(inName:String, inSection:String="", inAllowMissing:Bool = false)
   {
      var full_name = findIncludeFile(inName);
      if (full_name!="")
      {
         var oldInclude = mCurrentIncludeFile;
         mCurrentIncludeFile = full_name;

         var make_contents = sys.io.File.getContent(full_name);
         var xml_slow = Xml.parse(make_contents);

         parseXML(new Fast(xml_slow.firstElement()),inSection);

         mCurrentIncludeFile = oldInclude;
      }
      else if (!inAllowMissing)
      {
         Log.error("Could not find include file \"" + inName + "\"");
         //throw "Could not find include file " + name;
      }
   }



   static function set64(outDefines:Hash<String>, in64:Bool)
   {
      if (in64)
      {
         outDefines.set("HXCPP_M64","1");
         outDefines.remove("HXCPP_32");
      }
      else
      {
         outDefines.set("HXCPP_M32","1");
         outDefines.remove("HXCPP_M64");
      }
   }

   public function substitute(str:String):String
   {
      while( mVarMatch.match(str) )
      {
         var sub = mVarMatch.matched(1);
         if (sub.substr(0,8)=="haxelib:")
         {
            sub = PathManager.getHaxelib(sub.substr(8));
            sub = PathManager.standardize(sub);
         }
         else
            sub = mDefines.get(sub);

         if (sub==null) sub="";
         str = mVarMatch.matchedLeft() + sub + mVarMatch.matchedRight();
      }

      return str;
   }

   public function valid(inEl:Fast,inSection:String):Bool
   {
      if (inEl.x.get("if") != null)
      {   
         var value = inEl.x.get("if");
         var optionalDefines = value.split("||");
         var matchOptional = false;
         for (optional in optionalDefines)
         {
            var requiredDefines = optional.split(" ");
            var matchRequired = true;
            for (required in requiredDefines)
            {
               var check = StringTools.trim(required);
               if (check != "" && !defined(check))
               {   
                  matchRequired = false;
               }
            }
            if (matchRequired)
            {   
               matchOptional = true;
            }
         }
         if (optionalDefines.length > 0 && !matchOptional)
         {
            return false;
         }
      }
      
      if (inEl.has.unless)
      {
         var value = substitute(inEl.att.unless);
         var optionalDefines = value.split("||");
         var matchOptional = false;
         for (optional in optionalDefines)
         {
            var requiredDefines = optional.split(" ");
            var matchRequired = true;
            for (required in requiredDefines)
            {
               var check = StringTools.trim(required);
               if (check != "" && !defined(check))
               {
                  matchRequired = false;
               }
            }
            if (matchRequired)
            {
               matchOptional = true;
            }
         }
         if (optionalDefines.length > 0 && matchOptional)
         {
            return false;
         }
      }
      
      if (inEl.has.ifExists)
         if (!FileSystem.exists( substitute(inEl.att.ifExists) )) return false;
      
      if (inSection!="")
      {
         if (inEl.name!="section")
            return false;
         if (!inEl.has.id)
            return false;
         if (inEl.att.id!=inSection)
            return false;
      }
      
      return true;
   }
}
