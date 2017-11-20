# Infinite Uptime IDE firmware repository #

This repository contains several firmware versions, depending on the functionnalities and the board (see folder list below).

**Before anything, please refer to the [Wiki](https://github.com/infinite-uptime/productivity/wiki).**


## Repository content ##
- **/IU_ESP82XX_WiFi_UART_Bridge_STA_OTA_V0.0** A firmware to flash to the ESP8285, that can be used for OTA (untested).
- **/IU_ESP82XX_WiFi_UART_Bridge_STA_V0.0** A firmware to flash to the ESP8285.
- **/butterfly_firmware** The current firmware for the IDE+
- **/butterfly_firmware_former+low_power_mode** *DEPRECATED* An intermediary firmware for the IDE+, which defines low power modes
- **/calibration_ui** A UI, written in Python, to calibrate IDEs and print calibration reports
- **/datasheets** The datasheets of all the components used so far on one of IU boards
- **/libraries** All libraries that are referenced by one or more firmwares in this directory
- **/teensy_MOTOR** Reference version of the Teensy firmware (see teensy_standard_firmware) for motors and rotating equipments
- **/teensy_NON_MOTOR** Reference version of the Teensy firmware (see teensy_standard_firmware) for all machines that are not motors or rotating equipments
- **/teensy_idle_firmware** Version of the Teensy firmware (see teensy_standard_firmware) where the last feature (by default audio dB) has been replaced by the maxAscent.
- **/teensy_press_firmware** Version of the Teensy firmware (see teensy_standard_firmware) with press features (total accel energy over 512ms, 4x total accel energies over 128ms, audio DB)
- **/teensy_standard_firmware** The firmware for the IDE (that is based on a Teensy) with standard features (total accel energy over 512ms, velocities X, Y and Z, temperature and audio DB)
- **ble.js** a node.js script for checking BLE communication with the module; this script was used to run end-to-end battery test. After BLE pairing, this module will print connection timestamp and disconnction timestamp.
- **config.json and config.yaml** Example config files to configure an IDE+ remotely
