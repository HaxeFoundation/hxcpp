import haxe.io.Path;
import haxe.crypto.Md5;
import Lambda;

using StringTools;

class GenMsvc
{
   var mDefines:Map<String, String>;
   var mTargets:Map<String, Target>;
   var mCompiler:Compiler;
   var platform:String;
   var defaultCxxStandard:Null<Int>;

   public function new(defines:Map<String, String>, targets:Map<String, Target>, compiler:Compiler, inPlatform:String, inDefaultCxxStandard:Null<Int>)
   {
      mDefines = defines;
      mTargets = targets;
      mCompiler = compiler;
      platform = inPlatform;
      defaultCxxStandard = inDefaultCxxStandard;
   }

   public function generate(inTarget:String)
   {
      if (!BuildTool.isMsvc())
         Log.error("Cannot generate Visual Studio project: MSVC toolchain is not being used. Use -Dtoolchain=msvc or build on Windows.");

      if (!mTargets.exists(inTarget))
         Log.error("Could not find build target \"" + inTarget + "\"");

      if (mCompiler == null)
         Log.error("No compiler defined for the current build target");

      var target:Target = mTargets.get(inTarget);
      target.checkError();

      var mainProjName = target.mOutput;
      if (mainProjName=="")
         Log.error('Target "' + inTarget + '" - has no output defined.');
      Log.v('Generating Visual Studio project for target "' + inTarget + '" with output "' + mainProjName + '"');

      var msvcVer = mDefines.get("MSVC_VER");

      // MSVC_VER is the cl.exe major version (e.g., 19 for VS 2015-2022)
      // Map to platform toolset and VS version
      var platformToolset = "v143"; // Default to VS 2022
      var vsVersion = "17";

      if (msvcVer != null)
      {
         var ver = Std.parseInt(msvcVer);
         if (ver != null)
         {
            if (ver >= 19)
            {
               // cl.exe 19.x spans VS 2015-2022; default to latest (VS 2022)
               platformToolset = "v143";
               vsVersion = "17";
            }
            else if (ver >= 18)
            {
               // cl.exe 18.x = VS 2013
               platformToolset = "v120";
               vsVersion = "12";
            }
            else if (ver >= 17)
            {
               // cl.exe 17.x = VS 2012
               platformToolset = "v110";
               vsVersion = "11";
            }
            else
            {
               // Older versions
               platformToolset = "v100";
               vsVersion = "10";
            }
         }
      }

      var configuration = mDefines.exists("debug") ? "Debug" : "Release";

      // Use the main target's output directory as base for all project files
      var outputDir = target.mOutputDir;
      if (outputDir == "") outputDir = ".";
      outputDir = Path.normalize(sys.FileSystem.fullPath(outputDir));

      var projectFiles = new Array<{name:String, path:String, guid:String}>();
      function findGroup(inId:String)
      {
        for (group in target.mFileGroups)
          if (group.mId == inId)
            return group;
        return null;
      }
      var mainGroup:FileGroup = findGroup("__main__");
      if (mainGroup == null)
         mainGroup = findGroup("__msvc_main__");
      if (mainGroup == null)
         mainGroup = findGroup(mainProjName);
      Log.v('Using FileGroup "' + mainGroup.mId + '" for main project source files');
   
      // Generate a vcxproj for each FileGroup (as static library)
      for (group in target.mFileGroups)
      {
         // Skip FileGroups with no files
         if ( Lambda.empty(group.mFiles) )
            continue;

         // __main__ group goes directly into main project
         if (group == mainGroup)
            continue;

         var projName = group.mId;
         var projPath = outputDir + "/" + projName + ".vcxproj";
         var guid = generateGuid(projName + "_vcxproj");

         var info = generateFileGroupVcxproj(projName, group, guid, configuration, platformToolset, outputDir);
         var vcxproj = info.xml;
         var groupProjectFiles = info.files;

         writeFileIfChanged(projPath, vcxproj);

         var filtersPath = projPath + ".filters";
         var filters = generateFilters(groupProjectFiles);
         writeFileIfChanged(filtersPath, filters);

         projectFiles.push({name: projName, path: projName + ".vcxproj", guid: guid});
      }

      // Generate main target vcxproj that links everything (and includes __main__ files directly)
      var mainProjPath = outputDir + "/" + mainProjName + ".vcxproj";
      var mainGuid = generateGuid(mainProjName + "_vcxproj");

      var mainInfo = generateMainVcxproj(mainProjName, target, mainGuid, configuration, platformToolset, outputDir, projectFiles, mainGroup);
      var mainVcxproj = mainInfo.xml;
      var mainProjectFiles = mainInfo.files;
      writeFileIfChanged(mainProjPath, mainVcxproj);

      var mainFiltersPath = mainProjPath + ".filters";
      var mainFilters = generateFilters(mainProjectFiles);
      writeFileIfChanged(mainFiltersPath, mainFilters);

      // Insert main project first so it becomes the startup project in the solution
      projectFiles.insert(0, {name: mainProjName, path: mainProjName + ".vcxproj", guid: mainGuid});

      // Generate .sln file named after the target's output
      var slnPath = outputDir + "/" + target.mOutput + ".sln";
      var sln = generateSln(target.mOutput, projectFiles, configuration, vsVersion);
      writeFileIfChanged(slnPath, sln);
      Log.info('Generated Visual Studio solution "' + slnPath + '"');
   }

   function generateFileGroupVcxproj(name:String, group:FileGroup, guid:String, configuration:String, platformToolset:String, outputDir:String):{xml:String, files:Array<String>}
   {
      var xml = new StringBuf();
      xml.add('<?xml version="1.0" encoding="utf-8"?>\r\n');
      xml.add('<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\r\n');

      // Project Configurations
      xml.add('  <ItemGroup Label="ProjectConfigurations">\r\n');
      xml.add('    <ProjectConfiguration Include="$configuration|$platform">\r\n');
      xml.add('      <Configuration>$configuration</Configuration>\r\n');
      xml.add('      <Platform>$platform</Platform>\r\n');
      xml.add('    </ProjectConfiguration>\r\n');
      xml.add('  </ItemGroup>\r\n');

      // Globals
      xml.add('  <PropertyGroup Label="Globals">\r\n');
      xml.add('    <ProjectGuid>{$guid}</ProjectGuid>\r\n');
      xml.add('    <RootNamespace>$name</RootNamespace>\r\n');
      xml.add('    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\r\n');
      xml.add('  </PropertyGroup>\r\n');

      xml.add('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />\r\n');

      // Configuration properties - FileGroups are static libraries
      xml.add('  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'$configuration|$platform\'" Label="Configuration">\r\n');
      xml.add('    <ConfigurationType>StaticLibrary</ConfigurationType>\r\n');
      xml.add('    <UseDebugLibraries>${configuration == "Debug" ? "true" : "false"}</UseDebugLibraries>\r\n');
      xml.add('    <PlatformToolset>$platformToolset</PlatformToolset>\r\n');
      xml.add('    <CharacterSet>MultiByte</CharacterSet>\r\n');
      xml.add('  </PropertyGroup>\r\n');

      xml.add('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />\r\n');

      // Output directories
      var intDir = "$(Configuration)\\" + name + "\\";
      xml.add('  <PropertyGroup>\r\n');
      xml.add('    <OutDir>$(Configuration)\\</OutDir>\r\n');
      xml.add('    <IntDir>$intDir</IntDir>\r\n');
      xml.add('    <TargetName>$name</TargetName>\r\n');
      xml.add('  </PropertyGroup>\r\n');

      // Collect compiler flags from all files using compiler.getArgs()
      var flags = collectCompilerFlags(group, outputDir);

      xml.add('  <ItemDefinitionGroup Condition="\'$(Configuration)|$(Platform)\'==\'$configuration|$platform\'">\r\n');
      xml.add('    <ClCompile>\r\n');
      xml.add('      <WarningLevel>Level3</WarningLevel>\r\n');
      xml.add('      <Optimization>${configuration == "Debug" ? "Disabled" : "MaxSpeed"}</Optimization>\r\n');
      if (configuration == "Debug")
         xml.add('      <BasicRuntimeChecks>EnableFastChecks</BasicRuntimeChecks>\r\n');
      xml.add('      <RuntimeLibrary>${flags.runtimeLibrary}</RuntimeLibrary>\r\n');
      if (flags.bufferSecurityCheck != null)
         xml.add('      <BufferSecurityCheck>${flags.bufferSecurityCheck ? "true" : "false"}</BufferSecurityCheck>\r\n');
      xml.add('      <PreprocessorDefinitions>${xmlEscape(flags.preprocessorDefs.join(";"))}</PreprocessorDefinitions>\r\n');
      if (flags.includeDirs.length > 0)
         xml.add('      <AdditionalIncludeDirectories>${flags.includeDirs.join(";")}</AdditionalIncludeDirectories>\r\n');
      if (flags.additionalOptions.length > 0)
         xml.add('      <AdditionalOptions>${xmlEscape(flags.additionalOptions.join(" "))} %(AdditionalOptions)</AdditionalOptions>\r\n');
      xml.add('      <MultiProcessorCompilation>true</MultiProcessorCompilation>\r\n');

      // Set C++ standard
      var cxxStandard = defaultCxxStandard != null ? defaultCxxStandard : 17;
      if (cxxStandard >= 20)
         xml.add('      <LanguageStandard>stdcpp20</LanguageStandard>\r\n');
      else if (cxxStandard >= 17)
         xml.add('      <LanguageStandard>stdcpp17</LanguageStandard>\r\n');
      else if (cxxStandard >= 14)
         xml.add('      <LanguageStandard>stdcpp14</LanguageStandard>\r\n');

      xml.add('    </ClCompile>\r\n');
      xml.add('    <Lib>\r\n');
      xml.add('    </Lib>\r\n');
      xml.add('  </ItemDefinitionGroup>\r\n');

      var projectFiles = addSourceFiles(xml, group, outputDir);

      xml.add('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />\r\n');
      xml.add('</Project>\r\n');

      return {xml: xml.toString(), files: projectFiles};
   }

   function generateMainVcxproj(name:String, target:Target, guid:String, configuration:String, platformToolset:String, outputDir:String, fileGroupProjects:Array<{name:String, path:String, guid:String}>, mainGroup:FileGroup):
     { xml:String, files:Array<String> }
   {
      var xml = new StringBuf();
      xml.add('<?xml version="1.0" encoding="utf-8"?>\r\n');
      xml.add('<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\r\n');

      // Project Configurations
      xml.add('  <ItemGroup Label="ProjectConfigurations">\r\n');
      xml.add('    <ProjectConfiguration Include="$configuration|$platform">\r\n');
      xml.add('      <Configuration>$configuration</Configuration>\r\n');
      xml.add('      <Platform>$platform</Platform>\r\n');
      xml.add('    </ProjectConfiguration>\r\n');
      xml.add('  </ItemGroup>\r\n');

      // Globals
      xml.add('  <PropertyGroup Label="Globals">\r\n');
      xml.add('    <ProjectGuid>{$guid}</ProjectGuid>\r\n');
      xml.add('    <RootNamespace>$name</RootNamespace>\r\n');
      xml.add('    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\r\n');
      xml.add('  </PropertyGroup>\r\n');

      xml.add('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />\r\n');

      // Configuration properties
      var configType = switch (target.mToolID) {
         case "exe": "Application";
         case "dll": "DynamicLibrary";
         case "static_link": "StaticLibrary";
         default: "Application";
      };

      xml.add('  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'$configuration|$platform\'" Label="Configuration">\r\n');
      xml.add('    <ConfigurationType>$configType</ConfigurationType>\r\n');
      xml.add('    <UseDebugLibraries>${configuration == "Debug" ? "true" : "false"}</UseDebugLibraries>\r\n');
      xml.add('    <PlatformToolset>$platformToolset</PlatformToolset>\r\n');
      xml.add('    <CharacterSet>MultiByte</CharacterSet>\r\n');
      xml.add('  </PropertyGroup>\r\n');

      xml.add('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />\r\n');

      // Output directories
      var intDir = "$(Configuration)\\" + name + "\\";
      xml.add('  <PropertyGroup>\r\n');
      xml.add('    <OutDir>.\\</OutDir>\r\n');
      xml.add('    <IntDir>$intDir</IntDir>\r\n');
      xml.add('    <TargetName>${target.mOutput}</TargetName>\r\n');
      xml.add('  </PropertyGroup>\r\n');

      // Collect compiler flags from files in mainGroup using compiler.getArgs()
      var flags = collectCompilerFlags(mainGroup, outputDir);
      // Add _CONSOLE for main project (before %(PreprocessorDefinitions)) unless no_console is defined
      if (!mDefines.exists("no_console"))
         flags.preprocessorDefs.insert(flags.preprocessorDefs.length - 1, "_CONSOLE");

      xml.add('  <ItemDefinitionGroup Condition="\'$(Configuration)|$(Platform)\'==\'$configuration|$platform\'">\r\n');
      xml.add('    <ClCompile>\r\n');
      xml.add('      <WarningLevel>Level3</WarningLevel>\r\n');
      xml.add('      <Optimization>${configuration == "Debug" ? "Disabled" : "MaxSpeed"}</Optimization>\r\n');
      if (configuration == "Debug")
         xml.add('      <BasicRuntimeChecks>EnableFastChecks</BasicRuntimeChecks>\r\n');
      xml.add('      <RuntimeLibrary>${flags.runtimeLibrary}</RuntimeLibrary>\r\n');
      if (flags.bufferSecurityCheck != null)
         xml.add('      <BufferSecurityCheck>${flags.bufferSecurityCheck ? "true" : "false"}</BufferSecurityCheck>\r\n');
      xml.add('      <PreprocessorDefinitions>${xmlEscape(flags.preprocessorDefs.join(";"))}</PreprocessorDefinitions>\r\n');
      if (flags.includeDirs.length > 0)
         xml.add('      <AdditionalIncludeDirectories>${flags.includeDirs.join(";")}</AdditionalIncludeDirectories>\r\n');
      if (flags.additionalOptions.length > 0)
         xml.add('      <AdditionalOptions>${xmlEscape(flags.additionalOptions.join(" "))} %(AdditionalOptions)</AdditionalOptions>\r\n');
      xml.add('      <MultiProcessorCompilation>true</MultiProcessorCompilation>\r\n');

      var cxxStandard = defaultCxxStandard != null ? defaultCxxStandard : 17;
      if (cxxStandard >= 20)
         xml.add('      <LanguageStandard>stdcpp20</LanguageStandard>\r\n');
      else if (cxxStandard >= 17)
         xml.add('      <LanguageStandard>stdcpp17</LanguageStandard>\r\n');
      else if (cxxStandard >= 14)
         xml.add('      <LanguageStandard>stdcpp14</LanguageStandard>\r\n');

      xml.add('    </ClCompile>\r\n');

      // Linker settings
      if (target.mTool == "linker")
      {
         xml.add('    <Link>\r\n');
         xml.add('      <GenerateDebugInformation>true</GenerateDebugInformation>\r\n');

         // Collect libraries - include FileGroup static libs
         var libs = new Array<String>();
         var libPaths = new Array<String>();

         // Add FileGroup static libraries
         for (proj in fileGroupProjects)
            libs.push('$(Configuration)\\${proj.name}.lib');

         for (lib in target.mLibs)
            libs.push(lib);
         for (lib in target.mAutoLibs)
            libs.push(lib);
         for (path in target.mLibPaths)
            libPaths.push(makeRelativePath(outputDir, path));

         if (libs.length > 0)
            xml.add('      <AdditionalDependencies>${libs.join(";")};%(AdditionalDependencies)</AdditionalDependencies>\r\n');
         if (libPaths.length > 0)
            xml.add('      <AdditionalLibraryDirectories>${libPaths.join(";")}</AdditionalLibraryDirectories>\r\n');
         if (target.mToolID == "exe")
         {
            var subsystem = mDefines.exists("no_console") ? "Windows" : "Console";
            xml.add('      <SubSystem>$subsystem</SubSystem>\r\n');
         }

         xml.add('    </Link>\r\n');
      }

      xml.add('  </ItemDefinitionGroup>\r\n');

      // Project references to FileGroup projects
      if (fileGroupProjects.length > 0)
      {
         xml.add('  <ItemGroup>\r\n');
         for (proj in fileGroupProjects)
         {
            xml.add('    <ProjectReference Include="${proj.path}">\r\n');
            xml.add('      <Project>{${proj.guid}}</Project>\r\n');
            xml.add('    </ProjectReference>\r\n');
         }
         xml.add('  </ItemGroup>\r\n');
      }

      // Include __main__ group files directly in main project
      var mainProjectFiles = addSourceFiles(xml, mainGroup, outputDir);

      xml.add('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />\r\n');
      xml.add('</Project>\r\n');

      return {xml: xml.toString(), files: mainProjectFiles};
   }

   function generateSln(name:String, projects:Array<{name:String, path:String, guid:String}>, configuration:String, vsVersion:String):String
   {
      var sln = new StringBuf();

      // Solution header
      var formatVersion = "12.00";
      var vsVersionComment = switch (vsVersion) {
         case "17": "Visual Studio Version 17";
         case "16": "Visual Studio Version 16";
         case "15": "Visual Studio Version 15";
         case "14": "Visual Studio 14";
         default: "Visual Studio Version 17";
      };

      sln.add('\u00EF\u00BB\u00BF\r\n'); // UTF-8 BOM
      sln.add('Microsoft Visual Studio Solution File, Format Version $formatVersion\r\n');
      sln.add('# $vsVersionComment\r\n');

      var cppProjectTypeGuid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942";

      // Projects
      for (proj in projects)
      {
         sln.add('Project("{$cppProjectTypeGuid}") = "${proj.name}", "${proj.path}", "{${proj.guid}}"\r\n');
         sln.add('EndProject\r\n');
      }

      // Global sections
      sln.add('Global\r\n');

      // Solution configuration platforms
      sln.add('\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n');
      sln.add('\t\t$configuration|$platform = $configuration|$platform\r\n');
      sln.add('\tEndGlobalSection\r\n');

      // Project configuration platforms
      sln.add('\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n');
      for (proj in projects)
      {
         sln.add('\t\t{${proj.guid}}.$configuration|$platform.ActiveCfg = $configuration|$platform\r\n');
         sln.add('\t\t{${proj.guid}}.$configuration|$platform.Build.0 = $configuration|$platform\r\n');
      }
      sln.add('\tEndGlobalSection\r\n');

      sln.add('\tGlobalSection(SolutionProperties) = preSolution\r\n');
      sln.add('\t\tHideSolutionNode = FALSE\r\n');
      sln.add('\tEndGlobalSection\r\n');

      sln.add('EndGlobal\r\n');

      return sln.toString();
   }

   // Write file only if content has changed
   static function writeFileIfChanged(path:String, content:String) : Void
   {
      if (sys.FileSystem.exists(path))
      {
         var existing = sys.io.File.getContent(path);
         if (existing == content)
         {
            Log.v("Unchanged: " + path);
            return;
         }
      }
      PathManager.mkdir(path, true);
      sys.io.File.saveContent(path, content);
      Log.info("Wrote: " + path);
   }

   // Generate a GUID from a string (deterministic)
   static function generateGuid(input:String):String
   {
      var hash = Md5.encode(input);
      // Format as GUID: 8-4-4-4-12
      return hash.substr(0, 8).toUpperCase() + "-" +
             hash.substr(8, 4).toUpperCase() + "-" +
             hash.substr(12, 4).toUpperCase() + "-" +
             hash.substr(16, 4).toUpperCase() + "-" +
             hash.substr(20, 12).toUpperCase();
   }

   // Collect compiler flags from all files in a FileGroup using compiler.getArgs()
   function collectCompilerFlags(group:FileGroup, outputDir:String):{includeDirs:Array<String>, preprocessorDefs:Array<String>, additionalOptions:Array<String>, runtimeLibrary:String, bufferSecurityCheck:Null<Bool>}
   {
      var includeDirs = new Array<String>();
      var preprocessorDefs = new Array<String>();
      var additionalOptions = new Array<String>();
      var runtimeLibrary:String = null;
      var bufferSecurityCheck:Null<Bool> = null;

      // Add HXCPP include
      if (mDefines.exists("HXCPP"))
         includeDirs.push(normalizeDosName(mDefines.get("HXCPP") + "/include"));

      // Add group's directory as include
      var groupDir:String = getGroupRoot(group);

      // Collect flags from each file using compiler.getArgs()
      if (group != null)
      {
         for (file in group.mFiles)
         {
            var fileArgs = mCompiler.getArgs(file);

            for (flagStr in fileArgs)
            {
               if (flagStr.indexOf("/I") ==0 || flagStr.indexOf("-I") == 0)
               {
                  var incPath = normalizeDosName(flagStr.substr(2), groupDir);
                  if (includeDirs.indexOf(incPath) < 0)
                     includeDirs.push(incPath);
               }
               else if (flagStr.indexOf("/D") == 0)
               {
                  var def = flagStr.substr(2);
                  if (preprocessorDefs.indexOf(def) < 0)
                     preprocessorDefs.push(def);
               }
               // Detect RuntimeLibrary flags and convert to MSVC XML values
               else if (flagStr == "/MTd" || flagStr == "-MTd")
                  runtimeLibrary = "MultiThreadedDebug";
               else if (flagStr == "/MT" || flagStr == "-MT")
                  runtimeLibrary = "MultiThreaded";
               else if (flagStr == "/MDd" || flagStr == "-MDd")
                  runtimeLibrary = "MultiThreadedDebugDLL";
               else if (flagStr == "/MD" || flagStr == "-MD")
                  runtimeLibrary = "MultiThreadedDLL";
               // Detect BufferSecurityCheck flags
               else if (flagStr == "/GS-" || flagStr == "-GS-")
                  bufferSecurityCheck = false;
               else if (flagStr == "/GS" || flagStr == "-GS")
                  bufferSecurityCheck = true;
               else if (flagStr.indexOf("/") == 0 || flagStr.indexOf("-") == 0)
               {
                  if (additionalOptions.indexOf(flagStr) < 0)
                     additionalOptions.push(flagStr);
               }
            }
         }
      }

      // Add standard defines
      if (mDefines.exists("debug"))
         preprocessorDefs.push("_DEBUG");
      else
         preprocessorDefs.push("NDEBUG");
      preprocessorDefs.push("%(PreprocessorDefinitions)");

      // Default runtime library if not detected from flags
      if (runtimeLibrary == null)
         runtimeLibrary = mDefines.exists("debug") ? "MultiThreadedDebugDLL" : "MultiThreadedDLL";

      return {includeDirs: includeDirs, preprocessorDefs: preprocessorDefs, additionalOptions: additionalOptions, runtimeLibrary: runtimeLibrary, bufferSecurityCheck: bufferSecurityCheck};
   }

   function getGroupRoot(group:FileGroup):String
   {
      var groupDir = group.mDir;
      if (!Path.isAbsolute(groupDir))
      {
         var buildDir = mDefines.get("HXCPP_BUILD_DIR");
         if (buildDir != null)
            groupDir = buildDir + "/" + groupDir;
      }
      groupDir = normalizeDosName(groupDir);
      return groupDir;
   }

   // Add source files from a FileGroup to the vcxproj XML
   function addSourceFiles(xml:StringBuf, group:FileGroup, outputDir:String):Array<String>
   {
      if (group == null || Lambda.empty(group.mFiles))
         return [];

      var sourceFiles = new Array<{path:String, objDir:String}>();
      var headerFiles = new Array<String>();
      var resourceFiles = new Array<String>();
      var groupDir = getGroupRoot(group);


      var projectFiles = new Array<String>();
      for (file in group.mFiles)
      {
         var filePath = file.mDir + file.mName;
         if (!Path.isAbsolute(filePath))
         {
            filePath = Path.normalize(groupDir + "/" + filePath);
         }

         //trace("Processing file: " + filePath + " " + groupDir);
         var ext = Path.extension(file.mName).toLowerCase();

         if (ext == "cpp" || ext == "c" || ext == "cc" || ext == "cxx")
         {
            // Build unique object path using 8-char MD5 hash of normalized source directory
            var dirPath = file.mDir;
            var nameDir = Path.directory(file.mName);
            if (nameDir != "" && nameDir != ".")
               dirPath += nameDir + "/";
            
            var objSubDir = "";
            if (dirPath != "" && dirPath != "." && dirPath != "./")
            {
               var normalizedPath = Path.normalize(dirPath);
               objSubDir = Md5.encode(normalizedPath).substr(0, 8) + "\\";
            }
            
            sourceFiles.push({path: filePath, objDir: objSubDir});
         }
         else if (ext == "h" || ext == "hpp" || ext == "hxx")
            headerFiles.push(filePath);
         else if (ext == "rc")
            resourceFiles.push(filePath);
         else
            continue;
         projectFiles.push(filePath);
      }

      if (sourceFiles.length > 0)
      {
         xml.add('  <ItemGroup>\r\n');
         for (src in sourceFiles)
         {
            if (src.objDir != "")
            {
               xml.add('    <ClCompile Include="${src.path}">\r\n');
               xml.add('      <ObjectFileName>$$(IntDir)${src.objDir}</ObjectFileName>\r\n');
               xml.add('    </ClCompile>\r\n');
            }
            else
               xml.add('    <ClCompile Include="${src.path}" />\r\n');
         }
         xml.add('  </ItemGroup>\r\n');
      }

      if (headerFiles.length > 0)
      {
         xml.add('  <ItemGroup>\r\n');
         for (relPath in headerFiles)
            xml.add('    <ClInclude Include="$relPath" />\r\n');
         xml.add('  </ItemGroup>\r\n');
      }

      if (resourceFiles.length > 0)
      {
         xml.add('  <ItemGroup>\r\n');
         for (relPath in resourceFiles)
            xml.add('    <ResourceCompile Include="$relPath" />\r\n');
         xml.add('  </ItemGroup>\r\n');
      }

      return projectFiles;
   }

   // Collect absolute file paths from a FileGroup
   function collectAbsoluteFilePaths(group:FileGroup):Array<String>
   {
      var result = new Array<String>();
      if (group == null || Lambda.empty(group.mFiles))
         return result;

      var groupDir = getGroupRoot(group);

      for (file in group.mFiles)
      {
         var filePath = file.mDir + file.mName;
         if (!Path.isAbsolute(filePath))
            filePath = Path.normalize(groupDir + "/" + filePath);
         result.push(filePath);
      }

      return result;
   }

   // Generate .vcxproj.filters file to organize files by directory
   function generateFilters(projectFiles:Array<String>):String
   {
      var xml = new StringBuf();
      xml.add('<?xml version="1.0" encoding="utf-8"?>\r\n');
      xml.add('<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\r\n');

      if (projectFiles == null || projectFiles.length == 0)
      {
         xml.add('</Project>\r\n');
         return xml.toString();
      }

      // Collect all unique directory paths and file entries
      var filters = new Map<String, Bool>();
      var fileEntries = new Array<{path:String, filter:String, elementType:String}>();

      for (filePath in projectFiles)
      {
         var ext = Path.extension(filePath).toLowerCase();
         var elementType = if (ext == "cpp" || ext == "c" || ext == "cc" || ext == "cxx") "ClCompile"
                          else if (ext == "h" || ext == "hpp" || ext == "hxx") "ClInclude"
                          else if (ext == "rc") "ResourceCompile"
                          else "None";

         if (elementType == "None")
            continue;

         // Get the directory path for the filter - use leaf directory for absolute paths
         var dirPath = Path.directory(filePath);
         var filterPath = "";
         
         if (dirPath != "" && dirPath != "." && dirPath != "./")
         {
            var normalizedDir = normalizeDosName(dirPath);
            // Use just the leaf directory name to avoid deep hierarchies
            var parts = normalizedDir.split("\\");
            filterPath = parts[parts.length - 1];
            if (filterPath != "")
               filters.set(filterPath, true);
         }

         fileEntries.push({path: filePath, filter: filterPath, elementType: elementType});
      }

      // Write filter definitions
      if (Lambda.count(filters) > 0)
      {
         xml.add('  <ItemGroup>\r\n');
         for (filterPath in filters.keys())
         {
            var filterGuid = generateGuid("filter_" + filterPath);
            xml.add('    <Filter Include="$filterPath">\r\n');
            xml.add('      <UniqueIdentifier>{$filterGuid}</UniqueIdentifier>\r\n');
            xml.add('    </Filter>\r\n');
         }
         xml.add('  </ItemGroup>\r\n');
      }

      // Write file entries with their filters
      if (fileEntries.length > 0)
      {
         xml.add('  <ItemGroup>\r\n');
         for (entry in fileEntries)
         {
            if (entry.filter != "")
            {
               xml.add('    <${entry.elementType} Include="${entry.path}">\r\n');
               xml.add('      <Filter>${entry.filter}</Filter>\r\n');
               xml.add('    </${entry.elementType}>\r\n');
            }
            else
               xml.add('    <${entry.elementType} Include="${entry.path}" />\r\n');
         }
         xml.add('  </ItemGroup>\r\n');
      }

      xml.add('</Project>\r\n');
      return xml.toString();
   }


   // Normalize a file path: normalize and convert to backslashes
   static function normalizeDosName(path:String, baseDir:String = null):String
   {
      path = StringTools.replace (path, "\\\\", "\\");
      path = StringTools.replace (path, "\\\\", "\\");
      path = StringTools.replace (path, "//", "/");
      path = StringTools.replace (path, "//", "/");
      path =  StringTools.replace( Path.normalize(path), "/", "\\");
      if (path.endsWith("\\"))
         path = path.substr(0, path.length - 1);
      if (baseDir!=null && !Path.isAbsolute(path))
         path = baseDir + "\\" + path;
      return path;
   }

   // Escape special XML characters - with additional backslash for quote for command line
   static function xmlEscape(s:String):String
   {
      return s.split("&").join("&amp;").split("<").join("&lt;").split(">").join("&gt;").split("\"").join("\\&quot;");
   }

   // Convert absolute path to relative path from a base directory
   static function makeRelativePath(fromDir:String, toPath:String):String
   {
      fromDir = Path.normalize(fromDir);
      toPath = Path.normalize(toPath);

      // If already relative, return as-is
      if (!Path.isAbsolute(toPath))
         return normalizeDosName(toPath);

      var fromParts = fromDir.split("/");
      var toParts = toPath.split("/");

      // Find common prefix
      var commonLength = 0;
      while (commonLength < fromParts.length && commonLength < toParts.length &&
             fromParts[commonLength].toLowerCase() == toParts[commonLength].toLowerCase())
      {
         commonLength++;
      }

      // Build relative path
      var result = new Array<String>();
      for (i in commonLength...fromParts.length)
         result.push("..");
      for (i in commonLength...toParts.length)
         result.push(toParts[i]);

      if (result.length == 0)
         return ".";
      return result.join("\\");
   }
}
