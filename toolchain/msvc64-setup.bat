setlocal enabledelayedexpansion
@if exist "%HXCPP_MSVC%\..\..\VC\" (
	@if not exist "%HXCPP_MSVC%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
		@echo Error: the specified MSVC version does not have vcvarsx86_amd64.bat setup script
	) else (
		@echo "%HXCPP_MSVC%"
		@call "%HXCPP_MSVC%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
		@echo HXCPP_VARS
		@set
	)
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -arch=amd64 -app_platform=Desktop -no_logo
		@echo HXCPP_VARS
		@set
	) else (
		echo Warning: Could not find Visual Studio 2017 VsDevCmd
	)
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
	@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 -app_platform=Desktop -no_logo
	@echo HXCPP_VARS
	@set
) else if exist "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@echo "%VS140COMNTOOLS%"
	@call "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
	@echo HXCPP_VARS
	@set
	@echo HXCPP_HACK_PDBSRV=1
) else if exist "%VS120COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@echo "%VS120COMNTOOLS%"
	@call "%VS120COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%VS110COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@echo "%VS110COMNTOOLS%"
	@call "%VS110COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
	@echo HXCPP_VARS
	@set
) else (
	echo Error: 64bit is not automatically supported for this version of VC. Set HXCPP_MSVC_CUSTOM and manually configure the executable, library and include paths
)
