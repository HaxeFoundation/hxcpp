setlocal enabledelayedexpansion
@if exist "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
    @call "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
        @set "INCLUDE=%WindowsSdkDir%Include;!INCLUDE!"
        @set "PATH=%WindowsSdkDir%bin\x64;!PATH!"
        @set "LIB=%WindowsSdkDir%Lib\%WindowsSDKLibVersion%um\x64;!LIB!"
        @set "LIBPATH=%VS140COMNTOOLS%\..\..\VC\lib\store\references;!LIBPATH!"
    @echo HXCPP_VARS
    @set
    @echo HXCPP_HACK_PDBSRV=1
) else (
    echo Warning: Could not find x64 environment variables for Visual Studio 2015
)
