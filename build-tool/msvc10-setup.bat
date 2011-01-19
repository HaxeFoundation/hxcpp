@if exist "%VS100COMNTOOLS%\vsvars32.bat" (
@if "%VS100COMNTOOLS%"=="" goto error_no_VS100COMNTOOLSDIR
@call "%VS100COMNTOOLS%\vsvars32.bat"
@echo HXCPP_VARS
@set
) else (
@echo VS100COMNTOOLS not set correctly
)

