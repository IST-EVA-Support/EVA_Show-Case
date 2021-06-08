@echo off
rem ADLINK voice alert plugin build
setlocal enabledelayedexpansion

::if the python file has already exist in eva plugins/python folder, remove it
set pythonpluginpath="C:\ADLINK\eva\plugins\python\voiceAlert.py"
if exist %pythonpluginpath% (
	call :message_out "voiceAlert.py exists, will remove it." 3
	del %pythonpluginpath%
)
echo.

:: copy to python plugin
call :message_out "Start copy voiceAlert.py to eva plugins." 5
set alertpath=%~dp0voiceAlert.py
echo F | xcopy /Y %alertpath% %pythonpluginpath%
echo.

:: re-source the environment
call :message_out "start re-source" 5
if exist "c:\ADLINK\eva\scripts\setup_eva_envs.bat" (
   echo ## Setup EVA environment
   call c:\ADLINK\eva\scripts\setup_eva_envs.bat
)
echo.

call :message_out "Copy voiceAlert.py and re-source process completed!" 5

endlocal
exit /b


::-------------------------------------------------------

:message_out
if %2 equ 0 (
	echo [42m%~1[0m
)
if %2 equ 1 (
	echo [36m%~1[0m
)
if %2 equ 2 (
	echo [33m%~1[0m
)
if %2 equ 3 (
	echo [7;31m%~1[0m
)
if %2 equ 4 (
	echo [45m%~1[0m
)
if %2 equ 5 (
	echo [46m%~1[0m
)
goto:eof

:eof