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
#define BUTTERFLY_V04

/***** Optionnal components *****/
//#define RTD_DAUGHTER_BOARD


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
    Interface Types
============================================================================= */

namespace InterfaceType
{
    enum option : uint8_t {INT_USB   = 0,
                           INT_BLE   = 1,
                           INT_WIFI  = 2,
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
    enum option : uint8_t {WIRED = 0,  // Send over Serial
                           BLE   = 1,  // Send over Bluetooth Low Energy
                           WIFI  = 2,  // Send over WiFi
                           STORE = 3,  // Store in SPI Flash to stream later
                           COUNT = 4};
}


/* =============================================================================
    Operation Presets
============================================================================= */

/**
 * Usage Mode are user controlled, they describe how the device is being used
 */
namespace UsageMode
{
    enum option : uint8_t {CALIBRATION = 0,
                           EXPERIMENT  = 1,
                           OPERATION   = 2,
                           COUNT       = 3};
    // Related default config
    const AcquisitionMode::option acquisitionModeDetails[COUNT] =
    {
        AcquisitionMode::FEATURE,
        AcquisitionMode::RAWDATA,
        AcquisitionMode::FEATURE,
    };
    const StreamingMode::option streamingModeDetails[COUNT] =
    {
        StreamingMode::WIRED,
        StreamingMode::WIRED,
        StreamingMode::BLE,
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
