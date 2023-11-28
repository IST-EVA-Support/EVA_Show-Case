#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

ModelNetwork=${1:-onnx};
translatorplugin=${2:-adtrans_detection_customvision_py};

message_out "Start running Cookie-onnx demo..."

if [ $ModelNetwork == "onnx" ]
then
    message_out "using model: onnx"
    if [ $translatorplugin == "adtrans_detection_customvision_py" ]
    then
        message_out "using plugin: adtrans_detection_customvision_py"
		gst-launch-1.0 filesrc location=Cookie_H264.mov ! decodebin ! nvvidconv ! videoconvert ! adonnx mean="0.0,0.0,0.0" model=Cookie-inspection.onnx rgbconv=True engine-id="adonnx" query="//" ! adtrans_detection_customvision_py class-num=3 input-height=512 input-width=512 label-file=adlink-onnx-cookie-labels.txt threshold=0.1 query="//" ! draw_roi ! videoconvert ! xvimagesink 
    else
        message_out "input invalid translator plugin name."
    fi
else
    message_out "input invalid model name."
fi
