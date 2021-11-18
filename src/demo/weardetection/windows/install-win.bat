@echo off
rem ADLINK windows batch installer
setlocal enabledelayedexpansion 
::cls
set localPath=%cd%
if not (%1)==() (
	set ModelNetwork=%1
) else (
	set ModelNetwork=ssd_mobilenet
)

rem Install related python package
pip3 install -r ..\requirements.txt

call :message_out "=========== *** Start to install the Showcase 2 for windows *** ===========" 0
call :message_out "*********** Building geofence plugin ***********" 1
::do compiling
echo Start to build geofence plugin.
set geofencebuildbat=%localPath%\..\..\..\plugins\geofence\geofence-build.bat
call %geofencebuildbat%
call :message_out "*********** Building weardetection plugin ***********" 1
echo Start to build weardetection plugin.
set weardetectionbuildbat=%localPath%\..\..\..\plugins\weardetection\weardetection-build.bat
call %weardetectionbuildbat%
echo build geofence and weardetection finished.
echo.

rem download demo video 
call :message_out "*********** Download wear-detection-demo-1.mp4 ***********" 1
set demofile=%localPath%\wear-detection-demo-1.mp4
if exist %demofile% (
	echo "wear-detection-demo-1.mp4" exist!!
) else (
	echo "wear-detection-demo-1.mp4" does not exist
	set url="https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase2/wear-detection-demo-1.mp4"
	call :downloadfile !url!
	echo download demo video finished!
)
echo.

rem download model <mobilenetssd>
:: mobilenetSSDv2
call :message_out "*********** Download model ***********" 1
if %ModelNetwork% equ ssd_mobilenet (
	echo processing model: mobilenetSSDv2
	set modelfile=%localPath%\geo_fencing_ssd_v2.uff
	if not exist !modelfile! (
		echo "geo_fencing_ssd_v2.uff" does not exist
		set url="https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/geo_fencing_ssd_v2.zip"
		call :downloadfile !url!
		echo download demo model finished!
			
		rem unzip the model file and delete zip file
		tar -xf geo_fencing_ssd_v2.zip
		del /f geo_fencing_ssd_v2.zip
	)
	
	if exist !modelfile! (
		call :message_out "mobilenetSSDv2 model exist, and need optimize." 3
		rem <do optimize here>
		call :message_out "Optimizing the model will take time, please wait......" 3
		call %localPath%\..\..\..\scripts\optimize_ssd_mobilenet.bat geo_fencing_ssd_v2.uff mobilenetSSDv2_geofencing.engine
	)
)
:: yolov3
if %ModelNetwork% equ yolov3 (
	echo processing model: yolov3
	set modelfile=%localPath%\adlink-yolov3-geo-fencing.onnx
	if not exist !modelfile! (
		echo "adlink-yolov3-geo-fencing.onnx" does not exist
		set url="https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-yolov3-geo-fencing.zip"
		call :downloadfile !url!
		echo download demo model finished!
		
		rem unzip the model file and delete zip file
		tar -xf adlink-yolov3-geo-fencing.zip
		del /f adlink-yolov3-geo-fencing.zip
	)
	
	if exist !modelfile! (
		call :message_out "yolov3 model exist, and need optimize." 3
		rem <do optimize here>
		call :message_out "Optimizing the model will take time, please wait......" 3
		call %localPath%\..\..\..\scripts\optimize_yolov3.bat adlink-yolov3-geo-fencing.onnx adlink-yolov3-geo-fencing.engine
	)
)
echo.

rem geofencing model label file
call :message_out "*********** Download model label file ***********" 1
if %ModelNetwork% equ ssd_mobilenet (
	echo processing label: mobilenetSSDv2 label
	set labelfile=%localPath%\adlink-mobilenetSSDv2-geo-fencing-label.txt
	if exist !labelfile! (
		echo "adlink-mobilenetSSDv2-geo-fencing-label.txt" exist!!
	) else (
		echo "adlink-mobilenetSSDv2-geo-fencing-label.txt" does not exist
		set url="https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-mobilenetSSDv2-geo-fencing-label.txt"
		call :downloadfile !url!
		echo download geofencing model label file finished!
	)
)
if %ModelNetwork% equ yolov3 (
	echo processing label: yolov3 label
	set labelfile=%localPath%\adlink-yolov3-geo-fencing-label.txt
	if exist !labelfile! (
		echo "adlink-yolov3-geo-fencing-label.txt" exist!!
	) else (
		echo "adlink-yolov3-geo-fencing-label.txt" does not exist
		set url="https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-yolov3-geo-fencing-label.txt"
		call :downloadfile !url!
		echo download geofencing model label file finished!
	)
)
echo.

rem weardetection area predefined file
call :message_out "*********** Download alert-def-area.txt ***********" 1
set defareafile=%localPath%\alert-def-area.txt
if exist %defareafile% (
	echo "alert-def-area.txt" exist!!
) else (
	echo "alert-def-area.txt" does not exist
	set url="https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase2/alert-def-area.txt"
	call :downloadfile !url!
	echo download weardetection predefined area file finished!
)
echo.

rem Deploy python alert plugin.
call :message_out "*********** email alert plugin ***********" 1
set emailalertbuildbat=%localPath%\..\..\..\plugins\alert\email\emailAlert-build.bat
call %emailalertbuildbat%

call :message_out "*********** voice alert plugin ***********" 1
set voicealertbuildbat=%localPath%\..\..\..\plugins\alert\voice\voiceAlert-build.bat
call %voicealertbuildbat%
echo.

call :message_out "=========== *** Installation of Showcase 1 for windows completed. *** ===========" 0
call :message_out "You could run this demo by execute run-win.bat directly." 4
call :message_out "** run mobilenetSSDv2, execute run-win.bat directly." 4
call :message_out "** run yolov3, run run-win.bat with argument yolov3." 4

endlocal
exit /b

::-------------------------------------------------------

:downloadfile
call :message_out "Start downloading the file...%~n1%~x1" 2
curl %1 -o %~n1%~x1
goto:eof

:message_out
rem echo %~1
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
goto:eof

:eof
