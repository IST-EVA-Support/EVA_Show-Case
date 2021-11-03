#!/bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

ModelPruned=${1:-no};

message_out "Start running part preparation demo..."

if [ $ModelPruned == "no" ]
then
    message_out "[without pruned model] run first demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-1.mp4 ! decodebin ! nvvideoconvert ! videoconvert ! adrt model=model/dssd_resnet18_epoch_3400_fp32.engine device=0 ! adtrans_ssd max-count=50 label= model/SHMC-label.txt threshold=0.2 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait
    
    message_out "[without pruned model] run second demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-2.mp4 ! decodebin ! nvvideoconvert ! videoconvert ! adrt model=model/dssd_resnet18_epoch_3400_fp32.engine device=0 ! adtrans_ssd max-count=50 label= model/SHMC-label.txt threshold=0.2 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait 
    
    message_out "[without pruned model] run third demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-idle.mp4 ! decodebin ! nvvideoconvert ! videoconvert ! adrt model=model/dssd_resnet18_epoch_3400_fp32.engine device=0 ! adtrans_ssd max-count=50 label= model/SHMC-label.txt threshold=0.2 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait
    
elif [ $ModelPruned == "yes" ]
then
    message_out "[pruned model] run first demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-1.mp4 ! decodebin ! nvvideoconvert ! videoconvert ! adrt model=model/dssd_resnet18_epoch_810_fp32.engine device=0 ! adtrans_ssd max-count=50 label= model/SHMC-label.txt threshold=0.2 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait
    
    message_out "[pruned model] run second demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-2.mp4 ! decodebin ! nvvideoconvert ! videoconvert ! adrt model=model/dssd_resnet18_epoch_810_fp32.engine device=0 ! adtrans_ssd max-count=50 label= model/SHMC-label.txt threshold=0.2 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait
    
    message_out "[pruned model] run third demo video"
    gst-launch-1.0 filesrc location=prepare-assembly-idle.mp4 ! decodebin ! nvvideoconvert ! videoconvert ! adrt model=model/dssd_resnet18_epoch_810_fp32.engine device=0 ! adtrans_ssd max-count=50 label= model/SHMC-label.txt threshold=0.2 ! partpreparation alert-type=ready ! partassembly target-type=ready ! voice_alert alert-type=idling speech-content="idling" ! videoconvert ! xvimagesink & wait

else
    message_out "input invalid parameter."
fi

