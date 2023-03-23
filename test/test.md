## How it works
This folder contains scripts to help you build and test the plugin.

## `test_octo_sdk.sh`
This script is used to test the plugin build with specific octo sdk version. It will build plugin package to the device and run the plugin. You can use this script to test if plugin run successfully with specific octo sdk version.

### Usage
```bash
# Run the script with specific octo sdk version
./test/test_octo_sdk.sh <app_guid> <executable_name> <octo_sdk_version> 

# Example
./test/test_octo_sdk.sh a8e873a1-e5df-43a2-928a-745ff9c94dfb plugin-hello 3.1.0
```
Websocket connected expected output:
```plain
OnTLSInit
OnOpen
SendNotifyPluginUpdate
...
```

Failed output like :
```plain
OnTLSInit
OnFail
terminate called after throwing an instance of 'websocketpp::exception'
  what():  invalid state
Aborted (core dumped)
```