setlocal enabledelayedexpansion
@if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.arm64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -arch=arm64 -app_platform=Desktop -no_logo
		@echo HXCPP_VARS
		@set
	) else (
		echo Warning: Could not find Visual Studio VsDevCmd
	)
) else (
	echo Error: arm64 is not automatically supported for this version of VC.
)
