setlocal enabledelayedexpansion
@if exist "%HXCPP_MSVC%\..\..\VC\vcvarsall.bat" (
	@echo "%HXCPP_MSVC%"
	@call "%HXCPP_MSVC%\..\..\VC\vcvarsall.bat" amd64
	@set
) else if exist "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS120COMNTOOLS%"
	@call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
   @if defined HXCPP_WINXP_COMPAT (
     @set "INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Include;!INCLUDE!"
     @set "PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Bin;!PATH!"
     @set "LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Lib;!LIB!"
     @set HXCPP_XP_DEFINE=_USING_V120_SDK71_
   )
	@set
) else if exist "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS110COMNTOOLS%"
	@call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
   @if defined HXCPP_WINXP_COMPAT (
     @set "INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Include;!INCLUDE!"
     @set "PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Bin;!PATH!"
     @set "LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Lib;!LIB!"
     @set HXCPP_XP_DEFINE=_USING_V110_SDK71_
   )
	@set
) else if exist "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS100COMNTOOLS%"
	@call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
	@set
) else if exist "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS90COMNTOOLS%"
	@call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
	@set
) else if exist "%VS80COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS80COMNTOOLS%"
	@call "%VS80COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
	@set
) else if exist "%VS71COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS71COMNTOOLS%"
	@call "%VS71COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
	@set
) else if exist "%VS70COMNTOOLS%\..\..\VC\vcvarsall.bat" (
	@echo "%VS70COMNTOOLS%"
	@call "%VS70COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	@echo HXCPP_VARS
	@set
) else (
	echo Warning: Could not find environment variables for Visual Studio 64
)
