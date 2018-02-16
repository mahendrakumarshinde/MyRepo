#ifndef BOARDDEFINITION_H
#define BOARDDEFINITION_H

#include <Arduino.h>


/* =============================================================================
    Optionnal hardware specification

    Comment / uncomment lines below to define the optionnal components currently
    used.
============================================================================= */

/***** Board version *****/
#define BUTTERFLY_V03
//#define BUTTERFLY_V04
//#define DRAGONFLY_V03

/***** WiFi Option *****/
//#define EXTERNAL_WIFI
#define INTERNAL_ESP8285

/***** Optionnal components *****/
//#define RTD_DAUGHTER_BOARD

/***** GPS Option *****/
//#define NO_GPS

/***** Firmware version *****/
const char FIRMWARE_VERSION[6] = "1.0.0";

#endif // BOARDDEFINITION_H
