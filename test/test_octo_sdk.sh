#!/bin/bash -e

get_dockerfile_by_arch() {
    local ostype="$(uname -s)"
    local cputype="$(uname -m)"
    local dockerfile_suffix=""

    if [ "$ostype" = Linux ]; then
        if [ "$(uname -o)" = Android ]; then
            ostype=Android
        fi
    fi

    case "$ostype" in
    Linux)
        ostype=linux
        ;;
    *)
        err "unsupport OS type: $ostype"
        ;;
    esac

    case "$cputype" in
    aarch64 | arm64)
        cputype=aarch64
        dockerfile_suffix="jetson"
        ;;
    x86_64 | x86-64 | x64 | amd64)
        cputype=x86_64
        dockerfile_suffix="x86_64"
        ;;
    *)
        err "unsupport CPU type: $cputype"
        ;;
    esac
    echo "Dockerfile.${dockerfile_suffix}"
}

if [[ $1 == "" ]] || [[ $2 == "" ]] || [[ $3 == "" ]]; then
    echo "Usage: $0 <app_guid> <executable_name> <sdk_version>"
    exit 1
fi

APP_GUID=$1
EXECUTABLE_NAME=$2
SDK_VERSION=$3
OUTPUT_DIR="output"
PLUGIN_PACKAGE="hello_plugin.tar.gz"
DOCKERFILE=$(get_dockerfile_by_arch)

mkdir ${OUTPUT_DIR}
docker build -f ${DOCKERFILE} --progress=plain --build-arg OCTO_SDK_VERSION=${SDK_VERSION} --build-arg OUTPUT_NAME=${PLUGIN_PACKAGE} --output ${OUTPUT_DIR} .
tar -xzf ${OUTPUT_DIR}/${PLUGIN_PACKAGE} -C ${OUTPUT_DIR}
./${OUTPUT_DIR}/${APP_GUID}/${EXECUTABLE_NAME} ${OUTPUT_DIR}/${APP_GUID}




