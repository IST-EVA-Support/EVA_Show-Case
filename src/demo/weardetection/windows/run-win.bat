@echo off
rem ADLINK windows batch script for running the gstreamer pipeline
set localPath=%cd%
if not (%1)==() (
	set ModelNetwork=%1
) else (
	set ModelNetwork=ssd_mobilenet
)

if %ModelNetwork% equ ssd_mobilenet (
	if "%~2"=="" (
		set translatorplugin=adtrans_ssd
	) else (
		set translatorplugin=%2
	)
)

if %ModelNetwork% equ yolov3 (
	if "%~2"=="" (
		set translatorplugin=adtrans_yolo
	) else (
		set translatorplugin=%2
	)
)

call :evaenv

set vsenvpathBuildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set vsenvpathCommunity="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %vsenvpathBuildTools% (
	call %vsenvpathBuildTools%
	goto:runpipeline
)
if exist %vsenvpathCommunity% (
	call %vsenvpathCommunity%
	goto:runpipeline
)

:runpipeline

if %ModelNetwork% equ ssd_mobilenet (
	echo using %ModelNetwork%
	if "%translatorplugin%"=="adtrans_ssd" (
		echo using plugin adtrans_ssd
		gst-launch-1.0 filesrc location=wear-detection-demo-1.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=mobilenetSSDv2_geofencing.engine batch=1 device=0 scale=0.008 mean="127 127 127" rgbconv=true ! adtrans_ssd max-count=5 label=adlink-mobilenetSSDv2-geo-fencing-label.txt threshold=0.1 ! geofencefoot alert-area-def=alert-def-area.txt area-display=true person-display=true alert-type=geofence ! weardetection target-type=geofence alert-type=weardetection accumulate-count=10 alert-display=true ! email_alert alert-type=weardetection receiver-address=youremail@domain.com ! voice_alert alert-type=weardetection ! videoconvert ! d3dvideosink
	)
	if "%translatorplugin%"=="adtranslator" (
		echo you are using adtranslator plugin, but adtranslator plugin is deprecated. Please use adtrans_ssd.
	)
)

if %ModelNetwork% equ yolov3 (
	echo using %ModelNetwork%
	if "%translatorplugin%"=="adtrans_yolo" (
		echo using plugin adtrans_yolo
		gst-launch-1.0 filesrc location=wear-detection-demo-1.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=adlink-yolov3-geo-fencing.engine scale=0.004 mean="0 0 0" device=0 batch=1 rgbconv=true ! adtrans_yolo class-num=4 blob-size="16,32,64" input-width=512 input-height=512 mask="(6,7,8),(3,4,5),(0,1,2)" label=adlink-yolov3-geo-fencing-label.txt use-sigmoid=true threshold=0.1 ! geofencefoot alert-area-def=alert-def-area.txt area-display=true person-display=true alert-type=geofence ! weardetection target-type=geofence alert-type=weardetection accumulate-count=70 alert-display=true ! email_alert alert-type=weardetection receiver-address=youremail@domain.com ! voice_alert alert-type=weardetection ! videoconvert ! d3dvideosink
	)
	if "%translatorplugin%"=="adtranslator" (
		echo you are using adtranslator plugin, but adtranslator plugin is deprecated. Please use adtrans_yolo.
		gst-launch-1.0 filesrc location=wear-detection-demo-1.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=adlink-yolov3-geo-fencing.engine scale=0.004 mean="0 0 0" device=0 batch=1 rgbconv=true ! adtranslator label=adlink-yolov3-geo-fencing-label.txt topology=yolov3 dims=1,27,16,16,1,27,32,32,1,27,64,64 input_width=512 engine-type=2 ! geofencefoot alert-area-def=alert-def-area.txt area-display=true person-display=true alert-type=geofence ! weardetection target-type=geofence alert-type=weardetection accumulate-count=10 alert-display=true ! email_alert alert-type=weardetection receiver-address=youremail@domain.com ! voice_alert alert-type=weardetection ! videoconvert ! d3dvideosink
	)
)
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