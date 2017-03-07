# IDE Firmware

##Modularization
From this version on, the firmware has been separated into C++ libraries, each library handling a specific component or functionality. The objective is to be able to develop a working Arduino Skectch for any hardware configuration with minimal effort, by including the libraries corresponding to the components.
Similarly, if an existing hardware evolves (some components change), it should be easy to make the firmware evolve by updating the list of included libraries.
Also, the development of a library for a new component (or the modification of an existing one) should be fairly easy since it requires minimal interaction with other libraries.

##Current libraries
We distinguish 3 types of libraries:
1. Logical libraries: are prefixed with "IU" followed by the logical functionnality it is handling
*eg: IUConfiguration.h, IUUtilities"
2. Computer Bus (I2C) libraries: are named the same as the board, with the prefix IUI2C 
*eg: IUI2CTeensy.h, IUI2CButterfly.h*
3. Component libraries: are named the same as the component itself, prefixed by “IU”. Components can be functionnal (eg: sensors, LED, etc) or connectivity related (eg: bluetooth or wifi) devices.
*eg: IUMPU9250.h or IUBMX055.h are 2 libraries for 2 different components. They both are accelerometer + gyroscope + mag combos.* 

Existing librairies are:
- Logical libraries:
  - IUUtilities
- I2C libraries:
  - IUI2CTeensy
  - IUI2CButterfly
- Connectivity component libraries:
  - IUBLE: Bluetooth Low Energy
- Functionnal component libraries:
  - IUBattery: Battery 
  - IUBMP280: Temperature + Pression sensor
  - IUBMX055: Accelerometer + Gyroscope + Magnetometer
  - IUI2S: Microphone
  - IUMPU9250: Accelerometer + Gyroscope + Magnetometer
  - IURGBLed: RGB led with 7 color display (+ off)

Functionnal components can be interpreted as endpoints, while connectivity components or I2C are intermediates. Intermediates and endpoints are of course inter-dependent. In this implementation, we choose to include the connectivity component and I2C in the functionnal component classes because it allows to adjust how each functionnal component uses connectivity without having to modify the connectivity classes themselves.

##Dependency
Note that libraries can depend on other libraries. The general logic is as follow:
- logical libraries:
  - can be included in any other library
  - should only include other logical libraries
- I2C libraries:
  - can be included in component libraries (both connectivity and functionnal components)
  - should only include logical libraries
- connectivity component libraries:
  - Can be included in functionnal component libraries
  - Should include any library except the functionnal component libraries
- functionnal component component libraries:
  - should never be included in other library
  - Should include any library except the other functionnal component libraries

##Library caracteristics
### Logical libraries
The formalization for these libraries is quite free. It can be a class or a set of constants or functions.
### I2C Libraries
An I2C library contains a single class that has to handle at least the following:
- hold its own hardware related configuration constants
- have the ability to self-activate / self-configure / self-initialize
- read and write bytes from / to given address and subaddress
- handle I2C read / write errors
- detect each components present on the board
- have a port (usually Serial) and reading and writing through it
- handle port read / write errors
### Connectivity component libraries
A connectivity library contains a single class that has at least the following functionnalities:
- hold its own hardware related configuration constants
- be able to hold and update its configuration variables
- have as I2C instance
- have the ability to self-activate / self-configure / self-initialize (by using the I2C instance)
- have a port to read / write (eg: over bluetooth, wifi, etc)
- handle read / write errors
- **optionnal**: BLE component and BLE library currently handle timestamping
## Functionnal component libraries
A functionnal component library contains a single class that has at least the following functionnalities:
- hold its own hardware related configuration constants
- be able to hold and update its configuration variables
- have an I2C instance
- have one or several connectivity component(s) (eg: one for bluetooth, one for wifi, etc)
- have the ability to self-activate / self-configure / self-initialize (by using the I2C instance)
- serve its purpose (eg: measure something in the case of a sensor, blink in the case of the LED, etc)
- post-process its own data (eg: compute total acceleration energy for the accelerometer) *- For this, one can leverage a logical library*
- send data through I2C port and through the port of each one of its connectivity components

##The role of the Sketch (*.ino file)
In IU IDE Firmware, the role of the sketch is to orchestrate how all the components work together. This includes:
- Before setup:
  - Instantiating an I2C object
  - Instantiating each component class
  - Define all possible measure functions (by leveraging each component) and create an array of pointers to them. (+ use hashMap to name them?)
  - Define all possible feature calculation functions (by leveraging each component), and store them in an array of pointers to them. (+ use hashMap to name them?)
  - Create a callback function that will be called independently from the main loop. This callback function will handle data measure and will test each measure functions (using the array of pointers) to see if they are active. If yes, then call them.
  - Create a compute_features function 
- During setup
  - Holding / fetching initial configuration data (**Should this be before setup?**)
  - Initializing the I2C and each components
  - Start the Audio component and pass it the callback function created before setup 
- During loop:
  - Handle recurring events
  - Handle timed events
  - Handle instructions received over the I2C port or any of the connectivity device
  - Handle states, modes and configuration updates

##Sensibility and scale of components and number format

###Sensor output format
Since we do computations using CMSIS arm_math library that handles q15_t numbers, it is convenient to store sensor output as q15_t numbers. Basically, it means that we convert every sensor output to a 16bit long format, either by droping the lest significant bits (>>) or by adding some that are equal to 0 (cast to int16_t then move bits to the left (<<)).
**Warning**
Special attention should be paid to format conversion, especially when handling variables (there is no compiler error or warning when casting a float to q15_t or vice versa).
**Reminder**
q15 are 15-fractional-bit number in binary. Check out conversion method here: https://en.wikipedia.org/wiki/Q_(number_format)#Conversion

Below are listed sensors and there sensibility and scale (before we convert the outputs to q15_t).

###MPU9250: accelerometer, gyroscope and magnetometer
Signed, MSB first
- Accelerometer: 
  - 3-axis, 16 bit long each, signed digital output. The output then range between [-2^15, 2^15 - 1].
  - Full-scale can be set to +/-2g, +/-4g, +/-8g, +/-16g, meaning that the LSB (least significant bit) is respectively 2^-14 (2 / 2^15)g, 2-13g, 2-12g, 2^-11g. 
- Gyroscope:
  - 3-axis, 16 bit long each, signed digital output. The output then range between [-2^15, 2^15 - 1].
  - Full-scale range can be set to  of +/-250, +/-500, +/-1000 and +/-2000 deg/sec. LSB is then respectively 250/2^15 deg/sec, 500/2^15 deg/sec, 1000/2^15 deg/sec, 2000/2^15 deg/sec.
- Magnetometer:
  - 3-axis, 14 bit long each, signed digital output. The output then range between [-2^13, 2^13 - 1].
  - Full scale range is +/-4800μT, so LSB is 4,8/2^13 T.

###MPU9250: accelerometer, gyroscope and magnetometer
Signed, MSB first
- Accelerometer: 
  - 3-axis, 12 bit long each, signed digital output. The output then range between [-2^11, 2^11 - 1].
  - Full-scale can be set to +/-2g, +/-4g, +/-8g, +/-16g, meaning that the LSB (least significant bit) is respectively 2^-10 (2 / 2^15)g, 2-9g, 2-8g, 2^-7g. 
- Gyroscope:
  - 3-axis, 16 bit long each, signed digital output. The output then range between [-2^15, 2^15 - 1].
  - Full-scale range can be set to  of +/-125, +/-250, +/-500, +/-1000 and +/-2000 deg/sec. LSB is then respectively 125/2^15 deg/sec, 250/2^15 deg/sec, 500/2^15 deg/sec, 1000/2^15 deg/sec, 2000/2^15 deg/sec.
- Magnetometer:
  - 3-axis, +/-1200μT on X and Y, +/-2500μT on Z.
  - Resolution is 0.3 μT

###INMP441 and ICS-43432: Microphone
Signed, MSB first as per I2S standard
- 24bit long data through I2S (see here: https://en.wikipedia.org/wiki/I%C2%B2S and here: 
- Sensitivity is −26dBFS (for a sine wave at 1 kHz and 94 dB SPL). This means that Full-Scale reading is 120dB (94dB - 120dB = -26dB).
NB: -26dB is the sine peak. The RMS level is -29dBFS (RMS is 3dB below for a 1KHz sine since 20log10(0.707) = -3)
- A good measure of sound volume is then just the RMS in DB. We use normalized sum of 20 * log10(abs(sample)).

###BMP280: Thermometer and barometer
BMP280 documentation specifies its own formula to access temperature and pressure. See BMP280 datasheet at section *3.11.3 Compensation formula* p22 and at section *8.2 Compensation formula in 32 bit fixed point* p45-46.
Documentation also states that both pressure and temperature values are expected to be received in 20 bit format, positive, stored in a 32 bit signed integer.
An example of algorith is available p23, otherwise "This algorithm is available to customers as reference C source code (“ BMP28x_ API”) from Bosch Sensortec and via its sales and distribution partners."







