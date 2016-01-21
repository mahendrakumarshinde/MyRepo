# Infinite Uptime

## Infinite Uptime Productivity (Bluetooth Machine Monitoring Module)

  * /Productivity  
    Firmware for Productivity - currently calculating vibration energy and 3 thresholds for a 4-level monitoring display. BLE only for state-change communication.
	
  * /android_demo  
    Demo firmware designed to work with IU Android application.

  * /battery_level_check  
    Example code for programatically checking battery voltage level.
  
  * /ble_firmware_main  
    Main firmware for BLE sensor module; separate descriptions for this [below](#main-ble-firmware).

  * /energy_feature_basic  
    Test code for energy feature based monitoring, statemachine for operating status, and two-way communication. Also used for power consumption tests.
  
  * /led_test  
    Example code for PWM based LED coloring.  

  * /libraries  
    All libraries that are referenced by one or more firmwares in this directory
  
  * /test_firmware_total 
    Firmware for functionality check; used mainly for the modules sent over to India, with Android application [BLE UUID Explorer](https://play.google.com/store/apps/details?id=ghostysoft.bleuuidexplorer). Separate documentation for functionality check procedure listed [below](#data-collection-procedure).
  
  * ble.js  
	node.js script for checking BLE communication with the module; this script was used to run end-to-end battery test. After BLE pairing, this module will print connection timestamp and disconnction timestamp.
