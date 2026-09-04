@echo off
set "InstallDir="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.arm64 -property installationPath`) do set "InstallDir=%%i"

if exist "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" x64_arm64 uwp
    echo HXCPP_VARS
    set
) else (
    echo Error: Could not find Visual Studio vcvarsall.bat at "%InstallDir%"
)
