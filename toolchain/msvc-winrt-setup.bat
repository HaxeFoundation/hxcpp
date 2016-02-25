setlocal enabledelayedexpansion
@if exist "%VS140COMNTOOLS%\vsvars32.bat" (
        FOR /F "usebackq skip=2 tokens=1-3" %%A IN (`REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows" /v CurrentVersion 2^>nul`) DO (
            set CurrentSDKVersion=%%C
        )
        if defined CurrentSDKVersion (
            @call "%VS140COMNTOOLS%\vsvars32.bat"
            @set "INCLUDE=%ProgramFiles(x86)%\Windows Kits\10\Include;!INCLUDE!"
            @set "PATH=%ProgramFiles(x86)%\Windows Kits\10\bin\x86;!PATH!"
            @set "LIB=%ProgramFiles(x86)%\Windows Kits\10\Lib\%CurrentSDKVersion%\um\x86;!LIB!"
            @set "LIBPATH=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\lib\store\references;!LIBPATH!"
            @echo HXCPP_VARS
            @set
        ) else (
            echo Warning: Could not find Windows SDK
        )
) else (
	echo Warning: Could not find environment variables for Visual Studio 2015
)