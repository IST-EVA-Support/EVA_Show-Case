#!/bin/bash

ModelNetwork=${1:-ssd_mobilenet};

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

${sudoString} apt -y update
${sudoString} apt -y install libgstreamer-plugins-base1.0-dev
${sudoString} apt -y install libopencv-dev python3-opencv
${sudoString} apt -y install wget
${sudoString} apt -y install libnvinfer-bin
../../plugins/toolkit/toolkit-build.sh
# download video
if [ -e "bosch.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase6/bosch.mp4
fi

if [ ! -d "models" ];
        then 
            message_out "Start download directory..."
            wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase6/models.zip
            sudo apt-get -y install unzip
            unzip models.zip
            rm models.zip
fi

 if [ ! -f "models/yolov4-416.engine" ]; 
 then

  message_out "Start convert onnx model to tensorrt..."
      /usr/src/tensorrt/bin/trtexec --onnx=./models/yolov4-416.onnx --buildOnly  --saveEngine=./models/yolov4-416.engine        
  fi  

  
../../plugins/toolkit/toolkit-build.sh 



message_out "Install related python package"
pip3 install -r requirements.txt
${sudoString} apt -y install gstreamer1.0-libav
${sudoString} apt-get -y install espeak


