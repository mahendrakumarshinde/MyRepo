# IDE Firmware Flashing Utility (Windows)

- This firmware flashing can be used to flash firmware to IDE on windows.  
- This utility is designed to flash multiple IDEs at a time. You can connect more than one IDE while using this script.    
- **DO NOT terminate the script or disconnect the device while FLASHING BRIDGE, this corrupts the DFU implemetation on the IDE and IDE can be recovered back to DFU mode with a hard reset only!**


## Installation

- Clone this repository or download a zip. 
- The utility is tested on 64bit win7/win10.

## Usage

1. Get the latest firmware release from IDE firmware repository. - [Firmware Releases](https://github.com/vksgaikwad3/productivity/releases)
2. Extract the firmware binary files from the downloaded release pacakge.
3. Place the binary files in "**bin/firmware_binaries**" directory of the flashing utility. (Refer to readme.txt in the "firmware_binaries" directory)
4. Run "**Flash.exe**" in "**bin**" directory.
5. You can find flashing reports in bin/reports directory.


## Adding custom backends for HTTP and MQTT endpoints (Only for IDE PLUS)
The file `backends.json` in the root directory contains MQTT and HTTP endpoints for custom backend platforms.  
To add a new endpoint, add a JSON element in the following format :  
```
{
    "key":"local",
    "backend":"Local development laptop setup",
    "mqtt":{"mqtt": {"mqttServerIP":"192.168.21.116","port":1883,"username":"","password":""}},      
    "http":{"httpConfig":{"host":"192.168.21.116","port":1234,"path":"/testendpoint","username":"","password":"","oauth":""}}
}
```
- __`key`__ is to be specified with the flash tool invocation, for e.g. `./flash.sh local`.  
- __`backend`__ is a description for the backend platform.  
- __`mqtt`__ and __`http`__ fields are the endpoints which will be sent to the device.
- __`key`__ needs to be unique and should not be any of the following : `gcp`, `aws`, `awschina`, `uat`.

## Usage

1. Get the latest firmware release from IDE firmware repository. 
2. Extract the firmware binary files from the downloaded release pacakge.
3. Place the binary files in "**bin/firmware_binaries**" directory of the flashing utility. (Refer to readme.md in the "firmware_binaries" directory)
4. Run either of the following executables to flash the firwmares for __IDE PLUS__. 
    ```
    ./flash.exe  
    ./flash_awschina.cmd  
    ./flash_gcp.cmd
    ```
    Note that if no backend is specified (i.e. `./flash.exe`), then no endpoint will be flashed to device.

5. If a custom backend platform has been added in `backends.json`, it can be specified during the flashing process for __IDE PLUS__ as follows:
    ```
    ./flash.exe <backend_key> 
    ```
    The __`backend_key`__ is the "key" specified for a backend in the backends.json configuration file.

    ```


## Notes

- The script implements a retry mechanism for flashing utilities in case they fail if device is not detected. **Do not** quit the script intermediately if an error message is shown in the output, the script will automatically retry flashing process, let the script finish it's complete run.
- Please disable antivirus software before beginning the download. Your antivirus may prevent the flash utility from being downloaded.

## Troubleshooting

1. _Error 001: Failed to Access USB devices._ This error indicates that you have no access to USB hardware devices. Please contact your system administrator. 
2. _Error 002: Failed to detect IDE._ First, unplug IDE then plug again. If this does not work, switch to another USB port of the computer. Finally, this error can be solved by using an USB hub.
3. _Error 004: Failed to reset IDE!_ Please ensure that no serial monitor program (Arduino Serial Monitor, Cutecom etc.) has opened serial port and run the script again.
4. _Error 005: Missing file xxx._ Please check file's existence and ensure that the specific files have been copied properly.
5. For other errors, please try to rerun the Flash.exe as administrator.  



