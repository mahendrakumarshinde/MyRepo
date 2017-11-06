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


## Communication message formatting ##

### Computer / Android App / Hub to device ###

**hex_msg_length,hex_cmd_id,comma_separated_args;**

**Comments:**
* Command length has a max size, that is used to define the size of the receiving buffer on the device: *Max cmd length = 50 char = 50 bytes*
* the message length in Hexadecimal is used to assert that we haven't missed some of the message
* See next section to check how command should be acknowledged.

**eg:**
Set thresholds for feature_id 1 to (5, 10, 15) - set_threshold command has id 12
10,12,1,5,10,15;
(note that 10(HEX) = 16(DEC))

### Device to Computer / Android App / Hub ###

**timestamp,MAC_address,msg_type,rest_of_message;**

**The message type can be:**
- Command acknowledgement: rest_of_message = hex_cmd_id
- raw data sending: rest_of_message = data_id,sample_length,comma_separated_samples
- FFT sending: rest_of_message = data_id,sample_length,comma_separated_coeffs (=> TODO: determine how to make profit of the FFT coeffs sparsity)
- feature sending: rest_of_message = feature_count,[feature_id,feature_value] * feature_count

**eg:**
- Command acknowledgement: 1492145778.80,94:54:93:0E:7D:D1,ACK,12;
- raw data sending: 1492145778.80,94:54:93:0E:7D:D1,RAW,AX,512,10.05,11.2,10.8,...,28;
- FFT sending: 1492145778.80,94:54:93:0E:7D:D1,FFT,AX,256,80,72,20,,,,,,12,45,6,,,;
- feature sending: 1492145778.80,94:54:93:0E:7D:D1,FEA,4,01,42.0,02,3.0,03,32,04,89;


### Wired command only ###
Some commands can only be sent via Serial, since there are meant to work while the device is wired. For these command, we use human readable words.
These commands are used to change the Usage Preset (eg: EXPERIMENT, CALIBRATION, back to OPERATION).
