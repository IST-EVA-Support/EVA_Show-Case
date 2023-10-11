#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

# Get current path
current_path=`pwd`


# Check is in ADLINK EVA container
sudoString="sudo"
if [ -e "/entrypoint.sh" ]
then
    sudoString=" "
fi
message_out "sudo String = ${sudoString}"


message_out "Start installing..."
# build required plugin
message_out "Building assembly plugin..."
${sudoString} apt update
${sudoString} apt -y install libgstreamer-plugins-base1.0-dev
${sudoString} apt -y install libopencv-dev python3-opencv
${sudoString} apt -y install wget
${sudoString} apt -y install libnvinfer-bin
pip3 install -r requirements.txt
../../plugins/assembly/assembly-build.sh


# ####
# download video
if [ -e "material-preparation.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [material-preparation.mp4]..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/material-preparation.mp4
fi
if [ -e "disassembly.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [disassembly.mp4]..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/disassembly.mp4
fi
if [ -e "order-incorrect.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [order-incorrect.mp4]..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/order-incorrect.mp4
fi


# download model and label zip file
if [ -e "yolov4-tiny-608.onnx" ]
then
    message_out "onnx model file exists."
else    
    message_out "Start download model..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/model-yolov4.zip

    # unzip it, then delete the zip file
    ${sudoString} apt-get -y install unzip
    unzip model-yolov4.zip
    rm model-yolov4.zip
fi

message_out "Start convert onnx model to tensorrt..."    
../../scripts/optimize_yolov4.sh yolov4-tiny-608.onnx yolov4-tiny-608.engine
message_out "Convert Done."


# python plugin
cd $current_path
message_out "Deploy alert plugin..."
../../plugins/alert/email/emailAlert-build.sh
../../plugins/alert/voice/voiceAlert-build.sh

message_out "Install related python package"
pip3 install -r requirements.txt
${sudoString} apt -y install gstreamer1.0-libav
${sudoString} apt-get -y install espeak
rm ~/.cache/gstreamer-1.0/regi*

message_out "Installation completed, you could run this demo by execute ./run.sh"

