#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

metadata_version=${1:-2};

message_out "Start running part assembly combination demo..."

if [ $metadata_version == "2" ]
then
    message_out "[metadata version 2] run first demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-1.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! videoconvert ! adrt model=yolov4-608.engine scale=0.0039 rgbconv=True mean="0 0 0" engine-id=adrt query="//" ! adtrans_yolo topology=1 label=SHMC-label.txt class-num=10 blob-size="76,38,19" mask="(0,1,2),(3,4,5),(6,7,8)" anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=ready query="//adrt" ! partassembly target-type=ready query="//adrt" ! voice_alert alert-type=idling speech-content="idling" query="//adrt" ! videoconvert ! xvimagesink & wait
    
    message_out "[metadata version 2] run second demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-2.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! videoconvert ! adrt model=yolov4-608.engine scale=0.0039 rgbconv=True mean="0 0 0" engine-id=adrt query="//" ! adtrans_yolo topology=1 label=SHMC-label.txt class-num=10 blob-size="76,38,19" mask="(0,1,2),(3,4,5),(6,7,8)" anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=ready query="//adrt" ! partassembly target-type=ready query="//adrt" ! voice_alert alert-type=idling speech-content="idling" query="//adrt" ! videoconvert ! xvimagesink & wait 
    
    message_out "[metadata version 2] run third demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-idle.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! videoconvert ! adrt model=yolov4-608.engine scale=0.0039 rgbconv=True mean="0 0 0" engine-id=adrt query="//" ! adtrans_yolo topology=1 label=SHMC-label.txt class-num=10 blob-size="76,38,19" mask="(0,1,2),(3,4,5),(6,7,8)" anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=ready query="//adrt" ! partassembly target-type=ready query="//adrt" ! voice_alert alert-type=idling speech-content="idling" query="//adrt" ! videoconvert ! xvimagesink & wait
    
elif [ $metadata_version == "1" ]
then
    message_out "[metadata version 1] run first demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-1.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! videoconvert ! adrt model=yolov4-608.engine scale=0.0039 rgbconv=True mean="0 0 0" ! adtrans_yolo topology=1 label=SHMC-label.txt class-num=10 blob-size="76,38,19" mask="(0,1,2),(3,4,5),(6,7,8)" anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait
    
    message_out "[metadata version 1] run second demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-2.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! videoconvert ! adrt model=yolov4-608.engine scale=0.0039 rgbconv=True mean="0 0 0" ! adtrans_yolo topology=1 label=SHMC-label.txt class-num=10 blob-size="76,38,19" mask="(0,1,2),(3,4,5),(6,7,8)" anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait
    
    message_out "[metadata version 1] run third demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-idle.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! nvvidconv ! videoconvert ! adrt model=yolov4-608.engine scale=0.0039 rgbconv=True mean="0 0 0" ! adtrans_yolo topology=1 label=SHMC-label.txt class-num=10 blob-size="76,38,19" mask="(0,1,2),(3,4,5),(6,7,8)" anchor="(12,16),(19,36),(40,28),(36,75),(76,55),(72,146),(142,110),(192,243),(459,401)" use-sigmoid=True input-width=608 input-height=608 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait

else
    message_out "input invalid parameter."
fi
