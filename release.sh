#!/bin/bash
TARGET_VERSION=$1
REGEX="project\((.+) VERSION (.+)\)"

# Find version part
cat CMakeLists.txt | while read LINE; do
    if [[ $LINE =~ $REGEX ]]; then
        echo ${BASH_REMATCH[2]} > CURRENT_VERSION.tmp
    fi
done

CURRENT_VERSION=$(cat CURRENT_VERSION.tmp)
if [[ $CURRENT_VERSION == "" ]]; then
    echo "Can't found version"
    rm CURRENT_VERSION.tmp
    exit 1
fi

if [[ $CURRENT_VERSION == $TARGET_VERSION ]]; then
    echo "Version already matched, don't need to update"
    rm CURRENT_VERSION.tmp
    exit 0
else
    echo "Current version: $CURRENT_VERSION replace to Target version: $TARGET_VERSION"
    sed -i -E "s/project\((.+) VERSION (.+)\)/project\(\1 VERSION $TARGET_VERSION\)/g" CMakeLists.txt
fi

rm CURRENT_VERSION.tmp