@echo off
rem ADLINK windows batch installer
call :evaenv
gst-launch-1.0 filesrc location=geo-fencing-demo.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=mobilenetSSDv2_geofencing.engine batch=1 device=0 scale=0.008 mean="127 127 127" rgbconv=true ! adtrans_ssd max-count=5 label=adlink-mobilenetSSDv2-geo-fencing-label.txt threshold=0.1 ! geofencebase alert-area-def=alert-def-area-geo.txt area-display=true person-display=true alert-type=geofence ! email_alert alert-type=geofence receiver-address=paul.lin@adlinktech.com ! voice_alert alert-type=geofence ! videoconvert ! d3dvideosink

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
:eof