import CopyFile.Overwrite;
import haxe.io.Path;
import haxe.Json;
import sys.io.Process;
import sys.FileSystem;

#if haxe4
import sys.thread.Thread;
import sys.thread.Mutex;
import sys.thread.Tls;
#elseif neko
import neko.vm.Thread;
import neko.vm.Mutex;
import neko.vm.Tls;
#else
import cpp.vm.Thread;
import cpp.vm.Mutex;
import cpp.vm.Tls;
#end

import haxe.crypto.Md5;

import Log.NORMAL;
import Log.BOLD;
import Log.ITALIC;
import Log.YELLOW;
import Log.WHITE;

using StringTools;

#if (haxe_ver>=4)
typedef XmlAccess = haxe.xml.Access;
#else
typedef XmlAccess = haxe.xml.Fast;
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
   public inline static var SupportedVersion = 430;

   var mDefines:Hash<String>;
   var mCurrentIncludeFile:String;
   var mIncludePath:Array<String>;
   var mCompiler:Compiler;
   var mStripper:Stripper;
   var mManifester:Manifester;
   var mPrelinkers:Prelinkers;
   var mLinkers:Linkers;
   var mCopyFiles:Array<CopyFile>;
   var mFileGroups:FileGroups;
   var mTargets:Targets;
   var mFileStack:Array<String>;
   var mMakefile:String;
   var mMagicLibs:Array<{name:String, replace:String}>;
   var mPragmaOnce:Map<String,Bool>;
   var mNvccFlags:Array<String>;
   var mNvccLinkFlags:Array<String>;
   var mDirtyList:Array<String>;
   var arm64:Bool;
   var m64:Bool;
   var m32:Bool;

   public static var os="";
   public static var sAllowNumProcs = true;
   public static var sCompileThreadCount = 1;
   public static var sThreadPool:ThreadPool;
   public static var sReportedThreads = -1;
   public static var HXCPP = "";
   public static var is64 = false;
   public static var isWindows = false;
   public static var isWindowsArm = false;
   public static var isLinux = false;
   public static var isRPi = false;
   public static var isMac = false;
   public static var targetKey:String;
   public static var instance:BuildTool;
   public static var helperThread = new Tls<Thread>();
   public static var destination:String;
   public static var outputs = new Array<String>();
   public static var groupMutex = new Mutex();
   static var mVarMatch = new EReg("\\${(.*?)}","");
   static var mNoDollarMatch = new EReg("{(.*?)}","");

   public static var exitOnThreadError = false;
   public static var threadExitCode = 0;
   public static var startDir:String;



   public function new(inJob:String,inDefines:Hash<String>,inTargets:Array<String>,
        inIncludePath:Array<String>, inDirtyList:Array<String> )
   {
      mDefines = inDefines;
      mFileGroups = new FileGroups();
      mCompiler = null;
      mStripper = null;
      mTargets = new Targets();
      mPrelinkers = new Prelinkers();
      mLinkers = new Linkers();
      mCurrentIncludeFile = "";
      mFileStack = [];
      mCopyFiles = [];
      mIncludePath = inIncludePath;
      mPragmaOnce = new Map<String,Bool>();
      mMagicLibs = [];
      mNvccFlags = [];
      mNvccLinkFlags = [];
      mMakefile = "";
      mDirtyList = inDirtyList;

      if (inJob=="cache")
      {
      }
      else
      {
         mMakefile = inJob;
         if (!PathManager.isAbsolute(mMakefile) && sys.FileSystem.exists(mMakefile))
            mMakefile = sys.FileSystem.fullPath(mMakefile);
         mDefines.set("HXCPP_BUILD_DIR", Path.addTrailingSlash(Path.directory(mMakefile)) );
      }


      instance = this;

      m64 = mDefines.exists("HXCPP_M64");
      m32 = mDefines.exists("HXCPP_M32");
      arm64 = mDefines.exists("HXCPP_ARM64");
      if (m64==m32 && !arm64)
      {
         var arch = getArch();

         // Default to the current OS version.  windowsArm runs m32 code too
         m64 = arch=="m64";
         m32 = arch=="m32";
         arm64 = arch=="arm64";
         mDefines.remove(m32 ? "HXCPP_M64" : "HXCPP_M32");
         set64(mDefines,m64,arm64);
      }

      Profile.setEntry("parse xml");

      include("toolchain/setup.xml");



      if (mDefines.exists("toolchain"))
      {
         if (!mDefines.exists("BINDIR"))
         {
            mDefines.set("BINDIR", Path.withoutDirectory(Path.withoutExtension(mDefines.get("toolchain"))));
         }
         if ( (new EReg("window","i")).match(os) )
            mDefines.set("windows_host","1");
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

      if (isMsvc())
      {
         mDefines.set("isMsvc","1");
         if (Std.parseInt(mDefines.get("MSVC_VER"))>=18)
            mDefines.set("MSVC18+","1");
      }

      include("toolchain/finish-setup.xml", false);


      if (mMakefile!="")
      {
         pushFile(mMakefile,"makefile");
         var make_contents = "";
         try {
            make_contents = sys.io.File.getContent(mMakefile);
         } catch (e:Dynamic) {
            Log.error("Could not open build file \"" + mMakefile + "\"");
            //println("Could not open build file '" + mMakefile + "'");
            //Sys.exit(1);
         }


         var xml_slow = Xml.parse(make_contents);
         var xml = new XmlAccess(xml_slow.firstElement());

         parseXML(xml,"",false);
         popFile();

         include("toolchain/" + mDefines.get("toolchain") + "-toolchain.xml", false);


         if (mDefines.exists("HXCPP_CONFIG"))
            include(mDefines.get("HXCPP_CONFIG"),"exes",true);
      }

      for(group in mFileGroups)
         group.filter(mDefines);

      if (Log.verbose) Log.println ("");

      // MSVC needs this before the toolchain file, Emscripten wants to set HXCPP_COMPILE_THREADS
      // If not already calculated in "setup"
      getThreadCount();

      var cached = CompileCache.init(mDefines);

      Profile.setEntry("setup cache");

      if (inJob=="cache")
      {
         if (!cached)
         {
            Log.error("HXCPP_COMPILE_CACHE is not set");
         }
         switch(inTargets[0])
         {
            case "days" :
               var days = inTargets[1]==null ? null : Std.parseInt(inTargets[1]);
               if (days==null)
               {
                  Log.error("cache days - expected day count");
                  Tools.exit(1);
               }
               CompileCache.clear(days,0,true,null);
            case "resize" :
               var mb = inTargets[1]==null ? null : Std.parseInt(inTargets[1]);
               if (mb==null)
               {
                  Log.error("cache resize - expected megabyte count");
                  Tools.exit(1);
               }
               CompileCache.clear(0,mb,true,inTargets[2]);

            case "clear" : CompileCache.clear(0,0,true,inTargets[1]);
            case "list" : CompileCache.list(false,inTargets[1]);
            case "details" : CompileCache.list(true,inTargets[1]);
            default:
              printUsage();
              Tools.exit(1);
         }
         return;
      }

      if (cached)
      {
         var cacheSize = mDefines.exists("HXCPP_CACHE_MB") ? Std.parseInt( mDefines.get("HXCPP_CACHE_MB") ) : 1000;
         if (cacheSize!=null && cacheSize>0)
            CompileCache.clear(0,cacheSize,false,null);
      }

      if (Log.verbose) Log.println ("");

      if (inTargets.remove("clear"))
      {
         Profile.setEntry("clear");
         for(target in mTargets.keys())
            cleanTarget(target,false);
       }

      if (inTargets.remove("clean"))
      {
         Profile.setEntry("clean");
         for(target in mTargets.keys())
            cleanTarget(target,true);
      }

      if (destination!=null && inTargets.length!=1)
      {
         Log.warn("Exactly one target must be specified with 'destination'.  Specified:" + inTargets );
         destination = null;
      }

      Profile.setEntry("build");
      for(target in inTargets)
         buildTarget(target,destination);

      var linkOutputs = mDefines.get("HXCPP_LINK_OUTPUTS");
      if (linkOutputs!=null)
         sys.io.File.saveContent(linkOutputs,outputs.join("\n")+"\n");
      if (Log.verbose)
      {
         for(out in outputs)
            Log.v(" generated " + out);
      }

      if (threadExitCode != 0)
         Tools.exit(threadExitCode);
   }

   public static function isDefault64()
   {
   }

   public function pushFile(inFilename:String, inWhy:String, inSection:String="")
   {
      Log.info("", " - \x1b[1mParsing " + inWhy + ":\x1b[0m " + inFilename + (inSection == "" ? "" : " \x1b[3m(section \"" + inSection + "\")\x1b[0m"));
      mFileStack.push(inFilename);
   }

   public function popFile()
   {
      mFileStack.pop();
   }

   public static function addOutput(inWhat:String, inWhere:String)
   {
      outputs.push(inWhat + "=" + inWhere);
   }

   public static function getThreadCount() : Int
   {
      if (instance==null)
         return sCompileThreadCount;
      var defs = instance.mDefines;
      if (sAllowNumProcs)
      {
         var thread_var = defs.exists("HXCPP_COMPILE_THREADS") ?
            defs.get("HXCPP_COMPILE_THREADS") : Sys.getEnv("HXCPP_COMPILE_THREADS");

         if (thread_var == null)
         {
            sCompileThreadCount = getNumberOfProcesses();
         }
         else
         {
            sCompileThreadCount = (Std.parseInt(thread_var)<2) ? 1 : Std.parseInt(thread_var);
         }
         if (sCompileThreadCount!=sReportedThreads)
         {
            sReportedThreads = sCompileThreadCount;
            Log.v("\x1b[33;1mUsing compile threads: " + sCompileThreadCount + "\x1b[0m");
         }
      }
      if (sCompileThreadCount>1 && sThreadPool==null)
         sThreadPool = new ThreadPool(sCompileThreadCount);

      return sCompileThreadCount;
   }

   public static function setThreadError(inCode:Int)
   {
      threadExitCode = inCode;
      if (exitOnThreadError)
         Tools.exit(inCode);
   }

   public function buildTarget(inTarget:String, inDestination:String)
   {
      //var dependDebug = function(s:String) Log.error(s);
      var dependDebug = null;

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

      var target:Target = mTargets.get(inTarget);
      target.checkError();

      for(sub in target.mSubTargets)
         buildTarget(sub,null);

      var threadPool = BuildTool.sThreadPool;


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

      mCompiler.objToAbsolute();

      if (target.mFileGroups.length > 0)
         PathManager.mkdir(mCompiler.mObjDir);

      var baseDir = Sys.getCwd();
      for(group in target.mFileGroups)
      {
         var useCache = CompileCache.hasCache && group.mUseCache;
         if (!useCache && group.mUseCache)
            Log.v("Ignoring compiler cache because HXCPP_COMPILE_CACHE is not valid.");

         var groupObjs = new Array<String>();

         if (group.mDir!="." && group.mSetImportDir)
            Sys.setCwd( PathManager.combine(baseDir, group.mDir ) );
         group.checkOptions(mCompiler.mObjDir);

         group.checkDependsExist();

         if (!mCompiler.initPrecompile(mDefines.get("USE_PRECOMPILED_HEADERS") ))
            group.dontPrecompile();

         group.preBuild();

         var to_be_compiled = new Array<File>();

         var cached = useCache && mCompiler.createCompilerVersion(group);

         var inList = new Array<Bool>();
         var groupIsOutOfDate = mDirtyList.indexOf(group.mId)>=0 || mDirtyList.indexOf("all")>=0;

         if (useCache)
         {
            Profile.push("compute hash");
            if (useCache && group.hasFiles() && threadPool!=null)
            {
               Log.initMultiThreaded();
               var names:Array<String> = Lambda.array(Lambda.map(group.mFiles, function(file:File) {return file.mName; }));
               threadPool.setArrayCount( names.length );
               threadPool.runJob( function(tid) {
                  var localCache = new Map<String,String>();

                  while(threadExitCode==0)
                  {
                     var id = sThreadPool.getNextIndex();
                     if (id<0)
                        break;

                     group.mFiles.get(names[id]).computeDependHash(localCache);
                  }
               } );
            }
            else
            {
               for(file in group.mFiles)
                  file.computeDependHash(null);
            }
            Profile.pop();
         }


         for(file in group.mFiles)
         {
            var obj_name = mCompiler.getCachedObjName(file);
            groupObjs.push(obj_name);
            var outOfDate = groupIsOutOfDate || file.isOutOfDate(obj_name, dependDebug);
            if (outOfDate)
            {
               if (dependDebug!=null)
                  dependDebug(mCompiler.getCacheString(file));
               to_be_compiled.push(file);
            }
            inList.push(outOfDate);
         }
         var someCompiled = to_be_compiled.length > 0;

         var pchStamp:Null<Float> = null;
         if (group.mPrecompiledHeader!="")
         {
            Profile.push("pch");

            var obj = mCompiler.precompile(group,cached || to_be_compiled.length==0);
            if (obj!=null)
            {
               pchStamp = FileSystem.stat(obj).mtime.getTime();
               groupObjs.push(obj);

               /*
               for(i in 0...group.mFiles.length)
               {
                  var obj_name = groupObjs[i];
                  if (!inList[i])
                  {
                     if (FileSystem.stat(obj_name).mtime.getTime() < pchStamp)
                     {
                        groupObjs.push(obj_name);
                        trace(' Add $obj_name');
                     }
                     else
                        trace(' Ok $obj_name');
                  }
                  else
                  {
                        trace(' Listed $obj_name  ' + group.mFiles[i].mName);
                  }
               }
               */
            }
            Profile.pop();
         }

         if (group.mConfig!="")
         {
            var lines = ["#ifndef HXCPP_CONFIG_INCLUDED","#define HXCPP_CONFIG_INCLUDED"];

            var flags = group.mCompilerFlags.concat(mCompiler.getCompilerDefines("haxe"));
            var define = ~/^-D([^=]*)=?(.*)/;
            for(flag in flags)
            {
                if (define.match(flag))
                {
                   var name = define.matched(1);
                   var val = define.matched(2);
                   lines.push("");
                   lines.push( '#if !defined($name) && !defined(NO_$name)' );
                   lines.push( '#define $name $val' );
                   lines.push( '#endif' );
                }
            }

            lines.push("");
            lines.push("#include <hxcpp.h>");
            lines.push("");
            lines.push("#endif");
            lines.push("");

            var filename = mDefines.exists("HXCPP_OUTPUT_CONFIG_NAME") ?
                           mDefines.get("HXCPP_OUTPUT_CONFIG_NAME") :
                           PathManager.combine( target.mOutputDir, group.mConfig );
            if (!PathManager.isAbsolute(filename))
               filename = PathManager.combine( Sys.getCwd(), filename);

            var content = lines.join("\n");
            if (!FileSystem.exists(filename) || sys.io.File.getContent(filename)!=content)
               sys.io.File.saveContent(filename, content);
            addOutput("config",filename);
         }


         var nvcc = group.mNvcc;
         var first = true;
         var groupHeader = (!Log.quiet && !Log.verbose) ? function()
         {
            if (first)
            {
               groupMutex.acquire();
               if (first)
               {
                  first = false;
                  Log.lock();
                  Log.println("");
                  Log.info("\x1b[33;1mCompiling group: " + group.mId + "\x1b[0m");
                  var message = "\x1b[1m" + (nvcc ? getNvcc() : mCompiler.mExe) + "\x1b[0m";
                  var flags = group.mCompilerFlags;
                  if (!nvcc)
                     flags = flags.concat(mCompiler.getFlagStrings());
                  else
                     flags = flags.concat( BuildTool.getNvccFlags() );

                  for (compilerFlag in flags)
                  {
                     if (StringTools.startsWith(compilerFlag, "-D"))
                     {
                        var index = compilerFlag.indexOf("(");
                        if (index > -1)
                        {
                           message += " \x1b[1m" + compilerFlag.substr(0, index) + "\x1b[0m\x1b[2m" + compilerFlag.substr(index) + "\x1b[0m";
                        }
                        else
                        {
                           message += " \x1b[1m" + compilerFlag + "\x1b[0m";
                        }
                     }
                     else
                     {
                        message += " \x1b[0m" + compilerFlag + "\x1b[0m";
                     }
                  }
                  message += " \x1b[2m...\x1b[0m \x1b[2mtags=" + group.mTags.split(",") + "\x1b[0m";
                  Log.info(message);
                  Log.unlock();
               }
               groupMutex.release();
            }
         } : null;

         Profile.push("compile");
         if (threadPool==null)
         {
            for(file in to_be_compiled)
               mCompiler.compile(file,-1,groupHeader,pchStamp);
         }
         else
         {
            Log.initMultiThreaded();
            var mutex = threadPool.mutex;
            var compiler = mCompiler;
            threadPool.setArrayCount(to_be_compiled.length);
            threadPool.runJob( function(threadId:Int) {
                  while(threadExitCode==0)
                  {
                     var index = threadPool.getNextIndex();
                     if (index<0)
                        break;
                     var file = to_be_compiled[index];

                     compiler.compile(file,threadId,groupHeader,pchStamp);
                  }
            });
         }
         Profile.pop();

         if (CompileCache.hasCache && group.mAsLibrary && mLinkers.exists("static_link"))
         {
            Profile.push("link libs");
            var linker = mLinkers.get("static_link");
            var targetDir = mCompiler.mObjDir;
            if (useCache)
            {
               targetDir = CompileCache.compileCache + "/" + group.getCacheProject() + "/lib";
               PathManager.mkdir(targetDir);
            }
            var libName = targetDir + "/" + mCompiler.getTargetPrefix() + "_" + group.getCacheProject();

            var libTarget = new Target(libName, "linker", "static_link" );
            linker.link(libTarget,groupObjs, mCompiler, [] );
            target.mAutoLibs.push(linker.mLastOutName);
            // Linux the libraries must be added again if the references were not resolved the firs time
            if (group.mAddTwice)
               target.mLibs.push(linker.mLastOutName);
            Profile.pop();
         }
         else if (nvcc)
         {
            var objDir = mCompiler.mObjDir;
            if (group.isCached())
               objDir = CompileCache.compileCache;
            var extraObj = linkNvccFiles(objDir, someCompiled, groupObjs, group.mId, mCompiler.mExt);
            groupObjs.push(extraObj);
            objs = objs.concat(groupObjs);
         }
         else
         {
            objs = objs.concat(groupObjs);
         }

         if (group.mDir!="." && group.mSetImportDir)
            Sys.setCwd( baseDir );
      }

      switch(target.mTool)
      {
         case "linker":
            Profile.push("linker");
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
            var extraDeps = [];
            var manifest = mDefines.get("manifestFile");
            if (manifest!=null)
               extraDeps.push(manifest);

            var linker = mLinkers.get(target.mToolID);
            var output = linker.link(target,objs, mCompiler, extraDeps);

            if (output!="")
            {
               if (mStripper!=null)
               {
                  if (target.mToolID=="exe" || target.mToolID=="dll")
                  {
                     if ( mDefines.exists("HXCPP_DEBUG_LINK_AND_STRIP") )
                     {
                        var unstripped = linker.getUnstrippedFilename(mCompiler.mObjDir,target);
                        //var unstripped = mCompiler.mObjDir + "/" + linker.getSimpleFilename(target);
                        Log.v("Save unstripped to " + unstripped);

                        var chmod = isWindows ? false : target.mToolID=="exe";
                        CopyFile.copyFile(output, unstripped, false, Overwrite.ALWAYS, chmod);
                     }

                     mStripper.strip(output);
                  }
               }

               if (manifest!=null && (target.mToolID=="exe" || target.mToolID=="dll") )
               {
                  if (mManifester==null)
                  {
                     Log.v('Could not find manifest tool for "$manifest" - ignoring');
                  }
                  else
                  {
                     //if (!PathManager.isAbsolute(manifest))
                        //manifest = PathManager.combine(startDir,manifest);
                     Log.v('Adding manifest "$manifest"');
                     mManifester.add(output, manifest,target.mToolID=="exe");
                  }
               }
            }


            var outFile = linker.mLastOutName;
            if (outFile!="" && !PathManager.isAbsolute(outFile) && sys.FileSystem.exists(mMakefile))
            {
               var baseFile = PathManager.standardize(mMakefile);
               var parts = baseFile.split("/");
               parts[ parts.length-1 ] = outFile;
               outFile = parts.join("/");
            }
            if (outFile!="")
               addOutput(target.mToolID, outFile);

            if (output!="" && inDestination!=null)
            {
               inDestination = substitute(inDestination,false);
               if (inDestination!="")
               {
                  if (!PathManager.isAbsolute(inDestination) && sys.FileSystem.exists(mMakefile))
                  {
                     var baseFile = PathManager.standardize(mMakefile);
                     var parts = baseFile.split("/");
                     parts[ parts.length-1 ] = inDestination;
                     inDestination = parts.join("/");
                  }

                  inDestination = PathManager.clean(inDestination);
                  var fileParts = inDestination.split("/");
                  fileParts.pop();
                  PathManager.mkdir(fileParts.join("/"));

                  var chmod = isWindows ? false : target.mToolID=="exe";
                  CopyFile.copyFile(output, inDestination, false, Overwrite.ALWAYS, chmod);
               }
            }
            Profile.pop();
      }

      if (mCopyFiles.length>0)
      {
         Profile.push("copy files");
         for(copyFile in mCopyFiles)
            if (copyFile.toolId==null || copyFile.toolId==target.mToolID)
               copyFile.copy(target.mOutputDir);
         Profile.pop();
      }

      if (restoreDir!="")
         Sys.setCwd(restoreDir);
   }

   function linkNvccFiles(objDir:String, hasChanged:Bool, nvObjs:Array<String>, inGroupName:String, objExt:String)
   {
      // nvcc -arch=sm_30 -dlink test1.o test2.o -o link.o
      // Sadly, nvcc has no 'fromFile' options, so we must do it from objDir
      var objDirLen = objDir.length;
      var last = objDir.substr(objDirLen-1);
      if (last!="/" && last!="\\")
         objDirLen++;
      var outFile = "nvcc_" + inGroupName + mCompiler.mExt;
      var fullFile = objDir + "/" + outFile;

      if (hasChanged || !sys.FileSystem.exists(fullFile) )
      {
         var maxObjs = 25;
         var shortObjs = nvObjs.map( function(f) return f.substr(objDirLen) );
         if (shortObjs.length>maxObjs)
         {
            var partObjs = new Array<String>();
            var p0 = 0;
            var n =shortObjs.length;
            var groupIdx = 0;
            while(p0<n)
            {
               var subName = "nvcc_" + inGroupName + "_" + (groupIdx++) + mCompiler.mExt;
               var remain = n-p0;
               var use = remain<maxObjs ? remain : remain<maxObjs*2 ? (remain>>1) : maxObjs;
               var files = shortObjs.slice(p0,p0+use);

               var flags = getNvccLinkFlags().concat(files).concat(["-o",subName]);
               var dbgFlags = getNvccLinkFlags().concat(["[.", "x"+subName.length,".]"]).concat(["-o",subName]);
               Log.v("Linking nvcc in " + objDir + ":" + getNvcc() + dbgFlags.join(" ") );
               ProcessManager.runCommand(objDir ,getNvcc(),  flags );
               partObjs.push(subName);
               p0 += use;
            }
            shortObjs = partObjs;
         }

         var flags = getNvccLinkFlags().concat(shortObjs).concat(["-o",outFile]);
         var dbgFlags = getNvccLinkFlags().concat(["[.", "x"+shortObjs.length,".]"]).concat(["-o",outFile]);
         Log.v("Linking nvcc in " + objDir + ":" + getNvcc() + dbgFlags.join(" ") );
         ProcessManager.runCommand(objDir ,getNvcc(),  flags );
      }
      return fullFile;
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

   public function createCompiler(inXML:XmlAccess,inBase:Compiler) : Compiler
   {
      var c = inBase;
      var id = inXML.has.id ? substitute(inXML.att.id) : null;
      var exe = inXML.has.exe ? substitute(inXML.att.exe) : null;
      if (inBase==null || inXML.has.replace)
      {
         c = new Compiler(id,exe);
      }
      else
      {
         if (id!=null)
            c.mID = id;
         if (exe!=null)
            c.mExe = exe;
      }
      c.mAddGCCIdentity = mDefines.exists("USE_GCC_FILETYPES");

      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "flag" : c.addFlag(substitute(el.att.value), el.has.tag?substitute(el.att.tag):"");
               case "cflag" : c.mCFlags.push(substitute(el.att.value));
               case "cppflag" : c.mCPPFlags.push(substitute(el.att.value));
               case "objcflag" : c.mOBJCFlags.push(substitute(el.att.value));
               case "rcflag" : c.mRcFlags.push( substitute((el.att.value)) );
               case "mmflag" : c.mMMFlags.push(substitute(el.att.value));
               case "pchflag" : c.mPCHFlags.push(substitute(el.att.value));
               case "objdir" : c.mObjDir = substitute((el.att.value));
               case "outflag" : c.mOutFlag = substitute((el.att.value));
               case "exe" : c.mExe = substitute((el.att.name));
               case "rcexe" : c.mRcExe = substitute((el.att.name));
               case "rcext" : c.mRcExt = substitute((el.att.value));
               case "ext" : c.mExt = substitute((el.att.value));
               case "pch" : c.setPCH( substitute((el.att.value)) );
               case "getversion" : c.mGetCompilerVersion = substitute((el.att.value));
               case "section" : createCompiler(el,c);
               case "include" :
                  var name = substitute(el.att.name);
                  var full_name = findIncludeFile(name);
                  if (full_name!="")
                  {
                     pushFile(full_name,"compiler");
                     var make_contents = sys.io.File.getContent(full_name);
                     var xml_slow = Xml.parse(make_contents);
                     createCompiler(new XmlAccess(xml_slow.firstElement()),c);
                     popFile();
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

   public function loadNvccXml()
   {
      var incName = findIncludeFile("nvcc-setup.xml");
      if (incName=="")
         incName = findIncludeFile('$HXCPP/toolchain/nvcc-setup.xml');
      if (incName=="")
        Log.error("Could not setup nvcc - missing nvcc-setup.xml");
      else if (!mPragmaOnce.get(incName))
      {
         pushFile(incName, "Nvcc");
         var make_contents = sys.io.File.getContent(incName);
         mPragmaOnce.set(incName,true);
         var xml = Xml.parse(make_contents);
         parseXML(new XmlAccess(xml.firstElement()),"", false);
         popFile();
      }
   }

   public static function setupNvcc()
   {
      instance.loadNvccXml();
   }


   public function createFileGroup(inXML:XmlAccess,inFiles:FileGroup,inName:String, inForceRelative:Bool, inTags:String):FileGroup
   {
      var dir = inXML.has.dir ? substitute(inXML.att.dir) : ".";
      if (inForceRelative)
         dir = PathManager.combine( Path.directory(mCurrentIncludeFile), dir );

      var group:FileGroup = inFiles==null ? new FileGroup(dir,inName, inForceRelative) :
                                  inXML.has.replace ? inFiles.replace(dir, inForceRelative) :
                                  inFiles;

      if (inTags!=null)
         group.mTags = inTags;

      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "file" :
                  var name = substitute(el.att.name);
                  var file:File = group.find(name);
                  if (file==null)
                  {
                     file = new File(name,group);
                     group.addFile( file );
                  }

                  if (el.has.tag)
                  {
                     var extra = substitute(el.att.tag);
                     file.setTags(group.mTags==null || group.mTags=="" ? extra : group.mTags+","+extra);
                  }
                  if (el.has.tags)
                     file.setTags( substitute(el.att.tags) );
                  if (el.has.filterout)
                     file.mFilterOut = substitute(el.att.filterout);
                  if (el.has.embedName)
                     file.mEmbedName = substitute(el.att.embedName);
                  if (el.has.scramble)
                     file.mScramble = substitute(el.att.scramble);
                  for(f in el.elements)
                     if (valid(f,"") && f.name=="depend")
                        file.mDepends.push( substitute(f.att.name) );
               case "section" : createFileGroup(el,group,inName,inForceRelative,null);
               case "cache" :
                  group.mUseCache = parseBool( substitute(el.att.value) );
                  if (el.has.project)
                     group.mCacheProject = substitute(el.att.project);
                  if (el.has.asLibrary)
                     group.mAsLibrary = true;
                  if (el.has.respectTimestamp)
                     group.mRespectTimestamp = true;
               case "tag" :
                   group.addTag( substitute(el.att.value) );
               case "addTwice" :
                  group.mAddTwice = true;
               case "depend" :
                  if (el.has.name)
                  {
                     var dateOnly = el.has.dateOnly && parseBool( substitute(el.att.dateOnly) );
                     group.addDepend( substitute(el.att.name), dateOnly );
                  }
                  else if (el.has.files)
                  {
                     var name = substitute(el.att.files);
                     if (!mFileGroups.exists(name))
                         Log.error( "Could not find filegroup for depend node:" + name );
                     group.addDependFiles(mFileGroups.get(name));
                  }
                  else
                     Log.error("depend node must have 'name' or 'files' attribute");
               case "hlsl" :
                  group.addHLSL( substitute(el.att.name), substitute(el.att.profile),
                  substitute(el.att.variable), substitute(el.att.target)  );
               case "options" : group.addOptions( substitute(el.att.name) );
               case "config" : group.mConfig = substitute(el.att.name);
               case "compilerflag" :
                  if (el.has.name)
                     group.addCompilerFlag( substitute(el.att.name) );
                  group.addCompilerFlag( substitute(el.att.value) );
               case "nvcc" :
                  setupNvcc();
                  group.mNvcc = true;
                  if (group.mTags=="haxe,static")
                     group.mTags=="nvcc";
               case "objprefix" :
                  group.mObjPrefix = substitute(el.att.value);
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
                     if (!mPragmaOnce.get(full_name))
                     {
                        pushFile(full_name, "FileGroup");
                        var make_contents = sys.io.File.getContent(full_name);
                        var xml_slow = Xml.parse(make_contents);
                        createFileGroup(new XmlAccess(xml_slow.firstElement()), group, inName, false,null);
                        popFile();
                     }
                  }
                  else
                  {
                     Log.error("Could not find include file \"" + subbed_name + "\"");

                  }
            }
      }

      return group;
   }

   public function createLinker(inXML:XmlAccess,inBase:Linker):Linker
   {
      var exe:String = inXML.has.exe ? substitute(inXML.att.exe) : null;
      if (inBase!=null && !inXML.has.replace && inBase.mExe==null)
         inBase.mExe = exe;
      var l = (inBase!=null && !inXML.has.replace) ? inBase : new Linker(exe);
      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "flag" : l.mFlags.push(substitute(el.att.value));
               case "ext" : l.mExt = (substitute(el.att.value));
               case "outflag" : l.mOutFlag = (substitute(el.att.value));
               case "libdir" : l.mLibDir = (substitute(el.att.name));
               case "lib" :
                  if (el.has.hxbase)
                     l.mLibs.push( substitute(el.att.hxbase) + mDefines.get("LIBEXTRA") + mDefines.get("LIBEXT") );
                  else if (el.has.base)
                     l.mLibs.push( substitute(el.att.base) + mDefines.get("LIBEXT") );
                  else
                     l.mLibs.push( substitute(el.att.name) );

               case "prefix" : l.mNamePrefix = substitute(el.att.value);
               case "ranlib" : l.mRanLib = (substitute(el.att.name));
               case "libpathflag" : l.mAddLibPath = (substitute(el.att.value));
               case "recreate" : l.mRecreate = (substitute(el.att.value)) != "";
               case "expandAr" : l.mExpandArchives = substitute(el.att.value) != "";
               case "fromfile" :
                  if (el.has.value)
                     l.mFromFile = substitute(el.att.value);
                  if (el.has.needsQuotes)
                     l.mFromFileNeedsQuotes = parseBool(substitute(el.att.needsQuotes));
               case "exe" : l.mExe = (substitute(el.att.name));
               case "section" : createLinker(el,l);
            }
      }

      return l;
   }

   public function createPrelinker(inXML:XmlAccess,inBase:Prelinker):Prelinker
   {
      var l = (inBase!=null && !inXML.has.replace) ? inBase : new Prelinker(substitute(inXML.att.exe));
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

   public function createManifester(inXML:XmlAccess,inBase:Manifester):Manifester
   {
      var s = (inBase!=null && !inXML.has.replace) ? inBase :
                 new Manifester(substitute(inXML.att.exe));
      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
                case "flag" : s.mFlags.push(substitute(el.att.value));
                case "outPre" : s.mOutPre = substitute(el.att.value);
                case "outPost" : s.mOutPost = substitute(el.att.value);
                case "exe" : s.mExe = substitute((el.att.name));
            }
      }

      return s;
   }


   public function createStripper(inXML:XmlAccess,inBase:Stripper):Stripper
   {
      var s = (inBase!=null && !inXML.has.replace) ? inBase :
                 new Stripper(substitute(inXML.att.exe));
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

   public function createTarget(inXML:XmlAccess,?inTarget:Target, inForceRelative) : Target
   {
      var target:Target = inTarget;
      var output = inXML.has.output ? substitute(inXML.att.output) : "";
      var tool = inXML.has.tool ? substitute(inXML.att.tool) : "";
      var toolid = inXML.has.toolid ? substitute(inXML.att.toolid) : "";

      if (target==null)
      {
         target = new Target(output,tool,toolid);
         if (inForceRelative)
            target.mBuildDir = Path.directory(mCurrentIncludeFile);
      }
      else
      {
         if (output!="")
            target.mOutput = output;
         if (tool!="")
            target.mTool = tool;
         if (toolid!="")
            target.mToolID = toolid;
      }

      for(el in inXML.elements)
      {
         if (valid(el,""))
            switch(el.name)
            {
               case "target" : target.mSubTargets.push( substitute(el.att.id) );
               case "merge" :
                  var name = substitute(el.att.id);
                  if (!mTargets.exists(name))
                     Log.error("Could not find target " + name + " to merge.");
                  target.merge( mTargets.get(name) );

               case "lib" :
                  if (el.has.hxbase)
                     target.mLibs.push( substitute(el.att.hxbase) + mDefines.get("LIBEXTRA") + mDefines.get("LIBEXT") );
                  else if (el.has.base)
                     target.mLibs.push( substitute(el.att.base) + mDefines.get("LIBEXT") );
                  else
                  {
                      var lib = substitute(el.att.name);
                      var found = false;
                      for(magicLib in mMagicLibs)
                      {
                         if (lib.endsWith(magicLib.name))
                         {
                            var replace = lib.substr(0, lib.length-magicLib.name.length) +
                                              magicLib.replace;
                            Log.v('Using $replace instead of $lib');
                            found = true;
                            include(replace, "", false, true );
                            break;
                         }
                      }
                      if (!found)
                         target.mLibs.push(lib);
                  }

               case "flag" : target.mFlags.push( substitute(el.att.value) );
               case "depend" : target.mDepends.push( substitute(el.att.name) );
               case "vflag" :
                  target.mFlags.push( substitute(el.att.name) );
                  target.mFlags.push( substitute(el.att.value) );
               case "dir" : target.mDirs.push( substitute(el.att.name) );
               case "outdir" : target.mOutputDir = substitute(el.att.name)+"/";
               case "ext" : target.setExt( (substitute(el.att.value)) );
               case "builddir" : target.mBuildDir = substitute(el.att.name);
               case "libpath" : target.mLibPaths.push( substitute(el.att.name) );
               case "fullouput" : target.mFullOutputName = substitute(el.att.name);
               case "fullunstripped" : target.mFullUnstrippedName = substitute(el.att.name);
               case "files" :
                  var id = el.att.id;
                  if (!mFileGroups.exists(id))
                     target.addError( "Could not find filegroup " + id );
                  else
                     target.addFiles( mFileGroups.get(id), el.has.asLibrary );
               case "section" : createTarget(el,target,false);
            }
      }

      return target;
   }

   public function defined(inString:String):Bool
   {
      if (inString=="this_dir")
         return true;
      return mDefines.exists(inString);
   }

   public function parseBool(inValue:String):Bool
   {
      return inValue=="1" || inValue=="t" || inValue=="true";
   }

   function findLocalIncludeFile(inBase:String):String
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
               {
                  return name;
               }
            }
            return "";
         }
      }
      if (FileSystem.exists(inBase))
         return inBase;
      return "";
   }

   function findIncludeFile(inBase:String):String
   {
      var result = findLocalIncludeFile(inBase);
      if (result!="" && !Path.isAbsolute(result))
         result =  Path.normalize( PathManager.combine( mCurrentIncludeFile=="" ? Sys.getCwd() : Path.directory(mCurrentIncludeFile), result ) );
      return result;
   }

   private static function getArch():String
   {
      if (isWindows)
      {
         if (isWindowsArm)
            return "arm64";
         var architecture = Sys.getEnv ("PROCESSOR_ARCHITEW6432");
         if (architecture != null && architecture.indexOf ("64") > -1)
         {
            return "m64";
         }
         else
         {
            return "m32";
         }
      }
      else
      {
         var process = new Process("uname", [ "-m" ]);
         var output = process.stdout.readAll().toString();
         var error = process.stderr.readAll().toString();
         process.exitCode();
         process.close();

         if ( (output.indexOf("aarch64") > -1) ||  (output.indexOf("arm64") > -1) )
         {
            return "arm64";
         }
         else if (output.indexOf("64") > -1)
         {
            return "m64";
         }
         else
         {
            return "m32";
         }
      }
   }

   static public function getMsvcVer()
   {
      return instance.mDefines.get("MSVC_VER");
   }

   static public function keepTemp()
   {
      return instance.mDefines.exists("HXCPP_KEEP_TEMP");
   }


   // Setting HXCPP_COMPILE_THREADS to 2x number or cores can help with hyperthreading
   public static function getNumberOfProcesses():Int
   {
      var cache = Log.verbose;
      Log.verbose = false;

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
         result = ProcessManager.runProcessLine("", "nproc", [], true, false, true, true);
         if (result == null)
         {
            var cpuinfo = ProcessManager.runProcess("", "cat", [ "/proc/cpuinfo" ], true, false, true, true);
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
         var output = ProcessManager.runProcess("", "/usr/sbin/system_profiler", [ "-detailLevel", "full", "SPHardwareDataType" ], true, false, true, true);
         if (output != null && cores.match(output))
         {
            result = cores.matched(1);
         }
      }

      Log.verbose = cache;

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

   public static function isMingw()
   {
      return instance.mDefines.get("toolchain")=="mingw";
   }

   public static function getNvcc()
   {
      return instance.mDefines.get("NVCC");
   }

   public static function getNvccLinkFlags() : Array<String>
   {
      return instance.mNvccLinkFlags;
   }

   public static function getNvccFlags() : Array<String>
   {
      return instance.mNvccFlags;
   }

   public static function copy(from:String, to:String)
   {
      Log.v('copy $from $to');

      try {
         if (FileSystem.isDirectory(to))
            to += "/" + Path.withoutDirectory(from);
         var bytes = sys.io.File.getBytes(from);
         sys.io.File.saveBytes(to,bytes);
      } catch(e:Dynamic)
      {
         Log.error('Could not copy file $from $to');
      }
   }


   static public function main()
   {
     try {
       runMain();
     }
     catch(e:Dynamic) {
       Log.error('Uncaught exception on main thread: $e\n${haxe.CallStack.toString(haxe.CallStack.exceptionStack())}');
       Tools.exit(1);
     }
     Tools.exit(0);
   }

   // Process args and environment.
   static function runMain()
   {
      var targets = new Array<String>();
      var defines = new Hash<String>();
      var include_path = new Array<String>();
      var makefile:String="";
      var optionsTxt = "Options.txt";

      Profile.start();

      include_path.push(".");

      var args = Sys.args();
      var env = Sys.environment();

      for(e in env.keys())
         defines.set(e, Sys.getEnv(e) );


      // Check for calling from haxelib ...
      if (args.length>0)
      {
         var last:String = (new Path(args[args.length-1])).toString();
         var isRootDir = last=="/";
         if (!isRootDir)
         {
            var slash = last.substr(-1);
            if (slash=="/"|| slash=="\\")
               last = last.substr(0,last.length-1);
         }
         if (isRootDir || (FileSystem.exists(last) && FileSystem.isDirectory(last)))
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

      startDir = Sys.getCwd();

      isWindows = (new EReg("window","i")).match(os);
      if (isWindows)
      {
         var proc = Sys.getEnv("PROCESSOR_IDENTIFIER");
         isWindowsArm = proc!=null && (new EReg("\\barm","i")).match(proc);
         if (isWindowsArm)
            defines.set("windows_arm_host", "1");
      }
      isMac = (new EReg("mac","i")).match(os);
      if (isMac)
         defines.set("mac_host", "1");
      isLinux = (new EReg("linux","i")).match(os);
      if (isLinux)
         defines.set("linux_host", "1");


      if (args.length>0 && args[0].endsWith(".cppia"))
      {
         var binDir = isWindows ? "Windows" : isMac ? "Mac64" : isLinux ? "Linux64" : null;
         if (binDir==null)
            Log.error("Cppia is not supported on this host.");
         var arch = getArch();
         var binDir = isWindows ? (isWindowsArm ? "WindowsArm64" : "Windows64" ) :
                       isMac ? "Mac64" :
                       isLinux ? ("Linux64") :
                       null;
         var exe = '$HXCPP/bin/$binDir/Cppia' + (isWindows ? ".exe" : "");
         if (!isWindows)
         {
            var phase = "find";
            try
            {
               var stat = FileSystem.stat(exe);
               if (stat==null)
                  throw "Could not find exe:" + exe;
               var mode = stat.mode;
               var exeFlags = (1<<0) | (1<<3) | (1<<6);
               if ( (mode&exeFlags) != exeFlags )
               {
                  var phase = "add exe permissions to";
                  if (Sys.command( "chmod", ["755", exe])!=0)
                     Log.error('Please use root access to add execute permissions to $exe');
               }
            }
            catch(e:Dynamic)
            {
               Log.error('Could not $phase Cppia host $exe ($e)');
            }
         }

         #if (haxe_ver < 3.3)
         // avoid issue of path with spaces
         // https://github.com/HaxeFoundation/haxe/issues/3603
         if (isWindows)
            exe = '"$exe"';
         #end

         Tools.exit( Sys.command( exe, args ) );
      }
      else if (args.length>0 && args[0].endsWith(".js"))
      {
         Setup.initHXCPPConfig(defines);
         Setup.setupEmscripten(defines);
         var node = defines.get("EMSCRIPTEN_NODE_JS");
         Log.v( node==null ? "EMSCRIPTEN_NODE_JS undefined, using 'node'" : 'Using $node from EMSCRIPTEN_NODE_JS');
         if (node=="" || node==null)
            node = "node";

         Log.v(  node + " " + args.join(" ") );
         Tools.exit( Sys.command( node, args ) );
      }
      else if (args.length==1 && args[0]=="defines")
      {
         var dir = '$HXCPP/tools/hxcpp';
         try
         {
            var defineMatch = ~/m*defines\.\w+\("(\w+)"/i;
            var allDefines = new Map<String,Bool>();
            for(file in FileSystem.readDirectory(dir))
               if (file.endsWith(".hx"))
                  for(line in sys.io.File.getContent(file).split("\n"))
                     if (defineMatch.match(line))
                        allDefines.set(defineMatch.matched(1),true);
            for(key in allDefines.keys())
               Sys.println(key);
         }
         catch(e:Dynamic)
         {
            Log.error('Could not read $dir : $e');
         }
         return;
      }


      isRPi = isLinux && Setup.isRaspberryPi();

      is64 = getArch()!="m32";
      var dirtyList = new Array<String>();

      var a = 0;
      while(a < args.length)
      {
         var arg = args[a];
         if (arg.substr(0,2)=="-D" || (~/^[a-zA-Z0-9_][a-zA-Z0-9_-]*=/).match(arg) )
         {
            var define = arg.substr(0,2)=="-D" ? arg.substr(2) : arg;
            var equals = define.indexOf("=");
            if (equals>0)
            {
               var value = define.substr(equals+1);
               define = define.substr(0,equals);
               if (define=="destination")
               {
                  destination = value;
               }
               else
                  defines.set(define,value);
            }
            else
               defines.set(define,"");
            if (define=="verbose")
               Log.verbose = true;
         }
         else if (arg=="-debug")
               defines.set("debug","1");
         else if (arg=="-no-options")
            optionsTxt = "";
         else if (arg=="-options")
         {
            a++;
            optionsTxt = args[a];
            if (optionsTxt==null)
               optionsTxt = "";
         }
         else if (arg=="-dirty")
         {
            a++;
            dirtyList.push(args[a]);
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

         a++;
      }

      if (defines.exists("HXCPP_TIMES"))
         Profile.enable();

      if (defines.exists("HXCPP_NO_COLOUR") || defines.exists("HXCPP_NO_COLOR"))
         Log.colorSupported = false;
      Log.verbose = Log.verbose || defines.exists("HXCPP_VERBOSE");
      Log.quiet = defines.exists("HXCPP_QUIET") && !Log.verbose;
      Log.mute = defines.exists("HXCPP_SILENT") && !Log.quiet && !Log.verbose;

      if ( optionsTxt!="" && makefile!="")
      {
         var path = PathManager.combine(haxe.io.Path.directory(makefile), optionsTxt);
         if (FileSystem.exists(path))
            try
            {
               var contents = sys.io.File.getContent(path);
               if (contents.substr(0,1)!=" ") // Is it New-style?
                  for(def in contents.split("\r").join("").split("\n"))
                  {
                     var equals = def.indexOf("=");
                     if (equals>0)
                     {
                        var name = def.substr(0,equals);
                        var value = def.substr(equals+1);
                        if (name=="hxcpp")
                        {
                           // Ignore
                        }
                        else if (name=="destination")
                            destination = value;
                        else
                           defines.set(name,value);
                     }
                  }
           }
           catch(e:Dynamic)
           {
              Log.error('Could not parse options file $path ($e)');
           }
      }

      Profile.setEntry("setup");
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

      if (defines.exists("tvos"))
      {
         if (defines.exists("simulator"))
            defines.set("appletvsim", "appletvsim");
         else if (!defines.exists ("appletvsim"))
            defines.set("appletvos", "appletvos");
         defines.set("appletv", "appletv");
      }



      if (makefile=="" || Log.verbose)
      {
         printBanner();
      }

      if (makefile=="")
      {
         printUsage();
      }
      else
      {
         Log.v('${BOLD}${YELLOW}Using makefile: $makefile${NORMAL}');
         Log.v('${BOLD}${YELLOW}Reading HXCPP config: ' + defines.get("HXCPP_CONFIG") + NORMAL);
         if (defines.exists("toolchain"))
            Log.v('${BOLD}{$YELLOW}Using target toolchain: ' + defines.get("toolchain") + NORMAL);
         else
            Log.v('${BOLD}${YELLOW}No specified toolchain${NORMAL}');
         if (Log.verbose) Log.println("");


         if (targets.length==0)
            targets.push("default");


         new BuildTool(makefile,defines,targets,include_path,dirtyList);
      }
   }


   static function printUsage()
   {
      Log.println('${YELLOW}Usage:${NORMAL}');
      Log.println(' ${BOLD}haxelib run hxcpp${NORMAL} file.xml ${ITALIC}${WHITE}[options]${NORMAL}');
      Log.println('   Build project from "file.xml".  options:');
      Log.println('    ${BOLD}-D${NORMAL}${ITALIC}value${NORMAL} -- Specify a define to use when processing other commands');
      Log.println('    ${BOLD}-verbose${NORMAL} -- Print additional information (when available)');
      Log.println('    ${BOLD}-dirty [groudId|all]${NORMAL} -- always rebuild files in given group');
      Log.println(' ${BOLD}haxelib run hxcpp${NORMAL} ${ITALIC}${WHITE}file.cppia${NORMAL}');
      Log.println('   Run cppia script using default Cppia host');
      Log.println(' ${BOLD}haxelib run hxcpp${NORMAL} ${ITALIC}${WHITE}file.js${NORMAL}');
      Log.println('    Run emscripten compiled scipt "file.js"');
      Log.println(' ${BOLD}haxelib run hxcpp${NORMAL} ${ITALIC}${WHITE}cache [command] [project]${NORMAL}');
      Log.println('   Perform command on cache, either on specific project or all. commands:');
      Log.println('    ${BOLD}clear${NORMAL} -- remove all files from cache');
      Log.println('    ${BOLD}days${NORMAL} #days -- remove files older than "days"');
      Log.println('    ${BOLD}resize${NORMAL} #megabytes -- Only keep #megabytes MB');
      Log.println('    ${BOLD}list${NORMAL} -- list cache usage');
      Log.println('    ${BOLD}details${NORMAL} -- list cache usage, per file');
      Log.println('');
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
      if ( (new EReg("window","i")).match(os) )
         defines.set("windows_host","1");

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
      else if (defines.exists("appletvos"))
      {
         defines.set("toolchain","appletvos");
         defines.set("appletv","appletv");
         defines.set("apple","apple");
         defines.set("BINDIR","AppleTV");
      }
      else if (defines.exists("appletvsim"))
      {
         defines.set("toolchain","appletvsim");
         defines.set("appletv","appletv");
         defines.set("apple","apple");
         defines.set("BINDIR","AppleTV");
      }
      else if (defines.exists("watchos"))
      {
         defines.set("toolchain","watchos");
         defines.set("apple","apple");
         defines.set("applewatch","applewatch");
         defines.set("BINDIR","watchos");
      }
      else if (defines.exists("watchsimulator"))
      {
         defines.set("toolchain","watchsimulator");
         defines.set("applewatch","applewatch");
         defines.set("apple","apple");
         defines.set("BINDIR","watchsimulator");
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
            {
               defines.set("windows_host","1");
               defines.set("ANDROID_HOST","windows");
            }
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
         defines.set("toolchain","tizen");
         defines.set("tizen","tizen");
         defines.set("BINDIR","Tizen");
      }
      else if (defines.exists("blackberry"))
      {
         defines.set("toolchain", "blackberry");
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
      }
      else if (defines.exists ("gcw0"))
      {
         defines.set ("toolchain", "gcw0");
         defines.set ("gcw0", "gcw0");
         defines.set ("BINDIR", "GCW0");
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
            set64(defines,m64,arm64);
            defines.set("windows","windows");
            defines.set("BINDIR",arm64 ? "WindowsArm64" : m64 ? "Windows64":"Windows");

            // Choose between MSVC and MINGW
            var useMsvc = true;

            if (defines.exists("mingw") || defines.exists("HXCPP_MINGW") || defines.exists("minimingw"))
               useMsvc = false;
            else if ( defines.exists("winrt") || defines.exists("HXCPP_MSVC_VER"))
               useMsvc = true;
            else
            {
                for(i in 8...24)
                {
                   if (Sys.getEnv("VS" + i + "0COMNTOOLS")!=null)
                   {
                      useMsvc = true;
                      break;
                   }
                }

                Log.v("Using Windows compiler: " + (useMsvc ? "MSVC" : "MinGW") );
            }

            if (useMsvc)
            {
               defines.set("toolchain","msvc");
               if ( defines.exists("winrt") )
                  defines.set("BINDIR",m64 ? "WinRT64":"WinRT");
            }
            else
            {
               defines.set("toolchain","mingw");
               defines.set("mingw","mingw");
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
         set64(defines,m64,arm64);
         // Cross-compile?
         if(defines.exists("windows"))
         {
            defines.set("toolchain","mingw");
            defines.set("xcompile","1");
            defines.set("BINDIR", arm64 ? "WindowsArm64" : m64 ? "Windows64":"Windows");
         }
         else
         {
            defines.set("toolchain","linux");
            defines.set("linux","linux");

            if (defines.exists("HXCPP_LINUX_ARMV7"))
            {
               defines.set("noM32","1");
               defines.set("noM64","1");
               defines.set("HXCPP_ARMV7","1");
               m64 = false;
            }
            else if (arm64 || defines.exists("HXCPP_LINUX_ARM64"))
            {
               defines.set("noM32","1");
               defines.set("noM64","1");
               defines.set("HXCPP_ARM64","1");
               m64 = true;
            }
            defines.set("BINDIR", m64 ? "Linux64":"Linux");
         }
      }
      else if ( (new EReg("mac","i")).match(os) )
      {
         set64(defines,m64, arm64);
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
            defines.set("BINDIR", arm64 ? "MacArm64" : m64 ? "Mac64":"Mac");
         }
      }
   }



   function setupAppleDirectories(defines:Hash<String>)
   {
      if (defines.exists("HXCPP_CLEAN_ONLY"))
         return;

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
            var best="0.0";
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
            if (best!="0.0")
               defines.set("IPHONE_VER",best);
         }
      }

      if (defines.exists("appletv") && !defines.exists("TVOS_VER"))
      {
         var dev_path = defines.get("DEVELOPER_DIR") + "/Platforms/AppleTVOS.platform/Developer/SDKs/";
         if (FileSystem.exists(dev_path))
         {
            var best="0.0";
            var files = FileSystem.readDirectory(dev_path);
            var extract_version = ~/^AppleTVOS(.*).sdk$/;
            for(file in files)
            {
               if (extract_version.match(file))
               {
                  var ver = extract_version.matched(1);
                  if (Std.parseFloat (ver)>Std.parseFloat (best))
                     best = ver;
               }
            }
            if (best!="0.0")
               defines.set("TVOS_VER",best);
         }
      }


      if (defines.exists("applewatch") && !defines.exists("WATCHOS_VER"))
      {
         var dev_path = defines.get("DEVELOPER_DIR") + "/Platforms/WatchOS.platform/Developer/SDKs/";
         if (FileSystem.exists(dev_path))
         {
            var best="0.0";
            var files = FileSystem.readDirectory(dev_path);
            var extract_version = ~/^WatchOS(.*).sdk$/;
            for(file in files)
            {
               if (extract_version.match(file))
               {
                  var ver = extract_version.matched(1);
                  if (Std.parseFloat (ver)>Std.parseFloat (best))
                     best = ver;
               }
            }
            if (best!="0.0")
               defines.set("WATCHOS_VER",best);
         }
      }


      if (defines.exists("macos") && !defines.exists("MACOSX_VER"))
      {
         var dev_path = defines.get("DEVELOPER_DIR") + "/Platforms/MacOSX.platform/Developer/SDKs/";
         if (FileSystem.exists(dev_path))
         {
            var best="0.0";
            var files = FileSystem.readDirectory(dev_path);
            var extract_version = ~/^MacOSX(.*).sdk$/;
            for(file in files)
            {
               if (extract_version.match(file))
               {
                  var ver = extract_version.matched(1);
                  var split_best = best.split(".");
                  var split_ver = ver.split(".");
                  if (Std.parseFloat(split_ver[0]) > Std.parseFloat(split_best[0]) || Std.parseFloat(split_ver[1]) > Std.parseFloat(split_best[1]))
                     best = ver;
               }
            }
            if (best!="0.0")
               defines.set("MACOSX_VER",best);
            else
               Log.v("Could not find MACOSX_VER!");
         }
      }

      if (!FileSystem.exists(defines.get("DEVELOPER_DIR") + "/Platforms/MacOSX.platform/Developer/SDKs/"))
      {
         defines.set("LEGACY_MACOSX_SDK","1");
      }
   }

   function parseXML(inXML:XmlAccess,inSection:String, forceRelative:Bool)
   {
      for(el in inXML.elements)
      {
         if (valid(el,inSection))
         {
            switch(el.name)
            {
               case "set" :
                  var name = substitute(el.att.name);
                  var value = substitute(el.att.value);
                  mDefines.set(name,value);
               case "unset" :
                  var name = substitute(el.att.name);
                  mDefines.remove(name);
               case "setup" :
                  var name = substitute(el.att.name);
                  Setup.setup(name,mDefines);
               case "echo" :
                  Log.info(substitute(el.att.value));
               case "setenv" :
                  var name = substitute(el.att.name);
                  var value = substitute(el.att.value);
                  mDefines.set(name,value);
                  Sys.putEnv(name,value);
               case "error" :
                  var error = substitute(el.att.value);
                  Log.error(error);
               case "path" :
                  var path = substitute(el.att.name);
                  Log.info("", " - \x1b[1mAdding path:\x1b[0m " + path);
                  var sep = mDefines.exists("windows_host") ? ";" : ":";
                  var add = path + sep + Sys.getEnv("PATH");
                  Sys.putEnv("PATH", add);
                  //trace(Sys.getEnv("PATH"));
               case "compiler" :
                  mCompiler = createCompiler(el,mCompiler);

               case "stripper" :
                  mStripper = createStripper(el,mStripper);
               case "manifest" :
                  mManifester = createManifester(el,mManifester);
               case "prelinker" :
                  var name = substitute(el.att.id);
                  if (mPrelinkers.exists(name))
                     createPrelinker(el,mPrelinkers.get(name));
                  else
                     mPrelinkers.set(name, createPrelinker(el,null) );
               case "linker" :
                  var name = substitute(el.att.id);
                  if (mLinkers.exists(name))
                     createLinker(el,mLinkers.get(name));
                  else
                     mLinkers.set(name, createLinker(el,null) );
               case "files" :
                  var name = substitute(el.att.id);
                  var tags = el.has.tags ? substitute(el.att.tags) : null;
                  if (mFileGroups.exists(name) )
                     createFileGroup(el, mFileGroups.get(name), name, false, tags);
                  else
                     mFileGroups.set(name,createFileGroup(el,null,name, forceRelative,tags));
               case "include", "import" :
                  var name = substitute(el.att.name);
                  var section = el.has.section ? substitute(el.att.section) : "";
                  include(name, section, el.has.noerror, el.name=="import" );
               case "target" :
                  var name = substitute(el.att.id);
                  var overwrite = name=="default";
                  if (el.has.overwrite)
                     overwrite = true;
                  if (el.has.append)
                     overwrite = false;
                  if (mTargets.exists(name) && !overwrite)
                     createTarget(el,mTargets.get(name), forceRelative);
                  else
                     mTargets.set( name, createTarget(el,null, forceRelative) );

               case "mkdir" :
                  var name = substitute(el.att.name);
                  try {
                     PathManager.mkdir(name);
                  } catch(e:Dynamic)
                  {
                     Log.error("Could not create directory " + name );
                  }


               case "copy" :
                   var from = substitute(el.att.from);
                   from = PathManager.combine( Path.directory(mCurrentIncludeFile), from );
                   var to = substitute(el.att.to);
                   to = PathManager.combine( Path.directory(mCurrentIncludeFile), to );
                   copy(from,to);

               case "copyFile" :
                  mCopyFiles.push(
                      new CopyFile(substitute(el.att.name),
                                   substitute(el.att.from),
                                   el.has.allowMissing ?  subBool(el.att.allowMissing) : false,
                                   el.has.overwrite ? substitute(el.att.overwrite) : Overwrite.ALWAYS,
                                   el.has.toolId ?  substitute(el.att.toolId) : null ) );
               case "section" :
                  parseXML(el,"",forceRelative);

               case "pleaseUpdateHxcppTool" :
                  checkToolVersion( substitute(el.att.version) );

               case "magiclib" :
                  mMagicLibs.push( {name: substitute(el.att.name),
                                    replace:substitute(el.att.replace) } );
               case "nvccflag" :
                  if (el.has.name)
                     mNvccFlags.push( substitute(el.att.name) );
                  mNvccFlags.push( substitute(el.att.value) );

               case "nvcclinkflag" :
                  if (el.has.name)
                     mNvccLinkFlags.push( substitute(el.att.name) );
                  mNvccLinkFlags.push( substitute(el.att.value) );

               case "pragma" :
                  if (el.has.once)
                     mPragmaOnce.set(mCurrentIncludeFile, parseBool(substitute(el.att.once)));
            }
         }
      }
   }

   public function checkToolVersion(inVersion:String)
   {
      var ver = Std.parseInt(inVersion);
      if (ver>3)
         Log.error("Your version of hxcpp.n is out-of-date.  Please update.");
   }

   public function resolvePath(inPath:String)
   {
      return PathManager.combine( mCurrentIncludeFile=="" ? Sys.getCwd() : Path.directory(mCurrentIncludeFile),
           inPath);
   }

   public function include(inName:String, inSection:String="", inAllowMissing:Bool = false, forceRelative=false)
   {
      var full_name = findIncludeFile(inName);
      if (full_name!="")
      {
         if (mPragmaOnce.get(full_name))
            return;

         pushFile(full_name, "include", inSection);
         // TODO - use mFileStack?
         var oldInclude = mCurrentIncludeFile;
         mCurrentIncludeFile = full_name;

         var make_contents = sys.io.File.getContent(full_name);
         var xml_slow = Xml.parse(make_contents);

         Profile.push( haxe.io.Path.withoutDirectory(inName) );
         parseXML(new XmlAccess(xml_slow.firstElement()),inSection, forceRelative);
         Profile.pop();

         mCurrentIncludeFile = oldInclude;
         popFile();
      }
      else if (!inAllowMissing)
      {
         Log.error("Could not find include file \"" + inName + "\"");
         //throw "Could not find include file " + name;
      }
   }



   static function set64(outDefines:Hash<String>, in64:Bool,isArm64=false)
   {
      if (isArm64)
      {
         outDefines.set("HXCPP_ARM64","1");
         outDefines.set("HXCPP_M64","1");
         outDefines.remove("HXCPP_32");
      }
      else if (in64)
      {
         outDefines.set("HXCPP_M64","1");
         outDefines.remove("HXCPP_32");
         outDefines.remove("HXCPP_ARM64");
      }
      else
      {
         outDefines.set("HXCPP_M32","1");
         outDefines.remove("HXCPP_M64");
         outDefines.remove("HXCPP_ARM64");
      }
   }

   public function dospath(path:String) : String
   {
      if (mDefines.exists("windows_host"))
      {
         path = path.split("\\").join("/");
         var filename = "";
         var parts = path.split("/");
         if (!FileSystem.isDirectory(path))
            filename = parts.pop();

         var oldDir = Sys.getCwd();
         var output = "";
         var err = "";
         Sys.setCwd(parts.join("\\"));
         try {
            var bat = '$HXCPP/toolchain/dospath.bat'.split("/").join("\\");
            var process = new Process("cmd",["/c",bat]);
            output = process.stdout.readAll().toString();
            output = output.split("\r")[0].split("\n")[0];
            err  = process.stderr.readAll().toString();
            process.close();
         } catch (e:Dynamic) { Log.error(e); }
         Sys.setCwd(oldDir);

         if (output=="")
            Log.error("Could not find dos path for " + path + " " + err);
         return output + "\\" + filename;
      }

      return path;
   }

   public function substitute(str:String,needDollar=true):String
   {
      var match = needDollar ? mVarMatch : mNoDollarMatch;
      while( match.match(str) )
      {
         var sub = match.matched(1);
         if (sub.startsWith("haxelib:"))
         {
            sub = PathManager.getHaxelib(sub.substr(8));
            sub = PathManager.standardize(sub);
         }
         else if (sub.startsWith("removeQuotes:"))
         {
            sub = mDefines.get(sub.substr(13));
            var len = sub.length;
            if (len>1 && sub.substr(0,1)=="\"" && sub.substr(len-1)=="\"")
               sub = sub.substr(1,len-2);
         }
         else if (sub.startsWith("dospath:") )
         {
            sub = dospath( mDefines.get(sub.substr(8)) );
         }
         else if (sub.startsWith("dir:") )
         {
            sub = dospath( mDefines.get(sub.substr(4)) );
            if (!FileSystem.isDirectory(sub))
            {
               sub = haxe.io.Path.directory(sub);
            }
         }
         else if (sub=="this_dir")
         {
            sub = Path.normalize(mCurrentIncludeFile=="" ? Sys.getCwd() :  Path.directory(mCurrentIncludeFile));
         }
         else
            sub = mDefines.get(sub);

         if (sub==null) sub="";
         str = match.matchedLeft() + sub + match.matchedRight();
      }

      return str;
   }

   public function subBool(str:String):Bool
   {
      var result = substitute(str);
      return result=="t" || result=="true" || result=="1";
   }

   public function valid(inEl:XmlAccess,inSection:String):Bool
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

      if (inEl.has.unlessApi)
      {
         var value = substitute(inEl.att.unlessApi);
         try {
            var val = Std.parseInt(value);
            if (val<=SupportedVersion)
               return false;
         } catch(e:Dynamic) { }
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
