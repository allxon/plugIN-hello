#!/bin/bash
CURRENT_SH_DIRECTORY=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

PLUGIN_NAME=plugin-hello
PLUGIN_SERVICE=${PLUGIN_NAME}.service
PLUGIN_APP_GUID=${ALLXON_PLUGIN_DIR##*/}

if [ -d $ALLXON_PLUGIN_DIR ]; then
    echo "ERROR: plugin $PLUGIN_APP_GUID already installed"
    exit 1
else 
    mkdir -p $ALLXON_PLUGIN_DIR || exit 1
fi

check_for_install() {
    echo "check for install..."
    if ! command -v file &> /dev/null; then
        echo "Warning: file command not found, we can't help to check architecture."
        return
    else
        # If users try to install this plugIN on non-Ubuntu x86 devices, then it will be returned
        local EXECUTABLE_DESCRIPTION=$(file $CURRENT_SH_DIRECTORY/$PLUGIN_APP_GUID/$PLUGIN_NAME)
        local ARCH=$(uname -i)

        if [[ "$ARCH" == "x86_64" ]]; then
            ARCH="x86-64"
        fi

        if [[ "$EXECUTABLE_DESCRIPTION" != *"$ARCH"* ]]; then
            >&2 echo "Warning: Host CPU architecture got from 'uname -i' is not same with installing package"
        fi
    fi
}

install_plugin_files() {
    echo "install plugin files..."
    cp -r ./$PLUGIN_APP_GUID/* $ALLXON_PLUGIN_DIR || exit 1
    echo "\
[Unit]
Description=Allxon Hello plugIN
Documentation=https://dms.allxon.com/

[Service]
Type=simple
ExecStart=${ALLXON_PLUGIN_DIR}/${PLUGIN_NAME} ${ALLXON_PLUGIN_DIR}
Environment="HOME=/root"
Restart=always
RestartSec=60

[Install]
WantedBy=multi-user.target
" > ${PLUGIN_SERVICE} || exit 1

    cp ./$PLUGIN_SERVICE /etc/systemd/system/ || exit 1
    echo "plugIN is installed to $ALLXON_PLUGIN_DIR"
}

initial_plugin_service_in_system() {
    echo "start service..."
    systemctl daemon-reload || exit 1
    chmod 644 /etc/systemd/system/$PLUGIN_SERVICE || exit 1
    systemctl enable --now $PLUGIN_SERVICE || exit 1
}

install_plugIN() {
    check_for_install
    install_plugin_files
    initial_plugin_service_in_system > /dev/null 2>&1
    sleep 1
    exit 0
}

install_plugIN
