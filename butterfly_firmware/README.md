# IDE Firmware #

## Dependencies ##

### Butterfly board ###
The Butterfly is a board developped by Tlera Corp (Kris Winer and his associate Thomas Roell). See comments [here](https://www.tindie.com/products/TleraCorp/butterfly-stm32l433-development-board/)
It requires to install custom board libraries (via the board manager in the Arduino IDE). Download .zip from [Thomas Roell GitHub](https://github.com/GrumpyOldPizza/arduino-STM32L4).

### Additional libraries ###
Required additionnal libraries can be found in this repository, as .zip folders:
- Arduino-MemoryFree-master.zip, also available (here)[https://github.com/mpflaga/Arduino-MemoryFree]
- arduinounit-master.zip, also available (here)[https://github.com/mmurdoch/arduinounit]
- IUButterflyI2S.zip, that is basically the same I2S library developped than in the Butterfly board package (see above), but with some modified constants. In particular, the I2S_BUFFER_SIZE need to be modified to adjust the I2S Tx callback rate.

These libraries can be installed in the Arduino IDE in the menu Sketch > Include Library > Add .ZIP Library.


## Modularization ##
From this version on, the firmware has been separated into C++ libraries, each library handling a specific component or functionality. The objective is to be able to develop a working Arduino Skectch for any hardware configuration with minimal effort, by including the libraries corresponding to the components.
Similarly, if existing hardware evolves (eg: a component changes), it should be easy to make the firmware evolve by updating the list of included libraries.
Also, the development of a library for a new component (or the modification of an existing one) should be fairly easy since it requires minimal interaction with other libraries.


## Current libraries ##
We distinguish 4 types of libraries:
1. Logical libraries: are prefixed with "IU" followed by the logical functionnality it is handling
*eg: IUConfiguration.h, IUUtilities"
2. Interface libraries: are prefixed with IU, followed by the name of the interface they are implementing, or the name of the component they are using. 
eg: IUI2C, IUBMD350
Note that the I2C library is a bit particular, because it is an interface between the user and the board, but also between all the components of the board. It must be instanciated first, and a pointer to it need to be passed to each instance of each component.
3. Sensor libraries: are named the same as the component itself, prefixed by “IU”.
*eg: IUMPU9250.h or IUBMX055.h are 2 libraries for 2 different components. They both are accelerometer + gyroscope + mag combos.*
4. Configurator and executor libraries: there are 3 of them - IUSensorConfigurator, IUFeatureConfigurator and IUConductor

Existing librairies are:
- Logical libraries:
  - IUABCFeature
  - IUABCInterface
  - IUABCProducer
  - IUABCSensor
  - IUFeature
  - IUUtilities
- Interface libraries:
  - IUI2C (the current one is developped for the Butterfly board specifically)
  - IUI2CTeensy (only for Teensy firmware version)
  - IUMD350: Bluetooth Low Energy
  - IUESP8285: Wifi
- Sensors libraries:
  - IUBattery: Battery 
  - IUBMP280: Temperature + Pression sensor
  - IUBMX055: Accelerometer + Gyroscope + Magnetometer
  - IUI2S: Microphone
  - IURGBLed: RGB led with 7 color display (+ off)
  - IUMPU9250: Accelerometer + Gyroscope + Magnetometer for the Teensy  (only for Teensy firmware version)
- Configurators and Conductor
  - IUSensorConfigurator
  - IUFeatureconfigurator 
  - Conductor

IUI2C is a class wrapping a lot of functionnalities that make use of the I2C (Inter-Integrated Circuit) protocol of the board. All other hardware related classes (basically all the classes except the logical libraries) have a pointer to it. 
Sensors can be interpreted as endpoints that acquire data. Endpoints make use of interfaces to send / receive data to / from the user.


## Dependency ##
Note that libraries can depend on other libraries. The general logic is as follow:
1. Logical libraries:
  - can be included in any other library
  - should only include other logical libraries
2. Interface libraries:
  - can be included in component libraries (note that I2C needs to be included in BLE and WiFi libraries, since those are components)
  - should only include logical libraries or I2C
3. Sensor libraries:
  - can be included only in Configurator or Conductor libraries.
  - Should include logical or interface libraries
4. Configurators and Conductor:
  - shouldn't be included in any other library.
  - can include all the other libraries (and the excutor includes the configurators)


## Library caracteristics ##
### Logical libraries ###
The formalization for these libraries is quite free. It can be a class or a set of constants or functions.
As of today, logical libraries regroup the following functionnality:
- Template classes for Interfaces and Sensors
- Classes for data Producers, and Features (which are both data producers and consumers).

In short, the logical libraries mainly implement the data pipeline within the firmware.
1. Sensors are Producers subclasses: they acquire data, and then pass it down to their data consumers.
2. Features are both data consumers and producers. They take data from a producer, and use it to produce new data.
3. Since Features are both producers and consummers, they can be chained to perform advanced data transformation.
4. Features also implements streaming function, to send data over USB Serial, BLE or WiFi.

### I2C Libraries ###
An I2C library contains a single class that has to handle at least the following:
- hold its own hardware related configuration constants
- have the ability to self-activate / self-configure / self-initialize
- read and write bytes from / to given address and subaddress
- handle I2C read / write errors
- detect each components present on the board
- have a port (usually Serial) and reading and writing through it
- handle port read / write errors

### Other interface libraries ###
Interface libraries contain a single class that has at least the following functionnalities:
- hold its own hardware related configuration constants
- be able to hold and update its configuration variables
- have a pointer to an I2C instance
- have the ability to self-activate / self-configure / self-initialize (by using the I2C instance)
- have a port to read / write (eg: over bluetooth, wifi, etc)
- handle read / write errors
- **optionnal**: BLE component and BLE library currently handle time synchronisation with the hub.

### Sensor libraries ###
A Sensor library contains a single class that has at least the following functionnalities:
- hold its own hardware related configuration constants
- be able to hold and update its configuration variables
- have a pointer to an I2C instance
- have the ability to self-activate / self-configure / self-initialize (by using the I2C instance)
- serve its purpose (eg: measure something in the case of a sensor, blink in the case of the LED, etc)
- send data through a given port

### Configurators and conductor ###
In IU IDE Firmware, the role of the conductor is to orchestrate (hence the name) how all the components work together. This includes:
- Instantiates the interfaces
- Instantiates the configurators for sensors and features.
- Instantiates the sensors and features through their configurators
- Handles data acquisitions, feature computation, data sending, time syncing, state and mode changes and configuration data reception

The conductor relies on the SensorConfigurator to handle everything that is sensor related: instantiation, configuration, sampling rate, data transmission to feature or through BLE...
It relies on the FeatureConfigurator for: feature instantiation (based on user requested configuration), configuration, feature computation, result trasnmission via BLE...


### The role of the Sketch (*.ino file) ###
Since all of the functionnalities are handled in IU libraries, the Sketch should only instanciate the Conductor and calls its functions in either the setup on main functions.

## Component Specifications ##

Current board include the following components:
- Butterfly board, with STM32L433C microprocessor
- BMD350: Bluetooth low energy
- ESP8285: WiFi
- BMX055: Accelerometer, Gyroscope, Magnetometer combo
- BMP280: Temperature and Pressure sensor combo
- CAM-M8Q: Geolocation device (GPS, Galileo, GLONASS, BeiDou)
- ICS43432: Sound sensor, with I2S protocol

Reference can be found in respective datasheet. Some usefull info are summarized below:


### Sensor output format ###
Since we do computations using CMSIS arm_math library that handles q15_t numbers, it is convenient to store sensor output as q15_t numbers or variant (eg q4.11). Basically, it means that we convert sensor output to a 16bit long format, either by droping the lest significant bits (>>) or by adding some that are equal to 0 (cast to int16_t then move bits to the left (<<)).
**Warning 1**
Special attention should be paid to format conversion, especially when handling variables (there is no compiler error or warning when casting a float to q15_t or vice versa). 
**Warning 2**
Different sensors have different output format: temperature and pressure data are float, audio data is 24bit converted to q15_t by dropping 8 least significant bits, BMX055 accelerometer data are Q4.11 (12bit data converted to 16bit), while MPU9250 accelerometer data are q15_t.
**Reminder**
q15 are 15-fractional-bit number in binary.
q4.11 are 4-integer-bit and 11-fractional-bit number in binary.
Check out conversion method on [wikipedia](https://en.wikipedia.org/wiki/Q_(number_format)#Conversion).
sensor sensibility and scale (before we convert the outputs to q15_t) are listed below.

### BMD350 ###
BMD350 comes with a pre-flashed firmware named BMDWare. You should read both BMD350 and BMDWare datasheets.

####MAC Address####
The BLE MAC address is written on the BMD350 chip on it, in the form ABXXXXXX. 'AB' is the firmware version (see datasheet for more details), and the X's are the 6 last hex digit of the MAC address. The Mac address is then 94:54:93:XX:XX:XX.
It is also stored on the chip memory and is available as long as full memory erase is performed (see datasheet). Read it via UICR register.
UICR Register:
NRF_UICR + 0x80 (0x10001080): MAC_Addr [0] (0xZZ)
NRF_UICR + 0x81 (0x10001081): MAC_Addr [1] (0xYY)
NRF_UICR + 0x82 (0x10001082): MAC_Addr [2] (0xXX)
NRF_UICR + 0x83 (0x10001083): MAC_Addr [3] (0x93)
NRF_UICR + 0x84 (0x10001084): MAC_Addr [4] (0x54)
NRF_UICR + 0x85 (0x10001085): MAC_Addr [5] (0x94)

####Beacon / iBeacon configuration####
Every beacon info can be configured, including the device name, UUID, major and minor numbers, advertissement info and rate, etc. Beacon configurations are retained even when the device is powered off.

####AT Command mode####
BMD350 has 2 modes that we use:
- a configuration mode named "AT Command Interface", that allows to interact with the BMDWare via I2C and the Serial port to configure the device. This include Beacon configuration and UART configuration.
- UART Pass-Through mode, that we use to stream data over bluetooth. We send data to the BLE module via Serial, and these data are automatically sent over bluetooth.
Note that UART Pass-Through mode has to be configured using the AT Mode, but that AT Mode actually have to be exited for UART Pass-Through to work.

####BLE utility app####
Some usefull Android / Apple apps exist, that allow to detect, connect to and read / write data to a BLE device. We have use "BLE UUID Explorer" and "Rigado Toolbox" so far.
Rigado Toolbox is specifically designed to work with BMD300 series (developed by Rigado). Some of its functionnalities are:
- UUID
- MAC Address = Serial Number. It is always in the form 94:54:93:XX:XX:XX. As said earlier, the last 6 hex digits are also available on the chip markings.
- If you have configured the chip in UART pass-through mode, you can use the test console of the app to send / receive info from device

### ESP8285 ###
The ESP8285 is basically an ESP8266 with additionnal SPI flash memory. See comments [here](https://www.tindie.com/products/onehorse/esp8285-development-board/).
It requires the [ESP8266 library](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi). Download it and install it using the Arduino IDE library manager.
NB: You'll have to download the whole repo (which is for the independent ESP8266 board). Download it but just use the ESP8266WiFi library.

### MPU9250: accelerometer, gyroscope and magnetometer ###
- Signed, MSB first
- Accelerometer: 
  - 3-axis, 16 bit long each, signed digital output. The output then range between [-2^15, 2^15 - 1].
  - Full-scale can be set to +/-2g, +/-4g, +/-8g, +/-16g, meaning that the LSB (least significant bit) is respectively 2^-14 (2 / 2^15)g, 2-13g, 2-12g, 2^-11g. 
- Gyroscope:
  - 3-axis, 16 bit long each, signed digital output. The output then range between [-2^15, 2^15 - 1].
  - Full-scale range can be set to  of +/-250, +/-500, +/-1000 and +/-2000 deg/sec. LSB is then respectively 250/2^15 deg/sec, 500/2^15 deg/sec, 1000/2^15 deg/sec, 2000/2^15 deg/sec.
- Magnetometer:
  - 3-axis, 14 bit long each, signed digital output. The output then range between [-2^13, 2^13 - 1].
  - Full scale range is +/-4800μT, so LSB is 4,8/2^13 T.

### BMX055: accelerometer, gyroscope and magnetometer ###
- Signed, LSB first
- Accelerometer: 
  - 3-axis, 12 bit long each, signed digital output. The output then range between [-2^11, 2^11 - 1].
  - Full-scale can be set to +/-2g, +/-4g, +/-8g, +/-16g.
  - Since raw output is 12bit long, we have 4 bits left that we can use (we can multiply the data by 2^4 = 16 without overflowing). We use this 4 bits to multiply the raw data by the resolution, meaning that the sensor output that is transmitted to features is already in G.
- Gyroscope:
  - 3-axis, 16 bit long each, signed digital output. The output then range between [-2^15, 2^15 - 1].
  - Full-scale range can be set to  of +/-125, +/-250, +/-500, +/-1000 and +/-2000 deg/sec. LSB is then respectively 125/2^15 deg/sec, 250/2^15 deg/sec, 500/2^15 deg/sec, 1000/2^15 deg/sec, 2000/2^15 deg/sec.
- Magnetometer:
  - 3-axis, +/-1200μT on X and Y, +/-2500μT on Z.
  - Resolution is 0.3 μT

### INMP441 and ICS-43432: Microphone ###
- Signed, MSB first as per I2S standard
- 24bit long data through I2S (see here: https://en.wikipedia.org/wiki/I%C2%B2S and here: 
- Sensitivity is −26dBFS (for a sine wave at 1 kHz and 94 dB SPL). This means that Full-Scale reading is 120dB (94dB - 120dB = -26dB).
NB: -26dB is the sine peak. The RMS level is -29dBFS (RMS is 3dB below for a 1KHz sine since 20log10(0.707) = -3)
- A good measure of sound volume is then just the RMS in DB. We use normalized sum of 20 * log10(abs(sample)).

### BMP280: Thermometer and barometer ###
BMP280 documentation specifies its own formula to access temperature and pressure. See BMP280 datasheet at section *3.11.3 Compensation formula* p22 and at section *8.2 Compensation formula in 32 bit fixed point* p45-46.
Documentation also states that both pressure and temperature values are expected to be received in 20 bit format, positive, stored in a 32 bit signed integer.
An example of algorith is available p23, otherwise "This algorithm is available to customers as reference C source code (“ BMP28x_ API”) from Bosch Sensortec and via its sales and distribution partners."


