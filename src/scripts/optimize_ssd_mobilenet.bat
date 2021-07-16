@echo off
rem ADLINK windows batch installer
setlocal enabledelayedexpansion 
set sourcefile=%~1
set destfile=%~2

trtexec.exe --uff=%sourcefile% --output=NMS --uffInput=Input,3,300,300 --batch=1 --maxBatch=1 --saveEngine=%destfile%

endlocal
exit /b