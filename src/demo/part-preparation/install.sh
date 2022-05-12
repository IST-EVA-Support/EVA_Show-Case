#!/bin/bash

modelPruned="no_pruned"

# GPU architecture checker
gpuArchChecker="jetson"

# echo "OPTIND starts at $OPTIND"
while getopts "m:g:?" optname
  do
    case "$optname" in
      "m")
#         echo "Option $optname is specified"
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

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

# Get current path
current_path=`pwd`


# Check is in ADLINK EVA container
sudoString="sudo"
if [ -e "/entrypoint.sh" ]
then
    sudoString=" "
fi
message_out "sudo String = ${sudoString}"


message_out "Start installing..."
# build required plugin
message_out "Building assembly plugin..."
${sudoString} apt -y install libgstreamer-plugins-base1.0-dev
../../plugins/assembly/assembly-build.sh


# ####
# download video
if [ -e "material-preparation.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [material-preparation.mp4]..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/material-preparation.mp4
fi
if [ -e "disassembly.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [disassembly.mp4]..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/disassembly.mp4
fi
if [ -e "order-incorrect.mp4" ]
then
    message_out "Demo video exists, skip."
else
    message_out "Start download demo video [order-incorrect.mp4]..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/order-incorrect.mp4
fi


# download model and label zip file
if [ -e "models.zip" ]
then
    message_out "model.zip exists, skip."
else
    message_out "Start download model.zip..."
    wget https://sftp.adlinktech.com/image/EVA/EVA_Show-Case/showcase4/model.zip
    # unzip it, then delete the zip file
    ${sudoString} apt-get -y install unzip
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
    ${sudoString} apt remove --purge --auto-remove cmake
    wget https://github.com/Kitware/CMake/releases/download/v3.13.5/cmake-3.13.5.tar.gz
    tar xvf cmake-3.13.5.tar.gz
    cd cmake-3.13.5/
    ./configure
    make -j$(nproc)
    ${sudoString} make install
    ${sudoString} ln -s /usr/local/bin/cmake /usr/bin/cmake
fi

# check device GPU architecture
cd $current_path
GPU_ARCHS=""
tensorRT_version=""
cuda_runtime_version=""
if [ $gpuArchChecker == "jetson" ]
then
    # check Jetson GPU architecture by https://github.com/jetsonhacks/jetsonUtilities
    if [ -e "jetsonUtilities" ]
    then
        rm -fr jetsonUtilities
    fi
    git clone https://github.com/jetsonhacks/jetsonUtilities
    cd jetsonUtilities/
    #GPU_ARCHS=$(python jetsonInfo.py | grep "CUDA Architecture" | cut -c 1-22 --complement | tr -d .)
    if [[ $(python jetsonInfo.py | grep -i "Xavier") != "" ]]
    then
        GPU_ARCHS="72"
    elif [[ $(python jetsonInfo.py | grep -i "TX2") != "" ]]
    then
        GPU_ARCHS="62"
    elif [[ $(python jetsonInfo.py | grep -i "Nano") != "" ]]
    then
        GPU_ARCHS="53"
    elif [[ $(python jetsonInfo.py | grep -i "TX1") != "" ]]
    then
        GPU_ARCHS="53"
    else
        message_out "Can not know the Jetson platform, and will exit"
        exit
    fi
    
    
    #tensorRT_version=$(python jetsonInfo.py | grep "TensorRT" | cut -c 1-11 --complement | cut -d . -f 1-3)
    tensorRT_version=$(dpkg -l | grep "TensorRT runtime libraries" | awk '{print $3}' | cut -f 1 -d "+" | cut -f 1 -d "-")
    jetson_name=$(python jetsonInfo.py | grep "NVIDIA Jetson" | cut -c 1-14 --complement)
    jetpack_version=$(python jetsonInfo.py | grep "JetPack" | cut -c 1-22,26-27 --complement | tr -d .)
    cd ..
    rm -fr jetsonUtilities
    
elif [ $gpuArchChecker == "x86" ]
then
    # check x86 GPU architecture by deviceQuery
    #message_out "x86 version of this demo is not support currently."
    message_out "Check x86 device GPU architecture"
    git clone https://github.com/NVIDIA-AI-IOT/deepstream_tao_apps.git
    cd deepstream_tao_apps/TRT-OSS/x86
    # source the nvcc path
    export PATH=/usr/local/cuda/bin:$PATH
    
    nvcc deviceQuery.cpp -o deviceQuery
    GPU_ARCHS=$(./deviceQuery | grep "CUDA Capability Major/Minor version number:" | cut -c 1-49 --complement | tr -d .)
    cuda_runtime_version=$(./deviceQuery | grep "CUDA Driver Version / Runtime Version" | cut -c 1-56 --complement | tr -d .)
    message_out "x86 cuda_runtime_version = ${cuda_runtime_version}"
    cd $current_path
    rm -fr deepstream_tao_apps
    
    #Get TensorRT version in x86
    tensorRT_version=$(dpkg -l | grep "ii  tensorrt" | cut -c 1-12 --complement | xargs | cut -d - -f 1 | cut -d . -f 1-3)
    message_out "tensorRT_version = ${tensorRT_version}"
else
    message_out "not support GPU architecture."
    exit
fi

message_out "GPU_ARCHS = ${GPU_ARCHS}"
message_out "tensorRT_version = ${tensorRT_version}"
message_out "cuda_runtime_version = ${cuda_runtime_version}"
message_out "jetson_name = ${jetson_name}"

cd $current_path
message_out "GPU_ARCHS = ${GPU_ARCHS}"
arch_array=("53" "61" "62" "70" "72" "75" "86")

if [[ " ${arch_array[*]} " =~ " ${GPU_ARCHS} " ]]; then
    message_out "supported arch and start to rebuild tensorrt..."
    
    if [ -e "TensorRT" ]
    then
        rm -fr TensorRT
    fi
    
    if [ $tensorRT_version == "7.1.3" ]
    then
        #git clone -b 21.03 https://github.com/nvidia/TensorRT
        git clone -b 20.09 https://github.com/nvidia/TensorRT
    elif [ $tensorRT_version == "8.2.1" ]
    then
        git clone -b 22.04 https://github.com/nvidia/TensorRT
    else
        message_out "This installation currently support TensorRT 7.1.3 and 8.2.1. And will exit the installation."
        exit
    fi
    cd TensorRT/
    git submodule update --init --recursive
    export TRT_SOURCE='pwd'
    cd $TRT_SOURCE
    mkdir -p build && cd build
    
    # Start to rebuild TensorRT OSS(Open Source Software)
    #x86/jetson use same TRT_LIB_DIR
    if [ $tensorRT_version == "7.1.3" ]
    then
        cmake .. -DGPU_ARCHS=$GPU_ARCHS -DTRT_LIB_DIR=/usr/lib/aarch64-linux-gnu/ -DCMAKE_C_COMPILER=/usr/bin/gcc -DTRT_BIN_DIR='pwd'/out
        make nvinfer_plugin -j$(nproc)
    elif [ $tensorRT_version == "8.2.1" ]
    then
        cmake .. -DGPU_ARCHS=$GPU_ARCHS  -DTRT_LIB_DIR=/usr/lib/aarch64-linux-gnu/ -DCMAKE_C_COMPILER=/usr/bin/gcc -DTRT_BIN_DIR='pwd'/out -DCUDA_VERSION=10.2 -DCMAKE_PREFIX_PATH=/usr/local/cuda-10.2
        make nvinfer_plugin -j$(nproc)
        if [ -e "libnvinfer_plugin.so.8.2.4" ]
        then
            cp libnvinfer_plugin.so.8.2.4 libnvinfer_plugin.so.${tensorRT_version}
        else
            message_out "There does not exit libnvinfer_plugin.so.${tensorRT_version}, might built error. And will exit"
            exit
        fi
    else
        message "TensorRT version does not match and will exit installation."
        exit
    fi
    
    message_out "Build OSS finished, and exit."
    
    message_out "Backup original libnvinfer_plugin.so.7.x.y/libnvinfer_plugin.so.8.x.y and replacing with the rebuild one"
    backup_folder=${HOME}/libnvinfer_plugin_bak
    backup_file=$(date +'%Y-%m-%d_%H-%M-%S')
#     backup_file_path="${backup_folder}/${original_plugin_name}_${backup_file}.bak"
    if [ ! -e backup_folder ]
    then
        mkdir $backup_folder
    fi
    
    # move file then copy file
    if [ $gpuArchChecker == "jetson" ]
    then
        original_plugin_name=$(ls /usr/lib/aarch64-linux-gnu | grep libnvinfer_plugin.so.${tensorRT_version})
        backup_file_path="${backup_folder}/${original_plugin_name}_${backup_file}.bak"
        message_out "original_plugin_name = ${original_plugin_name}"
        message_out "backup_file_path = ${backup_file_path}"
        #backup original libnvinfer_plugin.so.x.y to backup folder
        #check original_plugin_name is not "" to prevent copy system folder
        original_plugin_path=/usr/lib/aarch64-linux-gnu/$original_plugin_name
        if [ ! -d $original_plugin_path ]
        then
            ${sudoString} mv $original_plugin_path $backup_file_path
        fi
        
        #copy rebuild one
        rebuild_file=""
        if [ $tensorRT_version == "7.1.3" ]
        then
            rebuild_file=$(ls | grep libnvinfer_plugin.so.7.*)
            message_out "rebuild file in jetson = ${rebuild_file}"
        elif [ $tensorRT_version == "8.2.1" ]
        then
            rebuild_file=$(ls | grep libnvinfer_plugin.so.8.*)
            message_out "rebuild file in jetson = ${rebuild_file}"
        else
            message_out "No rebuild file exist and will exit"
            exit
        fi
        
        if [ -e $rebuild_file ]
        then
            ${sudoString} cp $rebuild_file $original_plugin_path
        fi
        ${sudoString} ldconfig
        
    elif [ $gpuArchChecker == "x86" ]
    then
        original_plugin_name=$(ls /usr/lib/x86_64-linux-gnu | grep libnvinfer_plugin.so.${tensorRT_version})
        backup_file_path="${backup_folder}/${original_plugin_name}_${backup_file}.bak"
        message_out "original_plugin_name = ${original_plugin_name}"
        message_out "backup_file_path = ${backup_file_path}"
        #backup original libnvinfer_plugin.so.x.y to backup folder
        #check original_plugin_name is not "" to prevent copy system folder
        original_plugin_path=/usr/lib/x86_64-linux-gnu/$original_plugin_name
        if [ ! -d $original_plugin_path ]
        then
            #if exist libnvinfer_plugin.so.x.y, backup it
            ${sudoString} mv $original_plugin_path $backup_file_path
        fi
        
        #copy rebuild one
        rebuild_file=$(ls | grep libnvinfer_plugin.so.7.*)
        if [ -d $original_plugin_path ]
        then
            #if not exist libnvinfer_plugin.so.x.y in system, modify the $original_plugin_path
            message_out "before modify, original_plugin_path = ${original_plugin_path}"
            original_plugin_path=/usr/lib/x86_64-linux-gnu/$rebuild_file 
            message_out "after modify, original_plugin_path = ${original_plugin_path}"
        fi
        message_out "rebuild file in x86 = ${rebuild_file}"
        
        if [ -e $rebuild_file ]
        then
            message_out "rebuild_file = ${rebuild_file}"
            message_out "original_plugin_path = ${original_plugin_path}"
            ${sudoString} cp $rebuild_file $original_plugin_path
        fi
        ${sudoString} ldconfig
    else
        message_out "Not x86 or Jetson device."
        exit
    fi

else
    message_out "Not supported arch and exit installation"
    exit
fi

# Install TAO Converter to convert etlt file to engine file
message_out "Start to Converting..."
cd $current_path
arch_jetson_name=("TX2" "Xavier NX (Developer Kit Version)" "AGX Xavier [32GB]")
message_out "jetson_name = ${jetson_name}"
if [[ " ${arch_jetson_name[*]} " =~ " ${jetson_name} " ]]; then
    if [ $jetpack_version == "44" ]
    then
        message_out "supported jetson device, jetpack 4.4 and start to convert etlt file..."
        #download tao-converter binary
        wget https://developer.nvidia.com/cuda102-trt71-jp44-0
        
        # unzip it, then delete the zip file
        ${sudoString} apt-get -y install unzip
        unzip -o cuda102-trt71-jp44-0
        rm cuda102-trt71-jp44-0
        cd jp4.4
        ${sudoString} chmod +x tao-converter
        
        #Install openssl library
        ${sudoString} apt-get -y install libssl-dev
        #Export the following environment variables
        export TRT_LIB_PATH=”/usr/lib/aarch64-linux-gnu”
        export TRT_INC_PATH=”/usr/include/aarch64-linux-gnu”
        
        message_out "Converting original model..."
        ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_3400_fp32.engine ../model/dssd_resnet18_epoch_3400.etlt
        
        message_out "Converting pruned model..."
        ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_810_fp32.engine ../model/dssd_resnet18_epoch_810.etlt
    elif [ $jetpack_version == "45" ]
    then
        message_out "supported jetson device, jetpack 4.5 and start to convert etlt file..."
        #download tao-converter binary
        wget https://developer.nvidia.com/tao-converter-jp4.5
        
        # unzip it, then delete the zip file
        ${sudoString} apt-get -y install unzip
        unzip -o tao-converter-jp4.5
        rm tao-converter-jp4.5
        cd jp4.5
        ${sudoString} chmod +x tao-converter
        
        #Install openssl library
        ${sudoString} apt-get -y install libssl-dev
        #Export the following environment variables
        export TRT_LIB_PATH=”/usr/lib/aarch64-linux-gnu”
        export TRT_INC_PATH=”/usr/include/aarch64-linux-gnu”
        
        message_out "Converting original model..."
        ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_3400_fp32.engine ../model/dssd_resnet18_epoch_3400.etlt
        
        message_out "Converting pruned model..."
        ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_810_fp32.engine ../model/dssd_resnet18_epoch_810.etlt
    elif [ $jetpack_version == "UNKNOWN" ]
    then
        if [ $sudoString == " " ] # in EVA container
        then
            message_out "supported jetson device, jetpack 4.6 and start to convert etlt file..."
            #download tao-converter binary
            wget https://developer.nvidia.com/tao-converter-jp4.6
            
            # unzip it, then delete the zip file
            ${sudoString} apt-get -y install unzip
            unzip -o tao-converter-jp4.6
            rm tao-converter-jp4.6
            cd tao-converter-jp46-trt8.0.1.6
            ${sudoString} chmod +x tao-converter
            
            #Install openssl library
            ${sudoString} apt-get -y install libssl-dev
            #Export the following environment variables
            export TRT_LIB_PATH=”/usr/lib/aarch64-linux-gnu”
            export TRT_INC_PATH=”/usr/include/aarch64-linux-gnu”
            
            message_out "Converting original model..."
            ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_3400_fp32.engine ../model/dssd_resnet18_epoch_3400.etlt
            
            message_out "Converting pruned model..."
            ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_810_fp32.engine ../model/dssd_resnet18_epoch_810.etlt
        else
            message_out "jetson device but not in container and will exit."
            exit
        fi
    else
        message_out "supported jetson device, but does not support this jetpack version: ${$jetpack_version}"
    fi
elif [ $gpuArchChecker == "x86" ]
then
    message_out "=======processing x86 convert============"
    #message_out "x86 cuda_runtime_version = ${cuda_runtime_version}"
    #message_out "tensorRT_version = ${tensorRT_version}"
    #download tao-converter binary, only support cuda10.2/cudnn8.0/tensorrt7.1
    wget https://developer.nvidia.com/cuda102-trt71
    
    # unzip it, then delete the zip file
    ${sudoString} apt-get -y install unzip
    unzip -o cuda102-trt71
    rm cuda102-trt71
    cd cuda10.2-trt7.1
    ${sudoString} chmod +x tao-converter
    
    #Install openssl library
    ${sudoString} apt-get -y install libssl-dev
    #Export the following environment variables
    export TRT_LIB_PATH=”/usr/lib/x86_64-linux-gnu”
    export TRT_INC_PATH=”/usr/include/x86_64-linux-gnu”
    
    message_out "Converting original model..."
    ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_3400_fp32.engine ../model/dssd_resnet18_epoch_3400.etlt
        
    message_out "Converting pruned model..."
    ./tao-converter -k NTBzNmJ0b2s3a3VpbGxhNjBqNDN1bmU4Y2o6MDY4YjM3NmUtZTIxYy00ZjQ5LWIzMTYtMmRiNmJhMDBiOGVm -d 3,512,512 -o NMS -m 1 -e ../model/dssd_resnet18_epoch_810_fp32.engine ../model/dssd_resnet18_epoch_810.etlt
else
    message_out "Not supported device platform."
fi



# python plugin
cd $current_path
message_out "Deploy alert plugin..."
../../plugins/alert/email/emailAlert-build.sh
../../plugins/alert/voice/voiceAlert-build.sh

message_out "Install related python package"
pip3 install -r requirements.txt
${sudoString} apt -y install gstreamer1.0-libav
${sudoString} apt-get -y install espeak
rm ~/.cache/gstreamer-1.0/regi*

message_out "Installation completed, you could run this demo by execute ./run.sh"
