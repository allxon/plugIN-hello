#!/bin/bash
CURRENT_SH_DIRECTORY=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
exec &> "${CURRENT_SH_DIRECTORY}/$(basename "${BASH_SOURCE[0]%.*}").output"

PLUGIN_NAME=plugin-hello
PLUGIN_SERVICE=${PLUGIN_NAME}.service
PLUGIN_APP_GUID=${ALLXON_PLUGIN_DIR##*/}

remove_plugin() {
    echo "remove plugin..."
    rm -rf ${ALLXON_PLUGIN_DIR} || exit 1
}

uninstall_plugIN() {
    if [ -f "/etc/systemd/system/${PLUGIN_SERVICE}" ]; then
        systemctl stop ${PLUGIN_SERVICE} 
        systemctl disable ${PLUGIN_SERVICE} || exit 1  
        rm /etc/systemd/system/${PLUGIN_SERVICE} || exit 1   
    else
        echo "/etc/systemd/system/${PLUGIN_SERVICE} not exist." 
    fi
    remove_plugin
    exit 0
}

uninstall_plugIN
