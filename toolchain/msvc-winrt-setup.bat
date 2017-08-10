setlocal enabledelayedexpansion
@if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
	@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -app_platform=UWP -no_logo
	@echo HXCPP_VARS
	@set
) else if exist "%VS140COMNTOOLS%\vsvars32.bat" (
    @call "%VS140COMNTOOLS%\vsvars32.bat"
        @set "INCLUDE=%WindowsSdkDir%Include;!INCLUDE!"
        @set "PATH=%WindowsSdkDir%bin\x86;!PATH!"
        @set "LIB=%WindowsSdkDir%Lib\%WindowsSDKLibVersion%um\x86;!LIB!"
        @set "LIBPATH=%VS140COMNTOOLS%\..\..\VC\lib\store\references;!LIBPATH!"
    @echo HXCPP_VARS
    @set
) else (   
    echo Warning: Could not find environment variables for Visual Studio 2015
)
