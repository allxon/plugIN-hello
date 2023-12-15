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
cd <EXTRACT_FOLDER_PATH>/<APP_GUID>
./plugin-hello $(pwd)
```

or Windows cmd.

```batch
cd <EXTRACT_FOLDER_PATH>\<APP_GUID>
plugin-hello.exe %cd%
```

# Build from Source

## Obtain Plugin Credential

You need to acquire a _Plugin Credential_, which represents your plugin identity. To do so, contact us to obtain `plugin_credential.json`, which includes a set of `APP_GUID` and `ACCESS_KEY`.
> **WARNING**: Each `plugin_credential.json` is paired with one plugin program. Different platforms or CPU architectures require different plugin credentials. Make sure you use the suitable `plugin_credential.json`.

## Docker build

```bash
sudo docker build .
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

# Specify octo sdk version
cmake -S . -B build -DCMAKE_BUILD_TYPE=<Debug|Release> -DPLUGIN_KEY=plugin_credential.json -DOCTO_SDK_VERSION=X.X.X

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

@REM Specify octo sdk version
cmake -G "Visual Studio 16 2019" -A x64 -S . -B "build" -DOCTO_SDK_VERSION=X.X.X

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
sudo docker build -o <OUTPUT_DIRECTORY> . -f [Dockerfile.x86_64|Dockerfile.aarch64]

# Specify octo sdk version
sudo docker build -o <OUTPUT_DIRECTORY> --build-arg CMAKE_ARGS="-DOCTO_SDK_VERSION=X.X.X" . -f [Dockerfile.x86_64|Dockerfile.aarch64]
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
sudo bash -c "$(wget -qO - https://get.allxon.net/plugIN/linux)" -s --app-guid <APP_GUID> --from-path <PLUGIN_PACKAGE>
```

### Windows

```batch
powershell -command "Invoke-WebRequest -OutFile %temp%\plugin-installer.bat https://get.allxon.net/plugIN/windows" && %temp%\plugin-installer.bat --app-guid <APP_GUID> --from-path <PLUGIN_PACKAGE>
```

Once installed, the plugin starts automatically.

If you want to uninstall the plugin, use the following commands:

### Linux

```bash
sudo bash -c "$(wget -qO - https://get.allxon.net/plugIN/linux)" -s --app-guid <APP_GUID> --uninstall
```

### Windows

```batch
powershell -command "Invoke-WebRequest -OutFile %temp%\plugin-installer.bat https://get.allxon.net/plugIN/windows" && %temp%\plugin-installer.bat --app-guid <APP_GUID> --uninstall
```

# Getting Started

Once configured, You can use the following predefined marcos:

- `PLUGIN_NAME`
- `PLUGIN_APP_GUID`
- `PLUGIN_ACCESS_KEY`
- `PLUGIN_VERSION`

```cpp
#include <iostream>
#include "octo/octo.h"
#include "websocket_client.h"

using namespace Allxon;

std::string Util::plugin_install_dir = "";

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        std::cout << "Please provide a plugin install directory." << std::endl;
        return 1;
    }
    else if (argc > 2)
    {
        std::cout << "Wrong arguments. Usage: device_plugin [plugin install directory]" << std::endl;
        return 1;
    }
    Util::plugin_install_dir = std::string(argv[1]);
    WebSocketClient web_client(std::make_shared<Octo>(
        PLUGIN_NAME, PLUGIN_APP_GUID,
        PLUGIN_ACCESS_KEY, PLUGIN_VERSION,
        Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_update_template.json")));
    web_client.run();
    return 0;
}
```
