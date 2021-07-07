#!/bin/bash

ModelNetwork=${1:-ssd_mobilenet};

echo "ModelNetwork: $ModelNetwork";

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

message_out "Start installing..."
message_out "Building geofence plugin..."
../../plugins/geofence/geofence-build.sh

if [ -e "geo-fencing-demo.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video..."
    wget https://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/geo-fencing-demo.mp4
fi

if [ $ModelNetwork == "ssd_mobilenet" ]
then
    if [ -e "mobilenetSSDv2_geofencing.engine" ]
    then
        message_out "Model exists, skip."
    else
        if [ -e "geo_fencing_ssd_v2.uff" ]
        then 
            message_out "uff model file exists."
        else    
            message_out "Start download model..."
            wget https://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/geo_fencing_ssd_v2.zip
            # unzip it, then delete the zip file
            sudo apt-get install unzip
            unzip geo_fencing_ssd_v2.zip
            rm geo_fencing_ssd_v2.zip
        fi

        message_out "Start convert uff model to tensorrt..."    
        ../../scripts/optimize_ssd_mobilenet.sh geo_fencing_ssd_v2.uff mobilenetSSDv2_geofencing.engine
        message_out "Convert Done."
    fi
elif [ $ModelNetwork == "yolov3" ]
then
    if [ -e "adlink-yolov3-geo-fencing.engine" ]
    then
        message_out "Yolov3 model exists, skip."
    else
        if [ -e "adlink-yolov3-geo-fencing.onnx" ]
        then 
            message_out "onnx model file exists."
        else    
            message_out "Start download model..."
            wget https://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-yolov3-geo-fencing.zip
            # unzip it, then delete the zip file
            sudo apt-get install unzip
            unzip adlink-yolov3-geo-fencing.zip
            rm adlink-yolov3-geo-fencing.zip
        fi

        message_out "Start convert onnx model to tensorrt..."
        ../../scripts/optimize_yolov3.sh adlink-yolov3-geo-fencing.onnx adlink-yolov3-geo-fencing.engine        
        message_out "Convert Done."
    fi
else
    message_out "Unsupported Model: $ModelNetwork" 
fi

if [ -e "alert-def-area-geo.txt" ]
then
    message_out "alert-def-area-geo.txt exists, skip."
else
    message_out "Start download area file..."
    wget https://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/alert-def-area-geo.txt
fi

if [ -e "adlink-mobilenetSSDv2-geo-fencing-label.txt" ]
then
    message_out "adlink-mobilenetSSDv2-geo-fencing-label.txt exists, skip."
else
    message_out "Start download label file..."
    wget https://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-mobilenetSSDv2-geo-fencing-label.txt
fi

message_out "Deploy alert plugin..."
../../plugins/alert/email/emailAlert-build.sh
../../plugins/alert/voice/voiceAlert-build.sh

message_out "Install related python package"
pip3 install -r requirements.txt
sudo apt install gstreamer1.0-libav
sudo apt-get install espeak
rm ~/.cache/gstreamer-1.0/regi*

message_out "Installation completed, you could run this demo by execute ./run.sh"
