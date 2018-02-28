#ifndef KEYWORDS_H
#define KEYWORDS_H

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


/* =============================================================================
    Axis
============================================================================= */

namespace Axis
{
    enum option : uint8_t {X     = 0,
                           Y     = 1,
                           Z     = 2,
                           COUNT = 3};
}


/* =============================================================================
    Component Power Modes
============================================================================= */

/**
 * Define the power mode at component level
 */
namespace PowerMode
{
    enum option : uint8_t {ACTIVE  = 0,
                           SLEEP   = 1,
                           SUSPEND = 2,
                           COUNT   = 3};
}


/* =============================================================================
    Operation Mode
============================================================================= */

/**
 * Define what type of data we want to acquire / compute
 */
namespace AcquisitionMode
{
    enum option : uint8_t {RAWDATA = 0,
                           FEATURE = 1,
                           NONE    = 2,
                           COUNT   = 3};
}

/**
 * Define the channel through which data will be sent
 */
namespace StreamingMode
{
    enum option : uint8_t {
        WIRED        = 0,       // Send over Serial
        BLE          = 1,       // Send over Bluetooth Low Energy
        WIFI         = 2,       // Send over WiFi
        WIFI_AND_BLE = 3,       // Send over both WiFi and BLE
        STORE        = 4,       // Store in SPI Flash to stream later
        COUNT        = 5};
}


/* =============================================================================
    Operation Presets
============================================================================= */

/**
 * Usage Mode are user controlled, they describe how the device is being used
 */
namespace UsageMode
{
    enum option : uint8_t {CALIBRATION     = 0,
                           EXPERIMENT      = 1,
                           OPERATION       = 2,
                           OPERATION_BIS   = 3,
                           COUNT           = 4};
    // Related default config
    const AcquisitionMode::option acquisitionModeDetails[COUNT] =
    {
        AcquisitionMode::FEATURE,
        AcquisitionMode::RAWDATA,
        AcquisitionMode::FEATURE,
        AcquisitionMode::FEATURE,
    };
}


/* =============================================================================
    Operation States
============================================================================= */

/**
 * Operation states describe the production status
 *
 * The state is determined by features values and user-defined thresholds.
 */
namespace OperationState
{
    enum option : uint8_t {IDLE    = 0,
                           NORMAL  = 1,
                           WARNING = 2,
                           DANGER  = 3,
                           COUNT   = 4};
}

#endif // KEYWORDS_H
