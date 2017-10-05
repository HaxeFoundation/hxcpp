setlocal enabledelayedexpansion
@if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -arch=amd64 -app_platform=UWP -no_logo
	        @set "LIB=%VCTOOLSINSTALLDIR%lib\x64\store;%WindowsSdkDir%Lib\%WindowsSDKLibVersion%um\x64;!LIB!"
	        @set "LIBPATH=%VCTOOLSINSTALLDIR%lib\x86\store\references;!LIBPATH!"
	        @echo HXCPP_VARS
	        @set
	        @echo HXCPP_HACK_PDBSRV=1
	) else (
		echo Warning: Could not find Visual Studio 2017 VsDevCmd
	)
) else if exist "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@call "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
		@set "INCLUDE=%WindowsSdkDir%Include;!INCLUDE!"
		@set "PATH=%WindowsSdkDir%bin\x64;!PATH!"
		@set "LIB=%WindowsSdkDir%Lib\%WindowsSDKLibVersion%um\x64;!LIB!"
		@set "LIBPATH=%VS140COMNTOOLS%\..\..\VC\lib\store\references;!LIBPATH!"
	@echo HXCPP_VARS
	@set
	@echo HXCPP_HACK_PDBSRV=1
) else (
	echo Warning: Could not find x64 environment variables for Visual Studio 2015 or 2017 compiler
)
