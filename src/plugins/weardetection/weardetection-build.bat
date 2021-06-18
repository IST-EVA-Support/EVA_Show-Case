@echo off
rem ADLINK weardetection plugin build
setlocal enabledelayedexpansion

::check vs2019 build exist
echo VS2019 env setting
::set buildenv="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
set buildenv="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
echo buildenv = %buildenv%
if not exist %buildenv% (
	call :message_out "build 2019 environment does not exist." 3
	goto:eof
)
call :message_out "build 2019 environment exists and set vs2019 build environment." 5
call %buildenv%

::run EVA environment CMD
echo EVA env setting
set evabuildenv="C:\ADLINK\eva\EVACMD.bat"
if not exist %evabuildenv% (
	call :message_out "EVACMD does not exist." 3
	goto:eof
)
call :message_out "EVACMD exists and set EVA environment." 5
call :evaenv

::check build folder. If exist delete it
set buildpath=%~dp0build
echo build folder : %buildpath%
if exist %buildpath% (
	call :message_out "build folder exist and will remove it." 4
	rmdir /S /Q %buildpath%
)

call :message_out "meson configure establishing ..." 4
::echo source path: %~dp0
meson setup --buildtype=release --backend=vs %buildpath% %~dp0 -Deva_root="C:\ADLINK\eva" -Dopencv_dir="C:\Program Files (x86)\Intel\openvino_2021\opencv"

call :message_out "meson configure finished and will build it by meson." 4
meson compile -C %buildpath%

::check if eva plugin folder. If exist weardetection plugin, remove it.
set weardetectionplugin="C:\ADLINK\eva\plugins\weardetection.dll"
if exist %weardetectionplugin% (
	call :message_out "weardetection.dll already exists in eva, will remove it." 3
	del %weardetectionplugin%
)
call :message_out "Start copy weardetection.dll to eva plugins." 5
echo F | xcopy /Y %buildpath%\weardetection.dll %weardetectionplugin%

::clear cache of gstreamer
if exist "%USERPROFILE%\AppData\Local\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64.bin" (
	del %USERPROFILE%\AppData\Local\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64.bin  
	call :message_out "registry exists, will clean it." 5	
)
else (
	call :message_out "registry is clean." 5
)

call :message_out "Build weardetection plugin completed!" 5

endlocal
exit /b


::-------------------------------------------------------

:evaenv
if exist "%USERPROFILE%\AppData\Local\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64.bin" (
   del %USERPROFILE%\AppData\Local\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64.bin   
)

if exist "c:\ADLINK\gstreamer\setupvars.bat" (
   call c:\ADLINK\gstreamer\setupvars.bat
)

if exist "C:\ADLINK\eva\VERSION" ( 
   for /f "delims=" %%x in (C:\ADLINK\eva\VERSION) do echo ## EVA SDK version: %%x
)

if exist "c:\ADLINK\eva\scripts\setup_eva_envs.bat" (
   echo ## Setup EVA environment
   call c:\ADLINK\eva\scripts\setup_eva_envs.bat
)
goto:eof

:message_out
if %2 equ 0 (
	echo [42m%~1[0m
)
if %2 equ 1 (
	REM echo [33m%~1[0m
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