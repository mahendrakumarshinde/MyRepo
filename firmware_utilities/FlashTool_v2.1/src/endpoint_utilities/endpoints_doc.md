## This document describes the purpose and example uses of functionality found within `endpoints.py`.
The IDE+ has the ability to be configured with different endpoints for both `MQTT` and `HTTP` protocols. A conbination of `HTTP` and `MQTT` endpoint will be referred to as a `backend`.

Thus, `endpoints.py` is a python interface to :
- Configure IDE+ with `HTTP` and `MQTT` endpoints by choosing a `backend`.
- Read current `HTTP` and `MQTT` endpoints.
- Read BLE MAC id of connected IDE+.

`endpoints.py` is to be called after device flashing process has finished. 

`endpoints.py` can only be used with IDE+ firmware version `>= 1.1.3`. Therefore, the flashing script which calls `endpoints.py` should ensure that the device being flashed has firmware version `>=1.1.3`. If this is not the case, `endpoints.py` should not be used.

## Adding custom backends for HTTP and MQTT endpoints (Only for IDE PLUS)
The file `backends.json` in the root directory contains `MQTT` and `HTTP` endpoints for custom backend platforms.  
To add a new endpoint, add a JSON element in the following format :  
```
{
    "key":"local",
    "backend":"Local development laptop setup",
    "mqtt":{"mqtt": {"mqttServerIP":"192.168.21.116","port":1883,"username":"","password":""}},      
    "http":{"httpConfig":{"host":"192.168.21.116","port":1234,"path":"/testendpoint","username":"","password":"","oauth":""}}
}
```
- __`key`__ is to be specified with the flash tool invocation, for e.g. `./flash.sh local`  
- __`backend`__ is a description for the backend platform. 
- __`mqtt`__ and __`http`__ fields are the endpoints which will be sent to the device.
- __`key`__ needs to be unique and should not be any of the following : `gcp`, `aws`, `awschina`, `uat`.

