# Hello Plugin
Below is a sample plugin to help you understand Allxon Octo framework.

# Run Pre-Build Executable

## Install Allxon Agent
The plugin infrastructure is based on Allxon Agent. To run the pre-build executable, [install](https://www.allxon.com/knowledge/install-allxon-agent-via-command-prompt) Allxon Agent first. 

## Download the Plugin Package
Go to the [release page](https://github.com/allxon/plugIN-hello/releases), then download your platform archive.

## Extract and Run
After your archive is downloaded, extract and run the archive.
```bash
cd [EXTRACT_FOLDER_PATH]/[APP_GUID]
./plugin-hello $(pwd)
```
or Windows cmd.
```batch
cd [EXTRACT_FOLDER_PATH]\[APP_GUID]
plugin-hello.exe %cd%
```

# Build from Source
## Clone all Submodule
```bash
git clone --recurse-submodules https://github.com/allxon/plugIN-hello.git
```

## Obtain Plugin Credential
You need to acquire a _Plugin Credential_, which represents your plugin identity. To do so, contact us to obtain `plugin_credential.json`, which includes a set of `APP_GUID` and `ACCESS_KEY`. 
> **WARNING**: Each `plugin_credential.json` is paired with one plugin program. Different platforms or CPU architectures require different plugin credentials. Make sure you use the suitable `plugin_credential.json`. 

## Docker build
```bash
sudo docker build -f <Dockerfile.x86_64|Dockerfile.jetson> .
```

## Install CMake
- CMake 3.23 - [installation link](https://cmake.org/download/)

## Dependency
- [Allxon Octo SDK](https://github.com/allxon/octo-sdk)
- OpenSSL

The _Allxon Octo SDK_ library is auto fetched when you configure cmake.

### Linux Debian
Install OpenSSL in Linux Debian.

```bash
apt-get update && apt-get install libssl-dev
```

### Other Linux Distribution & Windows
Follow the installation instructions on the [OpenSSL official site](https://www.openssl.org).


## Build and Run

### Linux
```bash
# Configuration Stage
cmake -S . -B build -DCMAKE_BUILD_TYPE=<Debug|Release> -DPLUGIN_KEY=plugin_credential.json

# Build Stage
cmake --build build

# Run after build
# You can run plugin-hello directly under the build/ folder, and pass resource_dir_linux through argument
build/plugin-hello resource_dir_linux
```

### Windows
```batch
@REM Configuration Stage
cmake -G "Visual Studio 16 2019" -A x64 -S . -B "build" -DPLUGIN_KEY=plugin_credential.json

@REM Build Stage
cmake --build build --config Release

@REM Run after build
@REM You can run plugin-hello directly under the build\ folder, and pass resource_dir_windows through argument
build\Release\plugin-hello.exe resource_dir_windows
```

# How to Deploy and Install
A _Plugin Package_ is an archived plugin which is workable with Allxon Plugin Center. (Contact us for detailed listing process)

The file naming convention of a plugin package is `plugin-hello-[version]-linux-[arch].tar.gz` (Linux) or `plugin-hello-[version]-win-[arch].zip` (Windows). 

## Deploy Plugin Package 

### Linux
```bash
# Deploy through docker, then you can get your plugin package under OUTPUT_DIRECTORY 
sudo docker build -f <Dockerfile.jetson|Dockerfile.x86_64> -o [OUTPUT_DIRECTORY] .
```
```bash
# Deploy through cmake, then you can get your plugin package under build directory
cmake --build build --target package
```

### Windows
```bash
# Deploy through cmake, then you can get your plugin package under build directory
cmake --build build --config <release|debug> --target package
```

## Test Plugin Installation through Plugin Installer Script
After building the plugin package, use the following commands to install and test it on your device.

### Linux
```bash
sudo wget -qO - https://get.allxon.net/plugIN/linux | sudo  bash -s -- --app-guid [APP_GUID] --from-path [PLUGIN_PACKAGE]
``` 

### Windows
```batch
powershell -command "Invoke-WebRequest -OutFile %temp%\plugin-installer.bat https://get.allxon.net/plugIN/windows" && %temp%\plugin-installer.bat --app-guid [APP_GUID] --from-path [PLUGIN_PACKAGE]
```

Once installed, the plugin starts automatically.

If you want to uninstall the plugin, use the following commands:
 
### Linux 
```bash
sudo wget -qO - https://get.allxon.net/plugIN/linux | sudo bash -s -- --app-guid [APP_GUID] --remove
``` 

### Windows
```batch
powershell -command "Invoke-WebRequest -OutFile %temp%\plugin-installer.bat https://get.allxon.net/plugIN/windows" && %temp%\plugin-installer.bat --app-guid [APP_GUID] --remove
```

# Getting Started
Once configured, cmake creates `WORKSPACE/build/build_info.h`. You can use the following predefined marcos:
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
        // if sign-in is successful, you can send it through websocket
        // e.g. 
        // enpoint.send(other_plugin_api_json_content);
    }

    if (json_validator.Verify(other_plugin_api_json_content))
    {
        // if verification is successful, it means json content is safe, and you can read it
    }
    return 0;
}
```
