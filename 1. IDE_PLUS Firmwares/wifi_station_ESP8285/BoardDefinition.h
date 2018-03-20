#ifndef BOARDDEFINITION_H
#define BOARDDEFINITION_H

#include <Arduino.h>


/* =============================================================================
    Device definition
============================================================================= */

/***** Firmware version *****/
const char FIRMWARE_VERSION[6] = "1.0.0";


/***** Device Type *****/
const uint8_t DEVICE_TYPE_LENGTH = 9;
const char DEVICE_TYPE[DEVICE_TYPE_LENGTH] = "ide_plus";


/* =============================================================================
    Debugging
============================================================================= */

/***** Test connection *****/
#ifdef IUDEBUG_ANY
    const char testSSID[19] = "AndroidHotspot6994";
    const char testPSK[13] = "f1b94630f970";
#endif  // IUDEBUG_ANY

#endif  // BOARDDEFINITION_H
