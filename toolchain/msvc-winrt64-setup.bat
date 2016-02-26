setlocal enabledelayedexpansion
@if exist "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
        @call "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
        @set "INCLUDE=%ProgramFiles(x86)%\Windows Kits\10\Include;!INCLUDE!"
        @set "PATH=%ProgramFiles(x86)%\Windows Kits\10\bin\x64;!PATH!"
        @set "LIB=%ProgramFiles(x86)%\Windows Kits\10\Lib\10.0.10586.0\um\x64;!LIB!"
        @set "LIBPATH=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\lib\store\references;!LIBPATH!"
        @echo HXCPP_VARS
        @set
        @echo HXCPP_HACK_PDBSRV=1
) else (
	echo Warning: Could not find x64 environment variables for Visual Studio 2015
)
