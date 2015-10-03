setlocal enabledelayedexpansion
@if exist "%HXCPP_MSVC%\..\..\VC\" (
	@if not exist "%HXCPP_MSVC%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
		@echo Error: the specified MSVC version does not have vcvarsx86_amd64.bat setup script
	) else (
		@echo "%HXCPP_MSVC%"
		@call "%HXCPP_MSVC%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
		@echo HXCPP_VARS
		@set
	)
) else if exist "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@echo "%VS140COMNTOOLS%"
	@call "%VS140COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
	@echo HXCPP_VARS
	@set
	@echo HXCPP_HACK_PDBSRV=1
) else if exist "%VS120COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@echo "%VS120COMNTOOLS%"
	@call "%VS120COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
	@echo HXCPP_VARS
	@set
) else if exist "%VS110COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat" (
	@echo "%VS110COMNTOOLS%"
	@call "%VS110COMNTOOLS%\..\..\VC\bin\x86_amd64\vcvarsx86_amd64.bat"
	@echo HXCPP_VARS
	@set
) else (
	echo Error: 64bit is not autmatically supported for this version of VC. Set HXCPP_MSVC_CUSTOM and manually configure the executable, library and include paths
)
