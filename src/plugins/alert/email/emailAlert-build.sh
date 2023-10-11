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


# if the python file has already exist in eva plugins/python folder, remove it
message_out "check python plugings folder..."
if [ -f "/opt/adlink/eva/plugins/python/emailAlert.py" ]
then
    message_out "/opt/adlink/eva/plugins/python/emailAlert.py exist, will remove it first."
    ${sudoString} rm /opt/adlink/eva/plugins/python/emailAlert.py
    message_out "/opt/adlink/eva/plugins/python/emailAlert.py is removed."
else
    message_out "/opt/adlink/eva/plugins/python/emailAlert.py does not exist, will copy it"
fi

# Start copy python plugings
message_out "Start to copy python email_alert plugin to eva plugins/python folder..."
${sudoString} cp $(get_script_dir)/emailAlert.py /opt/adlink/eva/plugins/python/.
message_out "Copy done."

# re-source the eva environment setup_eva_envs.sh to scan the python plugins
message_out "start re-source ......"
source /opt/adlink/eva/scripts/setup_eva_envs.sh


message_out "Copy and re-source process completed!"
