#!//bin/bash

LB='\033[1;33m'
NC='\033[0m' # No Color

# echo message with color
message_out(){
    echo -e "${LB}=> $1${NC}"
}

get_script_dir () {
    SOURCE="${BASH_SOURCE[0]}"
    # While $SOURCE is a symlink, resolve it
    while [ -h "$SOURCE" ]; do
        DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
        SOURCE="$( readlink "$SOURCE" )"
        # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
        [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
    done
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    echo "$DIR"
}

# Check is in ADLINK EVA container
sudoString="sudo"
if [ -e "/entrypoint.sh" ]
then
    sudoString=" "
fi
message_out "sudo String = ${sudoString}"


# if build folder exist, delete it
message_out "check ./build folder..."
if [ -d "./build" ] 
then
    message_out "Directory ./build exists, will delete it."
    rm -fr ./build
    message_out "./build folder deleted."
else
    message_out "./build does not exist and will build it."
fi

# configure the code
message_out "start to configure by meson..."
#meson build
cd $(get_script_dir)
#meson $(get_script_dir)/build -Dopencv_dir=/opt/intel/openvino_2021/opencv/
meson $(get_script_dir)/build -Dopencv_dir=/usr

# compile the code
message_out "start to compile by ninja..."
ninja -C build

# if the build dll has already exist in eva plugins folder, remove it
message_out "check eva plugings folder..."
if [ -f "/opt/adlink/eva/plugins/libtoolkit.so" ]
then
    message_out "/opt/adlink/eva/plugins/libtoolkit.so exist, will remove it first."
    ${sudoString} rm /opt/adlink/eva/plugins/libtoolkit.so
    message_out "${sudoString} rm /opt/adlink/eva/plugins/libtoolkit.so is removed."
else
    message_out "/opt/adlink/eva/plugins/libtoolkit.so does not exist, will copy it"
fi

# copy built library to eva plugins folder
message_out "start copying to /opt/adlink/eva/plugins/ ......"
${sudoString} cp ./build/libtoolkit.so /opt/adlink/eva/plugins/.
message_out "copy done"

# clear cache of gstreamer
message_out "clear cache of gstreamer..."
if [ -f "~/.cache/gstreamer-1.0/registry*" ]
then
    message out "registry exists, will clean it."
    rm ~/.cache/gstreamer-1.0/registry*
else
    message_out "registry is clean."
fi

message_out "Build process completed!"
