#ifndef BOARDDEFINITION_H
#define BOARDDEFINITION_H

#include <Arduino.h>


/* =============================================================================
    Optionnal hardware specification

    Comment / uncomment lines below to define the optionnal components currently
    used.
============================================================================= */

/***** Board version *****/
//#define BUTTERFLY_V03
//#define BUTTERFLY_V04
#define DRAGONFLY_V03

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


/* =============================================================================
    Test mode

    Comment / Uncomment the "define" lines to toggle / untoggle unit tests.
============================================================================= */

//#define UNITTEST  // Logical libraries unit test
//#define COMPONENTTEST  // Components unit test
//#define INTEGRATEDTEST  // Firmware integrated test


#if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
    const bool testMode = true;
#else
    const bool testMode = false;
#endif


/* =============================================================================
    Unit test includes
============================================================================= */

#ifdef UNITTEST
    #include "UnitTest/Test_Component.h"
    #include "UnitTest/Test_FeatureClass.h"
    #include "UnitTest/Test_FeatureComputer.h"
    #include "UnitTest/Test_FeatureGroup.h"
    #include "UnitTest/Test_Sensor.h"
    #include "UnitTest/Test_Utilities.h"
#endif
