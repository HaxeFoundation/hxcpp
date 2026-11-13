setlocal enabledelayedexpansion
@if exist "%HXCPP_MSVC%\vcvars32.bat" (
	@call "%HXCPP_MSVC%\vcvars32.bat"
	@echo HXCPP_VARS
	@set
	exit
) else if exist "%HXCPP_MSVC%\vsvars32.bat" (
	@call "%HXCPP_MSVC%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
	exit
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -arch=x86 -app_platform=Desktop -no_logo
		@echo HXCPP_VARS
		@set
		exit
	) else (
		echo Warning: Could not find Visual Studio VsDevCmd
	)
)
@if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
	@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -app_platform=Desktop -no_logo
	@echo HXCPP_VARS
	@set
) else if exist "%VS140COMNTOOLS%\vsvars32.bat" (
	@call "%VS140COMNTOOLS%\vsvars32.bat"
	@if defined HXCPP_WINXP_COMPAT (
		@set "INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Include;!INCLUDE!"
		@set "PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Bin;!PATH!"
		@set "LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Lib;!LIB!"
		@set HXCPP_XP_DEFINE=_USING_V140_SDK71_
	)
	@echo HXCPP_VARS
	@set
) else (
	echo Warning: Could not find environment variables for Visual Studio
)
