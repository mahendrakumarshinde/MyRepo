# Infinite Uptime IDE firmware repository #

This repository contains several firmware versions, depending on the functionnalities and the board (see folder list below).
Each firmware version has its own README, please refer to it.


## Repository content ##
* /IU_ESP82XX_WiFi_UART_Bridge_STA_V0.0
  A firmware to flash to
* /butterfly_firmware: The firmware for the IDE+, which is based on an Butterfly
* /calibration_ui: A UI written in Python to calibrate IDEs and print calibration reports
* /libraries: All libraries that are referenced by one or more firmwares in this directory
* /teensy_idle_firmware: Version of the Teensy firmware (see teensy_standard_firmware) where the last feature (by default audio dB) has been replaced by the maxAscent.
* /teensy_press_firmware: Version of the Teensy firmware (see teensy_standard_firmware) with press features (total accel energy over 512ms, 4x total accel energies over 128ms, audio DB)
* /teensy_standard_firmware: The firmware for the IDE (that is based on a Teensy) with standard features (total accel energy over 512ms, velocities X, Y and Z, temperature and audio DB)
* /teensy_zz_rework: Teensy firmware in a intermediary state. Component functionalities were separated in several libraries, but most of the logic is still in a single .ino file. We mostly keep this as an archive.
* ble.js: a node.js script for checking BLE communication with the module; this script was used to run end-to-end battery test. After BLE pairing, this module will print connection timestamp and disconnction timestamp.


## Flashing the boards & Arduino cores ##
So far, IU has been using the following modules, that each requires a different core:
* Teensy: Visit the Teensyduino site to [download](https://www.pjrc.com/teensy/td_download.html) and install the Teensy core
* Butterfly: Follow instructions on Thomas Roell [GrumpyOldPizza GitHub](https://github.com/GrumpyOldPizza/arduino-STM32L4) to download and the core
* ESP82XX (ESP8285 or ESP8266): Follow instructions on [ESP8266 GitHub](https://github.com/esp8266/Arduino) to download and the core.