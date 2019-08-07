#ifndef BOARDDEFINITION_H
#define BOARDDEFINITION_H

#include <Arduino.h>
#include <IPAddress.h>


/* =============================================================================
    Device definition
============================================================================= */

/***** Firmware version *****/
const char FIRMWARE_VERSION[8] = "1.1.3";


/***** Device Type *****/  
const uint8_t DEVICE_TYPE_LENGTH = 9;
const char DEVICE_TYPE[DEVICE_TYPE_LENGTH] = "ide_plus";


/* =============================================================================
    Optionnal hardware specification

    Comment / uncomment lines below to define the optionnal components currently
    used.
================================================================================ */

/***** Board version *****/
//#define BUTTERFLY_V03
//#define BUTTERFLY_V04
#define DRAGONFLY_V03

#define USE_LED_STRIP

/***** WiFi Options *****/
//#define USE_EXTERNAL_WIFI

/***** Optionnal components *****/
//#define RTD_DAUGHTER_BOARD

/***** GPS Options *****/
//#define WITH_CAM_M8Q
//#define WITH_MAX_M8Q

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
    Default configuration values
============================================================================= */

/***** MQTT server and credentials *****/
// US-WEST1-A server
//IPAddress MQTT_DEFAULT_SERVER_IP(35, 197, 32, 136);
// ASIA-SOUTH1-A (Mumbai) server
const IPAddress MQTT_DEFAULT_SERVER_IP(13,233,38,155);      //  IU-Default : 35, 200, 183, 103     ,Indicus -Testing IP   : 115,112,92,146  , Indicus-India IP [Production]  : 13,233,38,155
const uint16_t MQTT_DEFAULT_SERVER_PORT = 1883;             //  IU-Default : 1883                  ,Indicus -Testing Port : 41883           , Indicus-India Port[Production] : 1883

const uint8_t MQTT_CREDENTIALS_MAX_LENGTH = 25;
const char MQTT_DEFAULT_USERNAME[35] =  "ispl";             // IU-Username : ide_plus              ,Inducus Username Testing : guest        , Indicus-India Username[Production] : ispl
const char MQTT_DEFAULT_ASSWORD[35] =  "indicus";           // IU-Password : nW$Pg81o@EJD          ,Password                 :cnxmq2016     , Indicus-India Password[Production] : indicus


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
