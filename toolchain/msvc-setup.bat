setlocal enabledelayedexpansion
@if exist "%HXCPP_MSVC%\vcvars32.bat" (
	@call "%HXCPP_MSVC%\vcvars32.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%HXCPP_MSVC%\vsvars32.bat" (
	@call "%HXCPP_MSVC%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
		@set InstallDir=%%i
	)
	@if exist "!InstallDir!\Common7\Tools\VsDevCmd.bat" (
		@call "!InstallDir!\Common7\Tools\VsDevCmd.bat" -arch=x86 -app_platform=Desktop -no_logo
		@echo HXCPP_VARS
		@set
	) else (
		echo Warning: Could not find Visual Studio 2017 VsDevCmd
	)
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
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
) else if exist "%VS120COMNTOOLS%\vsvars32.bat" (
	@call "%VS120COMNTOOLS%\vsvars32.bat"
	@if defined HXCPP_WINXP_COMPAT (
		@if exist "%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\" (
			@set "INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\Include;!INCLUDE!"
			@set "PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\Bin;!PATH!"
			@set "LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\Lib;!LIB!"		
		) else if exist "%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\" (
			@set "INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Include;!INCLUDE!"
			@set "PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Bin;!PATH!"
			@set "LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Lib;!LIB!"
		)

		@set HXCPP_XP_DEFINE=_USING_V120_SDK71_
	)
	@echo HXCPP_VARS
	@set
) else if exist "%VS110COMNTOOLS%\vsvars32.bat" (
	@call "%VS110COMNTOOLS%\vsvars32.bat"
	@if defined HXCPP_WINXP_COMPAT (
		@set "INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Include;!INCLUDE!"
		@set "PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Bin;!PATH!"
		@set "LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Lib;!LIB!"
		@set HXCPP_XP_DEFINE=_USING_V110_SDK71_
	)
	@echo HXCPP_VARS
	@set
) else if exist "%VS100COMNTOOLS%\vsvars32.bat" (
	@call "%VS100COMNTOOLS%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%VS90COMNTOOLS%\vsvars32.bat" (
	@call "%VS90COMNTOOLS%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%VS80COMNTOOLS%\vsvars32.bat" (
	@call "%VS80COMNTOOLS%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%VS71COMNTOOLS%\vsvars32.bat" (
	@call "%VS71COMNTOOLS%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%VS70COMNTOOLS%\vsvars32.bat" (
	@call "%VS70COMNTOOLS%\vsvars32.bat"
	@echo HXCPP_VARS
	@set
) else (
	echo Warning: Could not find environment variables for Visual Studio
)
