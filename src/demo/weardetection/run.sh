#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

ModelNetwork=${1:-ssd_mobilenet};
translatorplugin=${2:-adtrans_ssd};

if [ $ModelNetwork == "yolov3" ]
then
    if [ $translatorplugin == "adtrans_ssd" ]
    then
        translatorplugin="adtrans_yolo"
        message_out $translatorplugin
    fi
fi

message_out "Start running geofence demo..."

if [ $ModelNetwork == "ssd_mobilenet" ]
then
    message_out "using model: ssd_mobilenet"
    if [ $translatorplugin == "adtrans_ssd" ]
    then
        message_out "using plugin: adtrans_ssd"
        gst-launch-1.0 filesrc location=wear-detection-demo-1.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=mobilenetSSDv2_geofencing.engine batch=1 device=0 scale=0.008 mean="127 127 127" rgbconv=true engine-id="adrt" query="//" ! adtrans_ssd max-count=5 label=adlink-mobilenetSSDv2-geo-fencing-label.txt threshold=0.1 ! geofencefoot alert-area-def=alert-def-area.txt area-display=true person-display=true alert-type=geofence query="//adrt[class in person]" ! weardetection target-type=geofence alert-type=weardetection accumulate-count=10 alert-display=true query="//adrt[class in person]" ! email_alert alert-type=weardetection receiver-address=youremail@domain.com query="//adrt" ! voice_alert alert-type=weardetection query="//adrt" ! videoconvert ! ximagesink
    elif [ $translatorplugin == "adtranslator" ]
    then
        message_out "you are using adtranslator plugin, but adtranslator plugin is deprecated. Please use adtrans_ssd."
    else
        message_out "input invalid translator plugin name."
    fi
elif [ $ModelNetwork == "yolov3" ]
then
    message_out "using model: yolov3"
    if [ $translatorplugin == "adtrans_yolo" ]
    then
        message_out "using plugin: adtrans_yolo"
        gst-launch-1.0 filesrc location=wear-detection-demo-1.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=adlink-yolov3-geo-fencing.engine scale=0.004 mean="0 0 0" device=0 batch=1 rgbconv=true engine-id="adrt" query="//" ! adtrans_yolo class-num=4 blob-size="16,32,64" input-width=512 input-height=512 mask="(6,7,8),(3,4,5),(0,1,2)" label=adlink-yolov3-geo-fencing-label.txt use-sigmoid=true threshold=0.1 ! geofencefoot alert-area-def=alert-def-area.txt area-display=true person-display=true alert-type=geofence query="//adrt[class in person]" ! weardetection target-type=geofence alert-type=weardetection accumulate-count=12 alert-display=true query="//adrt[class in person]" ! email_alert alert-type=weardetection receiver-address=youremail@domain.com query="//adrt" ! voice_alert alert-type=weardetection query="//adrt" ! videoconvert ! ximagesink
    elif [ $translatorplugin == "adtranslator" ]
    then
        message_out "you are using adtranslator plugin, but adtranslator plugin is deprecated. Please use adtrans_yolo."
#         gst-launch-1.0 filesrc location=wear-detection-demo-1.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! adrt model=adlink-yolov3-geo-fencing.engine scale=0.004 mean="0 0 0" device=0 batch=1 rgbconv=true ! adtranslator label=adlink-yolov3-geo-fencing-label.txt topology=yolov3 dims=1,27,16,16,1,27,32,32,1,27,64,64 input_width=512 engine-type=2 ! geofencefoot alert-area-def=alert-def-area.txt area-display=true person-display=true alert-type=geofence ! weardetection target-type=geofence alert-type=weardetection accumulate-count=30 alert-display=true ! email_alert alert-type=weardetection receiver-address=youremail@domain.com ! voice_alert alert-type=weardetection ! videoconvert ! ximagesink
    else
        message_out "input invalid translator plugin name."
    fi
else
    message_out "input invalid model name."
fi
