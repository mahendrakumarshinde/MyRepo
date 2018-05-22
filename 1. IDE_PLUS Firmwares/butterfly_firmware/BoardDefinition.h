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

/* Because IU sometimes needs to modify libraries, we implement our own
 * versionning.
 */
namespace IU_VERSION {
    namespace required {
        const uint8_t arduinojson[3] = {1, 0, 0};
        const uint8_t arduinomd5[3] = {1, 0, 0};
        const uint8_t arduinounit[3] = {1, 0, 0};
        const uint8_t iubutterflyi2s[3] = {1, 0, 0};
        const uint8_t iugnss[3] = {1, 0, 0};
        const uint8_t iuserial[3] = {1, 0, 0};
        const uint8_t iuwifiota[3] = {1, 0, 0};
        const uint8_t macaddress[3] = {1, 0, 0};
        const uint8_t memoryfree[3] = {1, 0, 0};
        const uint8_t timer[3] = {1, 0, 0};
    };  // required

    inline bool checkVersion(const uint8_t *requiredVersion,
                             const uint8_t *installedVersion)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            if (requiredVersion[0] != installedVersion[0])
            {
                return false;
            }
        }
        return true;
    }

    inline bool checkAllLibraryVersions()
    {
        return (
//            checkVersion(required::arduinojson, installed::arduinojson) &&
//            checkVersion(required::arduinomd5, installed::arduinomd5) &&
//            checkVersion(required::arduinounit, installed::arduinounit) &&
//            checkVersion(required::iubutterflyi2s, installed::iubutterflyi2s) &&
//            checkVersion(required::iugnss, installed::iugnss) &&
//            checkVersion(required::iuserial, installed::iuserial) &&
//            checkVersion(required::iuwifiota, installed::iuwifiota) &&
//            checkVersion(required::macaddress, installed::macaddress) &&
//            checkVersion(required::memoryfree, installed::memoryfree) &&
//            checkVersion(required::timer, installed::timer) &&
            true);
    }
};  // IU_VERSION


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
