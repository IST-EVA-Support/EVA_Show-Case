#!/bin/bash

ModelNetwork=${1:-onnx};

echo "ModelNetwork: $ModelNetwork";

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

message_out "Start installing..."
# build required plugin
#install onnx runtime ????
#https://nvidia.app.box.com/s/bfs688apyvor4eo8sf3y1oqtnarwafww
###message_out "Building geofence plugin..."
###sudo apt -y install libgstreamer-plugins-base1.0-dev
###../../plugins/geofence/geofence-build.sh
# download video
if [ -e "Cookie_H264.mov" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video..."
    wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase3/Cookie_H264.mov
fi

# download model
if [ $ModelNetwork == "onnx" ]
then
    if [ -e "Cookie_detection.onnx" ]
    then
        message_out "Cookie_detection model exists, skip."
    else
        message_out "Start download model..."
        wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase3/Cookie_detection.zip
		# unzip it, then delete the zip file
        sudo apt-get -y install unzip
        unzip Cookie_detection.zip
        rm Cookie_detection.zip
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
        wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase3/adlink-onnx-cookie-labels.txt
    fi
fi

# python plugin
message_out "Install related python package"
pip3 install -r requirements.txt
sudo apt -y install gstreamer1.0-libav
pip3 install Pillow
rm ~/.cache/gstreamer-1.0/regi*

message_out "Installation completed, you could run this demo by execute ./run.sh"

