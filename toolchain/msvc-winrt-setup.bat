setlocal enabledelayedexpansion
@if exist "%VS140COMNTOOLS%\vsvars32.bat" (
        FOR /F "usebackq skip=4 tokens=1-3" %%A IN (`REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10 2^>nul`) DO (
            set KitsRoot10Path=%%C
        )
        if defined KitsRoot10Path (
            @call "%VS140COMNTOOLS%\vsvars32.bat"
            @set "INCLUDE=%KitsRoot10Path%Include;!INCLUDE!"
            @set "PATH=%KitsRoot10Path%bin\x86;!PATH!"
            @if exist "%KitsRoot10Path%Lib\10.0.10586.0" (
                @set "LIB=%KitsRoot10Path%Lib\10.0.10586.0\um\x86;!LIB!"
            ) else ( 
                @set "LIB=%KitsRoot10Path%Lib\10.0.10240.0\um\x86;!LIB!"
            )
            @set "LIBPATH=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\lib\store\references;!LIBPATH!"
            @echo HXCPP_VARS
            @set
        ) else (
            echo Warning: Could not find Windows 10 Kits Path
        )
) else (   
    echo Warning: Could not find environment variables for Visual Studio 2015
)
