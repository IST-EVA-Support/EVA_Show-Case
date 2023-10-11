#!/bin/bash

ModelNetwork=${1:-onnx};

echo "ModelNetwork: $ModelNetwork";

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

# Check is in ADLINK EVA container
sudoString="sudo"
if [ -e "/entrypoint.sh" ]
then
    sudoString=" "
fi
message_out "sudo String = ${sudoString}"


message_out "Start installing..."
# build required plugin

# download video
if [ -e "Cookie_H264.mov" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase3/Cookie_H264.mov
fi

# download model
if [ $ModelNetwork == "onnx" ]
then
    if [ -e "Cookie_detection.onnx" ]
    then
        message_out "Cookie_detection model exists, skip."
    else
        message_out "Start download model..."
        wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase3/Cookie-inspection.zip
		# unzip it, then delete the zip file
        ${sudoString} apt-get -y install unzip
        unzip Cookie-inspection.zip
        rm Cookie-inspection.zip
    fi
else
    message_out "Unsupported Model: $ModelNetwork" 
fi
# download model label file
if [ $ModelNetwork == "onnx" ]
then
    if [ -e "adlink-onnx-cookie-labels.txt" ]
    then
        message_out "adlink-onnx-cookie-labels.txt exists, skip."
    else
        message_out "Start download label file..."
        wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase3/adlink-onnx-cookie-labels.txt
    fi
fi

# python plugin
message_out "Install related python package"
pip3 install -r requirements.txt
${sudoString} apt -y install gstreamer1.0-libav
pip3 install Pillow
rm ~/.cache/gstreamer-1.0/regi*

message_out "Installation completed, you could run this demo by execute ./run.sh"

