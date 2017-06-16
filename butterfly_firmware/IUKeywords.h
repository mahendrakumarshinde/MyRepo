#ifndef IUKEYWORDS_H
#define IUKEYWORDS_H

#include <Arduino.h>

/* ============================= Sensor Types ============================= */

// TODO move the sensorTypes from IUABCSensor to here


/* ============================= Operation Modes ============================= */

/**
 * Define the power mode at component level 
 * 
 * Power modes are controlled either automatically (auto-sleep) or by the user
 */
class powerMode
{ 
  public:
    enum option : uint8_t {ACTIVE  = 0,
                           SLEEP   = 1,
                           SUSPEND = 2,
                           COUNT   = 3};
};


/**
 * Define what type of data we want to acquire / compute
 */
class acquisitionMode
{
  public:
    enum option : uint8_t {RAWDATA = 0,
                           FEATURE = 1,
                           NONE    = 2,
                           COUNT   = 3};
};

/**
 * Define the channel through which data will be sent
 */
class streamingMode
{
  public:
    enum option : uint8_t {WIRED = 0,   // Send over Serial
                           BLE   = 1,   // Send over Bluetooth Low Energy
                           WIFI  = 2,   // Send over WiFi
                           STORE = 3,   // Store in SPI Flash for later streaming
                           COUNT = 4};
};
// TODO => Need to really enforce streamingMode, as most of the streaming functions have hardcoded ports
// (eg dumpDataThroughI2C) => Need to finish to define all the message format for the different channel (WIRED, BLE, WIFI)


/* ============================= Operation Presets ============================= */

/**
 * Usage Preset are user controlled, they describe how the device is being used
 */
class usagePreset
{
  public:
    enum option : uint8_t {CALIBRATION   = 0,
                           EXPERIMENT    = 1,
                           OPERATION     = 2,
                           COUNT         = 3};
    // Related default config
    static powerMode::option powerPresetDetails[option::COUNT];
    static acquisitionMode::option acquisitionModeDetails[option::COUNT];
    static streamingMode::option streamingModeDetails[option::COUNT];
};

/* ============================= Operation States ============================= */

/**
 * Operation states describe the production status, inferred from calculated features
 * and user-defined thresholds
 */
class operationState
{
  public:
    enum option : uint8_t {IDLE    = 0,
                           NORMAL  = 1,
                           WARNING = 2,
                           DANGER  = 3,
                           COUNT   = 4};
};

#endif // IUKEYWORDS_H
