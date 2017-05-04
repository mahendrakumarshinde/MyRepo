# Infinite Uptime IDE firmware repository #

This repository contains several firmware versions, depending on the functionnalities and the board (see folder list below).
Each firmware version has its own README, please refer to it.


## Infinite Uptime Productivity ##
* /butterfly_firmware
  Buttefly firmware, close to 1st production version, only velocities still need some adjustment
* /calibration_ui
  A UI written in Python to calibrate IDEs and print calibration reports
* /libraries  
  All libraries that are referenced by one or more firmwares in this directory
* /teensy_press_firmware
  Teensy firmware with press features (total accel energy over 512ms, 4x total accel energies over 128ms, audio DB)
* /teensy_standard_firmware
  Teensy firmware with standard features (total accel energy over 512ms, velocities X, Y and Z, temperature and audio DB)
* /teensy_zz_rework
  Teensy firmware in a intermediary state. Component functionalities were separated in several libraries, but most of the logic is still in a single .ino file. We mostly keep this as an archive.
* ble.js  
  node.js script for checking BLE communication with the module; this script was used to run end-to-end battery test. After BLE pairing, this module will print connection timestamp and disconnction timestamp.

