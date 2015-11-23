setlocal enabledelayedexpansion
@if exist "%VS140COMNTOOLS%\vsvars32.bat" (
	@call "%VS140COMNTOOLS%\vsvars32.bat"
		@set "INCLUDE=%ProgramFiles(x86)%\Windows Kits\10\Include;!INCLUDE!"
		@set "PATH=%ProgramFiles(x86)%\Windows Kits\10\bin\x86;!PATH!"
		@set "LIB=%ProgramFiles(x86)%\Windows Kits\10\Lib\10.0.10240.0\um\x86;!LIB!"
		@set "LIBPATH=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\lib\store\references;!LIBPATH!"

	@echo HXCPP_VARS
	@set

) else (
	echo Warning: Could not find environment variables for Visual Studio 2015
)