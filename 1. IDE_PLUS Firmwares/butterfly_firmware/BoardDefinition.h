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
    Optionnal hardware specification

    Comment / uncomment lines below to define the optionnal components currently
    used.
============================================================================= */

/***** Board version *****/
//#define BUTTERFLY_V03
//#define BUTTERFLY_V04
#define DRAGONFLY_V03

/***** WiFi Options *****/
//#define USE_EXTERNAL_WIFI

/***** Optionnal components *****/
//#define RTD_DAUGHTER_BOARD

/***** GPS Options *****/
#define NO_GPS

/* =============================================================================
    Library versionning
============================================================================= */

namespace IU_VERSION {
    namespace required {
        const uint8_t pubsubclient_major = 1;
        const uint8_t pubsubclient_minor = 1;
    };

    inline bool checkLibraryVersions()
    {
//        if (required.pubsubclient_major == installed.pubsubclient_major
//            )
//        {
//            return true;
//        }
        return false;
    }
};


/* =============================================================================
    Test mode

    Comment / Uncomment the "define" lines to toggle / untoggle unit tests.
============================================================================= */

//#define UNITTEST  // Logical libraries unit test
//#define COMPONENTTEST  // Components unit test
//#define INTEGRATEDTEST  // Firmware integrated test

#if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
    #include <ArduinoUnit.h>
#endif

#endif // BOARDDEFINITION_H
