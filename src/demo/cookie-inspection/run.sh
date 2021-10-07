#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

ModelNetwork=${1:-onnx};
translatorplugin=${2:-adtrans_tinyyolov2_py};

message_out "Start running Cookie-onnx demo..."

if [ $ModelNetwork == "onnx" ]
then
    message_out "using model: onnx"
    if [ $translatorplugin == "adtrans_tinyyolov2_py" ]
    then
        message_out "using plugin: adtrans_tinyyolov2_py"
        gst-launch-1.0 filesrc location=Cookie_H264.mov ! qtdemux ! h264parse ! avdec_h264 ! videoscale ! capsfilter caps="video/x-raw, height=(int)480, width=(int)640" ! videoconvert ! adonnx mean="0.0,0.0,0.0" model=Cookie_detection.onnx rgbconv=True ! adtrans_tinyyolov2_py class-num=3 input-height=512 input-width=512 label-file=adlink-onnx-cookie-labels.txt threshold=0.1 ! admetadumper ! admetadrawer ! videoconvert ! xvimagesink 
    else
        message_out "input invalid translator plugin name."
    fi
else
    message_out "input invalid model name."
fi




# gst-launch-1.0 filesrc location=geo-fencing-demo.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=mobilenetSSDv2_geofencing.engine batch=1 device=0 scale=0.008 mean="127 127 127" rgbconv=true ! adtrans_ssd max-count=5 label=adlink-mobilenetSSDv2-geo-fencing-label.txt threshold=0.1 ! geofencebase alert-area-def=alert-def-area-geo.txt area-display=true person-display=true alert-type=geofence ! email_alert alert-type=geofence receiver-address=youremail@domain.com ! voice_alert alert-type=geofence ! videoconvert ! ximagesink
