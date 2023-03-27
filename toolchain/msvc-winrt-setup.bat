setlocal enabledelayedexpansion
@if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -arch=x86 -app_platform=UWP -no_logo
	        @set "LIB=%VCTOOLSINSTALLDIR%lib\x86\store;%WindowsSdkDir%Lib\%WindowsSDKLibVersion%um\x86;!LIB!"
	        @set "LIBPATH=%VCTOOLSINSTALLDIR%lib\x86\store\references;!LIBPATH!"
		@echo HXCPP_VARS
		@set
	) else (
		echo Warning: Could not find Visual Studio VsDevCmd
	)
) else if exist "%VS140COMNTOOLS%\vsvars32.bat" (
	@call "%VS140COMNTOOLS%\vsvars32.bat"
	@set "INCLUDE=%WindowsSdkDir%Include;!INCLUDE!"
	@set "PATH=%WindowsSdkDir%bin\x86;!PATH!"
	@set "LIB=%WindowsSdkDir%Lib\%WindowsSDKLibVersion%um\x86;!LIB!"
	@set "LIBPATH=%VS140COMNTOOLS%\..\..\VC\lib\store\references;!LIBPATH!"
	@echo HXCPP_VARS
	@set
) else (
	echo Warning: Could not find x64 environment variables for Visual Studio compiler
)
