setlocal enabledelayedexpansion
@if exist "%HXCPP_MSVC%\..\..\VC\" (
	@if not exist "%hxcpp_msvc%\..\..\VC\Auxiliary\Build\vcvars%msvc_host_arch%_arm64.bat" (
		@echo Error: the specified MSVC version does not have vcvars%msvc_host_arch%_arm64.bat setup script
	) else (
		@echo "%HXCPP_MSVC%"
		@call "%HXCPP_MSVC%\..\..\VC\Auxiliary\Build\vcvars%msvc_host_arch%_amd64.bat"
		@echo HXCPP_VARS
		@set
	)
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -host_arch=%msvc_host_arch% -arch=arm64 -app_platform=Desktop -no_logo
		@echo HXCPP_VARS
		@set
	) else (
		echo Warning: Could not find Visual Studio VsDevCmd
	)
) else (
	echo Error: arm64 is not automatically supported for this version of VC.
)
