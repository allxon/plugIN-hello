#!/bin/bash -ex

# The script is used to create a release branch. It takes the version number as an argument and 
# updates the version number in the CMakeLists.txt file and the installer files. It then creates 
# a new branch with the version number as the branch name and pushes the changes to the remote 
# repository. The script is executed as follows:
#  
# ./make_release_branch.sh 1.0.0
# The script will create a new branch named 1.0.0 and push the changes to the remote repository. 

help() {
    echo "Usage: $0 <Version>"
    echo ""
    echo "Example: $0 1.0.0"
    exit 1
}

if [ $# -ne 1 ]; then
    echo "Invalid number of arguments"
    help
fi

VERSION=$1

if [ -z "${VERSION}" ]; then
    echo "Invalid version"
    help
fi

sed -i -E "s/project\((.+) VERSION (.+)\)/project\(\1 VERSION $VERSION\)/g" "CMakeLists.txt"

git checkout -b ${VERSION}
git add CMakeLists.txt
git commit -m "Release ${VERSION}"
 