# Hello plugIN
A Sample Allxon plugIN to help you understand Allxon plugIN framework.

# Run Pre-Build Executable

## Install Allxon Agent
plugIN infrastructure is base on Allxon Agent, You have to [install](https://www.allxon.com/knowledge/install-allxon-agent-via-command-prompt) first.

## Download plugIN package
Get your platform archive from [release page](https://github.com/allxon/plugIN-hello/releases).

## Extract and Run
After downloaded archive, extract your archive and run it
```bash
cd [EXTRACT_FOLDER_PATH]/[APP_GUID]
./plugin-hello $(pwd)
```
or Windows cmd.
```batch
cd [EXTRACT_FOLDER_PATH]\[APP_GUID]
plugin-hello.exe %cd%
```

# Build From Source
## Clone all Submodule
```bash
git clone --recurse-submodules https://github.com/allxon/plugIN-hello.git
```

## Get plugIN Key
_plugIN key_ repesent your plugIN identity, get your plugIN Key `plugin_key.json` from Allxon and Download in working directory, you should have `APP_GUID`, `ACCESS_KEY` in your `plugin_key.json`
> **WARNING**: Each `plugin_key.json` bind one plugIN program, if your plugIN deploy different platform or different cpu architecture, means your need different `plugin_key.json` 

## Docker build
```bash
sudo docker build -f <Dockerfile.x86_64|Dockerfile.aarch64> .
```

## Install CMake
- CMake 3.23 - [install link](https://cmake.org/download/)

## Dependency
- [Allxon plugIN SDK](https://github.com/allxon/plugIN-sdk-v2)
- OpenSSL

It will auto fetch _plugIN SDK_ library when you configure cmake.

### Linux Debian
Install OpenSSL in Linux Debian.

```bash
apt-get update && apt-get install libssl-dev
```

### Other Linux Distribution 
Follow install instruction on [OpenSSL offical site](https://www.openssl.org).

### Windows
It will auto fetch OpenSSL library when you configure cmake.

You can found `.dll` under `build/_deps/openssl_src/bin`.

## Build & Run

### Linux
```bash
# Configuration Stage
cmake -S . -B build -DCMAKE_BUILD_TYPE=<Debug|Release> -DPLUGIN_KEY=plugin_key.json

# Build Stage
cmake --build build

# Run after build
# You can run plugin-hello directly under build/ folder, and pass resource_dir_linux through argument
build/plugin-hello resource_dir_linux

# Install Stage
cmake --install build --prefix /opt/allxon/plugIN

# Run after Install
# and pass resource_dir_linux or installed directory through argument
/opt/allxon/plugIN/[APP_GUID]/plugin-hello /opt/allxon/plugIN/[APP_GUID]
```

### Windows
```batch
@REM Configuration Stage
cmake -G "Visual Studio 16 2019" -A Win32 -S . -B "build" -DPLUGIN_KEY=plugin_key.json

@REM Build Stage
cmake --build build --config <Debug|Release>

@REM Run after build
@REM You can run plugin-hello directly under build\ folder, and pass resource_dir_windows through argument
build\<Debug|Release>\plugin-hello.exe resource_dir_windows

@REM Install Stage
cmake --install build --prefix C:\ProgramData\allxon\plugIN

@REM Run after Install
@REM and pass resource_dir_windows or installed directory through argument
C:\ProgramData\allxon\plugIN\[APP_GUID]\plugin-hello.exe C:\ProgramData\allxon\plugIN\[APP_GUID]
```

# How to Deploy & Install
_plugIN package_ is a archive repesent a plugIN in Allxon plugIN ecosystem.

A plugIN package filename is named to `plugin-hello-[version]-linux-[arch].tar.gz` (linux) or `plugin-hello-[version]-win-[arch].zip` (windows). 

## Deploy plugIN packge 

### Linux
```bash
# Deploy through docker, then you can get your plugIN package under OUTPUT_DIRECTORY 
sudo docker build -f <Dockerfile.aarch64|Dockerfile.x86_64> -o [OUTPUT_DIRECTORY] .
```
```bash
# Deploy through cmake, then you can get your plugIN package under build directory
cmake --build build --target package
```

### Windows
```bash
# Deploy through cmake, then you can get your plugIN package under build directory
cmake --build build --config <release|debug> --target package
```

## Test plugIN Installation through plugIN online installer
After packing plugIN package, you can test your plugIN package on local before upload to Allxon plugIN ecosystem. 

### Linux
```bash
sudo wget -qO - https://get.allxon.net/plugIN/linux | sudo  bash -s -- --app-guid [APP_GUID] --from-path [PLUGIN_PACKAGE]
``` 

### Windows
```batch
powershell -command "Invoke-WebRequest -OutFile %temp%\plugin-installer.bat https://get.allxon.net/plugIN/windows" && %temp%\plugin-installer.bat --app-guid [APP_GUID] --from-path [PLUGIN_PACKAGE]
```

After installed, plugIN will start automatically.

If you wanna uninstall plugIN, use following command
 
### Linux 
```bash
sudo wget -qO - https://get.allxon.net/plugIN/linux | sudo bash -s -- --app-guid [APP_GUID] --remove
``` 

### Windows
```batch
powershell -command "Invoke-WebRequest -OutFile %temp%\plugin-installer.bat https://get.allxon.net/plugIN/windows" && %temp%\plugin-installer.bat --app-guid [APP_GUID] --remove
```

# Getting Start
After configure cmake, cmake would create `WORKSPACE/build/build_info.h`, so you can use following predefined marco:
- `PLUGIN_NAME` 
- `PLUGIN_APP_GUID`
- `PLUGIN_ACCESS_KEY`
- `PLUGIN_VERSION`

```cpp
#include <string>
#include "build_info.h"
#include "json_validator.h"

int main(int argc, char **argv)
{
    // notifyPluginUpdate json template
    std::string json_content = "{\"jsonrpc\": \"2.0\", \"method\": \"v2/notifyPluginUpdate\"...}"; 
    auto json_validator = JsonValidator(PLUGIN_NAME, PLUGIN_APP_GUID,
                                        PLUGIN_ACCESS_KEY, PLUGIN_VERSION,
                                        json_content); 
    
    std::string other_plugin_api_json_content;
    if (json_validator.Sign(other_plugin_api_json_content))
    {
        // if sign success, you can send it through websocket
        // e.g. 
        // enpoint.send(other_plugin_api_json_content);
    }

    if (json_validator.Verify(other_plugin_api_json_content))
    {
        // if verify success, means json content is safe, you can read it
    }
    return 0;
}
```