#!/usr/bin/bash -ex

NINJA_VERSION=$1
cputype=$(uname -m)

case "$cputype" in
aarch64 | arm64)
    cputype=aarch64
    ;;
x86_64 | x86-64 | x64 | amd64)
    cputype=x86_64
    ;;
*)
    err "unsupport CPU type: $cputype"
    ;;
esac

TARGET_TAR=ninja_${NINJA_VERSION}_${cputype}.tar.gz
wget --no-check-certificate https://dev.allxon.net/tool/${TARGET_TAR}
tar -xzf ${TARGET_TAR} -C /usr/bin