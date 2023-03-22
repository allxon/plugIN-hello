#!/usr/bin/bash -ex

CMAKE_VERSION=$1
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

wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-${cputype}.tar.gz
tar -xf cmake-${CMAKE_VERSION}-linux-${cputype}.tar.gz && cp -r cmake-${CMAKE_VERSION}-linux-${cputype}/* /usr