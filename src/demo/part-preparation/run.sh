#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

metadata_version=${1:-2};

message_out "Start running part preparation demo..."

if [ $metadata_version == "2" ]
then
    message_out "[metadata version 2] run first demo video"
    gst-launch-1.0 filesrc location=material-preparation.mp4 ! decodebin ! videoconvert ! adrt model=yolov4-tiny-608.engine scale=0.0039 rgbconv=True mean="0 0 0" engine-id=adrt query="//" ! adtrans_yolo topology=3 label=SHMC-label.txt class-num=10 blob-size="19,38" mask="(3,4,5),(1,2,3)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=order-incorrect query="//adrt" ! voice_alert alert-type=order-incorrect speech-content="order-incorrect" query="//" ! videoconvert ! xvimagesink & wait
    
    message_out "[metadata version 2] run second demo video"
    gst-launch-1.0 filesrc location=disassembly.mp4 ! decodebin ! videoconvert ! adrt model=yolov4-tiny-608.engine scale=0.0039 rgbconv=True mean="0 0 0" engine-id=adrt query="//" ! adtrans_yolo topology=3 label=SHMC-label.txt class-num=10 blob-size="19,38" mask="(3,4,5),(1,2,3)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=order-incorrect query="//adrt" ! voice_alert alert-type=order-incorrect speech-content="order-incorrect" query="//adrt" ! videoconvert ! xvimagesink & wait 
    
    message_out "[metadata version 2] run third demo video"
    gst-launch-1.0 filesrc location=order-incorrect.mp4 ! decodebin ! videoconvert ! adrt model=yolov4-tiny-608.engine scale=0.0039 rgbconv=True mean="0 0 0" engine-id=adrt query="//" ! adtrans_yolo topology=3 label=SHMC-label.txt class-num=10 blob-size="19,38" mask="(3,4,5),(1,2,3)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=order-incorrect query="//adrt" ! voice_alert alert-type=order-incorrect speech-content="order-incorrect" query="//adrt" ! videoconvert ! xvimagesink & wait
    
elif [ $metadata_version == "1" ]
then
    message_out "[metadata version 1] run first demo video"
    gst-launch-1.0 filesrc location=material-preparation.mp4 ! decodebin ! videoconvert ! adrt model=yolov4-tiny-608.engine scale=0.0039 rgbconv=True mean="0 0 0" ! adtrans_yolo topology=3 label=SHMC-label.txt class-num=10 blob-size="19,38" mask="(3,4,5),(1,2,3)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=order-incorrect ! voice_alert alert-type=order-incorrect speech-content="order-incorrect" ! videoconvert ! xvimagesink & wait
    
    message_out "[metadata version 1] run second demo video"
    gst-launch-1.0 filesrc location=disassembly.mp4 ! decodebin ! videoconvert ! adrt model=yolov4-tiny-608.engine scale=0.0039 rgbconv=True mean="0 0 0" ! adtrans_yolo topology=3 label=SHMC-label.txt class-num=10 blob-size="19,38" mask="(3,4,5),(1,2,3)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=order-incorrect ! voice_alert alert-type=order-incorrect speech-content="order-incorrect" ! videoconvert ! xvimagesink & wait 
    
    message_out "[metadata version 1] run third demo video"
    gst-launch-1.0 filesrc location=order-incorrect.mp4 ! decodebin ! videoconvert ! adrt model=yolov4-tiny-608.engine scale=0.0039 rgbconv=True mean="0 0 0" ! adtrans_yolo topology=3 label=SHMC-label.txt class-num=10 blob-size="19,38" mask="(3,4,5),(1,2,3)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=order-incorrect ! voice_alert alert-type=order-incorrect speech-content="order-incorrect" ! videoconvert ! xvimagesink & wait
else
    message_out "input invalid parameter."
fi

