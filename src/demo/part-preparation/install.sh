#!/bin/bash

# ModelNetwork=${1:-ssd_mobilenet};
# modelPruned=${1:-no_pruned}
modelPruned="no_pruned"

# echo "ModelNetwork: $ModelNetwork";

# GPU architecture checker
# gpuArchChecker=${1:-jetson}
gpuArchChecker="jetson"

# echo "OPTIND starts at $OPTIND"
while getopts "m:g:?" optname
  do
    case "$optname" in
      "m")
#         echo "Option $optname is specified"
#         VAR1=$OPTARG
        modelPruned=$OPTARG
        ;;
      "g")
#         echo "Option $optname has value $OPTARG"
        gpuArchChecker=$OPTARG
        ;;
      "?")
        echo "Unknown option $OPTARG"
        ;;
    esac
  done


# echo "modelPruned = ${modelPruned}"
# echo "gpuArchChecker = ${gpuArchChecker}"
# echo 
# exit

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

# Get current path
current_path=`pwd`

message_out "Start installing..."
# build required plugin
message_out "Building assembly plugin..."
sudo apt -y install libgstreamer-plugins-base1.0-dev
../../plugins/assembly/assembly-build.sh


# ####
# download video
if [ -e "material-preparation.avi" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [material-preparation.avi]..."
    wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/material-preparation.avi
fi
if [ -e "disassembly.avi" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [disassembly.avi]..."
    wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/disassembly.avi
fi
if [ -e "order-incorrect.avi" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [order-incorrect.avi]..."
    wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/order-incorrect.avi
fi
# # download video area define file
# if [ -e "alert-def-area-geo.txt" ]
# then
#     message_out "alert-def-area-geo.txt exists, skip."
# else
#     message_out "Start download area file..."
#     wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/alert-def-area-geo.txt
# fi

# download model and label zip file
if [ -e "models.zip" ]
then
    message_out "model.zip exists, skip."
else
    message_out "Start download model.zip..."
    wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/model.zip
    # unzip it, then delete the zip file
    sudo apt-get -y install unzip
    unzip -o model.zip
    rm model.zip
fi

# adjust cmake version (etlt required cmake version >= 3.13)
cmake_major_vrsion=$(cmake --version | grep "cmake version" | cut -c 1-14 --complement | cut -d . -f 1)
cmake_minor_vrsion=$(cmake --version | grep "cmake version" | cut -c 1-14 --complement | cut -d . -f 2)
cmake_version="${cmake_major_vrsion}.${cmake_minor_vrsion}"
message_out "cmake version = ${cmake_version}"

## cmake_required_Version=3.13
major=$((cmake_major_vrsion))
minor=$((cmake_minor_vrsion))
upgrade_cmake=0
if [ $major -lt 3 ]
then 
    message_out "version < 3.13"
    let upgrade_cmake=1
else
    if [ $major -eq 3 ]
    then
        if [ $minor -lt 13 ]
        then
            message_out "version < 3.13"
            let upgrade_cmake=1
        else
            message_out "version >= 3.13"
        fi
    else
        message_out "version >= 3.13"
    fi
fi

if [ $upgrade_cmake -eq 1 ]
then
    message_out "Start upgrading cmake to version 3.13"
    sudo apt remove --purge --auto-remove cmake
    wget https://github.com/Kitware/CMake/releases/download/v3.13.5/cmake-3.13.5.tar.gz
    tar xvf cmake-3.13.5.tar.gz
    cd cmake-3.13.5/
    ./configure
    make -j$(nproc)
    sudo make install
    sudo ln -s /usr/local/bin/cmake /usr/bin/cmake
fi

# check device GPU architecture by https://github.com/jetsonhacks/jetsonUtilities
cd $current_path
GPU_ARCHS=""
tensorRT_version=""
if [ $gpuArchChecker == "jetson" ]
then
    if [ -e "jetsonUtilities" ]
    then
        rm -fr jetsonUtilities
    fi
    git clone https://github.com/jetsonhacks/jetsonUtilities
    cd jetsonUtilities/
    GPU_ARCHS=$(python jetsonInfo.py | grep "CUDA Architecture" | cut -c 1-22 --complement | tr -d .)
    tensorRT_version=$(python jetsonInfo.py | grep "TensorRT" | cut -c 1-11 --complement | | cut -d . -f 1-3)
    
elif [ $gpuArchChecker == "x86" ]
then
    message_out "x86 version of this demo is not support currently."
    exit
else
    message_out "not support GPU architecture."
    exit
fi

cd $current_path
message_out "GPU_ARCHS = ${GPU_ARCHS}"
arch_array=("53" "61" "62" "70" "72" "75" "86")

if [[ " ${arch_array[*]} " =~ " ${GPU_ARCHS} " ]]; then
    message_out "supported arch and start to rebuild tensorrt..."
    
#     if [ -e "TensorRT" ]
#     then
#         rm -fr TensorRT
#     fi
#     
#     git clone -b 21.03 https://github.com/nvidia/TensorRT
#     cd TensorRT/
#     git submodule update --init --recursive
#     export TRT_SOURCE=`pwd`
#     cd $TRT_SOURCE
#     mkdir -p build && cd build
#     cmake .. -DGPU_ARCHS=$GPU_ARCHS -DTRT_LIB_DIR=/usr/lib/aarch64-linux-gnu/ -DCMAKE_C_COMPILER=/usr/bin/gcc -DTRT_BIN_DIR=`pwd`/out
# make nvinfer_plugin -j$(nproc)

    message_out "Backup original libnvinfer_plugin.so.7.x.y and replacing with the rebuild one"
    original_plugin_name=$(ls /usr/lib/aarch64-linux-gnu | grep libnvinfer_plugin.so.${tensorRT_version})
    echo $original_plugin_name
else
    message_out "Not supported arch and exit installation"
    exit
fi


# # download model
# if [ $ModelNetwork == "ssd_mobilenet" ]
# then
#     if [ -e "mobilenetSSDv2_geofencing.engine" ]
#     then
#         message_out "Model exists, skip."
#     else
#         if [ -e "geo_fencing_ssd_v2.uff" ]
#         then 
#             message_out "uff model file exists."
#         else    
#             message_out "Start download model..."
#             wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/geo_fencing_ssd_v2.zip
#             # unzip it, then delete the zip file
#             sudo apt-get -y install unzip
#             unzip geo_fencing_ssd_v2.zip
#             rm geo_fencing_ssd_v2.zip
#         fi
# 
#         message_out "Start convert uff model to tensorrt..."    
#         ../../scripts/optimize_ssd_mobilenet.sh geo_fencing_ssd_v2.uff mobilenetSSDv2_geofencing.engine
#         message_out "Convert Done."
#     fi
# elif [ $ModelNetwork == "yolov3" ]
# then
#     if [ -e "adlink-yolov3-geo-fencing.engine" ]
#     then
#         message_out "Yolov3 model exists, skip."
#     else
#         if [ -e "adlink-yolov3-geo-fencing.onnx" ]
#         then 
#             message_out "onnx model file exists."
#         else    
#             message_out "Start download model..."
#             wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-yolov3-geo-fencing.zip
#             # unzip it, then delete the zip file
#             sudo apt-get -y install unzip
#             unzip adlink-yolov3-geo-fencing.zip
#             rm adlink-yolov3-geo-fencing.zip
#         fi
# 
#         message_out "Start convert onnx model to tensorrt..."
#         ../../scripts/optimize_yolov3.sh adlink-yolov3-geo-fencing.onnx adlink-yolov3-geo-fencing.engine        
#         message_out "Convert Done."
#     fi
# else
#     message_out "Unsupported Model: $ModelNetwork" 
# fi
# # download model label file
# if [ $ModelNetwork == "ssd_mobilenet" ]
# then
#     if [ -e "adlink-mobilenetSSDv2-geo-fencing-label.txt" ]
#     then
#         message_out "adlink-mobilenetSSDv2-geo-fencing-label.txt exists, skip."
#     else
#         message_out "Start download label file..."
#         wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-mobilenetSSDv2-geo-fencing-label.txt
#     fi
# elif [ $ModelNetwork == "yolov3" ]
# then
#     if [ -e "adlink-yolov3-geo-fencing-label.txt" ]
#     then
#         message_out "adlink-yolov3-geo-fencing-label.txt exists, skip."
#     else
#         message_out "Start download label file..."
#         wget http://ftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase1/adlink-yolov3-geo-fencing-label.txt
#     fi
# fi
# # python plugin
# message_out "Deploy alert plugin..."
# ../../plugins/alert/email/emailAlert-build.sh
# ../../plugins/alert/voice/voiceAlert-build.sh
# 
# message_out "Install related python package"
# pip3 install -r requirements.txt
# sudo apt -y install gstreamer1.0-libav
# sudo apt-get -y install espeak
# rm ~/.cache/gstreamer-1.0/regi*
# 
# message_out "Installation completed, you could run this demo by execute ./run.sh"
# message_out "** run mobilenetSSDv2, execute run-win.bat directly."
# message_out "** run yolov3, run run-win.bat with argument yolov3."
