@echo off
rem ADLINK windows batch installer
setlocal enabledelayedexpansion 
set sourcefile=%~1
set destfile=%~2

trtexec --onnx=%sourcefile% --saveEngine=%destfile%

endlocal
exit /b