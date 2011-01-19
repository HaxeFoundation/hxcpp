@if exist "%VS100COMNTOOLS%\vsvars32.bat" (
@call "%VS100COMNTOOLS%\vsvars32.bat"
@echo HXCPP_VARS
@set
) else (
@echo VS100COMNTOOLS not set correctly
)

